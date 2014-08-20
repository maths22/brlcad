/*                      P A R A L L E L . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2014 United States Government as represented by
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
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <signal.h>

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#  include <sys/resource.h>
#endif

#ifdef linux
#  include <sys/types.h>
#  ifdef HAVE_SYS_WAIT_H
#    include <sys/wait.h>
#  endif
#  include <sys/stat.h>
#  include <sys/sysinfo.h>
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#  include <sys/types.h>
#  include <sys/param.h>
#  include <sys/sysctl.h>
#  ifdef HAVE_SYS_WAIT_H
#    include <sys/wait.h>
#  endif
#  include <sys/stat.h>
#endif

#ifdef __APPLE__
#  include <sys/types.h>
#  ifdef HAVE_SYS_WAIT_H
#    include <sys/wait.h>
#  endif
#  include <sys/stat.h>
#  include <sys/param.h>
#  include <sys/sysctl.h>
#endif

#ifdef __sp3__
#  include <sys/types.h>
#  include <sys/sysconfig.h>
#  include <sys/var.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_ULOCKS_H
#  include <ulocks.h>
#endif
#ifdef HAVE_SYS_SYSMP_H
#  include <sys/sysmp.h> /* for sysmp() */
#endif

#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif

#ifdef HAVE_SCHED_H
#  include <sched.h>
#else
#  ifdef HAVE_SYS_SCHED_H
#    include <sys/sched.h>
#  endif
#endif

/*
 * multithreading support for SunOS 5.X / Solaris 2.x
 */
#if defined(SUNOS) && SUNOS >= 52
#  include <sys/unistd.h>
#  include <thread.h>
#  include <synch.h>
#  define rt_thread_t thread_t
#endif /* SUNOS */

/*
 * multithread support built on POSIX Threads (pthread) library.
 */
#ifdef HAVE_PTHREAD_H
#  include <pthread.h>
#  define rt_thread_t pthread_t
#endif

#ifdef WIN32
#  define rt_thread_t HANDLE
#endif

#include "bio.h"

#include "bu/debug.h"
#include "bu/log.h"
#include "bu/malloc.h"
#include "bu/parallel.h"
#include "bu/str.h"

#include "./parallel.h"


struct thread_data {
    void (*user_func)(int, void *);
    void *user_arg;
    int cpu_id;
    int affinity;
};


int
bu_parallel_id(void)
{
    return thread_get_cpu();
}


int
bu_is_parallel(void)
{
    /* this routine is deprecated, do not use. */
    return 0;
}


void
bu_nice_set(int newnice)
{
#ifdef HAVE_SETPRIORITY
    int opri, npri;

#  ifndef PRIO_PROCESS     /* necessary for linux */
#    define PRIO_PROCESS 0 /* From /usr/include/sys/resource.h */
#  endif
    opri = getpriority(PRIO_PROCESS, 0);
    setpriority(PRIO_PROCESS, 0, newnice);
    npri = getpriority(PRIO_PROCESS, 0);

    if (UNLIKELY(bu_debug)) {
	bu_log("bu_nice_set() Priority changed from %d to %d\n", opri, npri);
    }

#else /* !HAVE_SETPRIORITY */
    /* no known means to change the nice value */
    if (UNLIKELY(bu_debug)) {
	bu_log("bu_nice_set(%d) Priority NOT changed\n", newnice);
    }
#endif  /* _WIN32 */
}


size_t
bu_avail_cpus(void)
{
    int ncpu = -1;

#ifdef PARALLEL

#  if defined(__sp3__)
    if (ncpu < 0) {
	int status;
	int cmd;
	int parmlen;
	struct var p;

	cmd = SYS_GETPARMS;
	parmlen = sizeof(struct var);
	if (sysconfig(cmd, &p, parmlen) != 0) {
	    bu_bomb("bu_parallel(): sysconfig error for sp3");
	}
	ncpu = p.v_ncpus;
    }
#  endif	/* __sp3__ */


#  ifdef __FreeBSD__
    if (ncpu < 0) {
	int maxproc;
	size_t len;
	len = 4;
	if (sysctlbyname("hw.ncpu", &maxproc, &len, NULL, 0) == -1) {
	    perror("sysctlbyname");
	} else {
	    ncpu = maxproc;
	}
    }
#  endif


#  if defined(__APPLE__)
    if (ncpu < 0) {
	size_t len;
	int maxproc;
	int mib[] = {CTL_HW, HW_AVAILCPU};

	len = sizeof(maxproc);
	if (sysctl(mib, 2, &maxproc, &len, NULL, 0) == -1) {
	    perror("sysctl");
	} else {
	    ncpu = maxproc; /* should be able to get sysctl to return maxproc */
	}
    }
#  endif /* __ppc__ */


#  if defined(HAVE_GET_NPROCS)
    if (ncpu < 0) {
	ncpu = get_nprocs(); /* GNU extension from sys/sysinfo.h */
    }
#  endif


#  if defined(_SC_NPROCESSORS_ONLN)
    /* SUNOS and linux (and now Mac 10.6+) */
    if (ncpu < 0) {
	ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	if (ncpu < 0) {
	    perror("Unable to get the number of available CPUs");
	}
    }
#endif


#if defined(_SC_NPROC_ONLN)
    if (ncpu < 0) {
	ncpu = sysconf(_SC_NPROC_ONLN);
	if (ncpu < 0) {
	    perror("Unable to get the number of available CPUs");
	}
    }
#endif


#  if defined(linux)
    if (ncpu < 0) {
	/* old linux method */
	/*
	 * Ultra-kludgey way to determine the number of cpus in a
	 * linux box--count the number of processor entries in
	 * /proc/cpuinfo!
	 */

#    define CPUINFO_FILE "/proc/cpuinfo"
	FILE *fp;
	char buf[128];

	fp = fopen (CPUINFO_FILE, "r");

	if (fp == NULL) {
	    perror (CPUINFO_FILE);
	} else {
	    ncpu = 0;
	    while (bu_fgets(buf, 80, fp) != NULL) {
		if (bu_strncmp (buf, "processor", 9) == 0) {
		    ncpu++;
		}
	    }
	    fclose (fp);
	}
    }
#  endif


#  if defined(_WIN32)
    /* Windows */
    if (ncpu < 0) {
	SYSTEM_INFO sysinfo;

	GetSystemInfo(&sysinfo);
	ncpu = (int)sysinfo.dwNumberOfProcessors;
    }
#  endif


#endif /* PARALLEL */

    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL)) {
	/* do not use bu_log() here, this can get called before semaphores are initialized */
	fprintf(stderr, "bu_avail_cpus: counted %d cpus.\n", ncpu);
    }

    if (LIKELY(ncpu > 0)) {
	return ncpu;
    }

    /* non-PARALLEL */
    return 1;
}


/**********************************************************************/


#ifdef PARALLEL

HIDDEN void
parallel_interface_arg(struct thread_data *user_thread_data)
{
    /* keep track of our parallel ID number */
    thread_set_cpu(user_thread_data->cpu_id);

    if (user_thread_data->affinity) {
	int ret;
	/* lock us onto a core corresponding to our parallel ID number */
	ret = parallel_set_affinity(user_thread_data->cpu_id);
	if (ret) {
	    bu_log("WARNING: encountered unexpected problem setting CPU affinity\n");
	}
    }

    (*(user_thread_data->user_func))(user_thread_data->cpu_id, user_thread_data->user_arg);
}


#if defined(_WIN32)
/**
 * A separate stub to call parallel_interface_arg that avoids a
 *  potential crash* on 64-bit windows and calls ExitThread to
 *  cleanly stop the thread.
 *  *See ThreadProc MSDN documentation.
 */
HIDDEN DWORD
parallel_interface_arg_stub(struct thread_data *user_thread_data)
{
    parallel_interface_arg(user_thread_data);
    ExitThread(0);
    return 0; /* Extraneous */
}
#endif

#endif /* PARALLEL */


struct parallel_info {
    int id; /* cpu+1 */
    int parent;
    size_t lim;
    size_t started;
    size_t finished;
};


typedef enum {
    PARALLEL_GET = 0,
    PARALLEL_PUT = 1
} parallel_action_t;


static struct parallel_info *
parallel_mapping(parallel_action_t action, int id, size_t max)
{
    /* container for keeping track of recursive invocation data, limits, current values */
    static struct parallel_info mapping[MAX_PSW] = {{0,0,0,0,0}};
    size_t got_cpu;

    switch (action) {
	case PARALLEL_GET:
	    if (id < 0) {
		bu_semaphore_acquire(BU_SEM_THREAD);
		for (got_cpu = 0; got_cpu < MAX_PSW; got_cpu++) {
		    if (mapping[got_cpu].id == 0) {
			mapping[got_cpu].id = got_cpu+1;
			break;
		    }
		}
		bu_semaphore_release(BU_SEM_THREAD);

		mapping[got_cpu].parent = bu_parallel_id();
		mapping[got_cpu].lim = max;
		mapping[got_cpu].finished = mapping[got_cpu].started = 0;
	    } else {
		got_cpu = id;
	    }

	    return &mapping[got_cpu];

	case PARALLEL_PUT:
	    mapping[id].started = mapping[id].finished = mapping[id].lim = mapping[id].parent = 0;
	    mapping[id].id = 0; /* separate to avoid race */
    }

    return NULL;
}


void
bu_parallel(void (*func)(int, void *), int ncpu, void *arg)
{
#ifndef PARALLEL

    if (!func)
	return; /* nothing to do */

    bu_log("bu_parallel(%d., %p):  Not compiled for PARALLEL machine, running single-threaded\n", ncpu, arg);
    /* do the work anyways */
    (*func)(0, arg);

#else

    struct thread_data *user_thread_data_bu;
    rt_thread_t thread_tbl[MAX_PSW];
    int avail_cpus = 1;
    int x;
    int i;

    /* number of threads created/ended */
    int nthreadc;
    int nthreade;

    char *libbu_affinity = NULL;

    /* OFF by default as modern schedulers are smarter than this. */
    int affinity = 0;

    /* ncpu == 0 means throttle our thread creation as slots become available */
    int throttle = 0;

    struct parallel_info *immediate_parent;

    /*
     * multithreading support for SunOS 5.X / Solaris 2.x
     */
#  if defined(SUNOS) && SUNOS >= 52
    static int concurrency = 0; /* Max concurrency we have set */
#  endif
#  if (defined(SUNOS) && SUNOS >= 52) || defined(HAVE_PTHREAD_H)
    rt_thread_t thread;
#  endif /* SUNOS */

    if (!func)
	return; /* nothing to do */

    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	bu_log("bu_parallel(%d, %p)\n", ncpu, arg);

    if (ncpu > MAX_PSW) {
	bu_log("WARNING: bu_parallel() ncpu(%d) > MAX_PSW(%d), adjusting ncpu\n", ncpu, MAX_PSW);
	ncpu = MAX_PSW;
    }

    libbu_affinity = getenv("LIBBU_AFFINITY");
    if (libbu_affinity)
	affinity = (int)strtol(libbu_affinity, NULL, 0x10);
    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL)) {
	if (affinity)
	    bu_log("CPU affinity enabled. (LIBBU_AFFINITY=%d)\n", affinity);
	else
	    bu_log("CPU affinity disabled.\n", affinity);
    }

    /* if we're in debug mode, allow additional cpus */
    if (!(bu_debug & BU_DEBUG_PARALLEL)) {
	/* otherwise, limit outselves to what is actually available */
	avail_cpus = bu_avail_cpus();
	if (ncpu > avail_cpus) {
	    bu_log("%d cpus requested, but only %d available\n", ncpu, avail_cpus);
	    ncpu = avail_cpus;
	}
    }

    immediate_parent = parallel_mapping(PARALLEL_GET, bu_parallel_id(), ncpu);

    if (ncpu < 1) {
	/* we are still our parent context, what is our threading
	 * parallelization limit?
	 */
	struct parallel_info *parent;

	throttle = 1;
	parent = immediate_parent;

	while (parent->lim == 0 && parent->parent > 0) {
	    parent = parallel_mapping(PARALLEL_GET, parent->parent, ncpu);
	}

	/* if the top-most parent is unspecified, use all available cpus */
	if (parent->lim == 0)
	    ncpu = bu_avail_cpus();
	else
	    ncpu = parent->lim;
    }

    user_thread_data_bu = (struct thread_data *)bu_calloc(ncpu, sizeof(*user_thread_data_bu), "struct thread_data *user_thread_data_bu");

    /* Fill in the data of user_thread_data_bu structures of all threads */
    for (x = 0; x < ncpu; x++) {
	struct parallel_info *next = parallel_mapping(PARALLEL_GET, -1, ncpu);

	user_thread_data_bu[x].user_func = func;
	user_thread_data_bu[x].user_arg  = arg;
	user_thread_data_bu[x].cpu_id    = next->id-1;
	user_thread_data_bu[x].affinity  = affinity;
    }

    /*
     * multithreading support for SunOS 5.X / Solaris 2.x
     */
#  if defined(SUNOS) && SUNOS >= 52

    thread = 0;
    nthreadc = 0;

    /* Give the thread system a hint... */
    if (ncpu > concurrency) {
	if (thr_setconcurrency(ncpu)) {
	    bu_log("ERROR parallel.c/bu_parallel(): thr_setconcurrency(%d) failed\n",
		   ncpu);
	    /* Not much to do, lump it */
	} else {
	    concurrency = ncpu;
	}
    }

    /* Create the threads */
    for (x = 0; x < ncpu; x++) {
	if (thr_create(0, 0, (void *(*)(void *))parallel_interface_arg, &user_thread_data_bu[x], 0, &thread)) {
	    bu_log("ERROR: bu_parallel: thr_create(0x0, 0x0, 0x%x, 0x0, 0, 0x%x) failed for processor thread # %d\n",
		   parallel_interface_arg, &thread, x);
	    /* Not much to do, lump it */
	} else {
	    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
		bu_log("bu_parallel(): created thread: (thread: 0x%x) (loop:%d) (nthreadc:%d)\n",
		       thread, x, nthreadc);

	    thread_tbl[nthreadc] = thread;
	    nthreadc++;
	}
    }

    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	for (i = 0; i < nthreadc; i++)
	    bu_log("bu_parallel(): thread_tbl[%d] = 0x%x\n", i, thread_tbl[i]);

    /*
     * Wait for completion of all threads.  We don't wait for threads
     * in order.  We wait for any old thread but we keep track of how
     * many have returned and whether it is one that we started
     */
    nthreade = 0;
    for (x = 0; x < nthreadc; x++) {
	if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	    bu_log("bu_parallel(): waiting for thread to complete:\t(loop:%d) (nthreadc:%d) (nthreade:%d)\n",
		   x, nthreadc, nthreade);

	if (thr_join((rt_thread_t)0, &thread, NULL)) {
	    /* badness happened */
	    perror("thr_join");
	    bu_log("thr_join() failed");
	}

	/* Check to see if this is one the threads we created */
	for (i = 0; i < nthreadc; i++) {
	    if (thread_tbl[i] == thread) {
		thread_tbl[i] = (rt_thread_t)-1;
		nthreade++;
		break;
	    }
	}

	if ((thread_tbl[i] != (rt_thread_t)-1) && i < nthreadc) {
	    bu_log("bu_parallel(): unknown thread %d completed.\n",
		   thread);
	}

	if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	    bu_log("bu_parallel(): thread completed: (thread: %d)\t(loop:%d) (nthreadc:%d) (nthreade:%d)\n",
		   thread, x, nthreadc, nthreade);
    }

    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	bu_log("bu_parallel(): %d threads created.  %d threads exited.\n",
	       nthreadc, nthreade);
#  endif	/* SUNOS */

#  if defined(HAVE_PTHREAD_H)

    nthreadc = 0;

    /* Create the posix threads.
     *
     * Start at 1 so we can treat the parent as thread 0.
     */
    for (x = 0; x < ncpu; x++) {
	pthread_attr_t attrs;
	pthread_attr_init(&attrs);
	pthread_attr_setstacksize(&attrs, 10*1024*1024);

	if (pthread_create(&thread, &attrs, (void *(*)(void *))parallel_interface_arg, &user_thread_data_bu[x])) {
	    bu_log("ERROR: bu_parallel: pthread_create(0x0, 0x0, 0x%lx, 0x0, 0, %p) failed for processor thread # %d\n",
		   (unsigned long int)parallel_interface_arg, (void *)&thread, x);
	    /* Not much to do, lump it */
	} else {
	    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL)) {
		bu_log("bu_parallel(): created thread: (thread: %p) (loop: %d) (nthreadc: %d)\n",
		       (void*)thread, x, nthreadc);
	    }

	    thread_tbl[nthreadc] = thread;
	    nthreadc++;
	    immediate_parent->started++;
	}
	/* done with the attributes after create */
	pthread_attr_destroy(&attrs);
    }


    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL)) {
	for (i = 0; i < nthreadc; i++) {
	    bu_log("bu_parallel(): thread_tbl[%d] = %p\n", i, (void *)thread_tbl[i]);
	}
#    ifdef SIGINFO
	/* may be BSD-only (calls _thread_dump_info()) */
	raise(SIGINFO);
#    endif
    }

    /*
     * Wait for completion of all threads.
     * Wait for them in order.
     */
    nthreade = 0;
    for (x = 0; x < nthreadc; x++) {
	int ret;

	if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	    bu_log("bu_parallel(): waiting for thread %p to complete:\t(loop:%d) (nthreadc:%d) (nthreade:%d)\n",
		   (void *)thread_tbl[x], x, nthreadc, nthreade);

	if ((ret = pthread_join(thread_tbl[x], NULL)) != 0) {
	    /* badness happened */
	    bu_log("pthread_join(thread_tbl[%d]=%p) ret=%d\n", x, (void *)thread_tbl[x], ret);
	}

	nthreade++;
	thread = thread_tbl[x];
	thread_tbl[x] = (rt_thread_t)-1;
	immediate_parent->finished++;
	parallel_mapping(PARALLEL_PUT, user_thread_data_bu[x].cpu_id, ncpu);

	if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	    bu_log("bu_parallel(): thread completed: (thread: %p)\t(loop:%d) (nthreadc:%d) (nthreade:%d)\n",
		   (void *)thread, x, nthreadc, nthreade);

    }

    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	bu_log("bu_parallel(): %d threads created.  %d threads exited.\n", nthreadc, nthreade);

#  endif /* end if posix threads */


#  ifdef WIN32
    /* Create the Win32 threads */
    nthreadc = 0;
    for (i = 0; i < ncpu; i++) {
	thread = CreateThread(
	    NULL,
	    0,
	    (LPTHREAD_START_ROUTINE)parallel_interface_arg_stub,
	    &user_thread_data_bu[i],
	    0,
	    NULL);

	nthreadc++;
	thread_tbl[i] = thread;

	/* Ensure that all successfully created threads are in sequential order.*/
	if (thread_tbl[i] == NULL) {
	    bu_log("bu_parallel(): Error in CreateThread, Win32 error code %d.\n", GetLastError());
	    --nthreadc;
	}
    }


    {
	/* Wait for other threads in the array */
	DWORD returnCode;
	returnCode = WaitForMultipleObjects(nthreadc, thread_tbl, TRUE, INFINITE);
	if (returnCode == WAIT_FAILED) {
	    bu_log("bu_parallel(): Error in WaitForMultipleObjects, Win32 error code %d.\n", GetLastError());
	}
    }

    nthreade = 0;
    for (x = 0; x < nthreadc; x++) {
	int ret;
	if ((ret = CloseHandle(thread_tbl[x]) == 0)) {
	    /* Thread didn't close properly if return value is zero; don't retry and potentially loop forever.  */
	    bu_log("bu_parallel(): Error closing thread %d of %d, Win32 error code %d.\n", x, nthreadc, GetLastError());
	}

	nthreade++;
	thread_tbl[x] = (rt_thread_t)-1;
    }
#  endif /* end if Win32 threads */

    /*
     * TODO: Ensure that all the threads are REALLY finished.  On some
     * systems, if threads core dump, the rest of the gang keeps
     * going, so this can actually happen.
     */

    if (UNLIKELY(bu_debug & BU_DEBUG_PARALLEL))
	bu_log("bu_parallel(%d) complete\n", ncpu);

    bu_free(user_thread_data_bu, "struct thread_data *user_thread_data_bu");

#endif /* PARALLEL */

    return;
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
