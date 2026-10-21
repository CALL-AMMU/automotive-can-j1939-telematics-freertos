#ifndef J1939_H
#define J1939_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard SAE J1939 PGNs for Electric Commercial Vehicles & Buses
#define J1939_PGN_EEC1       61444   // 0xF004: Electronic Engine/Motor Controller 1
#define J1939_PGN_ET1        65262   // 0xFEEE: Engine / Battery Coolant Temperature
#define J1939_PGN_CCVS       65265   // 0xFEF1: Cruise Control / Vehicle Speed
#define J1939_PGN_VEP        65271   // 0xFEF7: Battery Electrical Parameters (BMS)
#define J1939_PGN_DM1        65226   // 0xFECA: Active Diagnostic Trouble Codes

// Raw CAN 2.0B Frame (29-bit identifier)
typedef struct {
    uint32_t id;         // 29-bit Extended CAN Identifier
    uint8_t  len;        // Payload length (0-8 bytes)
    uint8_t  data[8];    // Data payload
    uint32_t timestamp;  // Capture timestamp in milliseconds
} CanFrame;

// J1939 Header parsed components
typedef struct {
    uint8_t  priority;
    uint32_t pgn;
    uint8_t  source_address;
    uint8_t  destination_address;
    bool     is_broadcast;
} J1939Header;

// Diagnostic Trouble Code (DTC)
typedef struct {
    uint32_t spn;         // Suspect Parameter Number
    uint8_t  fmi;         // Failure Mode Identifier
    uint8_t  occurrence;  // Occurrence Count
    bool     lamp_red;
    bool     lamp_amber;
} J1939Dtc;

// Decoded Vehicle Telematics State
typedef struct {
    // Battery & BMS
    float battery_voltage;       // Volts
    float battery_current;       // Amperes (positive = discharge, negative = charge)
    float state_of_charge;       // Percentage (0.0 - 100.0%)
    float battery_temp_c;        // Celsius
    
    // Motor & Drivetrain
    float motor_rpm;             // RPM
    float motor_torque_percent;  // % of reference torque
    float vehicle_speed_kmh;     // km/h
    
    // Diagnostic & Safety
    bool  high_voltage_interlock;// True = HV safe, False = HV open/fault
    uint8_t active_dtc_count;
    J1939Dtc active_dtcs[4];
    
    uint32_t last_update_ms;
} VehicleTelematicsState;

// Protocol Functions
J1939Header j1939_parse_id(uint32_t can_id);
uint32_t    j1939_build_id(uint8_t priority, uint32_t pgn, uint8_t sa, uint8_t da);
bool        j1939_decode_frame(const CanFrame* frame, VehicleTelematicsState* state);
const char* j1939_get_spn_description(uint32_t spn);
const char* j1939_get_fmi_description(uint8_t fmi);

#ifdef __cplusplus
}
#endif

#endif // J1939_H
