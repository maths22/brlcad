/*
 *			P I X - I P U . C
 *  Author -
 *	Lee A. Butler
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1992 by the United States Army.
 *	All rights reserved.
 *
 *	Options
 *	a	autosize image file
 *	c	clear framebuffer first
 *	d	SCSI device
 *	g	gamma
 *	h	1Kx1K
 *	m	mosaic
 *	n	scanlines (image)
 *	s	squaresize (image)
 *	w	width (image)
 *	x	file_xoffset
 *	y	file_yoffset
 *	z	zoom image display
 *	A	Autoscale
 *	M	Mag_factor
 *	R	Resolution
 *	C	# copies
 *	D	Divisor
 *	N	scr_height
 *	S	scr_height
 *	U	units ( i | m )
 *	W	scr_width
 *	X	scr_xoffset
 *	Y	scr_yoffset
 *	v	verbose;
 *	V	verbose;
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __sgi
#include "./canon.h"

/*
 *	M A I N
 *
 *	Call parse_args to handle command line arguments first, then
 *	process input.
 */
int main(ac,av)
int ac;
char *av[];
{
	int arg_index;
	struct dsreq *dsp;
	u_char	*img_buffer = (u_char *)NULL;
	int	img_bytes;
	FILE	*image_file;
	int i;

	if ((arg_index = parse_args(ac, av)) >= ac) {
		if (isatty(fileno(stdin)))
			usage("Specify image on cmd line or redirect from standard input\n");

		if (autosize) fprintf(stderr, "Cannot autosize stdin\n");

	} else if (arg_index+1 < ac)
		(void)fprintf(stderr,
			"%s: Excess command line arguments ignored\n", *av);
	else if (freopen(av[arg_index], "r", stdin) == NULL) {
		perror(av[arg_index]);
		return(-1);
	} else if (autosize &&
	    !fb_common_file_size( &width, &height, av[arg_index], 3)) {
	    	fprintf(stderr, "unable to autosize\n");
	}

	/* get a buffer for the image */
	img_bytes = width * height * 3;

	if ( ! (img_buffer=(u_char*)malloc(img_bytes)) ) {
		(void)fprintf(stderr,
			"Cannot allocate memory for %d by %d image\n",
			width, height);
		return(-1);
	}

	/* open the printer SCSI device */
	if ((dsp = dsopen(scsi_device, O_RDWR)) == NULL) {
		perror(scsi_device);
		usage("Cannot open SCSI device\n");
	}

	if (ipu_debug)
		fprintf(stderr, "Image is %dx%d (%d)\n", width, height, img_bytes);

	/* bring the image into memory */
	if ((i=fread(&img_buffer[0], 1, img_bytes, stdin)) != img_bytes) {
		(void)fprintf(stderr, "%s: Error reading image %d of %d bytes read\n", progname, i, img_bytes);
		return(-1);
	}

	if (conv == IPU_RESOLUTION) {
		if (scr_width)
			scr_width *= 400.0 / (double)param.i;
		else
			scr_width = width * 400.0 / (double)param.i;
		if (scr_height)
			scr_height *= 400.0 / (double)param.i;
		else
			scr_height = width * 400.0 / (double)param.i;
	} else if (conv == IPU_MAG_FACTOR) {
		if (scr_width)
			scr_width *= 400.0 / (double)param.i;
		else
			scr_width = width * 400.0 / (double)param.i;
		if (scr_height)
			scr_height *= 400.0 / (double)param.i;
		else
			scr_height = width * 400.0 / (double)param.i;
	}

	/* Wait for printer to finish what it was doing */
	ipu_acquire(dsp, 120);

	ipu_delete_file(dsp, 1);
	ipu_create_file(dsp, (char)1, IPU_RGB_FILE, width, height, 0);
	ipu_put_image(dsp, (char)1, width, height, img_buffer);

	ipu_print_config(dsp, units, divisor, conv,
			mosaic, gamma, tray);

	if (!strcmp(progname, "pix-ipu"))
		ipu_print_file(dsp, (char)1, copies, 0/*wait*/,
			scr_xoff, scr_yoff, scr_width, scr_height, &param);

	/* Wait for print operation to complete */
	ipu_acquire(dsp, 120);

	dsclose(dsp);
	return(0);
}

#else
int
main(ac, av)
int ac;
char *av[];
{
	fprintf(stderr,
		"%s only works on SGI(tm) systems with dslib support\n", *av);
	return(-1);
}
#endif
