/*
 *			G - J A C K . C
 *
 *  Program to convert a BRL-CAD model (in a .g file) to a JACK Psurf file,
 *  by calling on the NMG booleans.
 *
 *  Author -
 *	Michael J. Markowski
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 */

#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "../librt/debug.h"

RT_EXTERN(struct nmgregion *nmg_booltree_evaluate, (union tree *tp, CONST struct rt_tol *tol));
RT_EXTERN(union tree *nmg_booltree_leaf_tess, (struct db_tree_state *tsp, struct db_full_path *pathp, struct rt_external *ep, int id));

RT_EXTERN(union tree *do_region_end, (struct db_tree_state *tsp, struct db_full_path *pathp, union tree *curtree));

int		heap_find(), heap_insert();
struct vertex	**init_heap();
void		heap_increase();


static char	usage[] = "Usage: %s [-v] [-d] [-xX lvl] [-a abs_tol] [-r rel_tol] [-n norm_tol] [-p prefix] brlcad_db.g object(s)\n";

static int	NMG_debug;	/* saved arg of -X, for longjmp handling */
static int	verbose;
static int	debug_plots;	/* Make debugging plots */
static int	ncpu = 1;	/* Number of processors */
int		heap_cur_sz;	/* Next free spot in heap. */
static char	*prefix = NULL;	/* output filename prefix. */
static FILE	*fp_fig;	/* Jack Figure file. */
static struct db_i	*dbip;
static struct rt_vls		base_seg;
static struct rt_tess_tol	ttol;
static struct rt_tol		tol;
static struct model		*the_model;

static struct db_tree_state	jack_tree_state;	/* includes tol & model */

static int	regions_tried = 0;
static int	regions_done = 0;

/*
 *			M A I N
 */
int
main(argc, argv)
int	argc;
char	*argv[];
{
	char		*dot, *fig_file;
	int		i, ret;
	register int	c;
	double		percent;

#ifdef BSD
	setlinebuf( stderr );
#else
#	if defined( SYSV ) && !defined( sgi ) && !defined(CRAY2) && \
	 !defined(n16)
		(void) setvbuf( stderr, (char *) NULL, _IOLBF, BUFSIZ );
#	endif
#	if defined(sgi) && defined(mips)
		if( setlinebuf( stderr ) != 0 )
			perror("setlinebuf(stderr)");
#	endif
#endif

	jack_tree_state = rt_initial_tree_state;	/* struct copy */
	jack_tree_state.ts_tol = &tol;
	jack_tree_state.ts_ttol = &ttol;
	jack_tree_state.ts_m = &the_model;

	ttol.magic = RT_TESS_TOL_MAGIC;
	/* Defaults, updated by command line options. */
	ttol.abs = 0.0;
	ttol.rel = 0.01;
	ttol.norm = 0.0;

	/* XXX These need to be improved */
	tol.magic = RT_TOL_MAGIC;
	tol.dist = 0.005;
	tol.dist_sq = tol.dist * tol.dist;
	tol.perp = 1e-6;
	tol.para = 1 - tol.perp;

	the_model = nmg_mm();
	RT_LIST_INIT( &rt_g.rtg_vlfree );	/* for vlist macros */

	/* Get command line arguments. */
	while ((c = getopt(argc, argv, "a:dn:p:r:vx:P:X:")) != EOF) {
		switch (c) {
		case 'a':		/* Absolute tolerance. */
			ttol.abs = atof(optarg);
			break;
		case 'd':
			debug_plots = 1;
			break;
		case 'n':		/* Surface normal tolerance. */
			ttol.norm = atof(optarg);
			break;
		case 'p':		/* Prefix for Jack file names. */
			prefix = optarg;
			break;
		case 'r':		/* Relative tolerance. */
			ttol.rel = atof(optarg);
			break;
		case 'v':
			verbose++;
			break;
		case 'P':
			ncpu = atoi( optarg );
			rt_g.debug = 1;	/* XXX DEBUG_ALLRAYS -- to get core dumps */
			break;
		case 'x':
			sscanf( optarg, "%x", &rt_g.debug );
			break;
		case 'X':
			sscanf( optarg, "%x", &rt_g.NMG_debug );
			NMG_debug = rt_g.NMG_debug;
			break;
		default:
			fprintf(stderr, usage, argv[0]);
			exit(1);
			break;
		}
	}

	if (optind+1 >= argc) {
		fprintf(stderr, usage, argv[0]);
		exit(1);
	}

	/* Open brl-cad database */
	argc -= optind;
	argv += optind;
	if ((dbip = db_open(argv[0], "r")) == DBI_NULL) {
		perror(argv[0]);
		exit(1);
	}
	db_scan(dbip, (int (*)())db_diradd, 1);

	/* Create .fig file name and open it. */
	fig_file = rt_malloc(sizeof(prefix) + sizeof(argv[0] + 4), "st");
	/* Ignore leading path name. */
	if ((dot = strrchr(argv[0], '/')) != (char *)NULL) {
		if (prefix)
			strcat(strcpy(fig_file, prefix), 1 + dot);
		else
			strcpy(fig_file, 1 + dot);
	} else {
		if (prefix)
			strcat(strcpy(fig_file, prefix), argv[0]);
		else
			strcpy(fig_file, argv[0]);
	}

	/* Get rid of any file name extension (probably .g). */
	if ((dot = strrchr(fig_file, '.')) != (char *)NULL)
		*dot = (char)NULL;
	strcat(fig_file, ".fig");	/* Add required Jack suffix. */

	if ((fp_fig = fopen(fig_file, "w")) == NULL)
		perror(fig_file);
	fprintf(fp_fig, "figure {\n");
	rt_vls_init(&base_seg);		/* .fig figure file's main segment. */

	/* Walk indicated tree(s).  Each region will be output separately */
	ret = db_walk_tree(dbip, argc-1, (CONST char **)(argv+1),
		1,			/* ncpu */
		&jack_tree_state,
		0,			/* take all regions */
		do_region_end,
		nmg_booltree_leaf_tess);

	fprintf(fp_fig, "\troot=%s_seg.base;\n", rt_vls_addr(&base_seg));
	fprintf(fp_fig, "}\n");
	fclose(fp_fig);
	rt_free(fig_file, "st");
	rt_vls_free(&base_seg);

	percent = 0;
	if(regions_tried>0)  percent = ((double)regions_done * 100) / regions_tried;
	printf("Tried %d regions, %d converted successfully.  %g%%\n",
		regions_tried, regions_done, percent);

	return 0;
}


/*
 *			N M G _ B O O L T R E E _ E V A L U A T E
 *
 *  Given a tree of leaf nodes tesselated earlier by nmg_booltree_leaf_tess(),
 *  use recursion to do a depth-first traversal of the tree,
 *  evaluating each pair of boolean operations
 *  and reducing that result to a single nmgregion.
 *
 *  Usually called from a do_region_end() handler from db_walk_tree().
 *  For an example of one, see XXX.
 *
 * Swiped from mged/dodraw.c mged_nmg_doit().
 */
struct nmgregion *
nmg_booltree_evaluate(tp, tol)
register union tree		*tp;
CONST struct rt_tol		*tol;
{
	register struct nmgregion	*l;
	register struct nmgregion	*r;
	int			op;

	RT_CK_TOL(tol);

	switch(tp->tr_op) {
	case OP_NOP:
		return(0);
	case OP_NMG_TESS:
		/* Hit a tree leaf, just grab nmgregion and return it */
		r = tp->tr_d.td_r;
		tp->tr_d.td_r = (struct nmgregion *)NULL;	/* Disconnect */
		tp->tr_op = OP_NOP;	/* Keep quiet */
		return r;
	case OP_UNION:
		op = NMG_BOOL_ADD;
		break;
	case OP_INTERSECT:
		op = NMG_BOOL_ISECT;
		break;
	case OP_SUBTRACT:
		op = NMG_BOOL_SUB;
		break;
	default:
		rt_log("nmg_booltree_evaluate: bad op %d\n", tp->tr_op);
		return(0);
	}
	l = nmg_booltree_evaluate(tp->tr_b.tb_left, tol);
	r = nmg_booltree_evaluate(tp->tr_b.tb_right, tol);
	if (l == 0) {
		if (r == 0)
			return 0;
		if( op == NMG_BOOL_ADD )
			return r;
		/* For sub and intersect, if lhs is 0, result is null */
		nmg_kr(r);
		return 0;
	}
	if (r == 0) {
		if (l == 0)
			return 0;
		if( op == NMG_BOOL_ISECT )  {
			nmg_kr(l);
			return 0;
		}
		/* For sub and add, if rhs is 0, result is lhs */
		return l;
	}

	NMG_CK_REGION(r);
	NMG_CK_REGION(l);
	if (nmg_ck_closed_region(r) != 0)
	    	rt_log("nmg_booltree_evaluate:  WARNING, non-closed shell (r), barging ahead\n");
	if (nmg_ck_closed_region(l) != 0)
	    	rt_log("nmg_booltree_evaluate:  WARNING, non-closed shell (l), barging ahead\n");

	/* input r1 and r2 are destroyed, output is new r1 */
	r = nmg_do_bool(l, r, op, tol);

	NMG_CK_REGION(r);
	return r;
}

/*
 *			N M G _ B O O L T R E E _ L E A F _ T E S S
 *
 *  Called from db_walk_tree() each time a tree leaf is encountered.
 *  The primitive solid, in external format, is provided in 'ep',
 *  and the type of that solid (e.g. ID_ELL) is in 'id'.
 *  The full tree state including the accumulated transformation matrix
 *  and the current tolerancing is in 'tsp',
 *  and the full path from root to leaf is in 'pathp'.
 *
 *  Import the solid, tessellate it into an NMG, stash a pointer to
 *  the tessellation in a new tree structure (union), and return a
 *  pointer to that.
 *
 *  This routine must be prepared to run in parallel.
 */
union tree *
nmg_booltree_leaf_tess(tsp, pathp, ep, id)
struct db_tree_state	*tsp;
struct db_full_path	*pathp;
struct rt_external	*ep;
int			id;
{
	struct rt_db_internal	intern;
	struct nmgregion	*r1;
	union tree		*curtree;
	struct directory	*dp;

	RT_CK_TESS_TOL(tsp->ts_ttol);
	RT_CK_TOL(tsp->ts_tol);
	NMG_CK_MODEL(*tsp->ts_m);

	/* RT_CK_FULL_PATH(pathp) */
	dp = DB_FULL_PATH_CUR_DIR(pathp);
	RT_CK_DIR(dp);

	RT_INIT_DB_INTERNAL(&intern);
	if (rt_functab[id].ft_import(&intern, ep, tsp->ts_mat) < 0) {
		rt_log("nmg_booltree_leaf_tess(%s):  solid import failure\n", dp->d_namep);
	    	if (intern.idb_ptr)  rt_functab[id].ft_ifree(&intern);
	    	return(TREE_NULL);		/* ERROR */
	}
	RT_CK_DB_INTERNAL(&intern);

	if (rt_functab[id].ft_tessellate(
	    &r1, *tsp->ts_m, &intern, tsp->ts_ttol, tsp->ts_tol) < 0) {
		rt_log("nmg_booltree_leaf_tess(%s): tessellation failure\n", dp->d_namep);
		rt_functab[id].ft_ifree(&intern);
	    	return(TREE_NULL);
	}
	rt_functab[id].ft_ifree(&intern);

	NMG_CK_REGION(r1);

	GETUNION(curtree, tree);
	curtree->tr_op = OP_NMG_TESS;
	curtree->tr_d.td_r = r1;

	if (rt_g.debug&DEBUG_TREEWALK)
		rt_log("nmg_booltree_leaf_tess(%s) OK\n", dp->d_namep);

	return(curtree);
}

/*
*			D O _ R E G I O N _ E N D
*
*  This routine must be prepared to run in parallel.
*/
union tree *do_region_end(tsp, pathp, curtree)
register struct db_tree_state	*tsp;
struct db_full_path	*pathp;
union tree		*curtree;
{
	extern FILE		*fp_fig;
	struct nmgregion	*r;
	struct rt_list		vhead;

	RT_CK_TESS_TOL(tsp->ts_ttol);
	RT_CK_TOL(tsp->ts_tol);
	NMG_CK_MODEL(*tsp->ts_m);

	RT_LIST_INIT(&vhead);

	if (rt_g.debug&DEBUG_TREEWALK || verbose) {
		char	*sofar = db_path_to_string(pathp);
		rt_log("\ndo_region_end(%d %d%%) %s\n",
			regions_tried,
			regions_tried>0 ? (regions_done * 100) / regions_tried : 0,
			sofar);
		rt_free(sofar, "path string");
	}

	if (curtree->tr_op == OP_NOP)
		return  curtree;

	regions_tried++;
	/* Begin rt_bomb() protection */
	if( ncpu == 1 && RT_SETJUMP )  {
		/* Error, bail out */
		RT_UNSETJUMP;		/* Relinquish the protection */

		/* Sometimes the NMG library adds debugging bits when
		 * it detects an internal error, before rt_bomb().
		 */
		rt_g.NMG_debug = NMG_debug;	/* restore mode */

		/* Release the tree memory & input regions */
		db_free_tree(curtree);		/* Does an nmg_kr() */

		/* Get rid of (m)any other intermediate structures */
		if( (*tsp->ts_m)->magic != -1L )
			nmg_km(*tsp->ts_m);
	
		/* Now, make a new, clean model structure for next pass. */
		*tsp->ts_m = nmg_mm();
		goto out;
	}
	r = nmg_booltree_evaluate(curtree, &tol);
	RT_UNSETJUMP;		/* Relinquish the protection */
	regions_done++;
	if (r != 0) {
		FILE	*fp_psurf;
		int	i;
		struct rt_vls	file_base;
		struct rt_vls	file;

		rt_vls_init(&file_base);
		rt_vls_init(&file);
		rt_vls_strcpy(&file_base, prefix);
		rt_vls_strcat(&file_base, DB_FULL_PATH_CUR_DIR(pathp)->d_namep);
		/* Dots confuse Jack's Peabody language.  Change to '_'. */
		for (i = 0; i < file_base.vls_len; i++)
			if (file_base.vls_str[i] == '.')
				file_base.vls_str[i] = '_';

		/* Write color attribute to .fig figure file. */
		if (tsp->ts_mater.ma_override != 0) {
			fprintf(fp_fig, "\tattribute %s {\n",
				rt_vls_addr(&file_base));
			fprintf(fp_fig, "\t\trgb = (%f, %f, %f);\n",
				V3ARGS(tsp->ts_mater.ma_color));
			fprintf(fp_fig, "\t\tambient = 0.18;\n");
			fprintf(fp_fig, "\t\tdiffuse = 0.72;\n");
			fprintf(fp_fig, "\t}\n");
		}

		/* Write segment attributes to .fig figure file. */
		fprintf(fp_fig, "\tsegment %s_seg {\n", rt_vls_addr(&file_base));
		fprintf(fp_fig, "\t\tpsurf=\"%s.pss\";\n", rt_vls_addr(&file_base));
		if (tsp->ts_mater.ma_override != 0)
			fprintf(fp_fig,
				"\t\tattribute=%s;\n", rt_vls_addr(&file_base));
		fprintf(fp_fig, "\t\tsite base->location=trans(0,0,0);\n");
		fprintf(fp_fig, "\t}\n");

		if( rt_vls_strlen(&base_seg) <= 0 )  {
			rt_vls_vlscat( &base_seg, &file_base );
		} else {
			fprintf(fp_fig, "\tjoint %s_jt {\n",
				rt_vls_addr(&file_base));
			fprintf(fp_fig,
				"\t\tconnect %s_seg.base to %s_seg.base;\n",
				rt_vls_addr(&file_base),
				rt_vls_addr(&base_seg) );
			fprintf(fp_fig, "\t}\n");
		}

		rt_vls_vlscat(&file, &file_base);
		rt_vls_strcat(&file, ".pss");	/* Required Jack suffix. */

		/* Write psurf to .pss file. */
		if ((fp_psurf = fopen(rt_vls_addr(&file), "w")) == NULL)
			perror(rt_vls_addr(&file));
		else {
			nmg_to_psurf(r, fp_psurf);
			fclose(fp_psurf);
			if(verbose) rt_log("*** Wrote %s\n", rt_vls_addr(&file));
		}
		rt_vls_free(&file);

		/* Also write as UNIX-plot file, if desired */
		if( debug_plots )  {
			FILE	*fp;
			rt_vls_vlscat(&file, &file_base);
			rt_vls_strcat(&file, ".pl");

			if ((fp = fopen(rt_vls_addr(&file), "w")) == NULL)
				perror(rt_vls_addr(&file));
			else {
				struct rt_list	vhead;
				pl_color( fp,
					(int)(tsp->ts_mater.ma_color[0] * 255),
					(int)(tsp->ts_mater.ma_color[1] * 255),
					(int)(tsp->ts_mater.ma_color[2] * 255) );
				/* nmg_pl_r( fp, r ); */
				RT_LIST_INIT( &vhead );
				nmg_r_to_vlist( &vhead, r, 0 );
				rt_vlist_to_uplot( fp, &vhead );
				fclose(fp);
				if(verbose) rt_log("*** Wrote %s\n", rt_vls_addr(&file));
			}
			rt_vls_free(&file);
		}

		/* NMG region is no longer necessary */
		nmg_kr(r);
	}

	/*
	 *  Dispose of original tree, so that all associated dynamic
	 *  memory is released now, not at the end of all regions.
	 *  A return of TREE_NULL from this routine signals an error,
	 *  so we need to cons up an OP_NOP node to return.
	 */
	db_free_tree(curtree);		/* Does an nmg_kr() */

out:
	GETUNION(curtree, tree);
	curtree->tr_op = OP_NOP;
	return(curtree);
}

/*
*	N M G _ T O _ P S U R F
*
*	Convert an nmg region into Jack format.  This routine makes a
*	list of unique vertices and writes them to the ascii Jack
*	data base file.  Then a routine to generate the face vertex
*	data is called.
*/

int
nmg_to_psurf(r, fp_psurf)
struct nmgregion *r;		/* NMG region to be converted. */
FILE		*fp_psurf;	/* Jack format file to write vertex list to. */
{
	int			cnt, i, sz;
	struct edgeuse		*eu;
	struct faceuse		*fu;
	struct loopuse		*lu;
	struct shell		*s;
	struct vertex		*v, **verts[1];
	int			*map;	/* map from v->index to Jack vert # */

	map = (int *)rt_calloc(r->m_p->maxindex, sizeof(int *), "Jack vert map");

	sz = 1000;
	verts[0] = init_heap(sz);
	cnt = 0;

	for (RT_LIST_FOR(s, shell, &r->s_hd)) {
		/* Shell is made of faces. */
		for (RT_LIST_FOR(fu, faceuse, &s->fu_hd)) {
			NMG_CK_FACEUSE(fu);
			if (fu->orientation != OT_SAME)
				continue;
			for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd)) {
				NMG_CK_LOOPUSE(lu);
				if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_EDGEUSE_MAGIC) {
					for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
						NMG_CK_EDGEUSE(eu);
						NMG_CK_EDGE(eu->e_p);
						NMG_CK_VERTEXUSE(eu->vu_p);
						NMG_CK_VERTEX(eu->vu_p->v_p);
						NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
						cnt = heap_insert(verts, &sz, eu->vu_p->v_p);
					}
				} else if (RT_LIST_FIRST_MAGIC(&lu->down_hd)
					== NMG_VERTEXUSE_MAGIC) {
					v = RT_LIST_PNEXT(vertexuse, &lu->down_hd)->v_p;
					NMG_CK_VERTEX(v);
					NMG_CK_VERTEX_G(v->vg_p);
					cnt = heap_insert(verts, &sz, v);
				} else
					rt_log("nmg_to_psurf: loopuse mess up! (1)\n");
			}

			/* Shell contains loops. */
			for (RT_LIST_FOR(lu, loopuse, &s->lu_hd)) {
				NMG_CK_LOOPUSE(lu);
				if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_EDGEUSE_MAGIC) {
					for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
						NMG_CK_EDGEUSE(eu);
						NMG_CK_EDGE(eu->e_p);
						NMG_CK_VERTEXUSE(eu->vu_p);
						NMG_CK_VERTEX(eu->vu_p->v_p);
						NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
						cnt = heap_insert(verts, &sz, eu->vu_p->v_p);
					}
				} else if (RT_LIST_FIRST_MAGIC(&lu->down_hd)
					== NMG_VERTEXUSE_MAGIC) {
					v = RT_LIST_PNEXT(vertexuse, &lu->down_hd)->v_p;
					NMG_CK_VERTEX(v);
					NMG_CK_VERTEX_G(v->vg_p);
					cnt = heap_insert(verts, &sz, v);
				} else
					rt_log("nmg_to_psurf: loopuse mess up! (1)\n");
			}

			/* Shell contains edges. */
			for (RT_LIST_FOR(eu, edgeuse, &s->eu_hd)) {
				NMG_CK_EDGEUSE(eu);
				NMG_CK_EDGE(eu->e_p);
				NMG_CK_VERTEXUSE(eu->vu_p);
				NMG_CK_VERTEX(eu->vu_p->v_p);
				NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
				cnt = heap_insert(verts, &sz, eu->vu_p->v_p);
			}

			/* Shell contains a single vertex. */
			if (s->vu_p) {
				NMG_CK_VERTEXUSE(s->vu_p);
				NMG_CK_VERTEX(s->vu_p->v_p);
				NMG_CK_VERTEX_G(s->vu_p->v_p->vg_p);
				cnt = heap_insert(verts, &sz, s->vu_p->v_p);
			}

			if (RT_LIST_IS_EMPTY(&s->fu_hd) &&
				RT_LIST_IS_EMPTY(&s->lu_hd) &&
				RT_LIST_IS_EMPTY(&s->eu_hd) && !s->vu_p) {
				rt_log("WARNING nmg_to_psurf: empty shell\n");
			}
		}
	}

	/* XXX What to do if cnt == 0 ? */

	/* Print list of unique vertices and convert from mm to cm. */
	for (i = 1; i < cnt; i++)  {
		struct vertex			*v;
		register struct vertex_g	*vg;
		v = verts[0][i];
		NMG_CK_VERTEX(v);
		vg = v->vg_p;
		NMG_CK_VERTEX_G(vg);
		NMG_INDEX_ASSIGN( map, v, i );	/* map[v] = i */
		fprintf(fp_psurf, "%f\t%f\t%f\n",
			vg->coord[X] / 10.,
			vg->coord[Y] / 10.,
			vg->coord[Z] / 10.);
	}
	fprintf(fp_psurf, ";;\n");
	jack_faces(r, fp_psurf, verts[0], cnt-1, map);
	rt_free((char *)(verts[0]), "heap");
	rt_free( (char *)map, "Jack vert map" );
}


/*
*	J A C K _ F A C E S
*
*	Continues the conversion of an nmg into Jack format.  Before
*	this routine is called, a list of unique vertices has been
*	stored in a heap.  Using this heap and the nmg structure, a
*	list of face vertices is written to the Jack data base file.
*/
jack_faces(r, fp_psurf, verts, sz, map)
struct nmgregion *r;		/* NMG region to be converted. */
FILE		*fp_psurf;	/* Jack format file to write face vertices to. */
struct vertex	**verts;	/* Heap of vertex structs. */
int		sz;	/* Size of vertex heap. */
int		*map;
{
	point_t			vert;
	struct edgeuse		*eu;
	struct faceuse		*fu;
	struct loopuse		*lu;
	struct shell		*s;
	struct vertex		*v;

	for (RT_LIST_FOR(s, shell, &r->s_hd)) {
		/* Shell is made of faces. */
		for (RT_LIST_FOR(fu, faceuse, &s->fu_hd)) {
			NMG_CK_FACEUSE(fu);
			if (fu->orientation != OT_SAME)
				continue;
			for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd)) {
				NMG_CK_LOOPUSE(lu);
				if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_EDGEUSE_MAGIC) {
					for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
						NMG_CK_EDGEUSE(eu);
						NMG_CK_EDGE(eu->e_p);
						NMG_CK_VERTEXUSE(eu->vu_p);
						NMG_CK_VERTEX(eu->vu_p->v_p);
						NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
		    				fprintf(fp_psurf, "%d ", NMG_INDEX_GET(map,eu->vu_p->v_p));
					}
				} else if (RT_LIST_FIRST_MAGIC(&lu->down_hd)
		  			== NMG_VERTEXUSE_MAGIC) {
		  			v = RT_LIST_PNEXT(vertexuse, &lu->down_hd)->v_p;
					NMG_CK_VERTEX(v);
					NMG_CK_VERTEX_G(v->vg_p);
		  			fprintf(fp_psurf, "%d ", NMG_INDEX_GET(map,v));
				} else
					rt_log("jack_faces: loopuse mess up! (1)\n");
				fprintf(fp_psurf, ";\n");
			}
		}

		/* Shell contains loops. */
		for (RT_LIST_FOR(lu, loopuse, &s->lu_hd)) {
			NMG_CK_LOOPUSE(lu);
			if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_EDGEUSE_MAGIC) {
				for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
					NMG_CK_EDGEUSE(eu);
					NMG_CK_EDGE(eu->e_p);
					NMG_CK_VERTEXUSE(eu->vu_p);
					NMG_CK_VERTEX(eu->vu_p->v_p);
					NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
		  			fprintf(fp_psurf, "%d ", NMG_INDEX_GET(map,eu->vu_p->v_p));
				}
			} else if (RT_LIST_FIRST_MAGIC(&lu->down_hd)
				== NMG_VERTEXUSE_MAGIC) {
				v = RT_LIST_PNEXT(vertexuse, &lu->down_hd)->v_p;
				NMG_CK_VERTEX(v);
				NMG_CK_VERTEX_G(v->vg_p);
				fprintf(fp_psurf, "%d ", NMG_INDEX_GET(map,v));
			} else
				rt_log("jack_faces: loopuse mess up! (1)\n");
			fprintf(fp_psurf, ";\n");
		}

		/* Shell contains edges. */
		for (RT_LIST_FOR(eu, edgeuse, &s->eu_hd)) {
			NMG_CK_EDGEUSE(eu);
			NMG_CK_EDGE(eu->e_p);
			NMG_CK_VERTEXUSE(eu->vu_p);
			NMG_CK_VERTEX(eu->vu_p->v_p);
			NMG_CK_VERTEX_G(eu->vu_p->v_p->vg_p);
			fprintf(fp_psurf, "%d ", NMG_INDEX_GET(map,eu->vu_p->v_p));
		}
		if (RT_LIST_FIRST_MAGIC(&s->eu_hd) == NMG_EDGEUSE_MAGIC)
			fprintf(fp_psurf, ";\n");

		/* Shell contains a single vertex. */
		if (s->vu_p) {
			NMG_CK_VERTEXUSE(s->vu_p);
			NMG_CK_VERTEX(s->vu_p->v_p);
			NMG_CK_VERTEX_G(s->vu_p->v_p->vg_p);
			fprintf(fp_psurf, "%d;\n", NMG_INDEX_GET(map,s->vu_p->v_p));
		}

		if (RT_LIST_IS_EMPTY(&s->fu_hd) &&
			RT_LIST_IS_EMPTY(&s->lu_hd) &&
			RT_LIST_IS_EMPTY(&s->eu_hd) && !s->vu_p) {
			rt_log("WARNING jack_faces: empty shell\n");
		}

	}
	fprintf(fp_psurf, ";;\n");
}

/*
*	I N I T _ H E A P
*
*	Initialize an array-based implementation of a heap of vertex structs.
*	(Heap: Binary tree w/value of parent > than that of children.)
*/
struct vertex **
init_heap(n)
int	n;
{
	extern int	heap_cur_sz;
	struct vertex	**heap;

	heap_cur_sz = 1;
	heap = (struct vertex **)
		rt_malloc(1 + n*sizeof(struct vertex *), "heap");
	if (heap == (struct vertex **)NULL) {
		rt_log("init_heap: no mem\n");
		rt_bomb("");
	}
	return(heap);
}

/*
*	H E A P _ I N C R E A S E
*
*	Make a heap bigger to make room for new entries.
*/
void
heap_increase(h, n)
struct vertex	**h[1];
int	*n;
{
	struct vertex	**big_heap;
	int	i;

	big_heap = (struct vertex **)
		rt_malloc(1 + 3 * (*n) * sizeof(struct vertex *), "heap");
	if (big_heap == (struct vertex **)NULL)
		rt_bomb("heap_increase: no mem\n");
	for (i = 1; i <= *n; i++)
		big_heap[i] = h[0][i];
	*n *= 3;
	rt_free((char *)h[0], "heap");
	h[0] = big_heap;
}

/*
*	H E A P _ I N S E R T
*
*	Insert a vertex struct into the heap (only if it is
*	not already there).
*/
int
heap_insert(h, n, i)
struct vertex	**h[1];	/* Heap of vertices. */
int		*n;	/* Max size of heap. */
struct vertex	*i;	/* Item to insert. */
{
	extern int	heap_cur_sz;
	struct vertex	**new_heap, *tmp;
	int		cur, done;

	if (heap_find(h[0], heap_cur_sz, i, 1))	/* Already in heap. */
		return(heap_cur_sz);

	if (heap_cur_sz > *n)
		heap_increase(h, n);

	cur = heap_cur_sz;
	h[0][cur] = i;	/* Put at bottom of heap. */

	/* Bubble item up in heap. */
	done = 0;
	while (cur > 1 && !done)
		if (h[0][cur] < h[0][cur>>1]) {
			tmp          = h[0][cur>>1];
			h[0][cur>>1] = h[0][cur];
			h[0][cur]    = tmp;
			cur >>= 1;
		} else
			done = 1;
	heap_cur_sz++;
	return(heap_cur_sz);
}

/*
*	H E A P _ F I N D
*
*	See if a given vertex struct is in the heap.  If so,
*	return its location in the heap array.
*/
int
heap_find(h, n, i, loc)
struct vertex	**h;	/* Heap of vertexs. */
int		n;	/* Max size of heap. */
struct vertex	*i;	/* Item to search for. */
int		loc;	/* Location to start search at. */
{
	int		retval;

	if (loc > n || h[loc] > i)
		retval = 0;
	else if (h[loc] == i)
		retval = loc;
	else {
		loc <<= 1;
		retval = heap_find(h, n, i, loc);
		if (!retval)
			retval = heap_find(h, n, i, loc+1);
	}
	return(retval);
}
