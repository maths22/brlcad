/*
 *			S P H . H
 *
 *  Sphere data structure and function declarations.
 *
 *  Author -
 *	Phillip Dykstra
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */

typedef	struct	{
	int	ny;		/* Number of "y" bins */
	int	*nx;		/* Number of "x" bins per "y" bin */
	unsigned char **xbin;	/* staring addresses of "x" bins */
	unsigned char *_data;	/* For freeing purposes, start of data */
} spm_map_t;

#define	SPH_NULL (spm_map_t *)0

spm_map_t *spm_init();
void	spm_free();
void	spm_read();
void	spm_write();
int	spm_load();
int	spm_save();
int	spm_pix_load();
int	spm_pix_save();
void	spm_dump();
