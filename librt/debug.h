/*
 *  			D E B U G . H
 *  
 *  Various definitions permitting segmented debugging to
 *  be independently controled.
 *  
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 *  
 *  $Revision$
 */
#define DEBUG_OFF	0	/* No debugging */

/* These definitions are each for one bit */
#define DEBUG_SOLIDS	0x01	/* Print prep'ed solids */
#define DEBUG_HITS	0x02	/* Print hits used by view() */
#define DEBUG_ALLRAYS	0x04	/* Print all attempted shots */
#define DEBUG_ROOTS	0x08	/* Print details about rootfinder errors */
#define DEBUG_REGIONS	0x10	/* Print regions & boolean trees */
#define DEBUG_ARB8	0x20	/* Print voluminus ARB8 details */
#define DEBUG_PARTITION	0x40	/* Print ray partitioning process */
#define DEBUG_CUT	0x80	/* Print space cutting details */
#define DEBUG_BOXING	0x100	/* Print object/box checking details */
#define DEBUG_SHOOT	0x200	/* Info about shootray() processing */
#define DEBUG_PLOTBOX	0x400	/* Plot(3) the bounding boxes to stdout */
#define DEBUG_RAYWRITE	0x800	/* Ray(5V) view rays to stdout */
#define DEBUG_SPLINE	0x1000	/* Splines */

#define DEBUG_MEM	0x4000	/* Debug dynamic memory operations */
#define DEBUG_TESTING	0x8000	/* One-shot debugging flag */
