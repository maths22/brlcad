/*
 * record_seq.c - control the display of a series of images
 *	and record them frame by frame
 *
 * 	The interface to the framebuffer is that this program will execute
 *	a command int the form: display_image n    where n will be a sequence
 *	number from 0 to N-1 where N is the number of images specified
 *	in the command line of this command.
 *
 *	The user will typically provide a shell script
 *	that will display the appropriate image on the framebuffer.
 */

#include <stdio.h>
#include <strings.h>

/* #define DEBUG 1 /* Define to only print system commands */

#ifdef DEBUG
#define SYSTEM(c)	fprintf(stderr,"system(%s);\n",c)
#else
#define SYSTEM(c)	fprintf(stderr,"system(%s);\n",c); system(c)
#endif

#define CBARS_TIME	10		/* Normal */
/*#define CBARS_TIME	2		/* Testing mode */


record_seq(number_of_images, number_of_frames, start_seq_number)
int number_of_images, number_of_frames;
int start_seq_number;
{
	char cmd[100];
	int i;
	int scene_number = 1;
	int start_frame = 1;


	fprintf(stderr,
		"number of images is %d, number of frames per image is %d\n",
		number_of_images,number_of_frames);

	
	SYSTEM("cbars-fb");	/* Start out with color bars */

	/* Make initial scene title matte recording */
	SYSTEM("vas4 new");
	SYSTEM("vas4 reset_time");

	/* Handle the color bars specially. It is the first recording */
	fprintf(stderr,"Record color bars for %d seconds\n",CBARS_TIME);
	sprintf(cmd,"vas4 record %dsec", CBARS_TIME);
	SYSTEM(cmd);

	/* Now record the user files */
	for (i=start_seq_number; i < start_seq_number+number_of_images; i++) {
		sprintf(cmd,"display_image %d",i);
		SYSTEM(cmd);
		sprintf(cmd,"vas4 record %d",number_of_frames);
		SYSTEM(cmd);
	}

	/* Record last frame for 30 more seconds */
	fprintf(stderr,"Last image\n");
	SYSTEM("vas4 record 30sec\n");

	/* Wrap up by stopping the controller and rewind */
	SYSTEM("vas4 time0\n");
	SYSTEM("vas4 stop\n");

}
