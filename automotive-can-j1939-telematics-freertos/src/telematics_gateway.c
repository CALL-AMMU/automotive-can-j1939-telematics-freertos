#include "telematics_gateway.h"
#include <stdio.h>
#include <string.h>

void telematics_init(TelematicsGateway* gw, const char* vin, const char* operator_name) {
    memset(gw, 0, sizeof(TelematicsGateway));
    strncpy(gw->bus_vin, vin, sizeof(gw->bus_vin) - 1);
    strncpy(gw->fleet_operator, operator_name, sizeof(gw->fleet_operator) - 1);

    // Initial default safe metrics
    gw->state.battery_voltage = 650.0f; // 650V nominal EV bus traction battery
    gw->state.battery_current = 0.0f;
    gw->state.state_of_charge = 92.5f;  // 92.5% SoC
    gw->state.battery_temp_c = 28.0f;   // 28 C
    gw->state.high_voltage_interlock = true;
    gw->state.motor_rpm = 0.0f;
    gw->state.vehicle_speed_kmh = 0.0f;
}

bool telematics_enqueue_can_frame(TelematicsGateway* gw, const CanFrame* frame) {
    CanFrameQueue* q = &gw->rx_queue;
    if (q->count >= QUEUE_MAX_FRAMES) {
        q->dropped_frames++;
        return false;
    }

    q->buffer[q->head] = *frame;
    q->head = (q->head + 1) % QUEUE_MAX_FRAMES;
    q->count++;
    return true;
}

void telematics_process_tasks(TelematicsGateway* gw, uint32_t current_time_ms) {
    CanFrameQueue* q = &gw->rx_queue;

    // Process all pending frames in FreeRTOS RX queue
    while (q->count > 0) {
        CanFrame frame = q->buffer[q->tail];
        q->tail = (q->tail + 1) % QUEUE_MAX_FRAMES;
        q->count--;

        j1939_decode_frame(&frame, &gw->state);
        gw->frames_processed++;
    }

    // Check if sub-200ms dispatch timer fired
    if (current_time_ms - gw->last_dispatch_time >= TELEMETRY_INTERVAL_MS) {
        gw->last_dispatch_time = current_time_ms;
        gw->mqtt_packets_sent++;

        char json_buffer[512];
        telematics_generate_mqtt_payload(gw, json_buffer, sizeof(json_buffer));
        printf("[TELEMETRY DISPATCH @ %ums] -> %s\n", current_time_ms, json_buffer);
    }
}

int telematics_generate_mqtt_payload(const TelematicsGateway* gw, char* output_json, size_t max_len) {
    const VehicleTelematicsState* s = &gw->state;
    
    return snprintf(output_json, max_len,
        "{\"vin\":\"%s\",\"op\":\"%s\",\"soc\":%.1f,\"v_bus\":%.1f,\"i_bus\":%.1f,"
        "\"t_bat\":%.1f,\"rpm\":%.0f,\"spd\":%.1f,\"hvil\":%s,\"dtc_cnt\":%u}",
        gw->bus_vin, gw->fleet_operator,
        s->state_of_charge, s->battery_voltage, s->battery_current,
        s->battery_temp_c, s->motor_rpm, s->vehicle_speed_kmh,
        s->high_voltage_interlock ? "true" : "false",
        s->active_dtc_count
    );
}
