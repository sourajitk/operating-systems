#include <assert.h>
#include <stdlib.h>
#include "simulator.h"

// Events sorted by (time, type, id)
int simulatorEventCompare(void* data1, void* data2)
{
    event_t* event1 = (event_t*)data1;
    event_t* event2 = (event_t*)data2;
    if (event1->timestamp < event2->timestamp) {
        return -1;
    } else if (event1->timestamp == event2->timestamp) {
        if (event1->type < event2->type) {
            return -1;
        } else if (event1->type == event2->type) {
            if (event1->id < event2->id) {
                return -1;
            } else if (event1->id == event2->id) {
                return 0;
            } else {
                return 1;
            }
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

// Create a discrete event simulator
simulator_t* simulatorCreate()
{
    simulator_t* sim = malloc(sizeof(simulator_t));
    if (sim == NULL) {
        return NULL;
    }
    sim->queue = list_create(simulatorEventCompare);
    sim->simTime = 0;
    sim->id = 0;
    if (sim->queue == NULL) {
        free(sim);
        return NULL;
    }
    return sim;
}

// Destroy a discrete event simulator
void simulatorDestroy(simulator_t* sim)
{
    while (list_count(sim->queue) > 0) {
        simulatorRemoveEvent(sim, list_head(sim->queue));
    }
    list_destroy(sim->queue);
    free(sim);
}

// Add an event to the event queue
// sim - simulator
// timestamp - time of the event
// type - type of event
// callback - function to call at the time of the event
// callbackData - data to pass to the callback
// Returns an event reference that can be used to remove the event
list_node_t* simulatorSchedule(simulator_t* sim, uint64_t timestamp, event_type_t type, event_callback callback, void* callbackData)
{
    assert(timestamp >= simulatorSimTime(sim)); // ensure we don't go back in time
    event_t* event = malloc(sizeof(event_t));
    if (event == NULL) {
        return NULL;
    }
    event->timestamp = timestamp;
    event->type = type;
    event->id = sim->id++;
    event->callback = callback;
    event->callbackData = callbackData;
    list_node_t* node = list_insert(sim->queue, event);
    if (node == NULL) {
        free(event);
        return NULL;
    }
    return node;
}

// Remove an event from the event queue
// sim - simulator
// eventRef - reference to the event to remove, which is returned from simulatorSchedule
void simulatorRemoveEvent(simulator_t* sim, list_node_t* eventRef)
{
    free(list_data(eventRef));
    list_remove(sim->queue, eventRef);
}

// Run simulation until no more events
void simulatorRun(simulator_t* sim)
{
    while (list_count(sim->queue) > 0) {
        list_node_t* node = list_head(sim->queue);
        event_t* event = (event_t*)list_data(node);
        sim->simTime = event->timestamp;
        event->callback(event->callbackData);
        free(event);
        list_remove(sim->queue, node);
    }
}
