#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// SJF scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* current_queue;    // Current current_queue to hold jobs in SJF order
    job_t* job;               // Pointer to the current job being scheduled
} scheduler_SJF_t;

/*
 * We basically use this funciton to make sure that when
 * we create our list using schedulerSJFCreate(), we are
 * making sure that we are choosing the right job, making
 * it easier to maintain or modify the comparison rules 
 * without changing the list implementation.
 */ 
int job_priority(void* job_a, void* job_b) {
    // Cast the first job data to a job_t structure
    job_t* task_a = (job_t*)job_a;

    // Cast the second job data to a job_t structure
    job_t* task_b = (job_t*)job_b;

    // Retrieve the execution time of the first and second job
    uint64_t task_time_a = jobGetJobTime(task_a);
    uint64_t task_time_b = jobGetJobTime(task_b);

    // Compare the execution times of the two jobs
    // If the first job's time is greater, it has lower priority
    if (task_time_a > task_time_b) {
        return 1;
    }
    // If the first job's time is less, it has higher priority
    else if (task_time_a < task_time_b) {
        return -1;
    }

    // If execution times are equal, compare the job IDs as a tiebreaker
    // If the first job's ID is greater, it has lower priority
    if (jobGetId(task_a) > jobGetId(task_b)) {
        return 1;
    }
    // Otherwise, the second job has lower priority or they are equivalent
    else {
        return -1;
    }
}

// Creates and returns scheduler specific info
void* schedulerSJFCreate()
{
    scheduler_SJF_t* info = malloc(sizeof(scheduler_SJF_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    // Initialize the job current_queue with a priority comparison function
    info->current_queue = list_create(job_priority);
    if (info->current_queue == NULL) {
        free(info); // Clean up if current_queue creation fails
        return NULL; // Return NULL to indicate failure
    }
    // Set the current job pointer to NULL to mark no active job initially
    info->job = NULL;
    return info;
}

// Destroys scheduler specific info
void schedulerSJFDestroy(void* schedulerInfo)
{
    scheduler_SJF_t* info = (scheduler_SJF_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    // Destroy the job queue if it exists
    if (info->current_queue) {
        list_destroy(info->current_queue);
        // Set to NULL after destroying
        info->current_queue = NULL;
    }
    // Free the scheduler info struct itself
    free(info);
}

// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
void schedulerSJFScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_SJF_t* info = (scheduler_SJF_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    if (info->job == NULL) {
        // If no current job, assign the new job and schedule its completion
        info->job = job;

        // Calculate completion time for the job and schedule it
        uint64_t job_completion_time = jobGetJobTime(job) + currentTime;
        schedulerScheduleNextCompletion(scheduler, job_completion_time);
    } else {
        // Insert the job into the current_queue if a job is already in progress
        list_insert(info->current_queue, job);
    }
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerSJFCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_SJF_t* info = (scheduler_SJF_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    return NULL;
}
