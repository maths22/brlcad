/* 
 *			H I D E L I N E . C
 * 
 * Description -
 *	Takes the vector  list for the  current  display  and  raytraces
 *	along those vectors.  If the first point hit in the model is the
 *	same as that  vector,  continue  the line  through  that  point;
 *	otherwise,  stop  drawing  that  vector  or  draw  dotted  line.
 *	Produces Unix-plot type output.
 *
 *	The command is "H file.pl [stepsize] [%epsilon]". Stepsize is the
 *	number of segments into which the window size should be broken.
 *	%Epsilon specifies how close two points must be before they are
 *	considered equal. A good values for stepsize and %epsilon are 128
 *	and 1, respectively.
 *
 * Author -  
 *	Mark Huston Bowden  
 *	Research  Institute,  E-47 
 *	University of Alabama in Huntsville  
 *	Huntsville, AL 35899
 *	(205) 895-6467	UAH
 *	(205) 876-1089	Redstone Arsenal
 *
 * History -
 *	01 Aug 88		Began initial coding
 */

#include <stdio.h>
#include <string.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./solid.h"
#include "./dm.h"

#ifdef gould
#define MAXOBJECTS	1000
#define NAMELEN		40
#else
#define MAXOBJECTS	3000
#define NAMELEN		80
#endif /* gould */
#define VIEWSIZE	(2*Viewscale)
#define TRUE	1
#define FALSE	0

#define MOVE(v)	  VMOVE(last_move,(v))

#define DRAW(v)	{ vect_t t;\
		  MAT4X3PNT(t,model2view,last_move);\
		  pl_move(plotfp,(int)(2048*t[X]),(int)(2048*t[Y]));\
		  MAT4X3PNT(t,model2view,(v));\
		  pl_cont(plotfp,(int)(2048*t[X]),(int)(2048*t[Y])); }

extern struct db_i *dbip;	/* current database instance */
extern int numargs;
extern char *cmd_args[];	/* array of pointers to args */
extern float Viewscale;
extern mat_t view2model;
extern mat_t model2view;

fastf_t epsilon;
vect_t aim_point;
struct solid *sp;

int
f_hideline()
{
    FILE 	*plotfp;
    char 	visible;
    extern int 	hit_headon(),hit_tangent(),hit_overlap();
    int 	i,j,numobjs;
    char 	objname[MAXOBJECTS][NAMELEN],title[1];
    fastf_t 	len,u,step;
    FAST float 	ratio;
    vect_t	last_move;
    struct rt_i	*rtip;
    struct resource resource;
    register struct application a;
    register vect_t temp;
    register vect_t last,dir;
    register struct vlist *vp;

/*
 * Open Unix-plot file and initialize
 */
    if ((plotfp = fopen(cmd_args[1],"w")) == NULL) {
	(void)printf("f_hideline: unable to open \"%s\" for writing.\n",
								cmd_args[1]);
	return(1);
    }
    pl_space(plotfp,-2048,-2048,2048,2048);

/*
 * Build list of objects being viewed
 */
    numobjs = 0;
    FOR_ALL_SOLIDS(sp) {
	for (i = 0; i < numobjs; i++)
	    if (!strcmp(objname[i],sp->s_path[0]->d_namep))
		break;
	if (i == numobjs)
	    strncpy(objname[numobjs++],sp->s_path[0]->d_namep,NAMELEN);
    }

    (void)printf("Generating hidden-line drawing of the following regions:\n");
    for (i = 0; i < numobjs; i++)
	printf("\t%s\n",objname[i]);
/*
 * Initialization for librt
 */
    RES_INIT( &rt_g.res_syscall );
    RES_INIT( &rt_g.res_worker );
    RES_INIT( &rt_g.res_stats );
    RES_INIT( &rt_g.res_results );

    if ((rtip = rt_dirbuild(dbip->dbi_filename,title,0)) == RTI_NULL) {
	printf("f_hideline: unable to open model file \"%s\"\n",
		dbip->dbi_filename);
	return(1);
    }
    a.a_hit = hit_headon;
    a.a_miss = hit_tangent;
    a.a_overlap = hit_overlap;
    a.a_rt_i = rtip;
    a.a_resource = &resource;
    a.a_level = 0;
    a.a_onehit = 1;
    a.a_diverge = 0;
    a.a_rbeam = 0;

    if (numargs > 2) {
	sscanf(cmd_args[2],"%f",&step);
	step = Viewscale/step;
	sscanf(cmd_args[3],"%f",&epsilon);
	epsilon *= Viewscale/100;
    } else {
	step = Viewscale/256;
	epsilon = 0.1*Viewscale;
    }

    for (i = 0; i < numobjs; i++)
	if (rt_gettree(rtip,objname[i]) == -1)
	    printf("f_hideline: rt_gettree failed on \"%s\"\n",objname[i]);
/*
 * Crawl along the vectors raytracing as we go
 */
    VSET(temp,0,0,-1);				/* looking at model */
    MAT4X3VEC(a.a_ray.r_dir,view2model,temp);
    VUNITIZE(a.a_ray.r_dir);

    FOR_ALL_SOLIDS(sp) {

	ratio = sp->s_size / VIEWSIZE;		/* ignore if small or big */
	if (ratio >= dmp->dmr_bound || ratio < 0.001)
	    continue;
	
printf("Solid\n");
	for (vp = sp->s_vlist; vp != VL_NULL; vp = vp->vl_forw ) {
printf("\tVector\n");
	    if (vp->vl_draw == 0) {		/* move */
		VMOVE(last,vp->vl_pnt);
		MOVE(last);
	    } else {
		VSUB2(dir,vp->vl_pnt,last);	/* setup direction && length */
		len = MAGNITUDE(dir);
		VUNITIZE(dir);
		visible = FALSE;
printf("\t\tDraw 0 -> %g, step %g\n", len, step);
		for (u = 0; u <= len; u += step) {
		    VJOIN1(aim_point,last,u,dir);
		    MAT4X3PNT(temp,model2view,aim_point);
		    temp[Z] = 100;			/* parallel project */
		    MAT4X3PNT(a.a_ray.r_pt,view2model,temp);
		    if (rt_shootray(&a)) {
			if (!visible) {
			    visible = TRUE;
			    MOVE(aim_point);
			}
		    } else {
			if (visible) {
			    visible = FALSE;
			    DRAW(aim_point);
			}
		    }
		}
		if (visible)
		    DRAW(aim_point);
		VMOVE(last,vp->vl_pnt);		/* new last vertex */
	    }
	}
    }
    fprintf(plotfp,"PG;");
    fclose(plotfp);
    return(0);
}

/*
 * hit_headon - routine called by rt_shootray if ray hits model
 */

static int
hit_headon(ap,PartHeadp)
register struct application *ap;
struct partition *PartHeadp;
{
    register char diff_solid;
    register vect_t diff;
    register fastf_t len;

    if (PartHeadp->pt_forw->pt_forw != PartHeadp)
	printf("hit_headon: multiple partitions\n");

    VJOIN1(PartHeadp->pt_forw->pt_inhit->hit_point,ap->a_ray.r_pt,
	   PartHeadp->pt_forw->pt_inhit->hit_dist, ap->a_ray.r_dir);
    VSUB2(diff,PartHeadp->pt_forw->pt_inhit->hit_point,aim_point);

    diff_solid = strcmp(sp->s_path[0]->d_namep,
		 PartHeadp->pt_forw->pt_inseg->seg_stp->st_name);
    len = MAGNITUDE(diff);

    if (	NEAR_ZERO(len,epsilon)
       ||
		( diff_solid &&
	          VDOT(diff,ap->a_ray.r_dir) > 0 )
       )
	return(1);
    else
	return(0);
}

/*
 * hit_tangent - routine called by rt_shootray if ray misses model
 *
 *     We know we are shooting at the model since we are aiming at the
 *     vector list MGED created. However, shooting at an edge or shooting
 *     tangent to a curve produces only one intersection point at which
 *     time rt_shootray reports a miss. Therefore, this routine is really
 *     a "hit" routine.
 */

static int
hit_tangent(ap,PartHeadp)
register struct application *ap;
struct partition *PartHeadp;
{
    return(1);		/* always a hit */
}

/*
 * hit_overlap - called by rt_shootray if ray hits an overlap
 */

static int
hit_overlap(ap,PartHeadp)
register struct application *ap;
struct partition *PartHeadp;
{
    return(0);		/* never a hit */
}
