#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdint.h>
#include "linked_list.h"

typedef struct {
    list_t* queue; // event queue in sorted order
    uint64_t simTime; // simulator current time
    uint64_t id; // current event id
} simulator_t;

typedef enum {
    EVENT_COMPLETION, // job completion event
    EVENT_ARRIVAL // job arrival event
} event_type_t;

// Event callback type
// Callback function will be called at the scheduled time with the provided callbackData
typedef void (*event_callback)(void* callbackData);

typedef struct {
    uint64_t timestamp; // time at which callback is invoked
    event_type_t type; // event type
    uint64_t id; // event id
    event_callback callback; // callback to invoke
    void* callbackData; // data to pass to callback
} event_t;

// Gets simulator time
static inline uint64_t simulatorSimTime(simulator_t* sim)
{
    return sim->simTime;
}

// Events sorted by (time, type, id)
int simulatorEventCompare(void* data1, void* data2);

// Create and return a discrete event simulator
simulator_t* simulatorCreate();

// Destroy a discrete event simulator
void simulatorDestroy(simulator_t* sim);

// Add an event to the event queue
// sim - simulator
// timestamp - time of the event
// type - type of event
// callback - function to call at the time of the event
// callbackData - data to pass to the callback
// Returns an event reference that can be used to remove the event
list_node_t* simulatorSchedule(simulator_t* sim, uint64_t timestamp, event_type_t type, event_callback callback, void* callbackData);

// Remove an event from the event queue
// sim - simulator
// eventRef - reference to the event to remove, which is returned from simulatorSchedule
void simulatorRemoveEvent(simulator_t* sim, list_node_t* eventRef);

// Run simulation until no more events
void simulatorRun(simulator_t* sim);

#endif /* SIMULATOR_H */
