#include "common.cl"


/* hit_surfno is set to one of these */
#define REC_NORM_BODY	(1)		/* compute normal */
#define REC_NORM_TOP	(2)		/* copy tgc_N */
#define REC_NORM_BOT	(3)		/* copy reverse tgc_N */


struct rec_shot_specific {
    double rec_SoR[16];
    double rec_V[3];
};

int rec_shot(global struct hit *res, const double3 r_pt, const double3 r_dir, global const struct rec_shot_specific *rec)
{
    global const double *SoR = rec->rec_SoR;
    global const double *V = rec->rec_V;

    double3 dprime;		// D'
    double3 pprime;		// P'
    double k1, k2;		// distance constants of solution
    double3 xlated;		// translated vector
    struct hit hits[4];		// 4 potential hit points
    struct hit *hitp;		// pointer to hit point
    int nhits = 0;		// Number of hit points

    hitp = &hits[0];

    /* out, Mat, vect */
    const double f = 1.0/SoR[15];
    dprime.x = dot(vload3(0, &SoR[0]), r_dir) * f;
    dprime.y = dot(vload3(0, &SoR[4]), r_dir) * f;
    dprime.z = dot(vload3(0, &SoR[8]), r_dir) * f;
    xlated = r_pt - vload3(0, V);
    pprime.x = dot(vload3(0, &SoR[0]), xlated) * f;
    pprime.y = dot(vload3(0, &SoR[4]), xlated) * f;
    pprime.z = dot(vload3(0, &SoR[8]), xlated) * f;

    /*
     * Check for hitting the end plates.
     */
    if (!ZERO(dprime.z)) {
	k1 = -pprime.z / dprime.z;			// bottom plate
	k2 = (1.0 - pprime.z) / dprime.z;		// top plate

	hitp->hit_vpriv = pprime + k1 * dprime;		// hit'
	if (hitp->hit_vpriv.x * hitp->hit_vpriv.x +
	    hitp->hit_vpriv.y * hitp->hit_vpriv.y < (1.0 + SMALL_FASTF)) {
	    hitp->hit_dist = k1;
	    hitp->hit_surfno = REC_NORM_BOT;		// -H
	    hitp++; nhits++;
	}

	hitp->hit_vpriv = pprime + k2 * dprime;		// hit'
	if (hitp->hit_vpriv.x * hitp->hit_vpriv.x +
	    hitp->hit_vpriv.y * hitp->hit_vpriv.y < (1.0 + SMALL_FASTF)) {
	    hitp->hit_dist = k2;
	    hitp->hit_surfno = REC_NORM_TOP;		// +H
	    hitp++; nhits++;
	}
    }

    /* Check for hitting the cylinder.  Find roots of the equation,
     * using formula for quadratic w/ a=1
     */
    if (nhits != 2) {
	double b;		// coeff of polynomial
	double discriminant;	// root of radical, the discriminant
	double dx2dy2;

	dx2dy2 = 1 / (dprime.x*dprime.x + dprime.y*dprime.y);
	b = 2 * (dprime.x*pprime.x + dprime.y*pprime.y) * dx2dy2;
	discriminant = b*b - 4 * dx2dy2 * (pprime.x*pprime.x + pprime.y*pprime.y - 1);

	/* might want compare against tol_dist here? */

	if (NEAR_ZERO(discriminant, SMALL_FASTF)) {
	    /* if the discriminant is zero, it's a double-root grazer */
	    k1 = -b * 0.5;
	    hitp->hit_vpriv = pprime + k1 * dprime;	// hit'
	    if (hitp->hit_vpriv.z > -SMALL_FASTF && hitp->hit_vpriv.z < (1.0 + SMALL_FASTF)) {
		hitp->hit_dist = k1;
		hitp->hit_surfno = REC_NORM_BODY;	// compute N
		hitp++; nhits++;
	    }

	} else if (discriminant > SMALL_FASTF) {
	    /* if the discriminant is positive, there are two roots */

	    discriminant = sqrt(discriminant);
	    k1 = (-b+discriminant) * 0.5;
	    k2 = (-b-discriminant) * 0.5;

	    /*
	     * k1 and k2 are potential solutions to intersection with side.
	     * See if they fall in range.
	     */
	    hitp->hit_vpriv = pprime + k1 * dprime;	// hit'
	    if (hitp->hit_vpriv.z > -SMALL_FASTF && hitp->hit_vpriv.z < (1.0 + SMALL_FASTF)) {
		hitp->hit_dist = k1;
		hitp->hit_surfno = REC_NORM_BODY;	// compute N
		hitp++; nhits++;
	    }

	    hitp->hit_vpriv = pprime + k2 * dprime;	// hit'
	    if (hitp->hit_vpriv.z > -SMALL_FASTF && hitp->hit_vpriv.z < (1.0 + SMALL_FASTF)) {
		hitp->hit_dist = k2;
		hitp->hit_surfno = REC_NORM_BODY;	// compute N
		hitp++; nhits++;
	    }
	}
    }

    /* missed both ends and side? */
    if (nhits == 0)
	return 0;

    /* Prepare to collapse duplicate points.  Check for case where two
     * or more of the hits have the same distance, e.g. hitting at the
     * rim or down an edge.
     */
    if (nhits > 3) {
	/* collapse just one duplicate (4->3) */
	if (NEAR_EQUAL(hits[0].hit_dist, hits[3].hit_dist, rti_tol_dist)) {
	    nhits--; // discard [3]
	} else if (NEAR_EQUAL(hits[1].hit_dist, hits[3].hit_dist, rti_tol_dist)) {
	    nhits--; // discard [3]
	} else if (NEAR_EQUAL(hits[2].hit_dist, hits[3].hit_dist, rti_tol_dist)) {
	    nhits--; // discard [3]
	}
    }
    if (nhits > 2) {
	/* collapse any other duplicate (3->2) */
	if (NEAR_EQUAL(hits[0].hit_dist, hits[2].hit_dist, rti_tol_dist)) {
	    nhits--; // discard [2]
	} else if (NEAR_EQUAL(hits[1].hit_dist, hits[2].hit_dist, rti_tol_dist)) {
	    nhits--; // discard [2]
	} else if (NEAR_EQUAL(hits[0].hit_dist, hits[1].hit_dist, rti_tol_dist)) {
	    hits[1] = hits[2];
	    nhits--; // moved [2] to [1], discarded [2]
	}
    }

    /* sanity check that we don't end up with too many hits */
    if (nhits > 3) {
	/* count just the first two hits, to have something */
	nhits-=2;
    } else if (nhits > 2) {
	/* count just the first two hits, to have something */
	nhits--;
    } else if (nhits == 1) {
	/* Ray is probably tangent to body of cylinder or a single hit
	 * on only an end plate.  This could be considered a MISS, but
	 * to signal the condition, return 0-thickness hit.
	 */
	hits[1] = hits[0];
	nhits++; // replicate [0] to [1]
    }

    if (res == NULL) {
	return 2;	// HIT
    }

    if (hits[0].hit_dist < hits[1].hit_dist) {
	// entry is [0], exit is [1]
	res[0] = hits[0];
	res[1] = hits[1];
    } else {
	// entry is [1], exit is [0]
	res[0] = hits[1];
	res[1] = hits[0];
    }
    return 2;		// HIT
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
