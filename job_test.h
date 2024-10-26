// Test for ensuring job fields are not directly used

#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include <stdlib.h>

// Job information
// DO NOT DIRECTLY USE THESE FIELDS
// USE THE FUNCTIONS BELOW INSTEAD
typedef struct {
    uint64_t arrivalTime_do_not_use; // arrival time
    uint64_t jobTime_do_not_use; // job time
    uint64_t remainingTime_do_not_use; // remaining job time
    uint64_t id_do_not_use; // job id
} job_t;

// Create a new job
static inline job_t* jobCreate(uint64_t arrivalTime, uint64_t jobTime, uint64_t id)
{
    job_t* job = malloc(sizeof(job_t));
    if (job) {
        job->arrivalTime_do_not_use = arrivalTime;
        job->jobTime_do_not_use = jobTime;
        job->remainingTime_do_not_use = jobTime;
        job->id_do_not_use = id;
    }
    return job;
}
// Destroy a job
static inline void jobDestroy(job_t* job)
{
    free(job);
}
// Get arrival time
static inline uint64_t jobGetArrivalTime(job_t* job)
{
    return job->arrivalTime_do_not_use;
}
// Get job time
static inline uint64_t jobGetJobTime(job_t* job)
{
    return job->jobTime_do_not_use;
}
// Get remaining time
static inline uint64_t jobGetRemainingTime(job_t* job)
{
    return job->remainingTime_do_not_use;
}
// Set remaining time
static inline void jobSetRemainingTime(job_t* job, uint64_t remainingTime)
{
    job->remainingTime_do_not_use = remainingTime;
}
// Get completed time
static inline uint64_t jobGetCompletedTime(job_t* job)
{
    return job->jobTime_do_not_use - job->remainingTime_do_not_use;
}
// Get job id
static inline uint64_t jobGetId(job_t* job)
{
    return job->id_do_not_use;
}

#endif /* JOB_H */
