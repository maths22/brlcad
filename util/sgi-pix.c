/*
 * From scrsave.c
 */

#ifdef sgi
# ifdef mips
#	include <gl/gl.h>
# else
#	include <gl.h>
# endif
#endif

#include <stdio.h>

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	ABS(a)		((a)<0? -(a):(a))

char rbuf[2048];
char gbuf[2048];
char bbuf[2048];
char obuf[2048*3];
struct cmap {
	unsigned char red;
	unsigned char grn;
	unsigned char blu;
} cmap[4096];

static char usage[] = "\
Usage: sgi-pix [x1 x2 y1 y2] [outfile]\n";

main(argc,argv)
int argc;
char **argv;
{
#ifndef sgi
	fprintf(stderr, "sgi-pix:  This program only works on SGI machines\n");
	exit(1);
#else
	int i, y, gotfirst;
	int x1, x2, y1, y2;
	int xsize, ysize;
	int xorg, yorg;
	FILE *ofp;
	char *fname;
	int mode, planes;

	if(!(argc==1 || argc==2 || argc==5 || argc==6)) {
		fprintf(stderr,usage);
		exit(1);
	}
	if( argc > 4 ) {
		x1 = atoi(argv[1]);
		x2 = atoi(argv[2]);
		y1 = atoi(argv[3]);
		y2 = atoi(argv[4]);
	} else {
		x1 = 0;
		x2 = XMAXSCREEN;
		y1 = 0;
		y2 = YMAXSCREEN;
	}
	if( argc == 2 ) {
		fname = argv[1];
		ofp = fopen(fname,"w");
	} else if( argc == 6 ) {
		fname = argv[5];
		ofp = fopen(fname,"w");
	} else {
		fname = "-";
		ofp = stdout;
	}
	if( ofp == NULL ) {
		fprintf(stderr,"sgi-pix: can't open \"%s\"\n", fname);
		exit(2);
	}
	if( isatty(fileno(ofp)) ) {
		fprintf(stderr,"sgi-pix: refuse to send binary output to terminal\n");
		fprintf(stderr,usage);
		exit(1);
	}

	/* Convert rectangle edges to origin and size */
	xorg = MIN(x1,x2);
	yorg = MIN(y1,y2);
	if(xorg<0)
		xorg = 0;
	if(yorg<0)
		yorg = 0;
	xsize = ABS(x2-x1);
	ysize = ABS(y2-y1);
	if((xorg+xsize)>XMAXSCREEN)
		xsize = XMAXSCREEN-xorg;
	if((yorg+ysize)>YMAXSCREEN)
		ysize = YMAXSCREEN-yorg;
	xsize++;
	ysize++;
	fprintf(stderr,"(%d %d)\n", xsize, ysize);

#ifdef mips
	foreground();
	noport();
	winopen("sgi-pix");
	savescreen(ofp,x1,x2,y1,y2);
#else
	gbegin();
	foreground();
	noport();
	winopen("sgi-pix");
	cursoff();

	if((mode = getdisplaymode()) == 0) {
		/* RGB mode */
		fprintf(stderr,"RGB mode\n");
		savescreen(ofp,x1,x2,y1,y2);
	} else {
		if( mode == 1 )
			fprintf(stderr,"CMAP mode (single buffered)\n");
		else  {
			fprintf(stderr,"CMAP mode (double buffered)\n");
			swapbuffers();
		}
		planes = getplanes();
		fprintf(stderr,"%d planes\n", planes);
		for( i = 0; i < 4096; i++ ) {
			short r,g,b;
			getmcolor( i, &r, &g, &b );
			cmap[i].red = r;
			cmap[i].grn = g;
			cmap[i].blu = b;
		}
		cmap_savescreen(ofp,x1,x2,y1,y2);
		if( mode != 1 )  {
			/* Double buffered mode, swap 'em back */
			swapbuffers();
		}
	}
#endif
	return(0);
#endif
}

#ifdef sgi
savescreen(ofp,xorg,yorg,xsize,ysize)
FILE	*ofp;
int	xorg,yorg,xsize,ysize;
{
	int y, i;
	int pos, togo, n;

	screenspace();

#if !defined(mips)
	/* 3D only */
	viewport(0,1023,0,767);
	ortho2(-0.5,1023.5,-0.5,767.5);
#endif

	for(y=0; y<ysize; y++) {
#ifdef mips
		/* Note that gl_readscreen() can only do 256 pixels! */
		togo = xsize;
		pos = 0;
		while(togo) {
			n = togo;
			if(n>256)
				n = 256;
			cmov2i(xorg+pos,yorg+y);
			gl_readscreen(n,rbuf+pos,gbuf+pos,bbuf+pos);
			pos += n;
			togo -= n;
		}
#else
		cmov2i(xorg,yorg+y);
		readRGB(n,rbuf,gbuf,bbuf);
#endif
		for( i = 0; i < xsize; i++ ) {
			obuf[3*i] = rbuf[i];
			obuf[3*i+1] = gbuf[i];
			obuf[3*i+2] = bbuf[i];
		}
		if( fwrite(obuf,3,xsize,ofp) != xsize )  {
			perror("fwrite");
			exit(2);
		}
	}
}

cmap_savescreen(ofp,xorg,yorg,xsize,ysize)
FILE	*ofp;
int	xorg,yorg,xsize,ysize;
{
	int y, i;
	int pos, togo, n;
	Colorindex buff[1024];

	screenspace();

	for(y=0; y<ysize; y++) {
		cmov2i(xorg,yorg+y);
		readpixels(xsize,buff);

		for( i = 0; i < xsize; i++ ) {
			obuf[i*3] = cmap[buff[i]].red;
			obuf[i*3+1] = cmap[buff[i]].grn;
			obuf[i*3+2] = cmap[buff[i]].blu;
		}
		if( fwrite(obuf,3,xsize,ofp) != xsize )  {
			perror("fwrite");
			exit(2);
		}
	}
}
#endif
