/*
 *			A D C . C
 *
 * Functions -
 *	adcursor	implement the angle/distance cursor
 *	f_adc		control angle/distance cursor from keyboard
 *
 * Authors -
 *	Gary Steven Moss
 *	Paul J. Tanenbaum
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./ged.h"
#include "./dm.h"

#ifndef M_SQRT2
#define M_SQRT2		1.41421356237309504880
#endif

/*
 * These variables are global for the benefit of
 * the display portion of dotitles.
 */
fastf_t	curs_x;		/* cursor X position */
fastf_t	curs_y;		/* cursor Y position */
fastf_t	c_tdist;		/* Cursor tick distance */
fastf_t	angle1;		/* Angle to solid wiper */
fastf_t	angle2;		/* Angle to dashed wiper */


/*
 *			A D C U R S O R
 *
 * Compute and display the angle/distance cursor.
 */
void
adcursor()
{
	static double	pi = 3.14159265358979323264;
	static fastf_t	x1, Y1;	/* not "y1", due to conflict with math lib */
	static fastf_t	x2, y2;
	static fastf_t	x3, y3;
	static fastf_t	x4, y4;
	static fastf_t	d1, d2;
	static fastf_t	t1, t2;
	static long	idxy[2];

	/*
	 * Calculate a-d cursor displacement.
	 */
#define MINVAL	-2048
#define MAXVAL	 2047
	idxy[0] = dm_values.dv_xadc;
	idxy[1] = dm_values.dv_yadc;
	idxy[0] = (idxy[0] < MINVAL ? MINVAL : idxy[0]);
	idxy[0] = (idxy[0] > MAXVAL ? MAXVAL : idxy[0]);
	idxy[1] = (idxy[1] < MINVAL ? MINVAL : idxy[1]);
	idxy[1] = (idxy[1] > MAXVAL ? MAXVAL : idxy[1]);

	dmp->dmr_2d_line( MINVAL, idxy[1], MAXVAL, idxy[1], 0 ); /* Horiz */
	dmp->dmr_2d_line( idxy[0], MAXVAL, idxy[0], MINVAL, 0);  /* Vert */

	curs_x = (fastf_t) (idxy[0]);
	curs_y = (fastf_t) (idxy[1]);

	/*
	 * Calculate a-d cursor rotation.
	 */
	/* - to make rotation match knob direction */
	idxy[0] = -dm_values.dv_1adc;	/* solid line */
	idxy[1] = -dm_values.dv_2adc;	/* dashed line */
	angle1 = ((2047.0 + (fastf_t) (idxy[0])) * pi) / (4.0 * 2047.0);
	angle2 = ((2047.0 + (fastf_t) (idxy[1])) * pi) / (4.0 * 2047.0);

	/* sin for X and cos for Y to reverse sense of knob */
	d1 = cos (angle1) * 8000.0;
	d2 = sin (angle1) * 8000.0;
	x1 = curs_x + d1;
	Y1 = curs_y + d2;
	x2 = curs_x - d1;
	y2 = curs_y - d2;
	(void)clip ( &x1, &Y1, &x2, &y2 );

	x3 = curs_x + d2;
	y3 = curs_y - d1;
	x4 = curs_x - d2;
	y4 = curs_y + d1;
	(void)clip ( &x3, &y3, &x4, &y4 );

	dmp->dmr_2d_line( (int)x1, (int)Y1, (int)x2, (int)y2, 0 );
	dmp->dmr_2d_line( (int)x3, (int)y3, (int)x4, (int)y4, 0 );

	d1 = cos (angle2) * 8000.0;
	d2 = sin (angle2) * 8000.0;
	x1 = curs_x + d1;
	Y1 = curs_y + d2;
	x2 = curs_x - d1;
	y2 = curs_y - d2;
	(void)clip ( &x1, &Y1, &x2, &y2 );

	x3 = curs_x + d2;
	y3 = curs_y - d1;
	x4 = curs_x - d2;
	y4 = curs_y + d1;
	(void)clip ( &x3, &y3, &x4, &y4 );

	/* Dashed lines */
	dmp->dmr_2d_line( (int)x1, (int)Y1, (int)x2, (int)y2, 1 );
	dmp->dmr_2d_line( (int)x3, (int)y3, (int)x4, (int)y4, 1 );

	/*
	 * Position tic marks from dial 9.
	 */
	/* map -2048 - 2047 into 0 - 4096 * sqrt (2) */
	/* Tick distance */
	c_tdist = ((fastf_t)(dm_values.dv_distadc) + 2047.0) * M_SQRT2;

	d1 = c_tdist * cos (angle1);
	d2 = c_tdist * sin (angle1);
	t1 = 20.0 * sin (angle1);
	t2 = 20.0 * cos (angle1);

	/* Quadrant 1 */
	x1 = curs_x + d1 + t1;
	Y1 = curs_y + d2 - t2;
	x2 = curs_x + d1 -t1;
	y2 = curs_y + d2 + t2;
	if (clip ( &x1, &Y1, &x2, &y2 ) == 0) {
		dmp->dmr_2d_line( (int)x1, (int)Y1, (int)x2, (int)y2, 0 );
	}

	/* Quadrant 2 */
	x1 = curs_x - d2 + t2;
	Y1 = curs_y + d1 + t1;
	x2 = curs_x - d2 - t2;
	y2 = curs_y + d1 - t1;
	if (clip (&x1, &Y1, &x2, &y2) == 0) {
		dmp->dmr_2d_line( (int)x1, (int)Y1, (int)x2, (int)y2, 0 );
	}

	/* Quadrant 3 */
	x1 = curs_x - d1 - t1;
	Y1 = curs_y - d2 + t2;
	x2 = curs_x - d1 + t1;
	y2 = curs_y - d2 - t2;
	if (clip (&x1, &Y1, &x2, &y2) == 0) {
		dmp->dmr_2d_line( (int)x1, (int)Y1, (int)x2, (int)y2, 0 );
	}

	/* Quadrant 4 */
	x1 = curs_x + d2 - t2;
	Y1 = curs_y - d1 - t1;
	x2 = curs_x + d2 + t2;
	y2 = curs_y - d1 + t1;
	if (clip (&x1, &Y1, &x2, &y2) == 0) {
		dmp->dmr_2d_line( (int)x1, (int)Y1, (int)x2, (int)y2, 0 );
	}
}

/*
 *			F _ A D C
 */

static char	adc_syntax[] = "\
 adc		toggle display of angle/distance cursor\n\
 adc a1 #	set angle1\n\
 adc a2 #	set angle2\n\
 adc dst #	set radius (distance) of tick\n\
 adc dh #	reposition horizontally\n\
 adc dv #	reposition vertically\n\
 adc dx #	reposition in X direction\n\
 adc dy #	reposition in Y direction\n\
 adc dz #	reposition in Z direction\n\
 adc hv # #	reposition (view coordinates)\n\
 adc xyz # # #	reposition in front of a point (model coordinates)\n\
 adc reset	reset angles, location, and tick distance\n\
";
void f_adc (argc, argv)
int	argc;
char	**argv;
{
	char	*parameter;
	point_t	center_model;
	point_t	pt;		/* Value(s) provided by user */
	point_t	pt2, pt3;	/* Extra points as needed */
	int	i;

	if (argc == 1)
	{
		if (adcflag)  {
			dmp->dmr_light( LIGHT_OFF, BV_ADCURSOR );
			adcflag = 0;
		} else {
			dmp->dmr_light( LIGHT_ON, BV_ADCURSOR );
			adcflag = 1;
		}
		dmaflag = 1;
		return;
	}

	parameter = argv[1];
	argc -= 2;
	for (i = 0; i < argc; ++i)
	    pt[i] = atof(argv[i + 2]);
	VSET(center_model,
	    -toViewcenter[MDX], -toViewcenter[MDY], -toViewcenter[MDZ]);

	if( strcmp( parameter, "a1" ) == 0 )  {
		if (argc == 1) {
		    dm_values.dv_1adc = (1.0 - pt[0] / 45.0) * 2047.0;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp( parameter, "a2" ) == 0 )  {
		if (argc == 1) {
		    dm_values.dv_2adc = (1.0 - pt[0] / 45.0) * 2047.0;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if(strcmp(parameter, "dst") == 0)  {
		if (argc == 1) {
		    dm_values.dv_distadc = (pt[0] / (Viewscale * base2local * M_SQRT2) -1.0)
			* 2047.0;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp( parameter, "dh" ) == 0 )  {
		if (argc == 1) {
		    dm_values.dv_xadc += pt[0] * 2047.0 / (Viewscale * base2local);
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp( parameter, "dv" ) == 0 )  {
		if (argc == 1) {
		    dm_values.dv_yadc += pt[0] * 2047.0 / (Viewscale * base2local);
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp( parameter, "dx" ) == 0 )  {
		if (argc == 1) {
		    VSET(pt2, pt[0], 0.0, 0.0);
		    VSCALE(pt2, pt2, local2base);
		    MAT4X3VEC(pt3, Viewrot, pt2);
		    dm_values.dv_xadc += pt3[X] * 2047.0 / Viewscale;
		    dm_values.dv_yadc += pt3[Y] * 2047.0 / Viewscale;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp( parameter, "dy" ) == 0 )  {
		if (argc == 1) {
		    VSET(pt2, 0.0, pt[0], 0.0);
		    VSCALE(pt2, pt2, local2base);
		    MAT4X3VEC(pt3, Viewrot, pt2);
		    dm_values.dv_xadc += pt3[X] * 2047.0 / Viewscale;
		    dm_values.dv_yadc += pt3[Y] * 2047.0 / Viewscale;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp( parameter, "dz" ) == 0 )  {
		if (argc == 1) {
		    VSET(pt2, 0.0, 0.0, pt[0]);
		    VSCALE(pt2, pt2, local2base);
		    MAT4X3VEC(pt3, Viewrot, pt2);
		    dm_values.dv_xadc += pt3[X] * 2047.0 / Viewscale;
		    dm_values.dv_yadc += pt3[Y] * 2047.0 / Viewscale;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp(parameter, "hv") == 0)  {
		if (argc == 2) {
		    MAT4X3VEC(pt2, Viewrot, center_model);
		    VSCALE(pt, pt, local2base);
		    VSUB2(pt3, pt, pt2);
		    dm_values.dv_xadc = pt3[X] * 2047.0 / Viewscale;
		    dm_values.dv_yadc = pt3[Y] * 2047.0 / Viewscale;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp(parameter, "xyz") == 0)  {
		if (argc == 3) {
		    VSCALE(pt, pt, local2base);
		    VSUB2(pt2, pt, center_model);
		    MAT4X3VEC(pt3, Viewrot, pt2);
		    dm_values.dv_xadc = pt3[X] * 2047.0 / Viewscale;
		    dm_values.dv_yadc = pt3[Y] * 2047.0 / Viewscale;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp(parameter, "reset") == 0)  {
		if (argc == 0) {
		    dm_values.dv_xadc = dm_values.dv_yadc = 0;
		    dm_values.dv_1adc = dm_values.dv_2adc = 0;
		    dm_values.dv_distadc = 0;
		    dm_values.dv_flagadc = 1;
		    return;
	}} else if( strcmp(parameter, "help") == 0)  {
		(void) printf("Usage:\n");
		(void) fputs(adc_syntax, stdout);
		return;
	} else  {
		(void) printf("ADC: unrecognized parameter: '%s'\n", argv[1]);
		(void) printf("valid parameters: %s\n",
			    "<a1|a2|dst|dh|dv|hv|dx|dy|dz|xyz|reset|help>\n");
		return;
	}

	(void) printf("ADC: invalid syntax for parameter '%s'\n",
	    parameter);
	(void) printf("usage is:\n");
	(void) fputs(adc_syntax, stdout);
}
