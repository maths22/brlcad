/*
 *			V I E W H I D E
 *
 *  Ray Tracing program RTHIDE bottom half.
 *
 *  This module utilizes the RT library to interrogate a MGED
 *  model and plots a hidden-line removed UnixPlot file.
 *  This is accomplished by comparing region-id codes between
 *  pixels.  A vertical or horizontal line is plotted when a change in
 *  region-id codes is detected.
 *
 *  At present, the main use for this module is to generate
 *  UnixPlot file that can be read by various existing filters
 *  to produce a bas-relief line drawing of an MGED object: i.e.
 *  flat when viewed head-on, but with relief detail when seen at
 *  an angle.
 *  
 *  This is based on previous work done by Michael John Muuss.
 *
 *  Author -
 *	Susanne L. Muuss, J.D.
 *	Michael J. Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */


#ifndef lint
static char RCSrayhide[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./material.h"
#include "./ext.h"
#include "rdebug.h"

#define SEEKING_START_PT 0
#define FOUND_START_PT 1
#define CELLNULL ( (struct cell *) 0)
#define MAXANGLE 0.9961947		/* cos 5 */

struct cell {
	float	c_dist;		/* distance from emanation plane to in_hit */
	int	c_id;		/* region_id of component hit */
	point_t	c_hit;		/* 3-space hit point of ray */
	vect_t	c_normal;	/* surface normal at the hit point */
	vect_t	c_rdir;		/* ray direction, permits perspective */
};

static FILE	*plotfp;
extern	int	width;			/* # of pixels in X; picture width */

void		swapbuff();
void		cleanline();
void		horiz_cmp();
void		vert_cmp();
struct cell	*find_cell();
struct cell	*botp;			/* pointer to bottom line   */
struct cell	*topp;			/* pointer to top line	    */



int		use_air = 1;		/* Handling of air in librt */

int		using_mlib = 0;		/* Material routines NOT used */

/* Viewing module specific "set" variables */
struct structparse view_parse[] = {
	(char *)0,(char *)0,	0,			FUNC_NULL
};


char usage[] = "\
Usage:  rthide [options] model.g objects... >file.ray\n\
Options:\n\
 -s #		Grid size in pixels, default 512\n\
 -a Az		Azimuth in degrees	(conflicts with -M)\n\
 -e Elev	Elevation in degrees	(conflicts with -M)\n\
 -M		Read model2view matrix on stdin (conflicts with -a, -e)\n\
 -o model.g	Specify output file (default=stdout)\n\
 -U #		Set use_air boolean to # (default=1)\n\
 -x #		Set librt debug flags\n\
";

int	rayhit(), raymiss();

/*
 *  			V I E W _ I N I T
 *
 *  This routine is called by main().  It initializes the entire run, i.e.,
 *  it does things such as opening files, etc., which must be done before
 *  any other computations take place.  It is called only once per run.
 *  Pointers to rayhit() and raymiss() are set up and are later called from
 *  do_run().
 */

int
view_init( ap, file, obj, minus_o )
register struct application *ap;
char *file, *obj;
{

	if( outfp == NULL )
		outfp = stdout;

	ap->a_hit = rayhit;
	ap->a_miss = raymiss;
	ap->a_onehit = 1;

	output_is_binary = 0;		/* output is printable ascii */

	/* malloc() two buffers that have room for as many struct cell 's
	 * as the incoming file is wide (width), plus two for the border.
	 * Rather than using malloc(), though, rt_malloc() is used.  This
	 * has the advantage of inbuild error-checking and automatic aborting
	 * if there is no memory.  Also, rt_malloc() takes a string as its
	 * final parameter: this tells the usr exactly where memory ran out.
	 * The file_height is counted by using ap->a_y directly. The benefit
	 * of this is WHAT?
	 */

	botp = (struct cell *)rt_malloc(sizeof(struct cell) * (width + 2),
		"bottom cell buffer" );
	topp = (struct cell *)rt_malloc(sizeof(struct cell) * (width + 2),
		"top cell buffer" );

	/* Open a plotfile for writing and check that a valid file pointer
	 * has been acquired.
	 */

	plotfp = fopen("hide.pl", "w");
	if( plotfp == NULL)  {
		perror("hide.pl");
		exit(1);
	}

	/* Clear both in-buffers to ensure abscence of garbage.  Note 
	 * that the zero-filled "bottom" buffer now provides the first
	 * in-memory buffer for comparisons.
	 */

	cleanline(botp, width);
	cleanline(topp, width);

	return(0);		/* No framebuffer needed */
}

/*
 *			V I E W _ 2 I N I T
 *
 *  A null-function.
 *  View_2init is called by do_frame(), which in turn is called by
 *  main() in rt.c.
 */

void
view_2init( ap )
struct application	*ap;
{

	if( outfp == NULL )
		rt_bomb("outfp is NULL\n");

	regionfix( ap, "rtray.regexp" );		/* XXX */
}

/*
 *			R A Y M I S S
 *
 *  This function is called by rt_shootray(), which is called by
 *  do_frame(). Records coordinates where a miss is detected.
 */

int
raymiss( ap )
register struct application	*ap;
{

	struct	cell	*posp;		/* store the current cell position */

	posp = &(topp[ap->a_x + 1]);

	/*
	 * cleanline() zero-fills a buffer.  Therefore, it is possible to
	 * let this "line scrubber" do all the zero-filling for raymiss()
	 * by calling it before EVERY new topbuff is filled.  This would
	 * result in very inefficient code.  Thus, even on a miss, raymiss()
	 * will do its own zero-filling of the distance, region_id, surface
	 * normal, and the hit distance.  This prevents the image from being
	 * "smeared".
	 */

	posp->c_id = 0;
	posp->c_dist = 0;
	VSET(posp->c_hit, 0, 0, 0);
	VSET(posp->c_normal, 0, 0, 0);
	VSET(posp->c_rdir, 0, 0, 0);

	return(0);
}

/*
 *			V I E W _ P I X E L
 *
 *  This routine is called from do_run(), and in this case does nothing.
 */

void
view_pixel()
{
	return;
}

/*
 *			R A Y H I T
 *
 *  Rayhit() is called by rt_shootray() when a hit is detected.  It
 *  computes the hit distance, the region_id, the distance traveled by the
 *  ray, and the surface normal at the hit point.
 *  
 */

int
rayhit( ap, PartHeadp )
struct application *ap;
register struct partition *PartHeadp;
{
	register struct partition *pp = PartHeadp->pt_forw;
	struct	cell	*posp;			/* stores current cell position */
	fastf_t			dist;   	/* ray distance */
	int			region_id;	/* solid region's id */

	if( pp == PartHeadp )
		return(0);		/* nothing was actually hit?? */

	/*
	 * Output the ray data: screen plane (pixel) coordinates
	 * for x and y positions of a ray, region_id, and hit_distance.
	 * The x and y positions are represented by ap->a_x and ap->a_y.
	 *
	 *  Assume all rays are parallel.
	 */

	dist = pp->pt_inhit->hit_dist;
	region_id = pp->pt_regionp->reg_regionid;
	posp = &(topp[ap->a_x + 1]);

	/* Calculate the hit normal and the hit distance.  This is done
	 * by giving RT_HIT_NORM() the address of the hit partition so it
	 * can fill this in.  From there the hit point and the hit normal
	 * can be extracted.
	 */

	RT_HIT_NORM(pp->pt_inhit, pp->pt_inseg->seg_stp, &(ap->a_ray));


	/* Now store the distance and the region_id in the appropriate
	 * cell struct: i.e., ap->a_x (the x-coordinate) +1 within the
	 * array of cell structs.
	 * Extract the hit distance and the hit normals from the hit structure
	 * and store in the cell structure.
	 * Since repeatedly computing topp[ap->a_x + 1] is very inefficient,
	 * the value of ap->a_x + 1 will be stored in a struct cell pointer
	 * to vitiate the need to recompute this value repeatedly. LATER.
	 */

	posp->c_id = region_id;
	posp->c_dist = dist;
	VMOVE(posp->c_hit, pp->pt_inhit->hit_point);
	VMOVE(posp->c_normal, pp->pt_inhit->hit_normal);
	VMOVE(posp->c_rdir, ap->a_ray.r_dir);
	return(0);
}

/*
 *			V I E W _ E O L
 *
 *  View_eol() is called by rt_shootray() in do_run().
 *  This routine is called by worker.c whenever there is a full scanline.
 *  worker.c figures out what is a full scanline.  Whenever there
 *  are two full buffers in memory, the horizontal and the vertical compares
 *  are done, as well as any plotting.  Then the "bottom" buffer is "dropped",
 *  and a new one is read into memory until end-of-file is reached.
 */

void	view_eol(ap)
struct application *ap;

{


	/* Now add 2 pixels to file_width to convert it to memory_width
	 * for doing the comparisons and determining the boundaries around 
	 * the picture.  Note that the file_height is simply expressed as
	 * ap->a_y.  It is not necessary to add 2 pixels to it for the
	 * boundary because that is taken care of by originally allocating
	 * the bottom cell buffer, and at the end by doing one extra top
	 * cell buffer.
	 */

	horiz_cmp(botp, width + 2, ap->a_y);
	vert_cmp(botp, topp, width + 2, ap->a_y);
	swapbuff(&botp, &topp);

}


/*
 *			V I E W _ E N D
 *
 *  View_end() is called by rt_shootray in do_run().  It is necessary to
 *  iterate one more time through the horizontal and vertical comparisons
 *  to put down a top border.  This routine is responsible for doing this.
 *  Note also that since view_end() takes an application structure pointer,
 *  the file height can be expressed directly as ap->a_y.  One might consider
 *  allocating a height variable set to this value....
 */

void
view_end(ap)
struct application *ap;
{

	cleanline(topp, width);
	horiz_cmp(botp, width + 2, ap->a_y);
	vert_cmp(botp, topp, width + 2, ap->a_y);

	/* Close plotfile. */
	fclose(plotfp);

	fflush(outfp);
}


/*
 *		H O R I Z O N T A L   C O M P A R I S O N
 *
 *  This routine takes three parameters: a pointer to a "bottom" buffer, the
 *  line width of the incoming file plus two border pixels (mem_width), and
 *  a y-coordinate (file_height).  It returns nothing.
 *
 *  Pixels on a scanline are compared to see if their region_id codes
 *  are the same.  If these are not identical, a vertical line is
 *  plotted to mark to boundary where the region_id codes change.  Likewise,
 *  the size of the angle between surface normals is checked: if it exceeds
 *  MAXANGLE, then a line s drawn.
 */

void
horiz_cmp(botp, mem_width, y)
struct cell	*botp;
int		mem_width;
int		y;

{
	int		x;
	struct	cell	*cellp;
	point_t		beg;		/* beginning point of line */
	point_t		end;		/* end point of line */
	vect_t		start;		/* start of vector */
	vect_t		stop;		/* end of vector */

	for (x=0; x < (mem_width-1); x++, botp++)  {

		/* If the region_ids of neighboring pixels do
		 * not match, compare their hit distances.  If
		 * either distance is zero, select the non-zero
		 * distance for plotting; otherwise, select the
		 * lesser of the two distances.
		 * Check the angle between surface normals to see
		 * whether a line needs to be drawn.  This is accomplished
		 * by finding the cosine of the angle between the two
		 * vectors with VDOT(), the dot product.  The result
		 * is compared against MAXANGLE, which must be determined
		 * experimentally.  This scheme will prevent curved surfaces,
		 * on which practically "every point is a surface", from
		 * being represented as dark blobs.
		 * Note that MAXANGLE needs to be greater than the cosine
		 * of the angle between the two vectors because as the angle
		 * between the increases, the cosine of said angle decreases.
		 * Also of interest is that one needs to say: plot if id's
		 * are not the same OR if either id is not 0 AND the cosine
		 * of the angle between the normals is less than MAXANGLE. 
		 * This test prevents the background from being shaded in.
		 * Furthermore, it is necessary to select the hit_point
		 */

		if (botp->c_id != (botp+1)->c_id ||
		   ( botp->c_id != 0 && 
		   (VDOT(botp->c_normal, (botp + 1)->c_normal) < MAXANGLE)))  {
							     
		   	cellp = find_cell(botp, (botp+1)); 

			/* Note that the coordinates must be expressed
		   	 * as MODEL coordinates.  This can be done by
		   	 * adding offsets to the hit_point.  Thus, 0.5*
		   	 * dx_model means moving 0.5 of a cell in model
		   	 * space, and replaces (x -1 +0.5) representing
		   	 * backing up one whole cell and then moving to
		   	 * the center of the new cell in file coordinates.
		   	 * In that case, the x represented the screen coords.
		   	 * Now, make the beginning point and the ending point.
			 */


		   	VJOIN2(beg, cellp->c_hit, 0.5, dx_model, -0.5, dy_model);
		   	VJOIN2(end, cellp->c_hit, 0.5, dx_model, 0.5, dy_model);

		   	/* Now fashion the starting and stopping vectors. */

		   	VJOIN1(start, beg, cellp->c_dist, cellp->c_rdir);
		   	VJOIN1(stop, end, cellp->c_dist, cellp->c_rdir);

			pdv_3line(plotfp, start, stop);
				
		}
	}
}

/*
 *		V E R T I C A L  C O M P A R I S O N
 *
 *  This routine takes four parameters: a pointer to a "top" buffer, a pointer
 *  to a "bottom" buffer, the file_width + two border pixels (mem_width), and
 *  a y-coordinate (line-count, or file_height).  It returns nothing.
 *
 *  Pixels residing on adjacent scanlines are compared to determine
 *  whether their region_id codes are the same.  If these are not
 *  identical, a horizontal line is plotted to mark the boundary where
 *  the region_id codes change.
 *
 */

void
vert_cmp(botp, topp, mem_width, y)
struct cell	*botp;
struct cell	*topp;
int		mem_width;
int		y;

{

	register int	x;
	struct	 cell	*cellp;
	struct	 cell	*start_cellp;
	int		state;
	vect_t		start;
	vect_t		stop;
	point_t		beg;
	point_t		end;
	
	VSET(beg, 0, 0, 0);


	state = SEEKING_START_PT;

	/* If the region_ids are not equal OR either region_id is not 0 AND
	 * the cosine of the angle between the normals is less that MAXANGLE,
	 * plot a line or shade the plot to produce surface normals or give
	 * a sense of curvature.
	 */

	for (x=0; x < mem_width; x++, botp++, topp++)  {
		if (botp->c_id != topp->c_id ||
		   ( botp->c_id != 0 && 
		   (VDOT(botp->c_normal, (botp + 1)->c_normal) < MAXANGLE)))  {
			if( state == FOUND_START_PT ) {
				continue;
			} else {
				/* find the correct cell. */
				start_cellp = find_cell(botp, topp);		

				/* Move to and remember left point */
				VJOIN2(beg, start_cellp->c_hit, -0.5, dx_model, 0.5, dy_model);
				VJOIN1(start, beg, start_cellp->c_dist, start_cellp->c_rdir);
				state = FOUND_START_PT;
			}
		} else {
			/* points are the same */

			if (state == FOUND_START_PT) {

				/* Draw to current left edge 
				/* Note that x and y must be converted back
				 * to file coordinates so that the file
				 * picture gets plotted.  The 0.5 factors
				 * are for centering. This is for (x-1-0.5),
				 * y-1+0.5).  These file coordinate must then
				 * be expressed in model space.  That is done
				 * by starting at the hit_pt and adding or
				 * subtracting 0.5 cell for centering.
				 */

				cellp = find_cell( start_cellp, find_cell( (botp-1), (topp-1) ) );
				VJOIN2(end, cellp->c_hit, -0.5, dx_model, 0.5, dy_model);
				VJOIN1(stop, end, cellp->c_dist, cellp->c_rdir);
				pdv_3line(plotfp, start, stop);
					state = SEEKING_START_PT;
			} else {
				continue;
			}
		}
	}
		
	/* Now check for end of scan-line. */
	if (state == FOUND_START_PT) {

			/* Note that x and y must be converted back
			 * to file coordinates so that the file
			 * picture gets plotted.  The 0.5 factors
			 * are for centering.  This is for (x-1-0.5), (y-1+0.5).
			 * These file coordinates must then be expressed in
			 * model space.  That is done by starting at the
			 * hit_pt and adding or subtracting 0.5 cell for
			 * centering.
			 */

		cellp = find_cell( (botp-1), (topp-1) );
		VJOIN2(end, cellp->c_hit, -0.5, dx_model, 0.5, dy_model);
		VJOIN1(stop, end, cellp->c_dist, cellp->c_rdir);

		pdv_3line(plotfp, start, stop);
		state = SEEKING_START_PT;
	}
}



/*
 *	           F I N D_ C E L L
 *
 *  This routine takes pointers to two cells.  This is more efficient (takes
 *  less space) than sending the hit_distances.  Furthermore, by selecting
 *  a cell, rather than just a distance, more information becomes available
 *  to the calling routine.
 *  If the region_ids of neighboring pixels do not match, compare their
 *  respective hit_distances.  If either distance is zero, select the
 *  non-zero distance for plotting; otherwise, select the lesser of the
 *  two distances.  Return a pointer to the cell with the smaller hit_distance.
 *  Using this hit_distance will be more esthetically pleasing for the bas-
 *  relief.
 */

struct	cell	*
find_cell ( cur_cellp, next_cellp)
struct	cell	*cur_cellp;
struct	cell	*next_cellp;
{
	struct cell	*cellp;

	if (cur_cellp->c_dist == 0)
		cellp = next_cellp;
	else if (next_cellp->c_dist == 0)
		cellp = cur_cellp;
	else if (cur_cellp->c_dist < next_cellp->c_dist )
		cellp = cur_cellp;
	else
		cellp = next_cellp;

	return (cellp);
}


/*		S W A P B U F F
 *
 *  This routine serves to swap buffer pointers: i.e., one buffer is read
 *  at a time.  The first buffer read becomes the "bottom buffer" the new
 *  buffer becomes the "top buffer".  Once the vertical comparison between
 *  the two buffers has been done, the "top buffer" now becomes the "bottom
 *  buffer" and is retained, while the erstwhile "bottom buffer" is the new
 *  "top buffer", which is overwritten when the next line of information is
 *  read.
 *  This routine takes as its parameters the address of two pointers to buffers.
 *  It manipulates these, but returns nothing.
 */

void
swapbuff(onepp, twopp)
struct cell	**onepp;		/* caveat: variables must start w/ letters */
struct cell 	**twopp;

{

	struct cell	*temp_p;	/* caveat: hyphens are read as "minus" */

	temp_p = *onepp;
	*onepp = *twopp;
	*twopp = temp_p;

}





/*		C L E A N L I N E
 *
 *  This routine takes as paramenters the address of a buffer and an integer
 *  reflecting the width of the file.  It proceeds to ZERO fill the buffer.
 *  This routine returns nothing.
 */

void
cleanline(inbuffp, file_width)
struct cell	*inbuffp;
int		file_width;

{

	int	i;

	for(i = 0; i < file_width + 2; i++, inbuffp++)  {
		inbuffp->c_id = '\0';
		inbuffp->c_dist = '\0';
		VSET(inbuffp->c_hit, 0, 0, 0);
		VSET(inbuffp->c_normal, 0, 0, 0);
		VSET(inbuffp->c_rdir, 0, 0, 0);
	}
}
