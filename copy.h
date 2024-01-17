#ifndef COPY_H
#define COPY_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int job_id;
    int status;    // -1 - not started, 0 - in progress, 1 - completed, 2 - canceled, 3 - paused,
    int progress;  // Progresul în bytes
} copyjob_stats;

typedef struct {
    int job_id;
    char src[256];
    char dst[256];
    int progress;
    int status;  // -1 - not started, 0 - in progress, 1 - completed, 2 - canceled, 3 - paused
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} copyjob_t;

// Declarații
void* copy_thread(void* arg);
copyjob_t copy_createjob(const char *src, const char *dst);
int copy_cancel(copyjob_t *job);
int copy_pause(copyjob_t *job);
int copy_progress(copyjob_t job);
int copy_stats(copyjob_t job, copyjob_stats *stats);



#endif
