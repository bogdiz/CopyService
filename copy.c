#include "copy.h"

int main(int argc, char *argv[]) {
    // if (argc < 3) {
    //     fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }
    // copyjob_t job = copy_createjob(argv[1], argv[2]);
    // pthread_t copy_thread_id;
    // if (pthread_create(&copy_thread_id, NULL, copy_thread, &job) != 0) {
    //     fprintf(stderr, "Error creating copy thread\n");
    //     exit(EXIT_FAILURE);
    // }
    
    clock_t start_time, end_time;
    start_time = clock();

    struct pollfd mypoll = {STDIN_FILENO, POLLIN|POLLPRI };

    FILE *file = fopen("config", "r");
    if (file == NULL) {
        perror("Eroare la deschiderea fisierului");
        return 1;
    }
    int nr_jobs, nr_threads;
    if (fscanf(file, "%d %d", &nr_jobs, &nr_threads) != 2) {
        fprintf(stderr, "Eroare la citirea din fisier\n");
        fclose(file);
        return 1;
    }
    fclose(file); 

  

    copyjob_t jobs[nr_jobs];
    pthread_t threads[nr_threads];

    char src[] = "five";
    for (int i = 0; i < nr_jobs; ++i){
        char dst[256];
        sprintf(dst, "dest_%d", i);
        jobs[i] = copy_createjob(src, dst);
    }


    int i = 0, k = nr_threads;
    int m = -1;
    while (i < nr_jobs){
        int p = 0;
        if (k){
            ++m;
            for (int j = 0; i < nr_jobs && j < nr_threads; ++j){
                pthread_create(&threads[j], NULL, copy_thread, &jobs[i]);
                jobs[i].status = 0;
                --k;
                ++p;
                ++i;
            }
        }
        int isCancelled = 0;

        printf("Press 'c' and Enter to cancel the copy job, 'p' to pause/unpause, 's' for stats: \n");
        fflush(stdout);
        char input;

        if (poll(&mypoll, 1, 5000)){
            scanf(" %c", &input);
        } else {
            input = 'n';
        }
        switch (input) {
            case 'c':
                isCancelled = 1;
                for (int t = m * nr_threads; t < i; ++t){
                    copy_cancel(&jobs[t]);
                }
                break;
            case 'p':
                for (int t = m * nr_threads; t < i; ++t)
                    copy_pause(&jobs[t]);

                scanf(" %c", &input);
                if (input == 'p'){
                    printf("Unpaused!\n");
                    for (int t = m * nr_threads; t < i; ++t)
                        copy_pause(&jobs[t]);                    
                    }

                break; 
            case 's':
                copyjob_stats stats;
                for (int t = 0; t < nr_jobs; ++t){
                    if (copy_stats(jobs[t], &stats)) {
                        printf("Job %d status: %d, progress: %d bytes\n", stats.job_id, stats.status, stats.progress);
                    } else {
                        printf("Failed to get job %d stats\n", jobs[t].job_id);
                    }
                }
            
            break;
            case 'n':
                    printf("Continuing...\n");
                    break;
            default:
                    printf("Invalid input. Try again.\n");
        }

        //

        if (!isCancelled)
            for (int j = 0; j < p; ++j) {
                pthread_join(threads[j], NULL);
                ++k;
            }
        else {
            k = nr_threads;
        }    
    }

    end_time = clock();
    double cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
    printf("\nTime taken by program: %f seconds\n", cpu_time_used);
   


    return 0;
}
int copy_pause(copyjob_t *job) {
    if (job->status == 0) {
     pthread_mutex_lock(&job->mutex);
        job->status = 3;  
        printf("Job %d paused\n", job->job_id);
        return 1;  
    } else if(job->status == 3){
        job->status = 0;
        pthread_mutex_unlock(&job->mutex);
    return 1;
    } else {
        printf("Job %d cannot be paused as it is not in progress or already completed\n", job->job_id);
        return 0;
    }
}


void* copy_thread(void* arg) {
    copyjob_t* job = (copyjob_t*)arg;
    FILE* source = fopen(job->src, "rb");
    FILE* dest = fopen(job->dst, "wb");

    if (source == NULL || dest == NULL) {
        perror("Error opening files");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        pthread_mutex_lock(&job->mutex);
        if (job->status == 2) {
            pthread_mutex_unlock(&job->mutex);
            break; 
        }
        
        pthread_mutex_unlock(&job->mutex);

        fwrite(buffer, 1, bytesRead, dest);
        job->progress += bytesRead;
    }

    fclose(source);
    fclose(dest);

    pthread_mutex_lock(&job->mutex);
    if (job->status == 0) {
        job->status = 1; 
        // printf("Job %d completed\n", job->job_id);
        printf("Thread %lu processed Job %d\n", pthread_self(), job->job_id);
    }
    pthread_cond_signal(&job->cond);
    pthread_mutex_unlock(&job->mutex);

    return NULL;
}


copyjob_t copy_createjob(const char *src, const char *dst) {
    copyjob_t job;
    static int next_job_id = 1;
    job.job_id = next_job_id++;
    snprintf(job.src, sizeof(job.src), "%s", src);
    snprintf(job.dst, sizeof(job.dst), "%s", dst);
    job.progress = 0;
    job.status = -1; 
    pthread_mutex_init(&job.mutex, NULL);
    pthread_cond_init(&job.cond, NULL);
    return job;
}

int copy_cancel(copyjob_t *job) {
    if (job->status == 0 || job->status==3) {
        job->status = 2;  // Canceled
        printf("Job %d canceled\n", job->job_id);
        pthread_cancel(job);        

        return 1;
    } else {
        printf("Job %d cannot be canceled as it is not in progress or already completed\n", job->job_id);
        return 0; 
    }
}
int copy_stats(copyjob_t job, copyjob_stats *stats) {
    pthread_mutex_lock(&job.mutex);
    stats->job_id = job.job_id;
    stats->status = job.status;
    stats->progress = job.progress;
    pthread_mutex_unlock(&job.mutex);
    return 1;  
}

int copy_progress(copyjob_t job) {
    pthread_mutex_lock(&job.mutex);
    int progress = job.progress;
    pthread_mutex_unlock(&job.mutex);
    return progress;
}

