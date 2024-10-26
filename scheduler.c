#include <stdio.h>
#include <string.h>
#include "scheduler.h"
#include "simulator.h"
#include "job.h"

// Creates a scheduler
// schedulerName - name of scheduler
// sim - simulator
// completionCallback - function to call upon job completion
// completionCallbackData - data to pass to completionCallback
// Returns scheduler on success or NULL otherwise
scheduler_t* schedulerCreate(const char* schedulerName, simulator_t* sim, completionCallback_fn completionCallback, void* completionCallbackData)
{
    scheduler_t* scheduler = malloc(sizeof(scheduler_t));
    if (scheduler == NULL) {
        return NULL;
    }
    scheduler->sim = sim;
    scheduler->completionCallback = completionCallback;
    scheduler->completionCallbackData = completionCallbackData;
    scheduler->completionEvent = NULL;
    if (strcmp(schedulerName, "FCFS") == 0) {
        INIT_SCHEDULER(scheduler, FCFS);
    } else if (strcmp(schedulerName, "LCFS") == 0) {
        INIT_SCHEDULER(scheduler, LCFS);
    } else if (strcmp(schedulerName, "SJF") == 0) {
        INIT_SCHEDULER(scheduler, SJF);
    } else if (strcmp(schedulerName, "MLFQ") == 0) {
        INIT_SCHEDULER(scheduler, MLFQ);
    } else {
        printf("Invalid scheduler type: %s\n", schedulerName);
        free(scheduler);
        return NULL;
    }
    scheduler->schedulerInfo = scheduler->create();
    if (scheduler->schedulerInfo == NULL) {
        free(scheduler);
        return NULL;
    }
    return scheduler;
}

// Destroys a scheduler
void schedulerDestroy(scheduler_t* scheduler)
{
    if (scheduler->completionEvent) {
        schedulerCancelNextCompletion(scheduler);
    }
    scheduler->destroy(scheduler->schedulerInfo);
    free(scheduler);
}

// Called at a job arrival to schedule the job
void schedulerScheduleJob(scheduler_t* scheduler, job_t* job)
{
    uint64_t currentTime = simulatorSimTime(scheduler->sim);
    scheduler->scheduleJob(scheduler->schedulerInfo, scheduler, job, currentTime);
}

// Called at a job completion
void schedulerCompleteJob(void* s)
{
    scheduler_t* scheduler = (scheduler_t*)s;
    scheduler->completionEvent = NULL;
    uint64_t currentTime = simulatorSimTime(scheduler->sim);
    job_t* job = scheduler->completeJob(scheduler->schedulerInfo, scheduler, currentTime);
    if (job) {
        scheduler->completionCallback(scheduler->completionCallbackData, job);
    }
}

// Schedule next completion at given time
// Returns true on success, false otherwise
bool schedulerScheduleNextCompletion(scheduler_t* scheduler, uint64_t timestamp)
{
    // Check if completion already scheduled
    if (scheduler->completionEvent) {
        return false;
    }
    scheduler->completionEvent = simulatorSchedule(scheduler->sim, timestamp, EVENT_COMPLETION, schedulerCompleteJob, scheduler);
    // Check for failure to schedule
    if (scheduler->completionEvent == NULL) {
        return false;
    }
    return true;
}

// Cancel next completion event
// Returns true on success, false otherwise
bool schedulerCancelNextCompletion(scheduler_t* scheduler)
{
    // Check if there isn't an existing completion to cancel
    if (scheduler->completionEvent == NULL) {
        return false;
    }
    simulatorRemoveEvent(scheduler->sim, scheduler->completionEvent);
    scheduler->completionEvent = NULL;
    return true;
}
