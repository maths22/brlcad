/*
 *			R T I F . C
 *
 *  Routines to interface to RT, and RT-style command files
 *
 * Functions -
 *	f_rt		ray-trace
 *	f_rrt		ray-trace using any program
 *	f_rtcheck	ray-trace to check for overlaps
 *	f_saveview	save the current view parameters
 *	f_rmats		load views from a file
 *	f_savekey	save keyframe in file
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
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "mater.h"
#include "./sedit.h"
#include "raytrace.h"
#include "./ged.h"
#include "./solid.h"
#include "./dm.h"

extern void	perror();
extern int	atoi(), execl(), fork(), nice(), wait();
extern long	time();

extern int	numargs;	/* number of args */
extern char	*cmd_args[];	/* array of pointers to args */

void		setup_rt();

/*
 *  			R T _ O L D W R I T E
 *  
 *  Write out the information that RT's -M option needs to show current view.
 *  Note that the model-space location of the eye is a parameter,
 *  as it can be computed in different ways.
 *  The is the OLD format, needed only when sending to RT on a pipe,
 *  due to some oddball hackery in RT to determine old -vs- new format.
 */
HIDDEN void
rt_oldwrite(fp, eye_model)
FILE *fp;
vect_t eye_model;
{
	register int i;

	(void)fprintf(fp, "%.9e\n", VIEWSIZE );
	(void)fprintf(fp, "%.9e %.9e %.9e\n",
		eye_model[X], eye_model[Y], eye_model[Z] );
	for( i=0; i < 16; i++ )  {
		(void)fprintf( fp, "%.9e ", Viewrot[i] );
		if( (i%4) == 3 )
			(void)fprintf(fp, "\n");
	}
	(void)fprintf(fp, "\n");
}

/*
 *  			R T _ W R I T E
 *  
 *  Write out the information that RT's -M option needs to show current view.
 *  Note that the model-space location of the eye is a parameter,
 *  as it can be computed in different ways.
 */
HIDDEN void
rt_write(fp, eye_model)
FILE *fp;
vect_t eye_model;
{
	register int i;

	(void)fprintf(fp, "viewsize %.9e;\n", VIEWSIZE );
	(void)fprintf(fp, "eye_pt %.9e %.9e %.9e;\n",
		eye_model[X], eye_model[Y], eye_model[Z] );
	(void)fprintf(fp, "viewrot ");
	for( i=0; i < 16; i++ )  {
		(void)fprintf( fp, "%.9e ", Viewrot[i] );
		if( (i%4) == 3 )
			(void)fprintf(fp, "\n");
	}
	(void)fprintf(fp, ";\n");
	(void)fprintf(fp, "start 0;\nend;\n");
}

/*
 *  			R T _ R E A D
 *  
 *  Read in one view in the old RT format.
 */
HIDDEN int
rt_read(fp, scale, eye, mat)
FILE	*fp;
fastf_t	*scale;
vect_t	eye;
mat_t	mat;
{
	register int i;
	double d;

	if( fscanf( fp, "%lf", &d ) != 1 )  return(-1);
	*scale = d*0.5;
	if( fscanf( fp, "%lf", &d ) != 1 )  return(-1);
	eye[X] = d;
	if( fscanf( fp, "%lf", &d ) != 1 )  return(-1);
	eye[Y] = d;
	if( fscanf( fp, "%lf", &d ) != 1 )  return(-1);
	eye[Z] = d;
	for( i=0; i < 16; i++ )  {
		if( fscanf( fp, "%lf", &d ) != 1 )
			return(-1);
		mat[i] = d;
	}
	return(0);
}

/*
 *			S E T U P _ R T
 *
 *  Set up command line for one of the RT family of programs,
 *  with all objects in view enumerated.
 */
#define LEN	128
static char	*rt_cmd_vec[LEN];

void
setup_rt( vp )
register char	**vp;
{
	register struct solid *sp;
	register int i;

	/*
	 * Find all unique top-level entrys.
	 *  Mark ones already done with s_iflag == UP
	 */
	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;
	FOR_ALL_SOLIDS( sp )  {
		register struct solid *forw;

		if( sp->s_iflag == UP )
			continue;
		if( vp < &rt_cmd_vec[LEN] )
			*vp++ = sp->s_path[0]->d_namep;
		else  {
			(void)printf("ran out of rt_cmd_vec at %s\n",
				sp->s_path[0]->d_namep );
			break;
		}
		sp->s_iflag = UP;
		for( forw=sp->s_forw; forw != &HeadSolid; forw=forw->s_forw) {
			if( forw->s_path[0] == sp->s_path[0] )
				forw->s_iflag = UP;
		}
	}
	*vp = (char *)0;

	/* Print out the command we are about to run */
	vp = &rt_cmd_vec[0];
	while( *vp )
		(void)printf("%s ", *vp++ );
	(void)printf("\n");

}

/*
 *			R U N _ R T
 */
run_rt()
{
	register struct solid *sp;
	register int i;
	int pid, rpid;
	int retcode;
	int o_pipe[2];
	FILE *fp;

	(void)pipe( o_pipe );
	(void)signal( SIGINT, SIG_IGN );
	if ( ( pid = fork()) == 0 )  {
		(void)close(0);
		(void)dup( o_pipe[0] );
		for( i=3; i < 20; i++ )
			(void)close(i);
		(void)signal( SIGINT, SIG_DFL );
		(void)execvp( rt_cmd_vec[0], rt_cmd_vec );
		perror( rt_cmd_vec[0] );
		exit(16);
	}

	/* As parent, send view information down pipe */
	(void)close( o_pipe[0] );
	fp = fdopen( o_pipe[1], "w" );
	{
		vect_t temp;
		vect_t eye_model;

		VSET( temp, 0, 0, 1 );
		MAT4X3PNT( eye_model, view2model, temp );
		rt_oldwrite(fp, eye_model );
	}
	(void)fclose( fp );

	/* Wait for program to finish */
	while ((rpid = wait(&retcode)) != pid && rpid != -1)
		;	/* NULL */
	if( retcode != 0 )
		(void)printf("Abnormal exit status x%x\n", retcode);
	(void)signal(SIGINT, cur_sigint);

	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;

	return(retcode);
}

/*
 *			F _ R T
 */
void
f_rt()
{
	register char **vp;
	register int i;
	int retcode;
	char *dm;

	if( not_state( ST_VIEW, "Ray-trace of current view" ) )
		return;

	/*
	 * This may be a workstation where RT and MGED have to share the
	 * display, so let display go.  We will try to reattach at the end.
	 */
	dm = dmp->dmr_name;
	if(dmp->dmr_releasedisplay)
		release();

	vp = &rt_cmd_vec[0];
	*vp++ = "rt";
	*vp++ = "-s50";
	*vp++ = "-M";
	for( i=1; i < numargs; i++ )
		*vp++ = cmd_args[i];
	*vp++ = dbip->dbi_filename;

	setup_rt( vp );
	retcode = run_rt();
	if( retcode == 0 )  {
		/* Wait for a return, then reattach display */
		printf("Press RETURN to reattach\007\n");
		while( getchar() != '\n' )
			/* NIL */  ;
	}
	if(dmp->dmr_releasedisplay)
		attach( dm );
}

/*
 *			F _ R R T
 *
 *  Invoke any program with the current view & stuff, just like
 *  an "rt" command (above).
 *  Typically used to invoke a remote RT (hence the name).
 */
void
f_rrt()
{
	register char **vp;
	register int i;
	int	retcode;
	char	*dm;

	if( not_state( ST_VIEW, "Ray-trace of current view" ) )
		return;

	/*
	 * This may be a workstation where RT and MGED have to share the
	 * display, so let display go.  We will try to reattach at the end.
	 */
	dm = dmp->dmr_name;
	if(dmp->dmr_releasedisplay)
		release();

	vp = &rt_cmd_vec[0];
	for( i=1; i < numargs; i++ )
		*vp++ = cmd_args[i];
	*vp++ = dbip->dbi_filename;

	setup_rt( vp );
	retcode = run_rt();
	if( retcode == 0 )  {
		/* Wait for a return, then reattach display */
		printf("Press RETURN to reattach\007\n");
		while( getchar() != '\n' )
			/* NIL */  ;
	}
	if(dmp->dmr_releasedisplay)
		attach( dm );
}

/*
 *			F _ R T C H E C K
 *
 *  Invoke "rtcheck" to find overlaps, and display them as a vector overlay.
 */
void
f_rtcheck()
{
	register char **vp;
	register int i;
	struct vlhead	vhead;
	int	pid, rpid;
	int	retcode;
	int	o_pipe[2];
	int	i_pipe[2];
	FILE	*fp;
	struct solid *sp;

	if( not_state( ST_VIEW, "Overlap check in current view" ) )
		return;

	vp = &rt_cmd_vec[0];
	*vp++ = "rtcheck";
	*vp++ = "-s50";
	*vp++ = "-M";
	for( i=1; i < numargs; i++ )
		*vp++ = cmd_args[i];
	*vp++ = dbip->dbi_filename;

	setup_rt( vp );

	(void)pipe( o_pipe );			/* output from mged */
	(void)pipe( i_pipe );			/* input back to mged */
	(void)signal( SIGINT, SIG_IGN );
	if ( ( pid = fork()) == 0 )  {
		(void)close(0);
		(void)dup( o_pipe[0] );
		(void)close(1);
		(void)dup( i_pipe[1] );
		for( i=3; i < 20; i++ )
			(void)close(i);

		(void)signal( SIGINT, SIG_DFL );
		(void)execvp( rt_cmd_vec[0], rt_cmd_vec );
		perror( rt_cmd_vec[0] );
		exit(16);
	}

	/* As parent, send view information down pipe */
	(void)close( o_pipe[0] );
	fp = fdopen( o_pipe[1], "w" );
	{
		vect_t temp;
		vect_t eye_model;

		VSET( temp, 0, 0, 1 );
		MAT4X3PNT( eye_model, view2model, temp );
		rt_oldwrite(fp, eye_model );
	}
	(void)fclose( fp );

	/* Prepare to receive UNIX-plot back from child */
	(void)close(i_pipe[1]);
	fp = fdopen(i_pipe[0], "r");
	vhead.vh_first = vhead.vh_last = VL_NULL;
	(void)uplot_vlist( &vhead, fp );
	fclose(fp);

	/* Wait for program to finish */
	while ((rpid = wait(&retcode)) != pid && rpid != -1)
		;	/* NULL */
	if( retcode != 0 )
		(void)printf("Abnormal exit status x%x\n", retcode);
	(void)signal(SIGINT, cur_sigint);

	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;

	/* Add overlay */
	invent_solid( "OVERLAPS", &vhead );
	dmaflag++;
}

/*
 *			B A S E N A M E
 *  
 *  Return basename of path, removing leading slashes and trailing suffix.
 */
static char *
basename( p1, suff )
register char *p1, *suff;
{
	register char *p2, *p3;
	static char buf[128];

	p2 = p1;
	while (*p1) {
		if (*p1++ == '/')
			p2 = p1;
	}
	for(p3=suff; *p3; p3++) 
		;
	while(p1>p2 && p3>suff)
		if(*--p3 != *--p1)
			return(p2);
	strncpy( buf, p2, p1-p2 );
	return(buf);
}

/*
 *			F _ S A V E V I E W
 */
void
f_saveview()
{
	register struct solid *sp;
	register int i;
	register FILE *fp;
	char *base;

	if( (fp = fopen( cmd_args[1], "a")) == NULL )  {
		perror(cmd_args[1]);
		return;
	}
	base = basename( cmd_args[1], ".sh" );
	(void)chmod( cmd_args[1], 0755 );	/* executable */
	(void)fprintf(fp, "#!/bin/sh\nrt -M ");
	for( i=2; i < numargs; i++ )
		(void)fprintf(fp,"%s ", cmd_args[i]);
	(void)fprintf(fp,"$*\\\n -o %s.pix\\\n", base);
	(void)fprintf(fp," %s\\\n ", dbip->dbi_filename);

	/* Find all unique top-level entrys.
	 *  Mark ones already done with s_iflag == UP
	 */
	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;
	FOR_ALL_SOLIDS( sp )  {
		register struct solid *forw;	/* XXX */

		if( sp->s_iflag == UP )
			continue;
		(void)fprintf(fp, "'%s' ", sp->s_path[0]->d_namep);
		sp->s_iflag = UP;
		for( forw=sp->s_forw; forw != &HeadSolid; forw=forw->s_forw) {
			if( forw->s_path[0] == sp->s_path[0] )
				forw->s_iflag = UP;
		}
	}
	(void)fprintf(fp,"\\\n 2> %s.log\\\n", base);
	(void)fprintf(fp," <<EOF\n");
	{
		vect_t temp;
		vect_t eye_model;

		VSET( temp, 0, 0, 1 );
		MAT4X3PNT( eye_model, view2model, temp );
		rt_write(fp, eye_model);
	}
	(void)fprintf(fp,"\nEOF\n");
	(void)fclose( fp );
	
	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;
}

/*
 *			F _ R M A T S
 *
 * Load view matrixes from a file.  rmats filename [mode]
 *
 * Modes:
 *	-1	put eye in viewcenter (default)
 *	0	put eye in viewcenter, don't rotate.
 *	1	leave view alone, animate solid named "EYE"
 */
void
f_rmats()
{
	register FILE *fp;
	register struct directory *dp;
	register struct solid *sp;
	union record	rec;
	vect_t	eye_model;
	vect_t	xlate;
	vect_t	sav_center;
	vect_t	sav_start;
	int	mode;
	fastf_t	scale;
	mat_t	rot;
	register struct vlist *vp;

	if( not_state( ST_VIEW, "animate from matrix file") )
		return;

	if( (fp = fopen(cmd_args[1], "r")) == NULL )  {
		perror(cmd_args[1]);
		return;
	}
	mode = -1;
	if( numargs > 2 )
		mode = atoi(cmd_args[2]);
	switch(mode)  {
	case 1:
		if( (dp = db_lookup(dbip, "EYE", LOOKUP_NOISY)) == DIR_NULL )  {
			mode = -1;
			break;
		}
		db_get( dbip,  dp, &rec, 0 , 1);
		FOR_ALL_SOLIDS(sp)  {
			if( sp->s_path[sp->s_last] != dp )  continue;
			if( sp->s_vlist == VL_NULL )  continue;
			VMOVE( sav_start, sp->s_vlist->vl_pnt );
			VMOVE( sav_center, sp->s_center );
			printf("animating EYE solid\n");
			goto work;
		}
		/* Fall through */
	default:
	case -1:
		mode = -1;
		printf("default mode:  eyepoint at (0,0,1) viewspace\n");
		break;
	case 0:
		printf("rotation supressed, center is eyepoint\n");
		break;
	}
work:
	/* If user hits ^C, this will stop, but will leave hanging filedes */
	(void)signal(SIGINT, cur_sigint);
	while( !feof( fp ) &&
	    rt_read( fp, &scale, eye_model, rot ) >= 0 )  {
	    	switch(mode)  {
	    	case -1:
	    		/* First step:  put eye in center */
		       	Viewscale = scale;
		       	mat_copy( Viewrot, rot );
			MAT_DELTAS( toViewcenter,
				-eye_model[X],
				-eye_model[Y],
				-eye_model[Z] );
	    		new_mats();
	    		/* Second step:  put eye in front */
	    		VSET( xlate, 0, 0, -1 );	/* correction factor */
	    		MAT4X3PNT( eye_model, view2model, xlate );
			MAT_DELTAS( toViewcenter,
				-eye_model[X],
				-eye_model[Y],
				-eye_model[Z] );
	    		new_mats();
	    		break;
	    	case 0:
		       	Viewscale = scale;
			mat_idn(Viewrot);	/* top view */
			MAT_DELTAS( toViewcenter,
				-eye_model[X],
				-eye_model[Y],
				-eye_model[Z] );
			new_mats();
	    		break;
	    	case 1:
	    		/* Adjust center for displaylist devices */
	    		VMOVE( sp->s_center, eye_model );

	    		/* Adjust vector list for non-dl devices */
	    		if( sp->s_vlist == VL_NULL )  break;
	    		VSUB2( xlate, eye_model, sp->s_vlist->vl_pnt );
			for( vp = sp->s_vlist; vp != VL_NULL; vp = vp->vl_forw )  {
				VADD2( vp->vl_pnt, vp->vl_pnt, xlate );
			}
	    		break;
	    	}
		dmaflag = 1;
		refresh();	/* Draw new display */
	}
	if( mode == 1 )  {
    		VMOVE( sp->s_center, sav_center );
		if( sp->s_vlist != VL_NULL )  {
	    		VSUB2( xlate, sav_start, sp->s_vlist->vl_pnt );
			for( vp = sp->s_vlist; vp != VL_NULL; vp = vp->vl_forw )  {
				VADD2( vp->vl_pnt, vp->vl_pnt, xlate );
			}
		}
	}
	dmaflag = 1;
	fclose(fp);
}

/* Save a keyframe to a file */
void
f_savekey()
{
	register int i;
	register FILE *fp;
	fastf_t	time;
	vect_t	eye_model;
	vect_t temp;

	if( (fp = fopen( cmd_args[1], "a")) == NULL )  {
		perror(cmd_args[1]);
		return;
	}
	if( numargs > 2 ) {
		time = atof( cmd_args[2] );
		(void)fprintf(fp,"%f\n", time);
	}
	/*
	 *  Eye is in conventional place.
	 */
	VSET( temp, 0, 0, 1 );
	MAT4X3PNT( eye_model, view2model, temp );
	rt_oldwrite(fp, eye_model);
	(void)fclose( fp );
}

/*
 *			F _ P R E V I E W
 *
 *  Preview a new style RT animation scrtip.
 *  Note that the RT command parser code is used, rather than the
 *  MGED command parser, because of the differences in format.
 *  The RT parser expects command handlers of the form "cm_xxx()",
 *  and all communications are done via global variables.
 *
 *  For the moment, the only preview mode is the normal one,
 *  moving the eyepoint as directed.
 *  However, as a bonus, the eye path is left behind as a vector plot.
 */
#include "../rt/cmd.c"
static vect_t	rtif_eye_model;
static mat_t	rtif_viewrot;
static struct vlhead rtif_vhead;

void
f_preview()
{
	register FILE *fp;
	char	buf[512];

	if( not_state( ST_VIEW, "animate viewpoint from new RT file") )
		return;

	if( (fp = fopen(cmd_args[1], "r")) == NULL )  {
		perror(cmd_args[1]);
		return;
	}
	printf("eyepoint at (0,0,1) viewspace\n");

	/* If user hits ^C, this will stop, but will leave hanging filedes */
	(void)signal(SIGINT, cur_sigint);

	rtif_vhead.vh_first = rtif_vhead.vh_last = VL_NULL;

	while( read_cmd( fp, buf, sizeof(buf) ) >= 0 )  {
		if( do_cmd( buf ) < 0 )
			rt_log("command failed: %s\n", buf);
	}

	invent_solid( "EYE_PATH", &rtif_vhead );
	fclose(fp);
}

cm_start(argc, argv)
char	**argv;
int	argc;
{
	if( argc < 2 )
		return(-1);
	/* Has frame number */
	return(0);
}

cm_vsize(argc, argv)
char	**argv;
int	argc;
{
	if( argc < 2 )
		return(-1);
	Viewscale = atof(argv[1]);
	return(0);
}

cm_eyept(argc, argv)
char	**argv;
int	argc;
{
	vect_t	x, y;

	if( argc < 4 )
		return(-1);
	rtif_eye_model[X] = atof(argv[1]);
	rtif_eye_model[Y] = atof(argv[2]);
	rtif_eye_model[Z] = atof(argv[3]);
	/* Processing is deferred until cm_end() */
	return(0);
}

cm_vrot(argc, argv)
char	**argv;
int	argc;
{
	register int	i;

	if( argc < 17 )
		return(-1);
	for( i=0; i<16; i++ )
		rtif_viewrot[i] = atof(argv[i+1]);
	/* Processing is deferred until cm_end() */
	return(0);
}

cm_end(argc, argv)
char	**argv;
int	argc;
{
	vect_t	xlate;
	vect_t	new_cent;
	vect_t	xv, yv;			/* view x, y */
	vect_t	xm, ym;			/* model x, y */

	/* Record eye path as a polyline.  Move, then draws */
	ADD_VL( &rtif_vhead, rtif_eye_model, rtif_vhead.vh_first != VL_NULL );
	
	/* First step:  put eye at view center (view 0,0,0) */
       	mat_copy( Viewrot, rtif_viewrot );
	MAT_DELTAS( toViewcenter,
		-rtif_eye_model[X],
		-rtif_eye_model[Y],
		-rtif_eye_model[Z] );
	new_mats();
	/*  Second step:  put eye at view 0,0,1.
	 *  For eye to be at 0,0,1, the old 0,0,-1 needs to become 0,0,0.
	 */
	VSET( xlate, 0, 0, -1 );	/* correction factor */
	MAT4X3PNT( new_cent, view2model, xlate );
	MAT_DELTAS( toViewcenter,
		-new_cent[X],
		-new_cent[Y],
		-new_cent[Z] );
	new_mats();

#if 1
	/* Draw camera orientation notch to right (+X) and up (+Y) */
	VSET( xv, 0.05, 0, 0 );
	VSET( yv, 0, 0.05, 0 );
	MAT4X3PNT( xm, view2model, xv );
	MAT4X3PNT( ym, view2model, yv );
	ADD_VL( &rtif_vhead, xm, 1 );
	ADD_VL( &rtif_vhead, ym, 1 );
	ADD_VL( &rtif_vhead, rtif_eye_model, 1 );
#endif

	dmaflag = 1;
	refresh();	/* Draw new display */
	return(0);
}

cm_multiview(argc, argv)
char	**argv;
int	argc;
{
	return(-1);
}
cm_anim(argc, argv)
char	**argv;
int	argc;
{
	return(-1);
}
cm_tree(argc, argv)
char	**argv;
int	argc;
{
	return(-1);
}
cm_clean(argc, argv)
char	**argv;
int	argc;
{
	return(-1);
}
cm_set(argc, argv)
char	**argv;
int	argc;
{
	return(-1);
}
