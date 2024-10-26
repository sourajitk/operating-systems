#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>
#include <stdbool.h>
#include "simulator.h"
#include "scheduler.h"
#include "job.h"

typedef struct {
    FILE* traceFile; // trace file
    FILE* outFile; // output file
    simulator_t* sim; // simulator
    scheduler_t* scheduler; // scheduler
    job_t* currentJob; // current job
} trace_t;

// Run a trace
// traceFilename - path to trace file
// outFilename - path to output file
// scheduler - queue scheduler to evaluate
// Returns true on success, false otherwise
bool traceRun(const char* traceFilename, const char* outFilename, const char* schedulerName);

// Schedule the next arrival in the trace
// trace - trace
void traceScheduleNextArrival(trace_t* trace);

// Called when there's a job arrival
// t - trace
void traceArrivalCallback(void* t);

// Called when there's a job completion
// t - trace
void traceCompletionCallback(void* t, job_t* job);

#endif /* TRACE_H */
