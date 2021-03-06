/*                         C O L O R . C
 * BRL-CAD
 *
 * Copyright (c) 1997-2016 United States Government as represented by
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

#include "common.h"

#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include "bio.h"

#include "bu/color.h"
#include "bu/log.h"
#include "bu/malloc.h"


/* libfb defines replicated here to avoid a libfb dependency */
#define ACHROMATIC -1.0

#define HUE 0
#define SAT 1
#define VAL 2

#define RED 0
#define GRN 1
#define BLU 2


/* vmath/libbu routines replicated here to avoid a libbn dependency */
enum axis {
    X = 0,
    Y = 1,
    Z = 2
};
#define VSET(a, b, c, d) { (a)[X] = (b); (a)[Y] = (c); (a)[Z] = (d); }
#define VSETALL(a, s) { (a)[X] = (a)[Y] = (a)[Z] = (s); }
#define NEAR_ZERO(val, epsilon)	(((val) > -epsilon) && ((val) < epsilon))
#define V3ARGS(a) (a)[X], (a)[Y], (a)[Z]


void
bu_rgb_to_hsv(unsigned char *rgb, fastf_t *hsv)
{
    fastf_t red, grn, blu;
    fastf_t *hue = &hsv[HUE];
    fastf_t *sat = &hsv[SAT];
    fastf_t *val = &hsv[VAL];
    fastf_t max, min;
    fastf_t chroma;

    /*
     * Compute value
     */
    max = min = red = ((fastf_t)rgb[RED]) / 255.0;

    grn = ((fastf_t)rgb[GRN]) / 255.0;
    if (grn < min)
	min = grn;
    else if (grn > max)
	max = grn;

    blu = ((fastf_t)rgb[BLU]) / 255.0;
    if (blu < min)
	min = blu;
    else if (blu > max)
	max = blu;

    *val = max;

    /*
     * Compute saturation
     */
    chroma = max - min;
    if (max > 0.0)
	*sat = chroma / max;
    else
	*sat = 0.0;

    /*
     * Compute hue
     */
    if (NEAR_ZERO(*sat, SMALL_FASTF)) {
	*hue = ACHROMATIC;
    } else {
	if (NEAR_ZERO(red - max, SMALL_FASTF))      /* red == max */
	    *hue = (grn - blu) / chroma;
	else if (NEAR_ZERO(grn - max, SMALL_FASTF)) /* grn == max */
	    *hue = 2.0 + (blu - red) / chroma;
	else if (NEAR_ZERO(blu - max, SMALL_FASTF)) /* blu == max */
	    *hue = 4.0 + (red - grn) / chroma;

	/*
	 * Convert hue to degrees
	 */
	*hue *= 60.0;
	if (*hue < 0.0)
	    *hue += 360.0;
    }
}


int
bu_hsv_to_rgb(fastf_t *hsv, unsigned char *rgb)
{
    fastf_t float_rgb[3] = { 0.0, 0.0, 0.0 };
    fastf_t hue, sat, val;
    fastf_t hue_frac;
    fastf_t p, q, t;
    long int hue_int;

    hue = hsv[HUE];
    sat = hsv[SAT];
    val = hsv[VAL];

    if ((((hue < 0.0) || (hue > 360.0)) && (!NEAR_ZERO(hue - ACHROMATIC, SMALL_FASTF))) /* hue != ACHROMATIC */
	|| (sat < 0.0) || (sat > 1.0)
	|| (val < 0.0) || (val > 1.0)
	|| ((NEAR_ZERO(hue - ACHROMATIC, SMALL_FASTF)) && (sat > 0.0))) /* hue == ACHROMATIC */
    {
	bu_log("bu_hsv_to_rgb: Illegal HSV (%g, %g, %g)\n",
	       V3ARGS(hsv));
	return 0;
    }

    /* so hue == ACHROMATIC (or is ignored)	*/
    if (NEAR_ZERO(sat, SMALL_FASTF)) {
	VSETALL(float_rgb, val);
    } else {
	if (NEAR_ZERO(hue - 360.0, SMALL_FASTF))
	    hue = 0.0;
	hue /= 60.0;
	hue_int = lrint(floor((double)hue));
	hue_frac = hue - hue_int;
	p = val * (1.0 - sat);
	q = val * (1.0 - (sat * hue_frac));
	t = val * (1.0 - (sat * (1.0 - hue_frac)));
	switch (hue_int) {
	    case 0: VSET(float_rgb, val, t, p); break;
	    case 1: VSET(float_rgb, q, val, p); break;
	    case 2: VSET(float_rgb, p, val, t); break;
	    case 3: VSET(float_rgb, p, q, val); break;
	    case 4: VSET(float_rgb, t, p, val); break;
	    case 5: VSET(float_rgb, val, p, q); break;
	    default:
		bu_log("%s:%d: This shouldn't happen\n",
		       __FILE__, __LINE__);
		bu_bomb("unexpected condition encountered in bu_hsv_to_rgb\n");
	}
    }

    rgb[RED] = (unsigned char)lrint(float_rgb[RED] * 255.0);
    rgb[GRN] = (unsigned char)lrint(float_rgb[GRN] * 255.0);
    rgb[BLU] = (unsigned char)lrint(float_rgb[BLU] * 255.0);

    return 1;
}


int
bu_str_to_rgb(char *str, unsigned char *rgb)
{
    int num;
    unsigned int r = 0;
    unsigned int g = 0;
    unsigned int b = 0;

    if (UNLIKELY(!str || !rgb)) {
	return 0;
    }

    while (isspace((int)(*str)))
	++str;

    if (*str == '#') {
	if (strlen(++str) != 6)
	    return 0;
	sscanf(str, "%02x%02x%02x", (unsigned int *)&r, (unsigned int *)&g, (unsigned int *)&b);
    } else if (isdigit((int)(*str))) {
	num = sscanf(str, "%u/%u/%u", &r, &g, &b);
	if (num == 1) {
	    num = sscanf(str, "%u %u %u", &r, &g, &b);
	    if (num != 3)
		return 0;
	}
	if (r > 255)
	    r = 255;
	if (g > 255)
	    g = 255;
	if (b > 255)
	    b = 255;
    } else {
	return 0;
    }

    VSET(rgb, (fastf_t)r, (fastf_t)g, (fastf_t)b);

    return 1;
}

int
bu_color_to_rgb_chars(struct bu_color *cp, unsigned char *rgb)
{
    unsigned int r, g, b;
    if (UNLIKELY(!cp || !rgb)) {
	return 0;
    }
    r = (unsigned int)cp->buc_rgb[RED];
    g = (unsigned int)cp->buc_rgb[GRN];
    b = (unsigned int)cp->buc_rgb[BLU];

    rgb[0] = (unsigned char)r;
    rgb[1] = (unsigned char)g;
    rgb[2] = (unsigned char)b;

    return 1;
}


int
bu_color_from_rgb_chars(struct bu_color *cp, unsigned char *rgb)
{
    unsigned int r, g, b;
    if (UNLIKELY(!cp || !rgb)) {
	return 0;
    }

    r = (unsigned int)rgb[RED];
    g = (unsigned int)rgb[GRN];
    b = (unsigned int)rgb[BLU];


    cp->buc_rgb[RED] = (fastf_t)r;
    cp->buc_rgb[GRN] = (fastf_t)g;
    cp->buc_rgb[BLU] = (fastf_t)b;

    return 1;
}


int
bu_color_to_rgb_floats(struct bu_color *cp, fastf_t *rgb)
{
    if (UNLIKELY(!cp || !rgb)) {
	return 0;
    }

    rgb[0] = cp->buc_rgb[RED];
    rgb[1] = cp->buc_rgb[GRN];
    rgb[2] = cp->buc_rgb[BLU];

    return 1;
}


int
bu_color_from_rgb_floats(struct bu_color *cp, fastf_t *rgb)
{
    if (UNLIKELY(!cp || !rgb)) {
	return 0;
    }

    cp->buc_rgb[RED] = rgb[0];
    cp->buc_rgb[GRN] = rgb[1];
    cp->buc_rgb[BLU] = rgb[2];

    return 1;
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
