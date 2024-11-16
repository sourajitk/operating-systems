#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// MLFQ scheduler info
typedef struct {
    list_t* current_queue;  // Single queue for job management
    job_t* job;             // Tracks the last update time
} scheduler_MLFQ_t;

// Helper function to compare two integers and store the result
int comparison_result(uint64_t value_1, uint64_t value_2)
{
    if (value_1 < value_2) {
        return -1;
    } else if (value_1 > value_2) {
        return 1;
    } else {
        return 0;
    }
    // Use the helper function to store and return the comparison result
    return comparison_result(value_1, value_2);
}

// Helper function to calculate completed work for a job
uint64_t calculate_completed_work(job_t* task) {
    return jobGetJobTime(task) - jobGetRemainingTime(task);
}

/*
 * Compare two jobs based on the amount of work completed and, 
 * if needed, their job IDs. This determines the priority of
 * jobs within the MLFQ queue. The comparison ensures that jobs
 * with less completed work are prioritized using round-robin logic
 * as a secondary check for fairness.
 */

int task_completion_queue(void* job_a, void* job_b)
{
    // Create pointers for job-specific fields and functions for comparison.
    job_t* task_1 = (job_t*)job_a;
    job_t* task_2 = (job_t*)job_b;

    // Ensure tasks are not null (already assumed in the main logic)
    if (task_1 == NULL || task_2 == NULL) {
        // Should never happen
        return 0;
    }

    // Compare the amount of completed work for two tasks
    if (calculate_completed_work(task_1) < calculate_completed_work(task_2)) {
        return -1;
    } else if (calculate_completed_work(task_1) > calculate_completed_work(task_2)) {
        return 1;
    }

    // Compare the IDs of the two tasks to determine their order
    if (jobGetId(task_1) < jobGetId(task_2)) {
        return -1;
    } else if (jobGetId(task_1) > jobGetId(task_2)) {
        return 1;
    }

    return 0;
}

// Creates and returns scheduler specific info
void* schedulerMLFQCreate()
{
    scheduler_MLFQ_t* info = malloc(sizeof(scheduler_MLFQ_t));
    if (info == NULL) {
        return NULL;
    }

    // Initialize the job queue
    info->current_queue = list_create(NULL);
    if (info->current_queue == NULL) {
        free(info); // Clean up if queue creation fails
        return NULL; // Return NULL to indicate failure
    }

    // Set the current job pointer to NULL to mark no active job initially
    info->job = NULL;
    return info;
}

// Destroys scheduler specific info
void schedulerMLFQDestroy(void* schedulerInfo)
{
    scheduler_MLFQ_t* info = (scheduler_MLFQ_t*)schedulerInfo;
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
void schedulerMLFQScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_MLFQ_t* info = (scheduler_MLFQ_t*)schedulerInfo;

    // Add new jobs to the queue
    list_insert(info->current_queue, job);

    // Cancel any existing completion if necessary and reschedule
    schedulerCancelNextCompletion(scheduler);

    // Determine the next job to complete
    if (list_count(info->current_queue) > 0) {
        job_t* nextJob = list_data(list_head(info->current_queue));
        schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(nextJob));
    }
}


// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
job_t* schedulerMLFQCompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime)
{
    scheduler_MLFQ_t* info = (scheduler_MLFQ_t*)schedulerInfo;

    if (list_count(info->current_queue) > 0) {
        // Dequeue the next job from the queue
        list_node_t* node = list_tail(info->current_queue);
        job_t* completedJob = list_data(node);
        list_remove(info->current_queue, node);

        // Check if there are jobs remaining to reschedule
        schedulerCancelNextCompletion(scheduler);

        if (list_count(info->current_queue) > 0) {
            job_t* nextJob = list_data(list_head(info->current_queue));
            schedulerScheduleNextCompletion(scheduler, currentTime + jobGetRemainingTime(nextJob));
        }

        return completedJob;
    }

    return NULL; // No jobs to complete
}
