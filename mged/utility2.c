/*
 *			U T I L I T Y 2 . C
 *
 *
 *	f_tabobj()	tabs objects as they are stored in data file
 *	f_pathsum()	gives various path summaries
 *	f_copyeval()	copy an evaluated solid
 *	trace()		traces hierarchy of objects
 *	f_push()	control routine to push transformations to bottom of paths
 *	push()		pushes all transformations to solids at bottom of paths
 *	identitize()	makes all transformation matrices == identity for an object
 *
 */

#include <signal.h>
#include <stdio.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "./machine.h"	/* special copy */
#include "vmath.h"
#include "db.h"
#include "./ged.h"
#include "./sedit.h"
#include "./objdir.h"

extern int	args;		/* total number of args available */
extern int	argcnt;		/* holder for number of args added later */
extern int	newargs;	/* number of args from getcmd() */
extern int	numargs;	/* number of args */
extern char	*cmd_args[];	/* array of pointers to args */

static union record record;

#define MAXLINE		512	/* Maximum number of chars per line */





/*  	F _ T A B O B J :   tabs objects as they appear in data file
 */

f_tabobj( )
{
	register struct directory *dp;
	int ngran, nmemb;
	int i, j, k, kk;

	/* interupts */
	(void)signal( SIGINT, sig2 );

	for(i=1; i<numargs; i++) {
		if( (dp = lookup(cmd_args[i], LOOKUP_NOISY)) == DIR_NULL )
			continue;
		db_getrec(dp, (char *)&record, 0);
		if(record.u_id == ID_ARS_A) {
			(void)printf("%c %d %s ",record.a.a_id,record.a.a_type,record.a.a_name);
			(void)printf("%d %d %d %d\n",record.a.a_m,record.a.a_n,
				record.a.a_curlen,record.a.a_totlen);
			/* the b-records */
			ngran = record.a.a_totlen;
			for(j=1; j<=ngran; j++) {
				db_getrec(dp, (char *)&record, j);
				(void)printf("%c %d %d %d\n",record.b.b_id,record.b.b_type,record.b.b_n,record.b.b_ngranule);
				for(k=0; k<24; k+=6) {
					for(kk=k; kk<k+6; kk++)
						(void)printf("%10.4f ",record.b.b_values[kk]*base2local);
					(void)printf("\n");
				}
			}
		}

		if(record.u_id == ID_SOLID) {
			(void)printf("%c %d %s %d\n", record.s.s_id,
				record.s.s_type,record.s.s_name,
				record.s.s_cgtype);
			for(kk=0;kk<24;kk+=6){
				for(j=kk;j<kk+6;j++)
					(void)printf("%10.4f ",record.s.s_values[j]*base2local);
				(void)printf("\n");
			}
		}
		if(record.u_id == ID_COMB) {
			(void)printf("%c '%c' %s %d %d %d %d %d \n",
			record.c.c_id,record.c.c_flags,
			record.c.c_name,record.c.c_regionid,
			record.c.c_aircode,record.c.c_length,
			record.c.c_material,record.c.c_los);
			nmemb = record.c.c_length;
			for(j=1; j<=nmemb; j++) {
				db_getrec(dp, (char *)&record, j);
				(void)printf("%c %c %s\n",
					record.M.m_id,
					record.M.m_relation,
					record.M.m_instname);
				matrix_print( record.M.m_mat );
				(void)putchar('\n');
			}
		}
		if(record.u_id == ID_P_HEAD) {
			(void)printf("POLYGON: not implemented yet\n");
		}

		if(record.u_id == ID_BSOLID) {
			(void)printf("SPLINE: not implemented yet\n");
		}
	}
	return;
}


#define MAX_LEVELS 12
#define CPEVAL		0
#define LISTPATH	1
#define LISTEVAL	2

/* input path */
struct directory *obj[MAX_LEVELS];
int objpos;

/* print flag */
int prflag;

/* path transformation matrix ... calculated in trace() */
mat_t xform;

/*  	F _ P A T H S U M :   does the following
 *		1.  produces path for purposes of matching
 *      	2.  gives all paths matching the input path OR
 *		3.  gives a summary of all paths matching the input path
 *		    including the final parameters of the solids at the bottom
 *		    of the matching paths
 */


f_pathsum( )
{
	int i, flag, pos_in;

	/* pos_in = first member of path entered
	 *
	 *	paths are matched up to last input member
	 *      ANY path the same up to this point is considered as matching
	 */
	prflag = 0;

	pos_in = args = numargs;
	argcnt = 0;

	/* interupts */
	(void)signal( SIGINT, sig2 );

	/* find out which command was entered */
	if( strcmp( cmd_args[0], "paths" ) == 0 ) {
		/* want to list all matching paths */
		flag = LISTPATH;
	}
	if( strcmp( cmd_args[0], "listeval" ) == 0 ) {
		/* want to list evaluated solid[s] */
		flag = LISTEVAL;
	}

	/* get the path - ignore any input so far */
	(void)printf("Enter the path (space is delimiter): ");
	argcnt = getcmd(args);
	args += argcnt;
	objpos = argcnt;

	/* build directory pointer array for desired path */
	for(i=0; i<objpos; i++) {
		if( (obj[i] = lookup(cmd_args[pos_in+i], LOOKUP_NOISY)) == DIR_NULL)
			return;
	}

	mat_idn(identity);
	mat_idn( xform );

	trace(obj[0], 0, identity, flag);

	if(prflag == 0) {
		/* path not found */
		(void)printf("PATH:  ");
		for(i=0; i<objpos; i++)
			(void)printf("/%s",obj[i]->d_namep);
		(void)printf("  NOT FOUND\n");
	}

}



/*   	F _ C O P Y E V A L : copys an evaluated solid
 */

static union record saverec;

f_copyeval( )
{

	register struct directory *dp;
	int i, j, k, kk, ngran;
	int pos_in;
	float vec[3];

	prflag = 0;
	pos_in = args = numargs;
	argcnt = 0;

	/* interupts */
	(void)signal( SIGINT, sig2 );

	/* get the path - ignore any input so far */
	(void)printf("Enter the complete path: ");
	argcnt = getcmd(args);
	args += argcnt;
	objpos = argcnt;

	/* build directory pointer array for desired path */
	for(i=0; i<objpos; i++) {
		if( (obj[i] = lookup(cmd_args[pos_in+i], LOOKUP_NOISY)) == DIR_NULL)
			return;
	}

	/* check if last path member is a solid */
	db_getrec( obj[objpos-1], (char *)&record, 0);
	if(record.u_id != ID_SOLID && record.u_id != ID_ARS_A &&
		record.u_id != ID_BSOLID && record.u_id != ID_P_HEAD) {
		(void)printf("Bottom of path %s is not a solid\n",dp->d_namep);
		return;
	}

	/* get the new solid name */
	(void)printf("Enter the new solid name: ");
	argcnt = getcmd(args);

	/* check if new solid name already exists in description */
	if( lookup(cmd_args[args], LOOKUP_QUIET) != DIR_NULL ) {
		(void)printf("%s: already exists\n",cmd_args[args]);
		return;
	}

	mat_idn( identity );
	mat_idn( xform );

	trace(obj[0], 0, identity, CPEVAL);

	if(prflag == 0) {
		(void)printf("PATH:  ");
		for(i=0; i<objpos; i++)
			(void)printf("/%s",obj[i]->d_namep);
		(void)printf("  NOT FOUND\n");
		return;
	}

	/* No interupts */
	(void)signal( SIGINT, SIG_IGN );

	/* Have found the desired path - xform is the transformation matrix */
	/* xform matrix calculated in trace() */

	/* create the new solid */
	if(saverec.u_id == ID_ARS_A) {
		NAMEMOVE(cmd_args[args], saverec.a.a_name);
		ngran = saverec.a.a_totlen;
		if( (dp = dir_add(saverec.a.a_name, -1, DIR_SOLID, ngran+1)) == DIR_NULL )
			return;
		db_alloc( dp, ngran+1 );
		db_putrec(dp, &saverec, 0);

		/* apply transformation to the b-records */
		for(i=1; i<=ngran; i++) {
			db_getrec( obj[objpos-1], (char *)&record, i );
			if(i == 1) {
				/* vertex */
				MAT4X3PNT( vec, xform,
						&record.b.b_values[0] );
				VMOVE(&record.b.b_values[0], vec);
				kk = 1;
			}


			/* rest of the vectors */
			for(k=kk; k<8; k++) {
				MAT4X3VEC( vec, xform,
						&record.b.b_values[k*3] );
				VMOVE(&record.b.b_values[k*3], vec);
			}
			kk = 0;

			/* write this b-record */
			db_putrec(dp, &record, i);
		}
		return;
	}

	if(saverec.u_id == ID_BSOLID) {
		(void)printf("B-SPLINEs not implemented\n");
		return;
	}

	if(saverec.u_id == ID_P_HEAD) {
		(void)printf("POLYGONs not implemented\n");
		return;
	}

	if(saverec.u_id == ID_SOLID) {
		NAMEMOVE(cmd_args[args], saverec.s.s_name);
		if( (dp = dir_add(saverec.s.s_name, -1, DIR_SOLID, 1)) == DIR_NULL )
			return;
		MAT4X3PNT( vec, xform, &saverec.s.s_values[0] );
		VMOVE(&saverec.s.s_values[0], vec);
		for(i=3; i<=21; i+=3) {
			MAT4X3VEC( vec, xform, &saverec.s.s_values[i] );
			VMOVE(&saverec.s.s_values[i], vec);
		}
		db_alloc( dp, 1 );
		db_putrec( dp, &saverec, 0 );
		return;
	}

}



/*	trace()		traces heirarchy of paths
 */

/* current path being traced */
extern struct directory *path[MAX_LEVELS];

trace( dp, pathpos, old_xlate, flag)
register struct directory *dp;
int pathpos;
mat_t old_xlate;
int flag;
{

	struct directory *nextdp;
	mat_t new_xlate;
	int nparts, i, k, j;
	int arslen, kk, npt, n;
	float vertex[3];
	float vec[3];

	if( pathpos >= MAX_LEVELS ) {
		(void)printf("nesting exceeds %d levels\n",MAX_LEVELS);
		for(i=0; i<MAX_LEVELS; i++)
			(void)printf("/%s", path[i]->d_namep);
		(void)printf("\n");
		return;
	}

	db_getrec(dp, (char *)&record, 0);

	if( record.u_id == ID_COMB ) {
		nparts = record.c.c_length;
		for(i=1; i<=nparts; i++) {
			db_getrec(dp, (char *)&record, i);
			path[pathpos] = dp;
			if( (nextdp = lookup(record.M.m_instname, LOOKUP_NOISY)) == DIR_NULL )
				continue;
			/* Recursive call */
			mat_mul(new_xlate, old_xlate, record.M.m_mat);

			trace(nextdp, pathpos+1, new_xlate, flag);

		}
		return;
	}

	/* not a combination  -  should have a solid */
	/*
	 *	TO DO ..... include SPLINES and POLYGONS 
 	 */
	if(record.u_id != ID_SOLID && record.u_id != ID_ARS_A) {
		(void)printf("bad record type '%c' should be 'S' or 'A'\n",record.u_id);
		return;
	}

	/* last (bottom) position */
	path[pathpos] = dp;

	/* check for desired path */
	for(k=0; k<objpos; k++) {
		if(path[k]->d_addr != obj[k]->d_addr) {
			/* not the desired path */
			return;
		}
	}

	/* have the desired path up to objpos */
	mat_copy(xform, old_xlate);
	prflag = 1;

	if(flag == CPEVAL) { 
		/* save this record */
		db_getrec(dp, (char *)&saverec, 0);
		return;
	}

	/* print the path */
	for(k=0; k<pathpos; k++)
		(void)printf("/%s",path[k]->d_namep);
	if(flag == LISTPATH) {
		(void)printf("/%s\n",record.s.s_name);
		return;
	}

	/* NOTE - only reach here if flag == LISTEVAL */
	if(record.u_id == ID_SOLID) {
		(void)printf("/%s:\n",record.s.s_name);
		MAT4X3PNT(vec, xform, &record.s.s_values[0]);
		VMOVE(&record.s.s_values[0], vec);
		for(i=3; i<=21; i+=3) {
			MAT4X3VEC(	vec, xform,
					&record.s.s_values[i] );
			VMOVE(&record.s.s_values[i], vec);
		}
		/* put parameters in "nice" format and print */
		pr_solid( &record.s );
		for( i=0; i < es_nlines; i++ )
			(void)printf("%s\n",&es_display[ES_LINELEN*i]);

		/* If in solid edit, re-compute solid params */
		if(state == ST_S_EDIT)
			pr_solid(&es_rec.s);

		return;
	}

	if(record.u_id == ID_ARS_A) {
		(void)printf("/%s:\n",record.a.a_name);
		n = record.a.a_n;
		(void)printf("%d curves  %d points per curve\n",record.a.a_m,n);
		arslen = record.a.a_totlen;
		for(i=1; i<=arslen; i++) {
			db_getrec(dp, (char *)&record, i);
			if( (npt = (n - ((record.b.b_ngranule-1)*8))) > 8 )
				npt = 8;
			if(i == 1) {
				/* vertex */
				MAT4X3PNT(	vertex,
						xform,
						&record.b.b_values[0] );
				VMOVE( &record.b.b_values[0], vertex );
				kk = 1;
			}
			/* rest of vectors */
			for(k=kk; k<npt; k++) {
				MAT4X3VEC(	vec,
						xform,
						&record.b.b_values[k*3] );
				VADD2(	&record.b.b_values[k*3],
					vertex,
					vec	);
			}
			kk = 0;
			/* print this granule */
			for(k=0; k<npt; k+=2) {
				for(j=k*3; (j<(k+2)*3 && j<npt*3); j++) 
					(void)printf("%10.4f ",record.b.b_values[j]);
				(void)printf("\n");
			}

		}
		return;
	}
	if(record.u_id == ID_P_HEAD) {
		(void)printf("/%s:\n",record.p.p_name);
		(void)printf("POLYGON data print not implemented\n");
		return;
	}
	if(record.u_id == ID_BSOLID) {
		(void)printf("/%s:\n",record.B.B_name);
		(void)printf("B-SPLINE data print not implemented\n");
		return;
	}

}






/*
 *			M A T R I X _ P R I N T
 *
 * Print out the 4x4 matrix addressed by "m".
 */
matrix_print( m )
register matp_t m;
{
	register int i;

	for(i=0; i<16; i++) {
		if( (i+1)%4 )
			(void)printf("%f\t",m[i]);
		else if(i == 15)
			(void)printf("%f\t",m[i]);
		else
			(void)printf("%f\n",m[i]*base2local);
	}
}




/* structure to distinguish "pushed" solids */
struct idpush {
	char	i_name[NAMESIZE];
	mat_t	i_mat;
};
static struct idpush idpush, idbuf;

#define MAXSOL 2000
extern int discr[], idfd, rd_idfd;	/* from utility1 */
static int push_count;		/* count of solids to be pushed */
static int abort_flag;

/*
 *		F _ P U S H ( )
 *
 *	control routine for "pushing" transformations to bottom of paths
 *
 */
f_push( )
{

	struct directory *dp, *tdp;
	int i, j, k, kk, ii, ngran;
	float vec[3];

	(void)signal( SIGINT, sig2 );		/* interupts */

	/* open temp file */
	if( (idfd = creat("/tmp/mged_push", 0666)) < 0 ) {
		perror( "/tmp/mged_push" );
		return;
	}
	rd_idfd = open( "/tmp/mged_push", 2 );

	for(i=1; i<numargs; i++) {
		if( (dp = lookup(cmd_args[i], LOOKUP_NOISY)) == DIR_NULL ) {
			(void)printf("Skip this object\n");
			continue;
		}
		push_count = 0;		/* NO solids yet */
		abort_flag = 0;
		mat_idn( identity );
		push(dp, 0, identity);
		if( abort_flag ) {
			/* Cannot push transformations for this object */
			(void)printf("%s: push failed\n",cmd_args[i]);
			continue;
		}
		/* It's okay to "push" this object */
		(void)signal( SIGINT, SIG_IGN );	/* no interupts */
		(void)lseek(rd_idfd, 0L, 0);
		for(j=0; j<push_count; j++) {
			(void)read(rd_idfd, &idpush, sizeof idpush);

			/* apply transformation to this solid */
			if( (tdp = lookup(idpush.i_name, LOOKUP_QUIET)) == DIR_NULL ) {
				(void)printf("push: cannot find solid (%s)\n",
						idpush.i_name);
				continue;
			}
			db_getrec(tdp, (char *)&record, 0);
			switch( record.u_id ) {

			case ID_SOLID:
				MAT4X3PNT(	vec,
						idpush.i_mat,
						&record.s.s_values[0]	);
				VMOVE( &record.s.s_values[0], vec );
				for(k=3; k<=21; k+=3) {
					MAT4X3VEC(	vec,
							idpush.i_mat,
							&record.s.s_values[k]	);
					VMOVE( &record.s.s_values[k], vec );
				}
				db_putrec(tdp, (char *)&record, 0);
			break;

			case ID_ARS_A:
				/* apply transformation to the b-records */
				ngran = record.a.a_totlen;
				for(ii=1; ii<=ngran; ii++) {
					db_getrec( tdp, (char *)&record, ii );
					if(ii == 1) {
						/* vertex */
						MAT4X3PNT(	vec,
								idpush.i_mat,
								&record.b.b_values[0] );
						VMOVE( &record.b.b_values[0], vec );
						kk = 1;
					}

					/* rest of the vectors */
					for(k=kk; k<8; k++) {
						MAT4X3VEC(	vec,
								idpush.i_mat,
								&record.b.b_values[k*3] );
						VMOVE( &record.b.b_values[k*3], vec );
					}
					kk = 0;

					/* write this b-record */
					db_putrec(tdp, &record, ii);
				}
			break;

			/*
			 * NOTE:  these cases are checked in "push" so won't reach here.
			 *	When splines and polygons are implemented, must update
			 *	this section.
			 */
			case ID_BSOLID:
				(void)printf("WARNING: (%s) SPLINE not pushed with other elements\n",
						record.B.B_name);
			break;

			case ID_P_HEAD:
				(void)printf("WARNING: (%s) POLYGON not pushed with other elements\n",
						record.p.p_name);
			break;

			default:
				(void)printf("push: unknown solid type (%c) \n",
						record.u_id);
			}	/* end of switch */

		}

		/*
		 *	Finished all the paths for this object
		 *
		 *	Identitize all member matrices for this object
		 */
		identitize( dp );
		(void)printf("%s: transformations pushed\n",cmd_args[i]);
		(void)signal( SIGINT, sig2 );		/* interupts */
	}

	(void)unlink( "/tmp/mged_push\0" );
}



/*
 *				P U S H ( )
 *
 *	Given an object, traverses each path to bottom solid.
 *	This solid is checked against previous solids.
 * 	If this name appeared previously, then the matrices are checked.
 *	If matrices are different, then will not be able to "push" this object.
 *
 */


push( dp, pathpos, old_xlate )
struct directory *dp;
int pathpos;
mat_t	old_xlate;
{
	struct directory *nextdp;
	mat_t new_xlate;
	int nparts, i, k, j;
	int dchar = 0;

	if( abort_flag == 99 )	/* go no further */
		return;

	if( pathpos >= MAX_LEVELS ) {
		(void)printf("nesting exceeds %d levels\n",MAX_LEVELS);
		for(i=0; i<MAX_LEVELS; i++)
			(void)printf("/%s", path[i]->d_namep);
		(void)printf("\n");
		abort_flag = 1;
		return;
	}

	db_getrec(dp, (char *)&record, 0);
	if( record.u_id == ID_COMB ) {
		nparts = record.c.c_length;
		for(i=1; i<=nparts; i++) {
			db_getrec(dp, (char *)&record, i);
			path[pathpos] = dp;
			if( (nextdp = lookup(record.M.m_instname, LOOKUP_NOISY)) == DIR_NULL )
				continue;
			/* Recursive call */
			mat_mul(new_xlate, old_xlate, record.M.m_mat);
			push(nextdp, pathpos+1, new_xlate);
		}
		return;
	}

	/* not a combination  -  should have a solid */
	if(record.u_id == ID_BSOLID) {
		(void)printf("push: (%s) SPLINE not implemented yet - abort\n",
				record.B.B_name);
		abort_flag = 1;
		return;
	}
	if(record.u_id == ID_P_HEAD) {
		(void)printf("push: (%s) POLYGON not implemented yet - abort\n",
				record.p.p_name);
		abort_flag = 1;
		return;
	}
	if(record.u_id != ID_SOLID && record.u_id != ID_ARS_A) {
		(void)printf("bad record type '%c' should be 'S' or 'A'\n",record.u_id);
		abort_flag = 1;
		return;
	}

	/* last (bottom) position */
	path[pathpos] = dp;

	/* check if this is a new or old solid */
	if(record.u_id == ID_SOLID) {
		(void)strncpy(idpush.i_name, record.s.s_name, NAMESIZE);
	}
	else {
		(void)strncpy(idpush.i_name, record.a.a_name, NAMESIZE);
	}
	mat_copy(idpush.i_mat, old_xlate);

	dchar = 0;
	for(i=0; i<NAMESIZE; i++) {
		if(idpush.i_name[i] == 0) 
			break;
		dchar += (idpush.i_name[i] << (i&7));
	}

	for(i=0; i<push_count; i++) {
		if(dchar == discr[i]) {
			/* possible match --- check closer */
			(void)lseek(rd_idfd, i*(long)sizeof idpush, 0);
			(void)read(rd_idfd, &idbuf, sizeof idpush);
			if( strcmp( idpush.i_name, idbuf.i_name ) == 0 ) {
				/* names are the same ... check matrices */
				if( check_mat(idpush.i_mat, idbuf.i_mat) == 1 ) {
					/* matrices are also equal---same solid */
					return;
				}
				/* BAD ... matrices are diff but names the same */
				(void)printf("Cannot push: solid (%s) has conflicts\n",
						idpush.i_name);
				abort_flag = 1;
				return;
			}
		}
	}

	/* Have a NEW solid */
	discr[push_count++] = dchar;
	if(push_count > MAXSOL) {
		(void)printf("push: number of solids > max (%d)\n",MAXSOL);
		abort_flag = 99;
		return;
	}
	(void)lseek(idfd, 0L, 2);
	(void)write(idfd, &idpush, sizeof idpush);

	return;

}




/*
 *			I D E N T I T I Z E ( ) 
 *
 *	Traverses an objects paths, setting all member matrices == identity
 *
 */
identitize( dp )
struct directory *dp;
{

	struct directory *nextdp;
	int nparts, i;

	db_getrec(dp, (char *)&record, 0);
	if( record.u_id == ID_COMB ) {
		nparts = record.c.c_length;
		for(i=1; i<=nparts; i++) {
			db_getrec(dp, (char *)&record, i);

			/* set member matrix to identity */
			mat_idn( record.M.m_mat );
			db_putrec(dp, (char *)&record, i);

			if( (nextdp = lookup(record.M.m_instname, LOOKUP_NOISY)) == DIR_NULL )
				continue;
			/* Recursive call */
			identitize( nextdp );
		}
		return;
	}
	/* bottom position */
	return;
}





/*	C H E C K _ M A T ( )	 -  compares 4x4 matrices
 *		returns 1 if same.....0 otherwise
 */

check_mat( mat1, mat2 )
mat_t mat1, mat2;
{
	register int i;

	for(i=0; i<16; i++) {
		if( mat1[i] != mat2[i] )
			return( 0 );	/* different */
	}

	return( 1 );	/* same */

}





