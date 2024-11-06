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
    // Store the job that is being completed
    job_t* completed_job = info->job;

    // Locate the node for the completed job and remove it from the queue
    list_node_t* delete_node = list_find(info->current_queue, completed_job);
    if (delete_node) {
        list_remove(info->current_queue, delete_node);
        // Add the data we have from temp_node to next_node
        list_node_t* temp_node = list_head(info->current_queue);
        while (temp_node != NULL) {
            list_node_t* next_node = temp_node->next;
            if (temp_node->data == completed_job) {
                // Check the nodes before inserting into the list
                list_insert(info->current_queue, temp_node->data);
            }
            temp_node = next_node;
        }
    }

    // Determine if there are remaining jobs in the queue
    if (list_count(info->current_queue) > 0) {
            
        // Iterate and check head node data
        list_node_t* head_node = list_head(info->current_queue);
        if (head_node && head_node->data == completed_job) {
            list_insert(info->current_queue, head_node->data);
        }
        // Assign the last job in the queue as the new active job
        info->job = list_tail(info->current_queue)->data;

        // Calculate and schedule the next job's completion time
        uint64_t jobCompletionTime = currentTime + jobGetJobTime(info->job);
        schedulerScheduleNextCompletion(scheduler, jobCompletionTime);
    } else {
        // If the queue is empty, reset the active job to NULL
        info->job = NULL;
    }

    // Return the completed job
    return completed_job;
}
