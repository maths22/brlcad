/*
 *			R E G I O N . C
 */
#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "vmath.h"
#include "wdb.h"

extern char	name_it[];

extern struct wmember	*wmp;	/* array indexed by region number */

extern FILE	*infp;
extern FILE	*outfp;

extern int	reg_total;
extern int	version;

struct rcard  {
	char	rc_num[5];
	char	rc_null;
	struct	rcfields  {
		char	rcf_null;
		char	rcf_or;
		char	rcf_solid[5];
	}  
	rc_fields[9];
	char	rc_remark[11+3];
} rcard;

/*
 *			G E T R E G I O N
 *
 *  Use wmp[region_number] as head for each region.
 */
getregion()
{
	int i, j;
	int card;
	int	op;
	int reg_reg_flag;
	int	reg_num;
	char	inst_name[32];
	int	inst_num;
	char *cp;

	/* Pre-load very first region card */
	if( getline( &rcard ) == EOF )  {
		printf("getregion: premature EOF\n");
		return( -1 );
	}

top:
	reg_reg_flag = 0;

	for( card=0; ; card++ )  {
		if( card == 0 )  {
			/* First card is already in input buffer */
			rcard.rc_null = 0;	/* Null terminate rc_num */
			reg_num = atoi( rcard.rc_num );

			/* -1 region number terminates table */
			if( reg_num < 0 ) 
				return( 0 );		/* Done */

			namecvt( reg_num, wmp[reg_num].wm_name, 'r' );
		} else {
			if( getline( &rcard ) == EOF )  {
				printf("getregion: premature EOF\n");
				return( -1 );
			}
			rcard.rc_null = 0;	/* Null terminate rc_num */
			if( atoi( rcard.rc_num ) != 0 )  {
				/* finished with this region */
				break;
			}
		}

		cp = (char *) rcard.rc_fields;

		/* Scan each of the 9 fields on the card */
		for( i=0; i<9; i++ )  {
			struct wmember	*membp;

			cp[7] = 0;	/* clobber succeeding 'O' pos */

			/* check for "-    5" field which atoi will
			 *	return a zero
			 */
			if(cp[2] == '-' || cp[2] == '+') {
				/* remove any followin blanks */
				for(j=3; j<6; j++) {
					if(cp[j] == ' ') {
						cp[j] = cp[j-1];
						cp[j-1] = ' ';
					}
				}
			}

			inst_num = atoi( cp+2 );

			/* Check for null field -- they are to be skipped */
			if( inst_num == 0 )  {
				cp += 7;
				continue;	/* zeros are allowed as placeholders */
			}

			if( version == 5 )  {
				/* Region references region in Gift5 */
				if(rcard.rc_fields[i].rcf_or == 'g' ||
				   rcard.rc_fields[i].rcf_or == 'G')
					reg_reg_flag = 1;

				if( cp[1] == 'R' || cp[1] == 'r' ) 
					op = WMOP_UNION;
				else {
					if( inst_num < 0 )  {
						op = WMOP_SUBTRACT;
						inst_num = -inst_num;
					}  else  {
						op = WMOP_INTERSECT;
					}
				}
			} else {
				/* XXX this may actually be an old piece of code,
				 * rather than the V4 way of doing it. */
				if( cp[1] != ' ' )  {
					op = WMOP_UNION;
				}  else  {
					if( inst_num < 0 )  {
						op = WMOP_SUBTRACT;
						inst_num = -inst_num;
					}  else  {
						op = WMOP_INTERSECT;
					}
				}
			}

			/* In Gift5, regions can reference regions */
			if( reg_reg_flag )
				namecvt(inst_num, inst_name, 'r');
			else
				namecvt( inst_num, inst_name, 's' );
			reg_reg_flag = 0;

			membp = mk_addmember( inst_name, &wmp[reg_num] );
			membp->wm_op = op;
col_pr( inst_name);

			cp += 7;
		}
	}

	col_pr( wmp[reg_num].wm_name );

	/* The region will be output later in getid(), below */

	goto top;
}

struct idcard  {
	char	id_foo[80];
#if 0
#ifdef GIFT5
	char	id_region[5];
	char	id_rid[5];
	char	id_air[5];
	char	id_mat[5];
	char	id_los[5];
	char	id_waste[55];
#else
	char	id_region[10];
	char	id_rid[10];
	char	id_air[10];
	char	id_waste[44];
	char	id_mat[3];	/* use any existing material code */
	char	id_los[3];	/* use any existing los percentage */
#endif
#endif
} idcard;

/*
 *			G E T I D
 *
 * Load the region ID information into the structures
 */
getid()
{
	register int	i;
	int reg_num;
	int id;
	int air;
	int mat= -1;
	int los= -2;
	char buff[11];
	int	buflen;
	register struct wmember	*wp;

	if( getline( (char *) &idcard ) == EOF ||
	    ((char *) &idcard)[0] == '\n' )
		return( 0 );

	/* XXX needs to handle blanked out fields */
	if( version == 5 )  {
		sscanf( &idcard, "%5d%5d%5d%5d%5d",
			&reg_num, &id, &air, &mat, &los );
	} else {
		sscanf( &idcard, "%10d%10d%10d%*44s%3d%3d",
			&reg_num, &id, &air, &mat, &los );
	}
printf("reg_num=%d,id=%d,air=%d,mat=%d,los=%d\n", reg_num,id,air,mat,los);

#if 0
	if( version == 5 )
		buflen = 5;
	else
		buflen = 10;
	buff[buflen] = '\0';

	for(i=0; i<buflen; i++)
		buff[i] = idcard.id_region[i];
	reg_num = atoi( buff );

	for(i=0; i<buflen; i++)
		buff[i] = idcard.id_rid[i];
	id = atoi( buff );

	for(i=0; i<buflen; i++)
		buff[i] = idcard.id_air[i];
	air = atoi( buff );

	if( version == 5 )  {
		for(i=0; i<5; i++)
			buff[i] = idcard.id_mat[i];
		mat = atoi( buff );

		for(i=0; i<5; i++)
			buff[i] = idcard.id_los[i];
		los = atoi( buff );
	} else {
		idcard.id_mat[2] = '\0';
		mat = atoi( idcard.id_mat );

		for( i=0; i<3; i++ )
			buff[i] = idcard.id_los[i];
		buff[3] = '\0';
		los = atoi( buff );
	}
#endif

	wp = &wmp[reg_num];
	if( wp->wm_forw == wp )  {
		printf("Region %s is empty\n", wp->wm_name );
		return(1);	/* empty region */
	}

	mk_lrcomb( outfp, wp->wm_name, wp, reg_num,
		"", "", (char *)0, id, air, mat, los, 0 );

	/* Add region to the one group that it belongs to. */
	group_add( id, wp->wm_name );

	return( 1 );
}

#define NGROUPS	21
struct groups {
	struct wmember	grp_wm;
	int		grp_lo;
	int		grp_hi;
} groups[NGROUPS];
int	ngroups;

group_init()
{
	group_register( "g00", 0, 0 );
	group_register( "g0", 1, 99 );
	group_register( "g1", 100, 199);
	group_register( "g2", 200, 299 );
	group_register( "g3", 300, 399 );
	group_register( "g4", 400, 499 );
	group_register( "g5", 500, 599 );
	group_register( "g6", 600, 699 );
	group_register( "g7", 700, 799 );
	group_register( "g8", 800, 899 );
	group_register( "g9", 900, 999 );
	group_register( "g10", 1000, 1999 );
	group_register( "g11", 2000, 2999 );
	group_register( "g12", 3000, 3999 );
	group_register( "g13", 4000, 4999 );
	group_register( "g14", 5000, 5999 );
	group_register( "g15", 6000, 6999 );
	group_register( "g16", 7000, 7999 );
	group_register( "g17", 8000, 8999 );
	group_register( "g18", 9000, 9999 );
	group_register( "g19", 10000, 32767 );

}

group_register( name, lo, hi )
char	*name;
{
	char	nbuf[32];
	register struct wmember	*wp;

	if( ngroups >= NGROUPS )  {
		printf("Too many groups, ABORTING\n");
		exit(13);
	}
	wp = &groups[ngroups].grp_wm;

	sprintf( nbuf, "%s%s", name, name_it );
	strncpy( wp->wm_name, nbuf, sizeof(wp->wm_name) );

	wp->wm_forw = wp->wm_back = wp;

	groups[ngroups].grp_lo = lo;
	groups[ngroups].grp_hi = hi;
	ngroups++;
}

group_add( val, name )
register int	val;
char		*name;
{
	register int	i;

	for( i=ngroups-1; i>=0; i-- )  {
		if( val < groups[i].grp_lo )  continue;
		if( val > groups[i].grp_hi )  continue;
		goto add;
	}
	printf("Unable to find group for value %d\n", val);
	i = 0;

add:
	(void)mk_addmember( name, &groups[i].grp_wm );
}

group_write()
{
	register struct wmember	*wp;
	register int	i;

	for( i=0; i < ngroups; i++ )  {
		wp = &groups[i].grp_wm;
		/* Skip empty groups */
		if( wp->wm_forw == wp )  continue;

		mk_lfcomb( outfp, wp->wm_name, wp, -1 );

		col_pr( wp->wm_name );
	}
	/* Could make all-encompasing "all.g" group here */
}
