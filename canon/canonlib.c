#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./canon.h"

#define BUFSIZE 262144	/* 256Kbytes size of image transfer buffer */


int ipu_debug = 0;
#ifndef __sgi
int dsdebug = 0;
#endif

static void
toshort(dest, src)
u_char	*dest;
int	src;
{
	dest[0] = (u_char)(src >> 8);
	dest[1] = (u_char)src;
}
static void
toint(dest, src)
u_char	*dest;
int	src;
{
	dest[0] = (u_char)(src >> 24);
	dest[1] = (u_char)(src >> 16);
	dest[2] = (u_char)(src >> 8);
	dest[3] = (u_char)src;
}
#ifdef __sgi
/*	I P U _ A C Q U I R E
 *
 *	Wait for the IPU to finish what it was doing.  Exit program if IPU
 *	is unavailable.
 */
void
ipu_acquire(dsp, timeout)
struct dsreq *dsp;
int timeout;
{
	int i=0;
	int status=0;
	u_char buf[48];
	char *p;

	if (ipu_debug) fprintf(stderr, "ipu_acquire(%d)\n", timeout);

	while (testunitready00(dsp) && i < timeout)
		if (RET(dsp)==DSRT_NOSEL) {
			fprintf(stderr, "IPU not responding\n");
			exit(-1);
		} else {
			/* IPU busy */
			if (ipu_debug)
				fprintf(stderr, "ipu_acquire() sleeping at %d\r", i);

			sleep(5);
			i += 5;
		}


	if (ipu_debug)
		fprintf(stderr, "\n");

	if (i >= timeout) {
		fprintf(stderr, "IPU timeout after %d seconds\n", i);
		exit(-1);
	}

	/* now let's make sure we're talking to a "CANON IPU-" somthing */

	bzero(p=CMDBUF(dsp), 16);
	bzero(buf, sizeof(buf));
	p[0] = 0x12;
	p[4] = (u_char)sizeof(buf);
	CMDLEN(dsp) = 6;
	filldsreq(dsp, buf, sizeof(buf), DSRQ_READ|DSRQ_SENSE);

	if (!doscsireq(getfd(dsp), dsp) &&
	    bcmp(&buf[8], "CANON   IPU-", 12)) {
		fprintf(stderr,
			"ipu_inquire() device is not IPU-10/CLC500!\n%s\n",
			&buf[8]);
		exit(-1);
	}
}


/*	I P U _ I N Q U I R E
 *
 *	returns
 *		static string describing device
 */
char *
ipu_inquire(dsp)
struct dsreq *dsp;
{
	static char response[64];
	u_char buf[36];
	char *p;

	if (ipu_debug) fprintf(stderr, "inquire()\n");


	/* up to 11 characters of status */
	if (testunitready00(dsp) != 0) {
		if (RET(dsp)==DSRT_NOSEL)
			strcpy(response, "no response from device");
		else
			strcpy(response, "device not ready");
		return response;
	}

	bzero(p=CMDBUF(dsp), 16);
	p[0] = 0x12;
	p[4] = (u_char)sizeof(buf);
	CMDLEN(dsp) = 6;
	filldsreq(dsp, buf, sizeof(buf), DSRQ_READ|DSRQ_SENSE);

	bzero(response, sizeof(response));
	if (!doscsireq(getfd(dsp), dsp)) {
		/* 32 characters */
		(void)sprintf(response,
			"ISO Ver(%1u)  ECMA Ver(%1u) ANSI(%1u) ",
			buf[2] >> 6,
			(buf[2]>>3)&0x07,
			buf[2] & 0x07);

		/* 20 characters of Vendor/Product ID */
		bcopy(&buf[8], &response[strlen(response)], 20);
		if (bcmp(&buf[8], "CANON   IPU-", 12)) {
		    fprintf(stderr,
			"ipu_inquire() device is not IPU-10/CLC500!\n%s\n",
			response);
		    exit(-1);
		}
	}

	return response;
}

/*	I P U _ R E M O T E
 *
 *	Puts copier in "remote" mode.  In this mode copier functions
 *	are fully controllable by the IPU.
 *
 *	Can be used to "poll" printer done status.
 */
int
ipu_remote(dsp)
struct dsreq *dsp;
{
	register char *p;
	int i;

	if (ipu_debug) fprintf(stderr, "ipu_remote()\n");

	bzero(p=CMDBUF(dsp), 16);
	p[0] = 0xc7;

	CMDLEN(dsp) = 12;
	
	filldsreq(dsp, (u_char *)NULL, 0, DSRQ_SENSE);

	if (i=doscsireq(getfd(dsp), dsp)) {
		if (ipu_debug) fprintf(stderr, "ipu_remote failure %d\n", i);
		return -1;
	}
	return 0;
}

int ipu_ready(dsp)
struct dsreq *dsp;
{
	return(testunitready00(dsp));
}



/*	I P U _ C R E A T E _ F I L E
 *
 *	create a file in the IPU
 *
 *	Parameters
 *
 *	id	file id	(1-16 for palette/RGB files, 0 for bit-map)
 *	type	{ IPU_RGB_FILE | IPU_BITMAP_FILE | IPU_PALETTE_FILE }
 *	width	file image width
 *	height	file image height
 */
void
ipu_create_file(dsp, id, type, width, height, clear)
struct dsreq *dsp;
u_char id;		/* file identifier */
u_char type;		/* file type */
int width, height;
char clear;		/* boolean: clear file memory */
{
	char *p;
	u_char file_params[8];

	if (ipu_debug)
		fprintf(stderr, "ipu_create_file(%d, t=%d %dx%d, %d)\n",
			id, type, width, height, clear);

	bzero(p=CMDBUF(dsp), 16);
	p[0] = 0xc4;
	p[9] = 8;
	CMDLEN(dsp) = 12;

	bzero(file_params, sizeof(file_params));
	file_params[0] = (u_char)id;
	file_params[1] = type;
	if (!clear) file_params[2] = 0x80;
	toshort(&file_params[4], width);
	toshort(&file_params[6], height);

	filldsreq(dsp, file_params, sizeof(file_params),
		DSRQ_WRITE|DSRQ_SENSE);

	if ( doscsireq(getfd(dsp), dsp) )  {
		fprintf(stderr, "create_file(%d, %d by %d failed\n",
			(int)id, width, height);
		exit(-1);
	}
}

/*	I P U _ D E L E T E _ F I L E
 *
 *	Parameters
 *
 *	id	id of file to delete
 */
void
ipu_delete_file(dsp, id)
struct dsreq *dsp;
u_char id;
{
	register char *p;
	static short ids;

	if (ipu_debug) fprintf(stderr, "ipu_delete_file(%d)\n", id);

	bzero(p=CMDBUF(dsp), 16);
	p[0] = 0xc5;
	p[9] = 2;
	CMDLEN(dsp) = 12;

	ids = (id << 8) + id;

	filldsreq(dsp, (u_char *)&ids, 2, DSRQ_WRITE|DSRQ_SENSE);

	if (doscsireq(getfd(dsp), dsp)) {
		fprintf(stderr, "delete_file(%d)\n", id);
		exit(-1);;
	}
}

/*	I P U _ G E T _ I M A G E
 *
 *	copy image from IPU to host computer
 */
u_char *
ipu_get_image(dsp, id, sx, sy, w, h)
struct dsreq *dsp;
char id;	/* file  id */
int sx, sy;	/* upper left corner of image */
int w, h;	/* width/height of image portion to retrieve */
{
	register u_char *p;
	int size;

	if (ipu_debug) fprintf(stderr, "ipu_get_image()\n");

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0xc3;
	p[2] = id;
	CMDLEN(dsp) = 12;

	toshort(&p[3], sx);
	toshort(&p[5], sy);
	toshort(&p[7], w);
	toshort(&p[9], h);

	size = w * h * 3;

	if ((p = malloc( size )) == NULL) {
		fprintf(stderr, "malloc error\n");
		exit(-1);
	}

	filldsreq(dsp, p, size, DSRQ_READ|DSRQ_SENSE);
	if ( doscsireq(getfd(dsp), dsp) )  {
		fprintf(stderr, "get_image(%d, %d,%d, %d,%d) failed\n",
			(int)id, sx, sy, w, h);
		exit(-1);
	}

	return(p);
}



/*	I P U _ P U T _ I M A G E
 *
 *	load a BRLCAD PIX(5) image into an IPU file
 *
 *	Parameters
 *
 *	id	file id
 *	w	image width
 *	h	image height
 *	img	pointer to image data
 */
void
ipu_put_image(dsp, id, w, h, img)
struct dsreq *dsp;
char id;
int w, h;
u_char *img;
{
	u_char *ipubuf, *p;
	int saved_debug;

	int bytes_per_line, lines_per_buf, bytes_per_buf, img_line;
	int buf_no, buf_line, orphan_lines, fullbuffers, pixel, ip;
	u_char *scanline, *red, *grn, *blu, *r, *g, *b;
	struct timeval tp1, tp2;
	struct timezone tz;
	FILE *fd;

	saved_debug = dsdebug;
	dsdebug = 0;

	if (ipu_debug) {
		fprintf(stderr, "ipu_put_image(id:%d, w:%d  h:%d)\n",
			id, w, h);
		if (dsdebug)
			fd = fopen("put_image", "w");
	}
	bytes_per_line = w * 3;
	lines_per_buf = BUFSIZE / bytes_per_line;
	bytes_per_buf = bytes_per_line * lines_per_buf;

	if ((ipubuf = malloc(bytes_per_buf)) == NULL) {
		fprintf(stderr, "malloc error in ipu_put_image()\n");
		exit(-1);
	}

	fullbuffers = h / lines_per_buf;
	orphan_lines = h % lines_per_buf;

	red = &ipubuf[0];
	grn = &ipubuf[lines_per_buf*w];
       	blu = &ipubuf[lines_per_buf*w*2];

	if (ipu_debug) {
		fprintf(stderr, "%d buffers of %d lines, 1 buffer of %d lines\n",
			fullbuffers, lines_per_buf, orphan_lines);
		bzero(&tp1, sizeof(struct timeval));
		bzero(&tp2, sizeof(struct timeval));
		if (gettimeofday(&tp1, &tz))
			perror("gettimeofday()");
	}

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0xc2;			/* put image */
	p[2] = id;			/* file id */
	toshort(&p[7], w);		/* ipu img width */
	toshort(&p[9], lines_per_buf);	/* ipu img height */
	CMDLEN(dsp) = 12;

	img_line = 0;
	for (buf_no=0 ; buf_no < fullbuffers ; buf_no++) {
		/* fill a full buffer */
		for (buf_line = lines_per_buf ; buf_line-- > 0 ; img_line++) {
			/* move img_line to buf_line */
			scanline = &img[img_line*bytes_per_line];

			r = & red[buf_line*w];
			g = & grn[buf_line*w];
			b = & blu[buf_line*w];

			for (pixel=0,ip=0 ; pixel < w ; pixel++ ) {
				r[pixel] = scanline[ip++];
				g[pixel] = scanline[ip++];
				b[pixel] = scanline[ip++];
			}
		}


		/* send buffer to IPU */
		toshort(&p[5], h-img_line);	/* sy */
		filldsreq(dsp, ipubuf, bytes_per_buf, DSRQ_WRITE|DSRQ_SENSE);

		if (dsdebug)
			fwrite(ipubuf, bytes_per_buf, 1, fd);

		if ( doscsireq(getfd(dsp), dsp) )  {
			fprintf(stderr,
			    "\nput_image(%d, %d,%d) buffer %d failed\n",
			    (int)id, w, h, buf_no);
			exit(-1);
		} else if ( ipu_debug )
			fprintf(stderr, "buffer %d of %d\r", buf_no, fullbuffers);
	}


	if (orphan_lines) {
		grn = & ipubuf[orphan_lines*w];
		blu = & ipubuf[orphan_lines*w*2];

		if (ipu_debug)
			fprintf(stderr, "\nDoing %d orphans (img_line %d)\n",
				orphan_lines, img_line);

		for (buf_line = orphan_lines ; buf_line-- > 0 ; img_line++) {
			scanline = &img[img_line*bytes_per_line];
			r = & red[buf_line*w];
			g = & grn[buf_line*w];
			b = & blu[buf_line*w];

			for (pixel=0 ; pixel < w ; pixel++ ) {
				r[pixel] = scanline[pixel*3];
				g[pixel] = scanline[pixel*3+1];
				b[pixel] = scanline[pixel*3+2];
			}
		}

		/* send buffer to IPU
		 * y offset is implicitly 0
		 */
		bzero(p=(u_char *)CMDBUF(dsp), 16);
		p[0] = 0xc2;			/* put image */
		p[2] = id;			/* file id */
		toshort(&p[7], w);		/* ipu img width */
		p[10] = orphan_lines;		/* ipu img height */
		CMDLEN(dsp) = 12;


		filldsreq(dsp, ipubuf, orphan_lines*bytes_per_line,
			DSRQ_WRITE|DSRQ_SENSE);

		if (dsdebug)
			fwrite(ipubuf, orphan_lines*bytes_per_line, 1, fd);

		if ( doscsireq(getfd(dsp), dsp) )  {
			fprintf(stderr,
			    "\nipu_put_image(%d, %d,%d) orphan_lines failed\n",
			    (int)id, w, h);
			exit(-1);
		} else if ( ipu_debug )
			fprintf(stderr, "orphan_lines written\n");
	}

	if (tp1.tv_sec && ipu_debug) {
		if (gettimeofday(&tp2, &tz))
			perror("gettimeofday()");
		else
			fprintf(stderr, "image transferred in %ld sec.\n",
				tp2.tv_sec - tp1.tv_sec);
	}

	if (dsdebug)
		fclose(fd);
	free(ipubuf);
	dsdebug = saved_debug;
}

static unsigned char ipu_units[] = {
	0x23, /* Page Code print units */
	6,	/* Parameter Length */
	IPU_UNITS_INCH,
	0,	/* Reserved */
	1,	/* divisor MSB */
	0x90,	/* divisor LSB */
	0,	/* Reserved */
	0	/* Reserved */
};

static unsigned char pr_mode[12] = {
	0x25,	/* Page Code "Print Mode Parameters" */
	0x0a,	/* Parameter Length */
	0,	/* Size Conv */
	0,	/* Repeat mode */
	0,	/* Gamma Compensation type */
	0,	/* Paper tray selection */
	0, 0,	/* Upper/Lower tray paper codes */
	0, 0, 0, 0	/* Reserved */
};

/*	I P U _ P R I N T _ C O N F I G
 *
 *	Set configuration parameters for printer
 *
 *	units	IPU_UNITS_INCH | IPU_UNITS_MM
 *	divisor	1 10 100 132 400
 *	conv	IPU_AUTOSCALE IPU_AUTOSCALE_IND IPU_MAG_FACTOR IPU_RESOLUTION
 *	mosaic	boolean: perform mosaic
 *	gamma	style { IPU_GAMMA_STANDARD | IPU_GAMMA_RGB | IPU_GAMMA_CG }
 *	tray	tray selection
 */
void
ipu_print_config(dsp, units, divisor, conv, mosaic, gamma, tray)
struct dsreq *dsp;
char units;
int divisor;
u_char conv, mosaic, gamma;
int tray;
{
	register u_char *p;
	u_char params[255];
	int bytes;
	int save;

	if (ipu_debug)
		fprintf(stderr,
		"ipu_print_config(0x%0x, 0x%0x, 0x%0x, 0x%0x, 0x%0x, 0x%0x)\n",
		units, divisor, conv, mosaic, gamma, tray);

	save = dsdebug;

	pr_mode[2] = conv;
	if (mosaic) pr_mode[3] = 1;
	else pr_mode[3] = 0;
	pr_mode[4] = gamma;
	pr_mode[5] = (u_char)tray;

	ipu_units[2] = 0x23;	/* print mode parameters */
	ipu_units[2] = units;
	toshort(&ipu_units[4], divisor);

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0x15;
	p[1] = 0x10;
	CMDLEN(dsp) = 6;

	bzero(params, sizeof(params));
	bytes = 4;	/* leave room for the mode parameter header */

	bcopy(ipu_units, &params[bytes], sizeof(ipu_units));
	bytes += sizeof(ipu_units);

	bcopy(pr_mode, &params[bytes], sizeof(pr_mode));
	bytes += sizeof(pr_mode);

	p[4] = (u_char)bytes;
	params[3] = (u_char)(bytes - 4);

	filldsreq(dsp, params, bytes, DSRQ_WRITE|DSRQ_SENSE);

	if ( doscsireq(getfd(dsp), dsp) )  {
		fprintf(stderr, "error doing print config\n");
		exit(-1);
	}
	dsdebug = save;

}

/*	I P U _ P R I N T _ F I L E
 *
 *	id	IPU file id of image to print
 *	copies	# of prints of file
 *	wait	sync/async printing
 */
void
ipu_print_file(dsp, id, copies, wait, sx, sy, sw, sh, pr_param)
struct dsreq *dsp;
char id;
int copies, wait, sx, sy, sw, sh;
union ipu_prsc_param *pr_param;
{
	register u_char *p;
	char buf[18];

	if (ipu_debug) fprintf(stderr,
	"ipu_print_file(id=%d copies=%d wait=%d sx=%d sy=%d sw=%d sh=%d\n\
	0x%x 0x%x 0x%x 0x%x)\n", id, copies, wait, sx, sy, sw, sh,
	pr_param->c[0],pr_param->c[1],pr_param->c[2],pr_param->c[3]);
	                                        

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0xc1;
	toint(&p[6], sizeof(buf));
	CMDLEN(dsp) = 12;

	bzero(buf, sizeof(buf));
	buf[0] = id;
	if (copies < 0) {
		fprintf(stderr, "Cannot print %d copies\n", copies);
		exit(-1);
	} else if (copies > 99) {
		fprintf(stderr, "Cannot print more than 99 copies at once\n");
		exit(-1);
	}
	toshort(&buf[1], copies);
	if (!wait) buf[3] = 0x080;	/* quick return */
	toshort(&buf[6], sx);
	toshort(&buf[8], sy);
	toshort(&buf[10], sw);
	toshort(&buf[12], sh);
	bcopy(pr_param, &buf[14], 4);

	filldsreq(dsp, (u_char *)buf, sizeof(buf), DSRQ_WRITE|DSRQ_SENSE);
	if ( doscsireq(getfd(dsp), dsp) )  {
		fprintf(stderr, "error printing file %d\n", id);
		exit(-1);
	}
}
static unsigned char sc_mode[6] = {
	0x24,	/* Page Code "Scan Mode Parameters" */
	0x04,	/* Parameter Length */
	0,	/* Size Conv */
	0,	/* Scan */
	0, 0	/* Analog Input Rotation */
};
/*	I P U _ S C A N _ C O N F I G
 *
 *	Set configuration parameters for scanning
 *
 *	units	IPU_UNITS_INCH | IPU_UNITS_MM
 *	divisor	1 10 100 132 400
 *	conv	IPU_AUTOSCALE IPU_AUTOSCALE_IND IPU_MAG_FACTOR IPU_RESOLUTION
 *	field	boolean:  Frame scan / Field scan
 *	rotation	angle of image rotation (multiple of 90 degrees)
 */
void
ipu_scan_config(dsp, units, divisor, conv, field, rotation)
struct dsreq *dsp;
char units;
int divisor;
char conv;
char field;
short rotation;
{
	register u_char *p;
	u_char params[255];
	int bytes;

	if (ipu_debug) fprintf(stderr, "ipu_scan_config()\n");

	ipu_units[2] = 0x22;	/* scan mode parameters */
	ipu_units[2] = units;
	toshort(&ipu_units[4], divisor);

	sc_mode[2] = conv;
	if (field) sc_mode[3] = 1;
	else sc_mode[3] = 0;

	rotation = rotation % 360;
	if (rotation != 0 && rotation != 90 && rotation != 180 &&
	    rotation != 270) {
		fprintf(stderr,
		    "image rotation in 90 degree increments only\n");
		exit(-1);
	}
	toshort(&sc_mode[4], rotation);

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0x15;
	p[1] = 0x10;
	CMDLEN(dsp) = 6;

	bzero(params, sizeof(params));
	bytes = 4;	/* leave room for the mode parameter header */

	bcopy(ipu_units, &params[bytes], sizeof(ipu_units));
	bytes += sizeof(ipu_units);

	bcopy(sc_mode, &params[bytes], sizeof(sc_mode));
	bytes += sizeof(sc_mode);

	p[4] = (u_char)bytes;
	params[3] = (u_char)(bytes - 4);

	filldsreq(dsp, params, bytes, DSRQ_WRITE|DSRQ_SENSE);

	if ( doscsireq(getfd(dsp), dsp) )  {
		fprintf(stderr, "error doing scan config\n");
		exit(-1);
	}
}

/*	I P U _ S C A N _ F I L E
 *
 *
 */
void
ipu_scan_file(dsp, id, wait, sx, sy, w, h, sc_param)
struct dsreq *dsp;
char id, wait;
int sx, sy, w, h;
union ipu_prsc_param *sc_param;
{
	register u_char *p;
	char buf[18];

	if (ipu_debug) fprintf(stderr,
		"ipu_scan_file(id=%d wait=%d sx=%d sy=%d sw=%d sh=%d\n\
		0x%x 0x%x 0x%x 0x%x)\n", id, wait, sx, sy, w, h,
		sc_param->c[0],sc_param->c[1],
		sc_param->c[2],sc_param->c[3]);

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0xc0;
	toint(&p[6], sizeof(buf));
	CMDLEN(dsp) = 12;

	bzero(buf, sizeof(buf));
	buf[0] = id;

	if (!wait) buf[3] = 0x080;	/* quick return */
	toshort(&buf[6], sx);
	toshort(&buf[8], sy);
	toshort(&buf[10], w);
	toshort(&buf[12], h);
	bcopy(sc_param, &buf[14], 4);

	filldsreq(dsp, (u_char *)buf, sizeof(buf), DSRQ_WRITE|DSRQ_SENSE);
	if ( doscsireq(getfd(dsp), dsp) )  {
		fprintf(stderr, "error scanning file %d\n", id);
		exit(-1);
	}
}

/*	I P U _ L I S T _ F I L E S
 *
 *	list files in the IPU
 *
 *	returns a null terminated, newline separated list of files and
 *		associated file dimensions.
 */
char *
ipu_list_files(dsp)
struct dsreq *dsp;
{
#define IPU_FILE_DESC_SIZE 16	/* # bytes for file description */
#define IPU_LFPH_LEN 8		/* List File Parameters Header Length */
	u_char *p;
	static char buf[IPU_LFPH_LEN + IPU_MAX_FILES*IPU_FILE_DESC_SIZE];
	int len;
	int i;
	int file_count;

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0xc6;
	toint(&p[6], sizeof(buf));
	CMDLEN(dsp) = 12;

	filldsreq(dsp, (u_char *)buf, sizeof(buf), DSRQ_READ|DSRQ_SENSE);

	if (doscsireq(getfd(dsp), dsp)) {
		fprintf(stderr, "error in ipu_list_files()\n");
		exit(-1);
	}

	len = ((int)buf[0]<<8) + buf[1] + 2;
	file_count = (len - 8) / buf[2];

	if ((p=malloc(file_count*19+1)) == NULL) {
		fprintf(stderr, "malloc error in ipu_list_files()\n");
		exit(-1);
	} else {
		bzero(p, file_count*19+1);
	}

	for (i = 8 ; i < len ; i += buf[2]) {
		register char t;
		if (buf[i+1] == 0) t = 'B';
		else if (buf[i+1] == 2) t = 'R';
		else if (buf[i+1] == 3) t = 'P';
		else t = '?';

		sprintf((char *)&p[strlen( (char *)p)], "%1x.%c\t%4dx%4d\n", 
			buf[i], t,
			((int)buf[i+2]<<8)+(int)buf[i+3],
			((int)buf[i+4]<<8)+(int)buf[i+5]);
	}

	return (char *)p;
}

/*	I P U _ S T O P
 *
 * Parameters
 *	dsp	SCSI interface ptr;
 *	halt	boolean:  Halt printing (or just report status)
 *
 * returns number of pages which have been printed
 */
int
ipu_stop(dsp, halt)
struct dsreq *dsp;
int halt;
{
	register char *p;
	char buf[18];

	if (ipu_debug) fprintf(stderr, "ipu_stop(%d)\n", halt);

	bzero(p=CMDBUF(dsp), 16);
	p[0] = 0xcc;	/* scan file */
	if (halt)
		p[2] = 0x80;
	p[9] = 18;	/* param list len */
	CMDLEN(dsp) = 12;

	bzero(buf, sizeof(buf));
	filldsreq(dsp, (u_char *)buf, sizeof(buf), DSRQ_READ|DSRQ_SENSE);

	if (doscsireq(getfd(dsp), dsp) == -1) {
		fprintf(stderr, "Error doing ipu_stop()\n");
		exit(-1);
	}

	return ((int)buf[1] << 8) + (int)buf[2];
}


int
ipu_get_conf(dsp)
struct dsreq *dsp;
{
	register u_char *p;
	u_char params[255];
	int i;

	if (ipu_debug) fprintf(stderr, "ipu_config_printer()\n");

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0x1a;			/* mode sense */
	p[2] = 0x23;
	p[4] = sizeof(params);
	CMDLEN(dsp) = 6;

	bzero(params, sizeof(params));
	filldsreq(dsp, params, sizeof(params), DSRQ_READ|DSRQ_SENSE);

	if (doscsireq(getfd(dsp), dsp) == -1) {
		fprintf(stderr, "Error reading IPU configuration\n");
		exit(-1);;
	}

	if (params[0] == 11 && params[4] == 0x23) {
		p = & params[4];
		
		switch (p[2]) {
		case IPU_UNITS_INCH	:fprintf(stderr, "Units=Inch  ");
					break;
		case IPU_UNITS_MM	:fprintf(stderr, "Units=mm  ");
					break;
		default			:fprintf(stderr, "Units=??  ");
					break;
		}

		i = ((int)p[4] << 8) + (int)p[5];
		fprintf(stderr, "Divisor=%d(0x%x)\n", i, i);
	}

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0x1a;			/* mode sense */
	p[2] = 0x25;
	p[4] = sizeof(params);
	CMDLEN(dsp) = 6;

	bzero(params, sizeof(params));
	filldsreq(dsp, params, sizeof(params), DSRQ_READ|DSRQ_SENSE);

	if (doscsireq(getfd(dsp), dsp) == -1) {
		fprintf(stderr, "Error reading IPU configuration\n");
		exit(-1);;
	}

	if (params[0] == 15 && params[4] == 0x25) {
		p = & params[4];
		switch (p[2]) {
		case IPU_AUTOSCALE : fprintf(stderr, "conv=Autoscale\  ");
					break;
		case IPU_AUTOSCALE_IND : fprintf(stderr, "conv=Autoscale_ind\  ");
					break;
		case IPU_MAG_FACTOR : fprintf(stderr, "conv=Mag Factor\  ");
					break;
		case IPU_RESOLUTION : fprintf(stderr, "conv=Resolution\  ");
					break;
		default	: fprintf(stderr, "conv=Unknown conv.\  ");
					break;
		}

		fprintf(stderr, "Repeat=%d\  ", p[3]);
		
		switch (p[4]) {
		case IPU_GAMMA_STANDARD: fprintf(stderr, "gamma=std  ");
					break;
		case IPU_GAMMA_RGB: fprintf(stderr, "gamma=rgb  ");
					break;
		case IPU_GAMMA_CG: fprintf(stderr, "gamma=cg  ");
					break;
		default: fprintf(stderr, "Unknown gamma  ");
					break;
		}

		switch (p[5]) {
		case IPU_UPPER_CASSETTE : fprintf(stderr, "tray=upper\n");
			break;
		case IPU_LOWER_CASSETTE : fprintf(stderr, "tray=lower\n");
			break;
		case IPU_MANUAL_FEED : fprintf(stderr, "tray=Manual_Feed\n");
			break;
		default: fprintf(stderr, "tray=Unknown (%d)\n", p[5]);
			break;
		}

		fprintf(stderr, "Upper cassette paper=");
		switch (p[6]) {
		case 0x6: fputs("A3  ", stderr); break;
		case 0x8: fputs("A4R  ", stderr); break;
		case 0x18: fputs("B4  ", stderr); break;
		case 0x1a: fputs("B5R  ", stderr); break;
		case 0x28: fputs("A4  ", stderr); break;
		case 0x46: fputs("11\"x17\"  ", stderr); break;
		case 0x48: fputs("Letter R  ", stderr); break;
		case 0x58: fputs("Legal  ", stderr); break;
		case 0x68: fputs("Letter  ", stderr); break;
		case 0x80: fputs("Manual Feed  ", stderr); break;
		case 0xff: fputs("No cassette  ", stderr); break;
		default : fputs("unknown  ", stderr); break;
		}
		fprintf(stderr, "Lower cassette paper=");
		switch (p[7]) {
		case 0x6: fputs("A3\n", stderr); break;
		case 0x8: fputs("A4R\n", stderr); break;
		case 0x18: fputs("B4\n", stderr); break;
		case 0x1a: fputs("B5R\n", stderr); break;
		case 0x28: fputs("A4\n", stderr); break;
		case 0x46: fputs("11\"x17\"\n", stderr); break;
		case 0x48: fputs("Letter R\n", stderr); break;
		case 0x58: fputs("Legal\n", stderr); break;
		case 0x68: fputs("Letter\n", stderr); break;
		case 0x80: fputs("Manual Feed\n", stderr); break;
		case 0xff: fputs("No cassette\n", stderr); break;
		default : fputs("unknown\n", stderr); break;
		}
	}
}
int
ipu_get_conf_long(dsp)
struct dsreq *dsp;
{
	register u_char *p;
	static u_char params[65535];

	if (ipu_debug) fprintf(stderr, "ipu_config_printer()\n");

	bzero(p=(u_char *)CMDBUF(dsp), 16);
	p[0] = 0x5a;			/* mode sense */
	p[2] = 0x23;
	toint(&p[7], sizeof(params));
	CMDLEN(dsp) = 10;

	bzero(params, sizeof(params));
	filldsreq(dsp, params, sizeof(params), DSRQ_READ|DSRQ_SENSE);

	if (doscsireq(getfd(dsp), dsp) == -1) {
		fprintf(stderr, "Error reading IPU configuration\n");
		exit(-1);;
	}

	(void)fprintf(stderr, "Sense Data Length %u\n",
		((unsigned)params[0] << 8) + params[1]);

	(void)fprintf(stderr, "Block Descriptor Length %u\n",
		((unsigned)params[6] << 8) + params[7]);
}

#endif /* __sgi__ */

char *options = "P:p:Q:q:acd:g:hmn:s:t:vw:zAC:M:R:D:N:S:W:X:Y:U:V";
extern char *optarg;
extern int optind, opterr, getopt();

char *progname = "(noname)";
char *scsi_device = "/dev/scsi/sc0d6l3";
char gamma = IPU_GAMMA_CG;
char tray = IPU_UPPER_CASSETTE;
char conv = IPU_AUTOSCALE;
char clear = 0;
int width = 512;
int height = 512;
int zoom = 0;
int scr_width;
int scr_height;
int scr_xoff;
int scr_yoff;
int copies = 1;
int autosize = 0;
int units = IPU_UNITS_INCH;
int divisor = 0x190;
int mosaic = 0;

union ipu_prsc_param param;

#define MAX_ARGS	64
static char arg_buf[1024];
static int  len;
char	*arg_v[MAX_ARGS];
int	arg_c;
char *print_queue = "canon";

/*
 *	U S A G E --- tell user how to invoke this program, then exit
 */
void usage(s)
char *s;
{
	if (s) (void)fputs(s, stderr);

	(void) fprintf(stderr, "Usage: %s [options] [file]\n%s", progname, 
"	[-h] [-n scanlines] [-w width] [-s squareimagesize]\n\
	[-N outputheight] [-W outputwidth] [-S outputsquaresize]\n\
	[-X PageXOffset] [-Y PageYOffset]\n\
	[-a(utosize_input)] [-g(amma) { s | r | c }]\n\
	[-z(oom)] [-C(opies) {1-99}] [-m(osaic)]\n\
	[ -A(utoscale_output) | -M xmag:ymag | -R dpi ]\n\
	[-u(nits) { i|m } ] [-D divisor]\n\
	[-c(lear)] [-d SCSI_device] [-v(erbose)] [-V(erboser)]\n\
	[-q queue] [-p printer]\n");

	exit(1);
}

/*
 *	P A R S E _ A R G S --- Parse through command line flags
 */
int parse_args(ac, av)
int ac;
char *av[];
{
	int  c;
	char *p;

	if (  ! (progname=strrchr(*av, '/'))  )
		progname = *av;
	else
		++progname;

	strcpy(arg_buf, progname);
	len = strlen(arg_buf) + 1;
	arg_v[arg_c = 0] = arg_buf;
	arg_v[++arg_c] = (char *)NULL;

	/* Turn off getopt's error messages */
	opterr = 0;

	/* get all the option flags from the command line */
	while ((c=getopt(ac,av,options)) != EOF) {
		/* slup off a printer queue name */
		if (c == 'q' ||  c == 'p') {
			print_queue = optarg;
			continue;
		}

		/* add option & arg to arg_v */
		if (p=strchr(options, c)) {
			arg_v[arg_c++] = &arg_buf[len];
			arg_v[arg_c] = (char *)NULL;
			(void)sprintf(&arg_buf[len], "-%c", c);
			len += strlen(&arg_buf[len]) + 1;
			if (p[1] == ':') {
				arg_v[arg_c++] = &arg_buf[len];
				arg_v[arg_c] = (char *)NULL;
				(void)sprintf(&arg_buf[len], "%s", optarg);
				len += strlen(&arg_buf[len]) + 1;
			}
		}

		switch (c) {
		case 'a'	: autosize = !autosize; break;
		case 'c'	: clear = !clear; break;
		case 'd'	: if (isprint(*optarg))
					scsi_device = optarg;
				  else
				  	usage("-d scsi_device_name\n");
				break;
		case 'g'	: if (*optarg == 's')
					gamma = IPU_GAMMA_STANDARD;
				   else if (*optarg == 'r')
					gamma = IPU_GAMMA_RGB;
				   else if (*optarg == 'c')
					gamma = IPU_GAMMA_CG;
				   else
				   	usage("-g {std|rgb|cg}\n");
				break;
		case 'h'	: width = height = 1024; break;
		case 'm'	: mosaic = !mosaic; break;
		case 'n'	: if ((c=atoi(optarg)) > 0)
					height = c;
				  else
				  	usage("-n scanlines\n");
				  break;
		case 's'	: if ((c=atoi(optarg)) > 0)
					width = height = c;
				  else
				  	usage("-s squareimagesize\n");
				  break;
		case 'w'	: if ((c=atoi(optarg)) > 0)
					width = c;
				  else
				  	usage("-w imagewidth\n");
				  break;
		case 't'	:switch (*optarg) {
				case 'u'	:
				case 'U'	: tray = IPU_UPPER_CASSETTE;
						break;
				case 'l'	:
				case 'L'	: tray = IPU_LOWER_CASSETTE;
						break;
				case 'm'	:
				case 'M'	: tray = IPU_MANUAL_FEED;
						break;
				default:
					usage("-t {u|l|m}\n");
					break;
				}
				break;
		case 'U'	: if (*optarg == 'i')
					units = IPU_UNITS_INCH;
				  else if (*optarg == 'm')
				  	units = IPU_UNITS_MM;
				  else
				  	usage("invalid units\n");
				break;
		case 'z'	: zoom = !zoom;
				scr_width= scr_height= scr_xoff= scr_yoff= 0;
				break;
		case 'A'	: conv = IPU_AUTOSCALE;
				  bzero(&param, sizeof(union ipu_prsc_param));
				  break;
		case 'M'	: conv = IPU_MAG_FACTOR;
				  if ((c=atoi(optarg)) < 100 || c > 2000)
				  	usage("X Mag factor out of range 100-2000\n");
				  param.s[0] = c & 0x0ffff;
				  while (*optarg && *optarg++ != ':')
					;
				  if ((c = atoi(optarg)) < 100 || c > 2000)
				  	usage("Y Mag factor out of range 100-2000\n");
				  param.s[1] = c & 0x0ffff;
				  break;
		case 'R'	: if ((c=atoi(optarg)) > 0) {
					param.i = c;
					conv = IPU_RESOLUTION;
				} else {
					fprintf(stderr,
						"Resolution error (%d)\n",c);
					exit(-1);
				}
				if (ipu_debug)
					fprintf(stderr,
					"Res: %d 0%02x 0%02x 0%02x 0%02x\n",
					c, param.c[0], param.c[1], param.c[2],
					param.c[3]);
				break;
		case 'C'	: if ((c=atoi(optarg)) > 0 && c < 99)
					copies = c;
				else
					usage("-C [1-99]\n");
				break;
		case 'D'	: if ((c=atoi(optarg)) > 0)
					divisor = c;
				else
					usage("-D divisor\n");
				break;
		case 'N'	: if ((c=atoi(optarg)) > 0)
					scr_height = c;
				else
					usage("-N outputlines\n");
				break;
		case 'S'	: if ((c=atoi(optarg)) > 0)
					scr_width = scr_height = c;
				else
					usage("-S outputsquaresize\n");
				break;
		case 'W'	: if ((c=atoi(optarg)) > 0)
					scr_width = c;
				else
					usage("-W outputwidth\n");
				break;
		case 'X'	: if ((c=atoi(optarg)) >= 0)
					scr_xoff = c;
				else
					usage("-X pageXoffset\n");
				break;
		case 'Y'	: if ((c=atoi(optarg)) >= 0)
					scr_yoff = c;
				else
					usage("-X pageYoffset\n");
				break;
		case 'V'	: dsdebug = ! dsdebug;
		case 'v'	: ipu_debug = !ipu_debug; break;
		case '?'	:
		default		: fprintf(stderr, "Bad or help flag specified '%c'\n", c); break;
		}
	}
	return(optind);
}
