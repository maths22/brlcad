/*
 *			C H G V I E W . C
 *
 * Functions -
 *
 * The U. S. Army Ballistic Research Laboratory
 */

#include	<math.h>
#include	<signal.h>
#include	<stdio.h>
#include "ged_types.h"
#include "3d.h"
#include "sedit.h"
#include "ged.h"
#include "dir.h"
#include "solid.h"
#include "dm.h"
#include "vmath.h"

extern void	perror();
extern int	atoi(), execl(), fork(), nice(), wait();
extern long	time();

extern char	*filename;	/* Name of database file */
int		drawreg;	/* if > 0, process and draw regions */
extern int	numargs;	/* number of args */
extern char	*cmd_args[];	/* array of pointers to args */

static void	eedit();


/* DEBUG -- force view center */
/* Format: C x y z	*/
f_center()
{
	toViewcenter[MDX] = -atof( cmd_args[1] );
	toViewcenter[MDY] = -atof( cmd_args[2] );
	toViewcenter[MDZ] = -atof( cmd_args[3] );
	new_mats();
	dmaflag++;
}

f_rot()
{
	register char c = cmd_args[1][0];
	static float f;

	f = atof(cmd_args[2]) * degtorad;

	switch( c )  {
	case 'x':
	case 'X':
		usejoy( f, 0.0, 0.0 );
		break;
	case 'y':
	case 'Y':
		usejoy( 0.0, f, 0.0 );
		break;
	case 'z':
	case 'Z':
		usejoy( 0.0, 0.0, f );
		break;
	default:
		printf("Unknown axis '%s'\n", cmd_args[1]);
		break;
	}
}

/* DEBUG -- simulate button press */
/* Format: press button#	*/
f_press()
{
	int i;
	i = atoi( cmd_args[1] );
	button( i );
}

/* DEBUG -- force viewsize */
/* Format: view size	*/
f_view()
{
	float f;
	f = atof( cmd_args[1] );
	if( f < 0.01 ) f = 0.01;
	Viewscale = f / 2.0;
	new_mats();
	dmaflag++;
}

/* ZAP the display -- then edit anew */
/* Format: B object	*/
f_blast()
{

	f_zap();

	/*
	 * Force out the control list with NO solids being drawn,
	 * then the display processor will not mind when we start
	 * writing new subroutines out there...
	 */
	refresh();

	/* fall through */
	drawreg = 0;
	regmemb = -1;

	eedit();
}

/* Edit something (add to visible display) */
/* Format: e object	*/
f_edit()
{
	drawreg = 0;
	regmemb = -1;

	eedit();
}

/* Evaluated Edit something (add to visible display) */
/* E object */
f_evedit()
{
	drawreg = 1;
	regmemb = -1;
	eedit();
}

/*
 *			E E D I T
 *
 * B, e, and E commands uses this area as common
 */
static void
eedit()
{
	register struct directory *dp;
	register int i;
	static long stime, etime;	/* start & end times */

	(void)time( &stime );
	for( i=1; i < numargs; i++ )  {
		if( (dp = lookup( cmd_args[i], LOOKUP_NOISY )) == DIR_NULL )
			continue;

		/*
		 * Delete any portion of object remaining from previous draw
		 */
		eraseobj( dp );
		dmaflag++;
		refresh();
		dmaflag++;

		/*
		 * Draw this object as a ROOT object, level 0
		 * on the path, with no displacement, and
		 * unit scale.
		 */
		if( no_memory )  {
			(void)printf("No memory left so cannot draw %s\n",
				dp->d_namep);
			drawreg = 0;
			regmemb = -1;
			continue;
		}

		drawHobj( dp, ROOT, 0, identity );

		drawreg = 0;
		regmemb = -1;
	}
	(void)time( &etime );
	if( Viewscale == 1.0 )  {
		Viewscale = maxview;
		new_mats();
	}

	(void)printf("view (%ld sec)\n", etime - stime );
	dmaflag = 1;
}

/* Delete an object or several objects from the display */
/* Format: d object1 object2 .... objectn */
f_delobj()
{
	register struct directory *dp;
	register int i;

	for( i = 1; i < numargs; i++ )  {
		if( (dp = lookup( cmd_args[i], LOOKUP_NOISY )) != DIR_NULL )
			eraseobj( dp );
	}
	dmaflag = 1;
}

f_debug()
{
	pr_solids( &HeadSolid );
}

f_regdebug()
{
		regdebug ^= 1;	/* toggle */
}

/* List object information */
/* Format: l object	*/
f_list()
{
	register struct directory *dp;
	register int i;
	union record record;
	
	if( (dp = lookup( cmd_args[1], LOOKUP_NOISY )) == DIR_NULL )
		return;

	db_getrec( dp, &record, 0 );

	if( record.u_id == ID_SOLID )  {
		(void)printf("%s:  %s\n",
			dp->d_namep,record.s.s_type==GENARB8 ? "GENARB8" :
			record.s.s_type==GENTGC ? "GENTGC" :
			record.s.s_type==GENELL ? "GENELL": "TOR" );

		pr_solid( &record.s );

		for( i=0; i < es_nlines; i++ )
			(void)printf("%s\n",&es_display[ES_LINELEN*i]);
		return;
	}

	if( record.u_id == ID_ARS_A )  {
		(void)printf("%s:  ARS\n", dp->d_namep );
		db_getrec( dp, &record, 0 );
		(void)printf(" num curves  %d\n", record.a.a_m );
		(void)printf(" pts/curve   %d\n", record.a.a_n );
		db_getrec( dp, &record, 1 );
		(void)printf(" vertex      %.4f %.4f %.4f\n",
			record.b.b_values[0], record.b.b_values[1],
			record.b.b_values[2] );
		return;
	}
	if( record.u_id == ID_P_HEAD )  {
		(void)printf("%s:  %d granules of polygon data\n",
			dp->d_namep, dp->d_len-1 );
		return;
	}
	if( record.u_id != ID_COMB )  {
		(void)printf("%s: garbage!\n", dp->d_namep );
		return;
	}

	/* Combination */
	(void)printf("%s (len %d) ", dp->d_namep, dp->d_len-1 );
	if( record.c.c_flags == 'R' )
		(void)printf("REGION item=%d, air=%d, mat=%d, los=%d ",
			record.c.c_regionid, record.c.c_aircode,
			record.c.c_material, record.c.c_los );
	(void)printf("--\n", dp->d_len-1 );

	for( i=1; i < dp->d_len; i++ )  {
		db_getrec( dp, &record, i );
		(void)printf("  %c %s",
			record.M.m_relation, record.M.m_instname );

		if( record.M.m_brname[0] != '\0' )
			(void)printf(" br name=%s", record.M.m_brname );
#define Mat record.M.m_mat
		if( Mat[0] != 1.0  || Mat[5] != 1.0 || Mat[10] != 1.0 )
			(void)printf(" (Rotated)");
		if( Mat[MDX] != 0.0 || Mat[MDY] != 0.0 || Mat[MDZ] != 0.0 )
			(void)printf(" [%f,%f,%f]",
				Mat[MDX], Mat[MDY], Mat[MDZ]);
		(void)putchar('\n');
	}
#undef Mat
}

/* ZAP the display -- everything dropped */
/* Format: Z	*/
f_zap()
{
	register struct solid *sp;
	register struct solid *nsp;

	/* FIRST, reject any editing in progress */
	if( state != ST_VIEW )
		button( BE_REJECT );

	sp=HeadSolid.s_forw;
	while( sp != &HeadSolid )  {
		memfree( &(dmp->dmr_map), sp->s_addr, sp->s_bytes );
		nsp = sp->s_forw;
		DEQUEUE_SOLID( sp );
		FREE_SOLID( sp );
		sp = nsp;
	}
	dmaflag = 1;
}

f_status()
{
	printf("STATE=%s, ", state_str[state] );
	printf("maxview=%f, ", maxview);
	printf("Viewscale=%f\n", Viewscale);
	mat_print("toViewcenter", toViewcenter);
	mat_print("Viewrot", Viewrot);
	mat_print("model2view", model2view);
	mat_print("view2model", view2model);
	if( state != ST_VIEW )  {
		mat_print("model2objview", model2objview);
		mat_print("objview2model", objview2model);
	}
}

/* Fix the display processor after a hardware error, as best we can */
f_fix()
{
	dmp->dmr_restart();
	dmaflag = 1;		/* causes refresh() */
}

f_refresh()
{

	dmaflag = 1;		/* causes refresh() */
}

#define LEN 32
f_rt()
{
	register char **vp;
	register struct solid *sp;
	register int i;
	int pid, rpid;
	int retcode;
	int o_pipe[2];
	char *vec[LEN];
	FILE *fp;

	if( state != ST_VIEW )  {
		state_err( "Ray-trace of current view" );
		return;
	}
	vp = &vec[0];
	*vp++ = "rt";
	*vp++ = "-f";
	*vp++ = "-M";
	for( i=1; i < numargs; i++ )
		*vp++ = cmd_args[i];
	*vp++ = filename;

	/* Find all unique top-level entrys.
	 *  Mark ones already done with s_iflag == UP
	 */
	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;
	FOR_ALL_SOLIDS( sp )  {
		register struct solid *forw;	/* XXX */

		if( sp->s_iflag == UP )
			continue;
		if( vp < &vec[LEN] )
			*vp++ = sp->s_path[0]->d_namep;
		else
			printf("ran out of vec for %s\n",
				sp->s_path[0]->d_namep );
		sp->s_iflag = UP;
		for( forw=sp->s_forw; forw != &HeadSolid; forw=forw->s_forw) {
			if( forw->s_path[0] == sp->s_path[0] )
				forw->s_iflag = UP;
		}
	}
	*vp = (char *)0;

	vp = &vec[0];
	while( *vp )
		printf("%s ", *vp++ );
	printf("\n");

	(void)pipe( o_pipe );
	(void)signal( SIGINT, SIG_IGN );
	(void)signal( SIGQUIT, SIG_IGN );
	if ( ( pid = fork()) == 0 )  {
		close(0);
		dup( o_pipe[0] );
		for( i=3; i < 20; i++ )
			(void)close(i);

		(void)signal( SIGINT, SIG_DFL );
		(void)signal( SIGQUIT, SIG_DFL );
		(void)execvp( "rt", vec );
		perror( "rt" );
		exit(42);
	}
	/* Connect up to pipe */
	close( o_pipe[0] );
	fp = fdopen( o_pipe[1], "w" );

	/* Send out model2view matrix */
	for( i=0; i < 16; i++ )
		fprintf( fp, "%f ", model2view[i] );
	fclose( fp );
	
	/* Wait for rt to finish */
	while ((rpid = wait(&retcode)) != pid && rpid != -1)
		;	/* NULL */
	(void)signal(SIGINT, quit);
	(void)signal(SIGQUIT, sig3);

	FOR_ALL_SOLIDS( sp )
		sp->s_iflag = DOWN;
	(void)printf("done\n");
}
#undef LEN

void
f_attach()
{
	attach( cmd_args[1] );
}

void
f_release()
{
	release();
}
