/*
 *  			T E X T . C
 *  
 *  Texture map lookup
 */
#include <stdio.h>
#include "../h/machine.h"
#include "../h/vmath.h"
#include "../h/raytrace.h"
#include "material.h"

struct texture_specific {
	char	*tx_file[128];	/* Filename */
	int	tx_w;		/* Width of texture in pixels */
	int	tx_fw;		/* File width of texture in pixels */
	int	tx_l;		/* Length of pixels in lines */
	char	*tx_pixels;	/* Pixel holding area */
};
#define TX_NULL	((struct texture_specific *)0)

struct matparse texture_parse[] = {
	"file",		(int)(TX_NULL->tx_file),	"%s",
	"w",		(int)&(TX_NULL->tx_w),		"%d",
	"l",		(int)&(TX_NULL->tx_l),		"%d",
	"fw",		(int)&(TX_NULL->tx_fw),		"%d",
	(char *)0,	0,				(char *)0
};

extern texture_render();

int
texture_setup( rp )
register struct region *rp;
{
	register struct texture_specific *tp;

	GETSTRUCT( tp, texture_specific );
	rp->reg_ufunc = texture_render;
	rp->reg_udata = (char *)tp;

	tp->tx_file[0] = '\0';
	tp->tx_w = tp->tx_fw = tp->tx_l = -1;
	matlib_parse( rp->reg_mater.ma_matparm, texture_parse, (char *)tp );
	if( tp->tx_w < 0 )  tp->tx_w = 512;
	if( tp->tx_l < 0 )  tp->tx_l = tp->tx_w;
	if( tp->tx_fw < 0 )  tp->tx_fw = tp->tx_w;
	tp->tx_pixels = (char *)0;
	matlib_print( "texture", texture_parse, (char *)tp );
	return(1);
}


/*
 *  			T E X T U R E _ R E N D E R
 *  
 *  Given a u,v coordinate within the texture ( 0 <= u,v <= 1.0 ),
 *  return a pointer to the relevant pixel.
 *
 *  Note that .pix files are stored left-to-right, bottom-to-top,
 *  which works out very naturally for the indexing scheme.
 */
texture_render( ap, pp )
register struct application *ap;
register struct partition *pp;
{
	register struct texture_specific *tp =
		(struct texture_specific *)pp->pt_regionp->reg_udata;
	auto fastf_t uv[2];
	register unsigned char *cp;

	rt_functab[pp->pt_inseg->seg_stp->st_id].ft_uv(
		pp->pt_inseg->seg_stp, pp->pt_inhit, uv );

	/* If File could not be opened -- give debug colors */
top:
	if( tp->tx_file[0] == '\0' )  {
		VSET( ap->a_color, uv[0]*255, 0, uv[1]*255 );
		return(1);
	}
	/* Dynamic load of file -- don't read until first pixel needed */
	if( tp->tx_pixels == (char *)0 )  {
		char *linebuf;
		register int fd;
		register int i;

		if( (fd = open(tp->tx_file, 0)) < 0 )  {
			rt_log("texture_render(%s):  can't open\n", tp->tx_file);
			tp->tx_file[0] = '\0';
			goto top;
		}
		linebuf = rt_malloc(tp->tx_fw*3,"texture file line");
		tp->tx_pixels = rt_malloc(
			tp->tx_w * tp->tx_l * 3,
			tp->tx_file );
		for( i=0; i<tp->tx_l; i++ )  {
			if( read(fd,linebuf,tp->tx_fw*3) != tp->tx_fw*3 )  {
				rt_log("text_uvget: read error on %s\n", tp->tx_file);
				tp->tx_file[0] = '\0';
				(void)close(fd);
				rt_free(linebuf,"file line, error");
				goto top;
			}
			bcopy( linebuf, tp->tx_pixels + i*tp->tx_w*3, tp->tx_w*3 );
		}
		(void)close(fd);
		rt_free(linebuf,"texture file line");
	}
	/* u is left->right index, v is line number */
	cp = (unsigned char *)tp->tx_pixels +
	     ((int) (uv[1]*(tp->tx_l-1))) * tp->tx_w * 3 +	/* v */
	     ((int) (uv[0]*(tp->tx_w-1))) * 3;
	VSET( ap->a_color, *cp++/255., *cp++/255., *cp++/255.);
	return(1);
}

/*
 *			T E S T M A P _ R E N D E R
 *
 *  Render a map which varries red with U and blue with V values.
 *  Mostly useful for debugging ft_fv() routines.
 */
testmap_render( ap, pp )
register struct application *ap;
register struct partition *pp;
{
	auto fastf_t uv[2];

	rt_functab[pp->pt_inseg->seg_stp->st_id].ft_uv(
		pp->pt_inseg->seg_stp, pp->pt_inhit, uv );
	VSET( ap->a_color, uv[0], 0, uv[1] );
	return(1);
}

/*
 *			T E S T M A P _ S E T U P
 */
testmap_setup( rp )
register struct region *rp;
{
	rp->reg_ufunc = testmap_render;
	rp->reg_udata = (char *)0;
	return(1);
}
