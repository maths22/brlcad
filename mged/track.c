
/*
 *			T R A C K . C
 *
 *	f_amtrack():	Adds "tracks" to the data file given the required info
 *
 */

#include <signal.h>
#include <stdio.h>
#include <math.h>
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

#include "./machine.h"
#include "vmath.h"
#include "db.h"
#include "./ged.h"
#include "./objdir.h"
#include "./dm.h"

extern void aexists();
extern double atof();
extern int atoi();
extern char *strcat(), *strcpy();

double fabs();

extern int 	numargs;
extern char	*cmd_args[];
extern int	newargs;
extern int	args;
extern int	argcnt;

static int	Trackpos = 0;
static float	plano[4], plant[4];

static union record record;

#define MAXLINE	512

/*
 *
 *	F _ A M T R A C K ( ) :	adds track given "wheel" info
 *
 */

f_amtrack(  )
{

	register struct directory *dp;
	float fw[3], lw[3], iw[3], dw[3], tr[3];
	char solname[12], regname[12], grpname[9], oper[3];
	int i, j, memb[4];
	char temp[4];
	float temp1[3], temp2[3];
	int item, mat, los;

	/* interupts */
	(void)signal( SIGINT, sig2 );

	oper[0] = oper[2] = INTERSECT;
	oper[1] = SUBTRACT;

	args = numargs;
	argcnt = 0;

	/* get the roadwheel info */
	(void)printf("Enter X of the FIRST roadwheel: ");
	argcnt = getcmd(args);
	fw[0] = atof( cmd_args[args] ) * local2base;
	args += argcnt;
	(void)printf("Enter X of the LAST roadwheel: ");
	argcnt = getcmd(args);
	lw[0] = atof( cmd_args[args] ) * local2base;
	args += argcnt;
	if( fw[0] <= lw[0] ) {
		(void)printf("First wheel after last wheel - STOP\n");
		return;
	}
	(void)printf("Enter Z of the roadwheels: ");
	argcnt = getcmd(args);
	fw[1] = lw[1] = atof( cmd_args[args] ) * local2base;
	args += argcnt;
	(void)printf("Enter radius of the roadwheels: ");
	argcnt = getcmd(args);
	fw[2] = lw[2] = atof( cmd_args[args] ) * local2base;
	if( fw[2] <= 0 ) {
		(void)printf("Radius <= 0 - STOP\n");
		return;
	}
	args += argcnt;

	/* get the drive wheel info */
	(void)printf("Enter X of the drive (REAR) wheel: ");
	argcnt = getcmd(args);
	dw[0] = atof( cmd_args[args] ) * local2base;
	if( dw[0] >= lw[0] ) {
		(void)printf("DRIVE wheel not in the rear - STOP \n");
		return;
	}
	args += argcnt;
	(void)printf("Enter Z of the drive (REAR) wheel: ");
	argcnt = getcmd(args);
	dw[1] = atof( cmd_args[args] ) * local2base;
	args += argcnt;
	(void)printf("Enter radius of the drive (REAR) wheel: ");
	argcnt = getcmd(args);
	dw[2] = atof( cmd_args[args] ) * local2base;
	if( dw[2] <= 0 ) {
		(void)printf("Radius <= 0 - STOP\n");
		return;
	}
	args += argcnt;

	/* get the idler wheel info */
	(void)printf("Enter X of the idler (FRONT) wheel: ");
	argcnt = getcmd(args);
	iw[0] = atof( cmd_args[args] ) * local2base;
	args += argcnt;
	if( iw[0] <= fw[0] ) {
		(void)printf("IDLER wheel not in the front - STOP \n");
		return;
	}
	(void)printf("Enter Z of the idler (FRONT) wheel: ");
	argcnt = getcmd(args);
	iw[1] = atof( cmd_args[args] ) * local2base;
	args += argcnt;
	(void)printf("Enter radius of the idler (FRONT) wheel: ");
	argcnt = getcmd(args);
	iw[2] = atof( cmd_args[args] ) * local2base;
	if( iw[2] <= 0 ) {
		(void)printf("Radius <= 0 - STOP\n");
		return;
	}
	args += argcnt;

	/* get track info */
	(void)printf("Enter Y-MIN of the track: ");
	argcnt = getcmd(args);
	tr[2] = tr[0] = atof( cmd_args[args] ) * local2base;
	args += argcnt;
	(void)printf("Enter Y-MAX of the track: ");
	argcnt = getcmd(args);
	tr[1] = atof( cmd_args[args] ) * local2base;
	if( tr[0] == tr[1] ) {
		(void)printf("MIN == MAX ... STOP\n");
		return;
	}
	if( tr[0] > tr[1] ) {
		(void)printf("MIN > MAX .... will switch\n");
		tr[1] = tr[0];
		tr[0] = tr[2];
	}
	args += argcnt;
	(void)printf("Enter track thickness: ");
	argcnt = getcmd(args);
	tr[2] = atof( cmd_args[args] ) * local2base;
	if( tr[2] <= 0 ) {
		(void)printf("Track thickness <= 0 - STOP\n");
		return;
	}

	solname[0] = regname[0] = grpname[0] = 't';
	solname[1] = regname[1] = grpname[1] = 'r';
	solname[2] = regname[2] = grpname[2] = 'a';
	solname[3] = regname[3] = grpname[3] = 'c';
	solname[4] = regname[4] = grpname[4] = 'k';
	solname[5] = regname[5] = '.';
	solname[6] = 's';
	regname[6] = 'r';
	solname[7] = regname[7] = '.';
	grpname[5] = solname[8] = regname[8] = '\0';
	grpname[8] = solname[11] = regname[11] = '\0';
/*
	(void)printf("\nX of first road wheel  %10.4f\n",fw[0]);
	(void)printf("X of last road wheel   %10.4f\n",lw[0]);
	(void)printf("Z of road wheels       %10.4f\n",fw[1]);
	(void)printf("radius of road wheels  %10.4f\n",fw[2]);
	(void)printf("\nX of drive wheel       %10.4f\n",dw[0]);
	(void)printf("Z of drive wheel       %10.4f\n",dw[1]);
	(void)printf("radius of drive wheel  %10.4f\n",dw[2]);
	(void)printf("\nX of idler wheel       %10.4f\n",iw[0]);
	(void)printf("Z of idler wheel       %10.4f\n",iw[1]);
	(void)printf("radius of idler wheel  %10.4f\n",iw[2]);
	(void)printf("\nY MIN of track         %10.4f\n",tr[0]);
	(void)printf("Y MAX of track         %10.4f\n",tr[1]);
	(void)printf("thickness of track     %10.4f\n",tr[2]);
*/

/* Check for names to use:
 *	1.  start with track.s.1->10 and track.r.1->10
 *	2.  if bad, increment count by 10 and try again
 */

tryagain:	/* sent here to try next set of names */

	for(i=0; i<11; i++) {
		crname(solname, i);
		crname(regname, i);
		if(	(lookup(solname, LOOKUP_QUIET) != DIR_NULL)	||
			(lookup(regname, LOOKUP_QUIET) != DIR_NULL)	) {
			/* name already exists */
			solname[8] = regname[8] = '\0';
			if( (Trackpos += 10) > 500 ) {
				(void)printf("Track: naming error -- STOP\n");
				return;
			}
			goto tryagain;
		}
		solname[8] = regname[8] = '\0';
	}

	/* no interupts */
	(void)signal( SIGINT, SIG_IGN );

	record.s.s_id = ID_SOLID;

	/* find the front track slope to the idler */
	for(i=0; i<24; i++)
		record.s.s_values[i] = 0.0;

	slope(fw, iw, tr);
	VMOVE(temp2, &record.s.s_values[0]);
	crname(solname, 1);
	(void)strcpy(record.s.s_name, solname);
	record.s.s_type = GENARB8;
	record.s.s_cgtype = BOX;		/* BOX */
	if( wrobj(solname) ) 
		return;

	solname[8] = '\0';

	/* find track around idler */
	for(i=0; i<24; i++)
		record.s.s_values[i] = 0.0;
	record.s.s_type = GENTGC;
	record.s.s_cgtype = RCC;
	trcurve(iw, tr);
	crname(solname, 2);
	(void)strcpy(record.s.s_name, solname);
	if( wrobj( solname ) )
		return;
	solname[8] = '\0';
	/* idler dummy rcc */
	record.s.s_values[6] = iw[2];
	record.s.s_values[11] = iw[2];
	VMOVE(&record.s.s_values[12], &record.s.s_values[6]);
	VMOVE(&record.s.s_values[15], &record.s.s_values[9]);
	crname(solname, 3);
	(void)strcpy(record.s.s_name, solname);
	if( wrobj( solname ) )
		return;
	solname[8] = '\0';

	/* find idler track dummy arb8 */
	for(i=0; i<24; i++)
		record.s.s_values[i] = 0.0;
	crname(solname, 4);
	(void)strcpy(record.s.s_name, solname);
	record.s.s_type = GENARB8;
	record.s.s_cgtype = ARB8;		/* arb8 */
	crdummy(iw, tr, 1);
	if( wrobj(solname) )
		return;
	solname[8] = '\0';

	/* track slope to drive */
	for(i=0; i<24; i++)
		record.s.s_values[i] = 0.0;
	slope(lw, dw, tr);
	VMOVE(temp1, &record.s.s_values[0]);
	crname(solname, 5);
	(void)strcpy(record.s.s_name, solname);
	record.s.s_cgtype = BOX;		/* box */
	if(wrobj(solname))
		return;
	solname[8] = '\0';

	/* track around drive */
	for(i=0; i<24; i++)
		record.s.s_values[i] = 0.0;
	record.s.s_type = GENTGC;
	record.s.s_cgtype = RCC;
	trcurve(dw, tr);
	crname(solname, 6);
	(void)strcpy(record.s.s_name, solname);
	if( wrobj(solname) )
		return;
	solname[8] = '\0';

	/* drive dummy rcc */
	record.s.s_values[6] = dw[2];
	record.s.s_values[11] = dw[2];
	VMOVE(&record.s.s_values[12], &record.s.s_values[6]);
	VMOVE(&record.s.s_values[15], &record.s.s_values[9]);
	crname(solname, 7);
	(void)strcpy(record.s.s_name, solname);
	if( wrobj(solname) )
		return;
	solname[8] = '\0';

	/* drive dummy arb8 */
	for(i=0; i<24; i++)
		record.s.s_name[i] = 0.0;
	crname(solname, 8);
	(void)strcpy(record.s.s_name, solname);
	record.s.s_type = GENARB8;
	record.s.s_cgtype = ARB8;		/* arb8 */
	crdummy(dw, tr, 2);
	if( wrobj(solname) )
		return;
	solname[8] = '\0';
	
	/* track bottom */
	record.s.s_cgtype = ARB8;		/* arb8 */
	temp1[1] = temp2[1] = tr[0];
	bottom(temp1, temp2, tr);
	crname(solname, 9);
	(void)strcpy(record.s.s_name, solname);
	if( wrobj(solname) )
		return;
	solname[8] = '\0';

	/* track top */
	temp1[0] = dw[0];
	temp1[1] = temp2[1] = tr[0];
	temp1[2] = dw[1] + dw[2];
	temp2[0] = iw[0];
	temp2[2] = iw[1] + iw[2];
	top(temp1, temp2, tr);
	crname(solname, 10);
	(void)strcpy(record.s.s_name, solname);
	if( wrobj(solname) )
		return;
	solname[8] = '\0';

	/* add the regions */
	record.c.c_id = ID_COMB;
	record.c.c_flags = 'R';
	record.c.c_aircode = record.c.c_length = 0;
	record.c.c_regionid = 111;
	record.c.c_material = 0;
	record.c.c_los = 0;

	/* regions 3, 4, 7, 8 - dummy regions */
	for(i=3; i<5; i++) {
		regname[8] = '\0';
		crname(regname, i);
		(void)strcpy(record.c.c_name, regname);
		if( wrobj(regname) )
			return;
		regname[8] = '\0';
		crname(regname, i+4);
		(void)strcpy(record.c.c_name, regname);
		if( wrobj(regname) )
			return;
	}
	regname[8] = '\0';

	item = item_default;
	mat = mat_default;
	los = los_default;
	item_default = 500;
	mat_default = 1;
	los_default = 50;
	/* region 1 */
	memb[0] = 1;
	memb[1] = 4;
	crname(regname, 1);
	crregion(regname, oper, memb, 2, solname);
	solname[8] = regname[8] = '\0';

	/* region 2 */
	crname(regname, 2);
	memb[0] = 2;
	memb[1] = 3;
	memb[2] = 4;
	crregion(regname, oper, memb, 3, solname);
	solname[8] = regname[8] = '\0';

	/* region 5 */
	crname(regname, 5);
	memb[0] = 5;
	memb[1] = 8;
	crregion(regname, oper, memb, 2, solname);
	solname[8] = regname[8] = '\0';

	/* region 6 */
	crname(regname, 6);
	memb[0] = 6;
	memb[1] = 7;
	memb[2] = 8;
	crregion(regname, oper, memb, 3, solname);
	solname[8] = regname[8] = '\0';

	/* region 9 */
	crname(regname, 9);
	memb[0] = 9;
	memb[1] = 1;
	memb[2] = 5;
	oper[2] = SUBTRACT;
	crregion(regname, oper, memb, 3, solname);
	solname[8] = regname[8] = '\0';

	/* region 10 */
	crname(regname, 10);
	memb[0] = 10;
	memb[1] = 4;
	memb[2] = 8;
	crregion(regname, oper, memb, 3, solname);
	solname[8] = regname[8] = '\0';

	/* group all the track regions */
	j = 1;
	if( (i = Trackpos / 10 + 1) > 9 )
		j = 2;
	itoa(i, temp, j);
	(void)strcat(grpname, temp);
	grpname[8] = '\0';
	for(i=1; i<11; i++) {
		regname[8] = '\0';
		crname(regname, i);
		if( (dp=lookup(regname, LOOKUP_QUIET)) == DIR_NULL ) {
			(void)printf("group: %s will skip member: %s\n",grpname,regname);
			continue;
		}
		(void)combadd(dp, grpname, 0, UNION, 0, 0);
	}

	(void)printf("The track regions are in group %s\n",grpname);

	/* draw this track */
	dp = lookup(grpname, LOOKUP_QUIET);
	drawHobj( dp, ROOT, 0, identity, 0 );
	dmp->dmr_colorchange();
	dmaflag = 1;

	Trackpos += 10;
	item_default = item;
	mat_default = mat;
	los_default = los;
	grpname[5] = solname[8] = regname[8] = '\0';

	return;
}


crname(name, pos)
char name[];
int pos;
{
	int i, j;
	char temp[4];

	j=1;
	if( (i = Trackpos + pos) > 9 )
		j = 2;
	if( i > 99 )
		j = 3;
	itoa(i, temp, j);
	(void)strcat(name,temp);
	return;
}



wrobj( name )
char name[];
{
	struct directory *tdp;

	if( lookup(name, LOOKUP_QUIET) != DIR_NULL ) {
		(void)printf("amtrack naming error: %s already exists\n",name);
		return(-1);
	}
	if( (tdp = dir_add(name, -1, DIR_SOLID, 1)) == DIR_NULL )
		return( -1 );
	db_alloc(tdp, 1);
	db_putrec(tdp, &record, 0);
	return(0);
}


tancir( cir1, cir2 )
register float cir1[], cir2[];
{
	static fastf_t work[3], mag;
	FAST fastf_t f;
	static double temp, tempp, ang, angc;

	work[0] = cir2[0] - cir1[0];
	work[2] = cir2[1] - cir1[1];
	work[1] = 0.0;
	mag = MAGNITUDE( work );
	if( mag > 1.0e-20 || mag < -1.0e-20 )  {
		f = 1.0/mag;
	}  else {
		fprintf(stderr,"tancir():  0-length vector!\n");
		return;
	}
	VSCALE(work, work, f);
	temp = acos( work[0] );
	if( work[2] < 0.0 )
		temp = 6.28318512717958646 - temp;
	tempp = acos( (cir1[2] - cir2[2]) * f );
	ang = temp + tempp;
	angc = temp - tempp;
	if( (cir1[1] + cir1[2] * sin(ang)) >
	    (cir1[1] + cir1[2] * sin(angc)) )
		ang = angc;
	plano[0] = cir1[0] + cir1[2] * cos(ang);
	plano[1] = cir1[1] + cir1[2] * sin(ang);
	plant[0] = cir2[0] + cir2[2] * cos(ang);
	plant[1] = cir2[1] + cir2[2] * sin(ang);

	return;
}




slope( wh1, wh2 , t )
float wh1[], wh2[], t[];
{
	int i, j, switchs;
	float temp, del[3], mag, work[3], z, r, b;

	switchs = 0;
	if( wh1[2] < wh2[2] ) {
		switchs++;
		for(i=0; i<3; i++) {
			temp = wh1[i];
			wh1[i] = wh2[i];
			wh2[i] = temp;
		}
	}
	tancir(wh1, wh2);
	if( switchs ) {
		for(i=0; i<3; i++) {
			temp = wh1[i];
			wh1[i] = wh2[i];
			wh2[i] = temp;
		}
	}
	if(plano[1] <= plant[1]) {
		for(i=0; i<2; i++) {
			temp = plano[i];
			plano[i] = plant[i];
			plant[i] = temp;
		}
	}
	del[1] = 0.0;
	del[0] = plano[0] - plant[0];
	del[2] = plano[1] - plant[1];
	mag = MAGNITUDE( del );
	work[0] = -1.0 * t[2] * del[2] / mag;
	if( del[0] < 0.0 )
		work[0] *= -1.0;
	work[1] = 0.0;
	work[2] = t[2] * fabs(del[0]) / mag;
	b = (plano[1] - work[2]) - (del[2]/del[0]*(plano[0] - work[0]));
	z = wh1[1];
	r = wh1[2];
	if( wh1[1] >= wh2[1] ) {
		z = wh2[1];
		r = wh2[2];
	}
	record.s.s_values[2] = z - r - t[2];
	record.s.s_values[1] = t[0];
	record.s.s_values[0] = (record.s.s_values[2] - b) / (del[2] / del[0]);
	record.s.s_values[3] = plano[0] + (del[0]/mag) - work[0] - record.s.s_values[0];
	record.s.s_values[4] = 0.0;
	record.s.s_values[5] = plano[1] + (del[2]/mag) - work[2] - record.s.s_values[2];
	VADD2(&record.s.s_values[6], &record.s.s_values[3], work);
	VMOVE(&record.s.s_values[9], work);
	work[0] = work[2] = 0.0;
	work[1] = t[1] - t[0];
	VMOVE(&record.s.s_values[12], work);
	for(i=3; i<=9; i+=3) {
		j = i + 12;
		VADD2(&record.s.s_values[j], &record.s.s_values[i], work);
	}

	return;
}



crdummy( w, t, flag )
float w[3], t[3];
int flag;
{
	float temp, vec[3];
	int i, j;

	vec[1] = 0.0;
	if(plano[1] <= plant[1]) {
		for(i=0; i<2; i++) {
			temp = plano[i];
			plano[i] = plant[i];
			plant[i] = temp;
		}
	}

	vec[0] = w[2] + t[2] + 1.0;
	vec[2] = ( (plano[1] - w[1]) * vec[0] ) / (plano[0] - w[0]);
	if( flag > 1 )
		vec[0] *= -1.0;
	if(vec[2] >= 0.0)
		vec[2] *= -1.0;
	record.s.s_values[0] = w[0];
	record.s.s_values[1] = t[0] -1.0;
	record.s.s_values[2] = w[1];
	VMOVE(&record.s.s_values[3] , vec);
	vec[2] = w[2] + t[2] + 1.0;
	VMOVE(&record.s.s_values[6], vec);
	vec[0] = 0.0;
	VMOVE(&record.s.s_values[9], vec);
	vec[2] = 0.0;
	vec[1] = t[1] - t[0] + 2.0;
	VMOVE(&record.s.s_values[12], vec);
	for(i=3; i<=9; i+=3) {
		j = i + 12;
		VADD2(&record.s.s_values[j], &record.s.s_values[i], vec);
	}

	return;

}



trcurve( wh, t )
float wh[], t[];
{
	record.s.s_values[0] = wh[0];
	record.s.s_values[1] = t[0];
	record.s.s_values[2] = wh[1];
	record.s.s_values[4] = t[1] - t[0];
	record.s.s_values[6] = wh[2] + t[2];
	record.s.s_values[11] = wh[2] + t[2];
	VMOVE(&record.s.s_values[12], &record.s.s_values[6]);
	VMOVE(&record.s.s_values[15], &record.s.s_values[9]);
	return;
}


bottom( vec1, vec2, t )
float vec1[], vec2[], t[];
{
	float tvec[3];
	int i, j;

	VMOVE(&record.s.s_values[0], vec1);
	tvec[0] = vec2[0] - vec1[0];
	tvec[1] = tvec[2] = 0.0;
	VMOVE(&record.s.s_values[3], tvec);
	tvec[0] = tvec[1] = 0.0;
	tvec[2] = t[2];
	VADD2(&record.s.s_values[6], &record.s.s_values[3], tvec);
	VMOVE(&record.s.s_values[9], tvec);
	tvec[0] = tvec[2] = 0.0;
	tvec[1] = t[1] - t[0];
	VMOVE(&record.s.s_values[12], tvec);

	for(i=3; i<=9; i+=3) {
		j = i + 12;
		VADD2(&record.s.s_values[j], &record.s.s_values[i], tvec);
	}

	return;
}


top( vec1, vec2, t )
float vec1[], vec2[], t[];
{
	float tooch, del[3], tvec[3], mag;
	int i, j;

	tooch = t[2] * .25;
	del[0] = vec2[0] - vec1[0];
	del[1] = 0.0;
	del[2] = vec2[2] - vec1[2];
	mag = MAGNITUDE( del );
	VSCALE(tvec, del, tooch/mag);
	VSUB2(&record.s.s_values[0], vec1, tvec);
	VADD2(del, del, tvec);
	VADD2(&record.s.s_values[3], del, tvec);
	tvec[0] = tvec[2] = 0.0;
	tvec[1] = t[1] - t[0];
	VCROSS(del, tvec, &record.s.s_values[3]);
	mag = MAGNITUDE( del );
	if(del[2] < 0)
		mag *= -1.0;
	VSCALE(&record.s.s_values[9], del, t[2]/mag);
	VADD2(&record.s.s_values[6], &record.s.s_values[3], &record.s.s_values[9]);
	VMOVE(&record.s.s_values[12], tvec);

	for(i=3; i<=9; i+=3) {
		j = i + 12;
		VADD2(&record.s.s_values[j], &record.s.s_values[i], tvec);
	}

	return;
}


crregion( region, op, members, number, solidname )
char region[], op[], solidname[];
int members[], number;
{
	struct directory *dp;
	int i;

	for(i=0; i<number; i++) {
		solidname[8] = '\0';
		crname(solidname, members[i]);
		if( (dp = lookup(solidname, LOOKUP_QUIET)) == DIR_NULL ) {
			(void)printf("region: %s will skip member: %s\n",region,solidname);
			continue;
		}
		(void)combadd(dp, region, 1, op[i], 500+Trackpos+i, 0);
	}

	return;
}




/*	==== I T O A ( )
 *	convert integer to ascii  wd format
 */
itoa( n, s, w )
char	 s[];
int   n,    w;
{
	int	 c, i, j, sign;

	if( (sign = n) < 0 )	n = -n;
	i = 0;
	do	s[i++] = n % 10 + '0';	while( (n /= 10) > 0 );
	if( sign < 0 )	s[i++] = '-';

	/* blank fill array
	 */
	for( j = i; j < w; j++ )	s[j] = ' ';
	if( i > w )	(void)printf( "itoa: field length too small\n" );
	s[w] = '\0';
	/* reverse the array
	 */
	for( i = 0, j = w - 1; i < j; i++, j-- ) {
		c    = s[i];
		s[i] = s[j];
		s[j] =    c;
	}
}

