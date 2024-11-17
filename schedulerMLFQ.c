#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"
#include "job.h"
#include "linked_list.h"

// MLFQ scheduler info
typedef struct {
    /* IMPLEMENT THIS */
    uint64_t last_update_timestamp;   // Last time since the job was updated
    uint64_t mlfq_priority_level;     // MLFQ Priority level of the current job 
    list_t* current_queue;            // Current current_queue to hold jobs in MLFQ order
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

// Create a helper function to 0-out the scheduler info fields when needed
// [TODO check where else we might need to use it]
void initializeSchedulerInfoFields(scheduler_MLFQ_t* info) 
{
    info->last_update_timestamp = 0;
    info->mlfq_priority_level = 0;
}

// Creates and returns scheduler specific info
void* schedulerMLFQCreate()
{
    scheduler_MLFQ_t* info = malloc(sizeof(scheduler_MLFQ_t));
    if (info == NULL) {
        return NULL;
    }

    // Initialize the job queue
    info->current_queue = list_create(task_completion_queue);
    if (info->current_queue == NULL) {
        free(info); // Clean up if queue creation fails
        return NULL; // Return NULL to indicate failure
    }
    
    // Initialize list fields by zeroing them out
    initializeSchedulerInfoFields(info);
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

/*
 * So basically, we are trying to process a single unit of work for
 * the next job in the queue. This function handles the execution of
 * one time unit for the job at the head of the queue. If the job is
 * completed after processing, it is removed and returned. Otherwise,
 * the job is updated and reinserted into the queue for further processing.
 */
job_t* process_next_job(list_t* queue, scheduler_t* scheduler, uint64_t current_time)
{
    // Check if the queue is empty; if no jobs are available, return NULL.
    if (list_count(queue) == 0) {
        return NULL;
    }

    // Retrieve the first job (head of the queue) for processing.
    list_node_t* node = list_head(queue);
    job_t* job = list_data(node);
    uint64_t remaining_time = jobGetRemainingTime(job);

    // Check if the job can be completed with one unit of work.
    if (remaining_time <= 1) {
        // Job is complete
        list_remove(queue, node);
        return job;
    } else {
        // Process one unit of work
        jobSetRemainingTime(job, remaining_time - 1);
        list_remove(queue, node);
        list_insert(queue, job);
    }

    // Reschedule the next completion
    if (list_count(queue) > 0) {
        schedulerScheduleNextCompletion(scheduler, current_time + 1);
    }

    return NULL;
}

// Helper function to determine the minimum completion level and count jobs at that level
uint64_t get_jobs_at_min_level(list_t* queue, uint64_t* out_min_completion_level) {
    // Check if the queue is empty
    if (list_head(queue) == NULL) {
        *out_min_completion_level = 0;
        return 0;
    }

    // Identify the minimum completion level of jobs in the queue
    list_node_t* node = list_head(queue);
    uint64_t min_completion_level = jobGetJobTime(list_data(node)) - jobGetRemainingTime(list_data(node));

    // Count the number of jobs at the minimum completion level
    uint64_t jobs_at_min_level = 0;
    for (list_node_t* node = list_head(queue); node != NULL; node = list_next(node)) {
        job_t* current_job = list_data(node);
        uint64_t job_completion_level = jobGetJobTime(current_job) - jobGetRemainingTime(current_job);
        if (job_completion_level > min_completion_level) {
            break;
        }
        jobs_at_min_level = jobs_at_min_level + 1;
    }

    *out_min_completion_level = min_completion_level;
    return jobs_at_min_level;
}

// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
void schedulerMLFQScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime)
{
    scheduler_MLFQ_t* info = (scheduler_MLFQ_t*)schedulerInfo;
    /* IMPLEMENT THIS */
    // Calculate the elapsed time since the last scheduling event
    uint64_t time_used = currentTime - info->last_update_timestamp;

    // Exit early if no elapsed time or the queue is empty
    if (time_used <= 0 || list_count(info->current_queue) == 0) {
        // Add the new job to the queue and schedule the next completion
        if (info->current_queue != NULL && scheduler != NULL) {
            list_insert(info->current_queue, job);
            schedulerScheduleNextCompletion(scheduler, currentTime + 1);
            info->last_update_timestamp = currentTime;
        }
        return;
    }

    // Determine the minimum completion level and the number of jobs at that level
    uint64_t min_completion_level = 0;
    uint64_t jobs_at_min_level = get_jobs_at_min_level(info->current_queue, &min_completion_level);

    // Initialize the remaining time for processing
    uint64_t remaining_time = time_used;

    // Distribute the elapsed time across jobs at the minimum completion level
    for (list_node_t* node = list_head(info->current_queue); remaining_time > 0;) {
        if (node == NULL) {
            // Exit if no more jobs in the queue
            break;
        }

        job_t* current_job = list_data(node);
        uint64_t remaining_job_time = jobGetRemainingTime(current_job);

        // Allocate one unit of work or less if the job requires less time
        uint64_t allocated_work;
        if (remaining_job_time < 1) {
            allocated_work = remaining_job_time;
        }

        // Update the job's remaining time
        jobSetRemainingTime(current_job, remaining_job_time - allocated_work);

        // Remove the job from the queue and reinsert it if not yet complete
        list_remove(info->current_queue, node);
        if (remaining_job_time > allocated_work) {
            list_insert(info->current_queue, current_job);
        }

        // Update remaining time and move to the next node
        remaining_time -= allocated_work;
        node = list_head(info->current_queue);
    }

    // Cancel any previously scheduled completion event
    if (scheduler != NULL) {
        schedulerCancelNextCompletion(scheduler);
        // Check if the job queue is valid
        if (info->current_queue != NULL) {
            // Add the new job to the queue
            list_insert(info->current_queue, job);

            // Schedule the next completion event
            schedulerScheduleNextCompletion(scheduler, currentTime + 1);

            // Update the timestamp for the last scheduling event
            info->last_update_timestamp = currentTime;
        }
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
    /* IMPLEMENT THIS */
    job_t* completed_job = NULL;

    // Check if the scheduler info and current queue are valid
    if (info == NULL || info->current_queue == NULL) {
        // Exit early if the scheduler info or queue is invalid
        return NULL;
    }

    // Retrieve the job at the head of the queue if the queue is not empty
    list_node_t* node = list_head(info->current_queue);
    if (node != NULL) {
        job_t* current_job = list_data(node);

        // Ensure the current job is valid before proceeding
        if (current_job != NULL) {
            uint64_t remaining_time = jobGetRemainingTime(current_job);

            // Check if the job can be completed
            if (remaining_time <= 1) {
                // Remove the job from the queue as it's completed
                list_remove(info->current_queue, node);
                completed_job = current_job;
            } else {
                // Process one unit of work
                jobSetRemainingTime(current_job, remaining_time - 1);

                // Remove and reinsert the job to maintain queue order
                list_remove(info->current_queue, node);
                list_insert(info->current_queue, current_job);
            }

            // Schedule the next completion event only if jobs remain
            if (list_head(info->current_queue) != NULL) {
                schedulerScheduleNextCompletion(scheduler, currentTime + 1);
            }
        }
    }

    // Update the timestamp for the last event
    info->last_update_timestamp = currentTime;
    return completed_job;
}
