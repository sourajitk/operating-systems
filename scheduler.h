#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include "simulator.h"
#include "job.h"
#include "linked_list.h"

typedef struct scheduler scheduler_t;

// Creates and returns scheduler specific info
typedef void* (*scheduler_info_create_fn)();
// Destroys scheduler specific info
typedef void (*scheduler_info_destroy_fn)(void* schedulerInfo);
// Called to schedule a new job in the queue
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// job - new job being added to the queue
// currentTime - the current simulated time
typedef void (*schedule_job_fn)(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime);
// Called to complete a job in response to an earlier call to schedulerScheduleNextCompletion
// schedulerInfo - scheduler specific info from create function
// scheduler - used to call schedulerScheduleNextCompletion and schedulerCancelNextCompletion
// currentTime - the current simulated time
// Returns the job that is being completed
typedef job_t* (*complete_job_fn)(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime);

// Function to call once a job completes
// completionCallbackData - user specified data from when the scheduler was created
// job - job that is being completed
typedef void (*completionCallback_fn)(void* completionCallbackData, job_t* job);

typedef struct scheduler {
    scheduler_info_create_fn create; // scheduler specific create function
    scheduler_info_destroy_fn destroy; // scheduler specific destroy function
    schedule_job_fn scheduleJob; // scheduler specific schedule function
    complete_job_fn completeJob; // scheduler specific complete function
    void* schedulerInfo; // scheduler specific info
    simulator_t* sim; // simulator
    completionCallback_fn completionCallback; // function to call upon job completion
    void* completionCallbackData; // data to pass to callback function
    list_node_t* completionEvent; // completion event reference
} scheduler_t;

// Creates a scheduler
// schedulerName - name of scheduler
// sim - simulator
// completionCallback - function to call upon job completion
// completionCallbackData - data to pass to completionCallback
// Returns scheduler on success or NULL otherwise
scheduler_t* schedulerCreate(const char* schedulerName, simulator_t* sim, completionCallback_fn completionCallback, void* completionCallbackData);

// Destroys a scheduler
void schedulerDestroy(scheduler_t* scheduler);

// Called at a job arrival to schedule the job
void schedulerScheduleJob(scheduler_t* scheduler, job_t* job);

// Called at a job completion
void schedulerCompleteJob(void* s);

// Schedule next completion at given time
// Returns true on success, false otherwise
bool schedulerScheduleNextCompletion(scheduler_t* scheduler, uint64_t timestamp);

// Cancel next completion event
// Returns true on success, false otherwise
bool schedulerCancelNextCompletion(scheduler_t* scheduler);

// Defines scheduler specific functions
#define DEFINE_SCHEDULER(schedulerName)                                 \
    void* scheduler ## schedulerName ## Create();                       \
    void scheduler ## schedulerName ## Destroy(void* schedulerInfo);    \
    void scheduler ## schedulerName ## ScheduleJob(void* schedulerInfo, scheduler_t* scheduler, job_t* job, uint64_t currentTime); \
    job_t* scheduler ## schedulerName ## CompleteJob(void* schedulerInfo, scheduler_t* scheduler, uint64_t currentTime);

// Initializes scheduler specific functions
#define INIT_SCHEDULER(s, schedulerName) do {                           \
        (s)->create = scheduler ## schedulerName ## Create;             \
        (s)->destroy = scheduler ## schedulerName ## Destroy;           \
        (s)->scheduleJob = scheduler ## schedulerName ## ScheduleJob;   \
        (s)->completeJob = scheduler ## schedulerName ## CompleteJob;   \
    } while (0)

DEFINE_SCHEDULER(FCFS)
DEFINE_SCHEDULER(LCFS)
DEFINE_SCHEDULER(SJF)
DEFINE_SCHEDULER(MLFQ)

#endif /* SCHEDULER_H */
