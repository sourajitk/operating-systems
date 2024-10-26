#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trace.h"

// Print program usage info
void usage(char* program)
{
    printf("%s traceFile outFile scheduler\n", program);
    printf("Scheduler options:\n");
    printf("FCFS\n");
    printf("LCFS\n");
    printf("SJF\n");
    printf("MLFQ\n");
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        usage(argv[0]);
        return -1;
    }
    // Run the trace
    const char* traceFile = argv[1];
    const char* outFile = argv[2];
    const char* schedulerName = argv[3];
    if (!traceRun(traceFile, outFile, schedulerName)) {
        usage(argv[0]);
        return -2;
    }
    // Sort the output file by job id
    size_t len = 2*strlen(outFile) + strlen("sort -t, -k1n,1 -o  ") + 1;
    char* cmd = malloc(len);
    if (cmd == NULL) {
        return false;
    }
    if (snprintf(cmd, len, "sort -t, -k1n,1 -o %s %s", outFile, outFile) != len-1) {
        free(cmd);
        return false;
    }
    int ret = system(cmd);
    free(cmd);
    return ret;
}
