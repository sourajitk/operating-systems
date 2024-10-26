#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "trace.h"
#include "simulator.h"
#include "scheduler.h"
#include "job.h"

// Run a trace
// traceFilename - path to trace file
// outFilename - path to output file
// scheduler - queue scheduler to evaluate
// Returns true on success, false otherwise
bool traceRun(const char* traceFilename, const char* outFilename, const char* schedulerName)
{
    trace_t* trace = malloc(sizeof(trace_t));
    if (trace == NULL) {
        return false;
    }
    trace->traceFile = fopen(traceFilename, "r");
    if (trace->traceFile == NULL) {
        printf("Invalid trace file: %s\n", traceFilename);
        free(trace);
        return false;
    }
    trace->outFile = fopen(outFilename, "w");
    if (trace->outFile == NULL) {
        printf("Invalid output file: %s\n", outFilename);
        fclose(trace->traceFile);
        free(trace);
        return false;
    }
    trace->sim = simulatorCreate();
    if (trace->sim == NULL) {
        fclose(trace->outFile);
        fclose(trace->traceFile);
        free(trace);
        return false;
    }
    trace->scheduler = schedulerCreate(schedulerName, trace->sim, traceCompletionCallback, trace);
    if (trace->scheduler == NULL) {
        simulatorDestroy(trace->sim);
        fclose(trace->outFile);
        fclose(trace->traceFile);
        free(trace);
        return false;
    }
    traceScheduleNextArrival(trace);
    simulatorRun(trace->sim);
    schedulerDestroy(trace->scheduler);
    simulatorDestroy(trace->sim);
    fclose(trace->outFile);
    fclose(trace->traceFile);
    free(trace);
    return true;
}

// Schedule the next arrival in the trace
// trace - trace
void traceScheduleNextArrival(trace_t* trace)
{
    uint64_t id;
    uint64_t arrivalTime;
    uint64_t jobTime;
    if (fscanf(trace->traceFile, "%" SCNu64 ",%" SCNu64 ",%" SCNu64, &id, &arrivalTime, &jobTime) != 3) {
        assert(feof(trace->traceFile));
        return;
    }
    trace->currentJob = jobCreate(arrivalTime, jobTime, id);
    assert(trace->currentJob);
    list_node_t* eventRef = simulatorSchedule(trace->sim, jobGetArrivalTime(trace->currentJob), EVENT_ARRIVAL, traceArrivalCallback, trace);
    assert(eventRef);
}

// Called when there's a job arrival
// t - trace
void traceArrivalCallback(void* t)
{
    trace_t* trace = (trace_t*)t;
    schedulerScheduleJob(trace->scheduler, trace->currentJob);
    traceScheduleNextArrival(trace);
}

// Called when there's a job completion
// t - trace
void traceCompletionCallback(void* t, job_t* job)
{
    trace_t* trace = (trace_t*)t;
    fprintf(trace->outFile, "%" PRIu64 ",%" PRIu64 "\n", jobGetId(job), simulatorSimTime(trace->sim));
    jobDestroy(job);
}
