/*            S C R E E N E D _ P O I S S O N . C P P
 * BRL-CAD
 *
 * Copyright (c) 2015 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file screened_poisson.cpp
 *
 * Brief description
 *
 */
#include "common.h"

#include "vmath.h"
#include "raytrace.h"
#include "../other/PoissonRecon/Src/SPR.h"

#define DEFAULT_FULL_DEPTH 5
struct rt_vert {
    point_t p;
    vect_t n;
    int is_set;
};

struct npoints {
    struct rt_vert in;
    struct rt_vert out;
};

struct rt_point_container {
    struct npoints *pts;
    int pnt_cnt;
    int capacity;
};

struct rt_parallel_container {
    struct rt_i *rtip;
    struct resource *resp;
    struct bu_vls *logs;
    struct rt_point_container *npts;
    int ray_dir;
    int ncpus;
    fastf_t delta;

    point_t model_min;
    point_t model_max;
    fastf_t model_span[3];
    fastf_t perCpuSlabWidth[3];
};

/* add all hit point info to info list */
HIDDEN int
add_hit_pnts(struct application *app, struct partition *partH, struct seg *UNUSED(segs))
{

    struct partition *pp;
    struct soltab *stp;
    /*point_t hit_pnt;
    vect_t hit_normal;*/
    struct rt_point_container *c = (struct rt_point_container *)(app->a_uptr);
    struct npoints *npt;

    if (c->pnt_cnt > c->capacity-1) {
	c->capacity *= 4;
	c->pts = (struct npoints *)bu_realloc((char *)c->pts, c->capacity * sizeof(struct npoints), "enlarge results array");
    }

    RT_CK_APPLICATION(app);
    /*struct bu_vls *fp = (struct bu_vls *)(app->a_uptr);*/

    /* add all hit points */
    for (pp = partH->pt_forw; pp != partH; pp = pp->pt_forw) {

	npt = &(c->pts[c->pnt_cnt]);

	/* add "in" hit point info */
	stp = pp->pt_inseg->seg_stp;

	/* hack fix for bad tgc surfaces */
	if (bu_strncmp("rec", stp->st_meth->ft_label, 3) == 0 || bu_strncmp("tgc", stp->st_meth->ft_label, 3) == 0) {

	    /* correct invalid surface number */
	    if (pp->pt_inhit->hit_surfno < 1 || pp->pt_inhit->hit_surfno > 3) {
		pp->pt_inhit->hit_surfno = 2;
	    }
	    if (pp->pt_outhit->hit_surfno < 1 || pp->pt_outhit->hit_surfno > 3) {
		pp->pt_outhit->hit_surfno = 2;
	    }
	}


	VJOIN1(npt->in.p, app->a_ray.r_pt, pp->pt_inhit->hit_dist, app->a_ray.r_dir);
	RT_HIT_NORMAL(npt->in.n, pp->pt_inhit, stp, &(app->a_ray), pp->pt_inflip);
	npt->in.is_set = 1;
	//bu_vls_printf(fp, "%f %f %f %f %f %f\n", hit_pnt[0], hit_pnt[1], hit_pnt[2], hit_normal[0], hit_normal[1], hit_normal[2]);
	/* add "out" hit point info (unless half-space) */
	stp = pp->pt_inseg->seg_stp;
	if (bu_strncmp("half", stp->st_meth->ft_label, 4) != 0) {
	    VJOIN1(npt->out.p, app->a_ray.r_pt, pp->pt_outhit->hit_dist, app->a_ray.r_dir);
	    RT_HIT_NORMAL(npt->out.n, pp->pt_outhit, stp, &(app->a_ray), pp->pt_outflip);
	    npt->out.is_set = 1;
	    //bu_vls_printf(fp, "%f %f %f %f %f %f\n", hit_pnt[0], hit_pnt[1], hit_pnt[2], hit_normal[0], hit_normal[1], hit_normal[2]);
	}
	c->pnt_cnt++;
    }
    return 1;
}

/* don't care about misses */
HIDDEN int
ignore_miss(struct application *app)
{
    RT_CK_APPLICATION(app);
    //bu_log("miss!\n");
    return 0;
}

void
_rt_gen_worker(int cpu, void *ptr)
{
    int i, j;
    struct application ap;
    struct rt_parallel_container *state = (struct rt_parallel_container *)ptr;
    int dir1, dir2, dir3;
    fastf_t d[3];
    int n[3];
    RT_APPLICATION_INIT(&ap);
    ap.a_rt_i = state->rtip;
    ap.a_hit = add_hit_pnts;
    ap.a_miss = ignore_miss;
    ap.a_onehit = 0;
    ap.a_logoverlap = rt_silent_logoverlap;
    ap.a_resource = &state->resp[cpu];
    ap.a_uptr = (void *)(&state->npts[cpu]);

    /* Make sure we've got at least 10 steps in all 3 dimensions,
     * regardless of delta */
    for (i = 0; i < 3; i++) {
	n[i] = state->model_span[i]/state->delta;
	if(n[i] < 10) n[i] = 10;
	d[i] = state->model_span[i]/n[i];
    }

    dir1 = state->ray_dir;
    dir2 = (state->ray_dir+1)%3;
    dir3 = (state->ray_dir+2)%3;

    /* set ray direction */
    ap.a_ray.r_dir[dir1] = 1;
    ap.a_ray.r_dir[dir2] = 0;
    ap.a_ray.r_dir[dir3] = 0;
    
    ap.a_ray.r_pt[dir1] = state->model_min[dir1] -100;
    double slabBottom = (cpu - 1) * state->perCpuSlabWidth[dir3];
    double slabTop = cpu * state->perCpuSlabWidth[dir3];

    int slabStartLN = ceil(slabBottom / d[dir3]);
    int slabEndLN = ceil(slabTop / d[dir3]);

    for (j = slabStartLN; j <= slabEndLN; j++)
    {
	ap.a_ray.r_pt[dir3] = state->model_min[dir3] + (j * d[dir3]);

	int iAxisSteps = ceil(state->model_span[dir2] / d[dir2]);
	for (i = 0; i <= iAxisSteps; i++)
	{
	    ap.a_ray.r_pt[dir2] = state->model_min[dir2] + (i * d[dir2]);
	    rt_shootray(&ap);
	}
    }
}


HIDDEN int
_rt_generate_points(int **faces, int *num_faces, point_t **points, int *num_pnts, struct bu_ptbl *hit_pnts, struct db_i *dbip, const char *obj, fastf_t delta, int fidelity)
{
    int i, dir1, j;
    int ncpus = bu_avail_cpus();
    struct rt_parallel_container *state;
    struct bu_vls vlsstr;
    bu_vls_init(&vlsstr);

    if (!hit_pnts || !dbip || !obj) return -1;

    BU_GET(state, struct rt_parallel_container);

    state->rtip = rt_new_rti(dbip);
    state->delta = delta;

    /* load geometry object */
    if (rt_gettree(state->rtip, obj) < 0) return -1;
    rt_prep_parallel(state->rtip, 1);

    /* initialize all the per-CPU resources that are going to be used */
    state->resp = (struct resource *)bu_calloc(ncpus+1, sizeof(struct resource), "resources");
    for (i = 0; i < ncpus+1; i++) {
	rt_init_resource(&(state->resp[i]), i, state->rtip);
    }

    /* allocate storage for the points */
    state->npts = (struct rt_point_container *)bu_calloc(ncpus+1, sizeof(struct rt_point_container), "point container arrays");
    int n[3];

    /* get min and max points of bounding box */
    VMOVE(state->model_min, state->rtip->mdl_min);
    VMOVE(state->model_max, state->rtip->mdl_max);
    VSUB2(state->model_span, state->model_max, state->model_min);
    for (i = 0; i < 3; i++) {
	state->perCpuSlabWidth[i] = state->model_span[i] / ncpus;
	
	n[i] = (int)(state->model_span[i]/state->delta) + 2;
	if(n[i] < 12) n[i] = 12;
    }
    int total = 0;
    for (i = 0; i < 3; i++) total += n[i]*n[(i+1)%3];
    if (total > 1e6) total = 1e6;
    for (i = 0; i < ncpus+1; i++) {
	state->npts[i].pts = (struct npoints *)bu_calloc(total, sizeof(struct npoints), "npoints arrays");
	state->npts[i].pnt_cnt = 0;
	state->npts[i].capacity = total;
    }

    /* perform raytracing from the 3 views */
    for (dir1 = 0; dir1 < 3; dir1++) {
	state->ray_dir = dir1;
	state->ncpus = ncpus;
	state->delta = delta;
	bu_parallel(_rt_gen_worker, ncpus, (void *)state);
    }

    /* count the number of points */
    int out_cnt = 0;
    for (i = 0; i < ncpus+1; i++) {
	bu_log("%d, pnt_cnt: %d\n", i, state->npts[i].pnt_cnt);
	for (j = 0; j < state->npts[i].pnt_cnt; j++) {
	    struct npoints *npt = &(state->npts[i].pts[j]);
	    if (npt->in.is_set) out_cnt++;
	    if (npt->out.is_set) out_cnt++;
	}
    }

    if (!out_cnt) return 1;

    /* store points */
    struct rt_vert **rt_verts = (struct rt_vert **)bu_calloc(out_cnt, sizeof(struct rt_vert *), "output array");
    int curr_ind = 0;
    for (i = 0; i < ncpus+1; i++) {
	for (j = 0; j < state->npts[i].pnt_cnt; j++) {
	    struct npoints *npt = &(state->npts[i].pts[j]);
	    if (npt->in.is_set) {
		rt_verts[curr_ind] = &(npt->in);
		curr_ind++;
	    }
	    if (npt->out.is_set) {
		rt_verts[curr_ind] = &(npt->out);
		curr_ind++;
	    }
	}
    }

    /* perform SPR algorithm */
    (void)spr_surface_build(faces, num_faces, (double **)points, num_pnts,
			    (const struct cvertex **)rt_verts, out_cnt, fidelity);

    /* free memory */
    bu_free(rt_verts, "output array");
    rt_verts = NULL;
    for (i = 0; i < ncpus+1; i++) {
	bu_free(state->npts[i].pts, "npoints arrays");
    }
    bu_free(state->npts, "point container arrays");
    bu_free(state->resp, "resources");
    BU_PUT(state, struct rt_parallel_container);
    state = NULL;
    
    return 0;
}

extern "C" void
rt_generate_mesh(int **faces, int *num_faces, point_t **points, int *num_pnts,
		 struct db_i *dbip, const char *obj, fastf_t delta, int fidelity)
{
    fastf_t d = delta;
    struct bu_ptbl *hit_pnts;
    if (!faces || !num_faces || !points || !num_pnts) return;
    if (!dbip || !obj) return;
    BU_GET(hit_pnts, struct bu_ptbl);
    bu_ptbl_init(hit_pnts, 64, "hit pnts");
    if (NEAR_ZERO(d, SMALL_FASTF)) d = 1;
    if (_rt_generate_points(faces, num_faces, points, num_pnts, hit_pnts, dbip, obj, d, fidelity)) {
	(*num_faces) = 0;
	(*num_pnts) = 0;
	return;
    }
}
// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

