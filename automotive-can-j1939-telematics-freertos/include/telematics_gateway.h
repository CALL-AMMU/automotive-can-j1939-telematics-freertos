#ifndef TELEMATICS_GATEWAY_H
#define TELEMATICS_GATEWAY_H

#include "j1939.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_MAX_FRAMES 128
#define TELEMETRY_INTERVAL_MS 200 // Sub-200ms dispatch latency target

// Circular Queue for FreeRTOS Task Messaging
typedef struct {
    CanFrame buffer[QUEUE_MAX_FRAMES];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
    uint32_t dropped_frames;
} CanFrameQueue;

typedef struct {
    char bus_vin[18];             // Vehicle Identification Number
    char fleet_operator[32];      // e.g., "Olectra_Switch_Nashik_Fleet"
    VehicleTelematicsState state;
    CanFrameQueue rx_queue;
    uint32_t frames_processed;
    uint32_t mqtt_packets_sent;
    uint32_t last_dispatch_time;
    uint32_t max_processing_latency_us;
} TelematicsGateway;

// Gateway Lifecycle & Multitasking API
void telematics_init(TelematicsGateway* gw, const char* vin, const char* operator_name);
bool telematics_enqueue_can_frame(TelematicsGateway* gw, const CanFrame* frame);
void telematics_process_tasks(TelematicsGateway* gw, uint32_t current_time_ms);
int  telematics_generate_mqtt_payload(const TelematicsGateway* gw, char* output_json, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // TELEMATICS_GATEWAY_H
