#ifndef __FITNESS_H__
#define __FITNESS_H__

#define U_AXIS 0
#define V_AXIS 1
#define I_AXIS 2

/* Used for comparing rays */
#define STATUS_PP 1
#define STATUS_MP 2
#define STATUS_EMPTY 0

#define DB_OPEN_FAILURE -1
#define DB_DIRBUILD_FAILURE -2

struct part {
    struct bu_list  l;
    fastf_t	    inhit_dist;
    fastf_t	    outhit_dist;
};

struct fitness_state {
    char *name;
    struct part **rays; /* internal representation of raytraced source */
    struct db_i *db; /* the database the source and population are a part of */
    struct rt_i *rtip; /* current objects to be raytraced */

    struct resource resource[MAX_PSW]; /* memory resource for multi-cpu processing */
    int ncpu;
    int max_cpus;

    int res[2]; /*  ray resolution on u and v axes */
    double gridSpacing[2]; /* grid spacing on u and v axes */
    int row; /* current v axis index *///IS IT?
    
    int capture; /* flags whether to store the object */
    fastf_t diff; /* linear difference between source and object */
};

/* store a ray that hit */
int capture_hit(register struct application *ap, struct partition *partHeadp, struct seg *segs);
/* store a ray that missed */
int capture_miss(register struct application *ap);
/* compare a ray that hit to the same ray from source */
int compare_hit(register struct application *ap, struct partition *partHeadp, struct seg *segs);
/* compare a ray that missed to the same ray from source */
int compare_miss(register struct application *ap);
/* grab the next row of rays to be evaluated */
int get_next_row(void);
/* raytrace an object stored in fstate  either storing the rays or comparing them to the source */
void rt_worker(int cpu, genptr_t g);
/* prep for raytracing object, and call rt_worker for parallel processing */
int fit_rt (char *obj);
/* load database and prepare fstate for work */
int fit_prep(char *db, int rows, int cols);
/* cleanup */
void fit_clean(void);
/* store a given object as the source  */
void fit_store(char *obj);
/* update grid resolution */
void fit_updateRes(int rows, int cols);
/* returns total linear difference between object and source */
fastf_t fit_linDiff(char *obj);
/* clear the stored rays */
void rays_clean(void);



#endif


