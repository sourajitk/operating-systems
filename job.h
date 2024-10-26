#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include <stdlib.h>

// Job information
// DO NOT DIRECTLY USE THESE FIELDS
// USE THE FUNCTIONS BELOW INSTEAD
typedef struct {
    uint64_t arrivalTime; // arrival time
    uint64_t jobTime; // job time
    uint64_t remainingTime; // remaining job time
    uint64_t id; // job id
} job_t;

// Create a new job
static inline job_t* jobCreate(uint64_t arrivalTime, uint64_t jobTime, uint64_t id)
{
    job_t* job = malloc(sizeof(job_t));
    if (job) {
        job->arrivalTime = arrivalTime;
        job->jobTime = jobTime;
        job->remainingTime = jobTime;
        job->id = id;
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
    return job->arrivalTime;
}
// Get job time
static inline uint64_t jobGetJobTime(job_t* job)
{
    return job->jobTime;
}
// Get remaining time
static inline uint64_t jobGetRemainingTime(job_t* job)
{
    return job->remainingTime;
}
// Set remaining time
static inline void jobSetRemainingTime(job_t* job, uint64_t remainingTime)
{
    job->remainingTime = remainingTime;
}
// Get completed time
static inline uint64_t jobGetCompletedTime(job_t* job)
{
    return job->jobTime - job->remainingTime;
}
// Get job id
static inline uint64_t jobGetId(job_t* job)
{
    return job->id;
}

#endif /* JOB_H */
