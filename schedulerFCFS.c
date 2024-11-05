#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// FCFS scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    list_t* current_queue;    // Current current_queue to hold jobs in FCFS order
    job_t* job;               // Pointer to the current job being scheduled
} scheduler_FCFS_t;

// Creates and returns scheduler specific info
void* schedulerFCFSCreate()
{
    scheduler_FCFS_t* info = malloc(sizeof(scheduler_FCFS_t));
    if (info == NULL) {
        return NULL;
    }
    /* IMPLEMENT THIS */
    // Initialize all fields to 0/NULL
    memset(info, 0, sizeof(scheduler_FCFS_t));

    // Set up the job queue
    info->current_queue = list_create(NULL);

    // If queue creation failed, free info and return NULL
    if (!info->current_queue) {
        free(info);
        return NULL;
    }

    // Return the fully initialized structure
    return info;
}

// Destroys scheduler specific info
void schedulerFCFSDestroy(void* schedulerInfo)
{
    scheduler_FCFS_t* info = (scheduler_FCFS_t*)schedulerInfo;
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
void schedulerFCFSScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_FCFS_t* info = (scheduler_FCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    // Check if an active job is currently being processed
    if (info->job) {
        // If an active job exists, enqueue the new job for processing
        list_insert(info->current_queue, job);
    } else {
        // No job is currently being processed, so set the new job as the active job
        info->job = job;

        // Add the new active job to the processing queue
        list_insert(info->current_queue, info->job);

        // Calculate the job's completion time
        uint64_t jobCompletionTime = currentTime + jobGetJobTime(info->job);

        // Schedule the completion of the active job
        schedulerScheduleNextCompletion(scheduler, jobCompletionTime);
    }
}

// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerFCFSCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_FCFS_t* info = (scheduler_FCFS_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    return NULL;
}
