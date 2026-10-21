#include "j1939.h"
#include <stdio.h>
#include <string.h>

J1939Header j1939_parse_id(uint32_t can_id) {
    J1939Header hdr;
    hdr.priority = (uint8_t)((can_id >> 26) & 0x07);
    uint8_t dp = (uint8_t)((can_id >> 24) & 0x01);
    uint8_t pf = (uint8_t)((can_id >> 16) & 0xFF);
    uint8_t ps = (uint8_t)((can_id >> 8) & 0xFF);
    hdr.source_address = (uint8_t)(can_id & 0xFF);

    if (pf < 240) {
        // PDU1: Peer-to-peer (PS is destination address)
        hdr.pgn = (dp << 16) | (pf << 8);
        hdr.destination_address = ps;
        hdr.is_broadcast = false;
    } else {
        // PDU2: Broadcast (PS is group extension)
        hdr.pgn = (dp << 16) | (pf << 8) | ps;
        hdr.destination_address = 0xFF; // Global
        hdr.is_broadcast = true;
    }

    return hdr;
}

uint32_t j1939_build_id(uint8_t priority, uint32_t pgn, uint8_t sa, uint8_t da) {
    uint8_t dp = (uint8_t)((pgn >> 16) & 0x01);
    uint8_t pf = (uint8_t)((pgn >> 8) & 0xFF);
    uint8_t ps = (uint8_t)(pgn & 0xFF);

    if (pf < 240) {
        ps = da; // Destination in PDU1
    }

    return ((uint32_t)(priority & 0x07) << 26) |
           ((uint32_t)dp << 24) |
           ((uint32_t)pf << 16) |
           ((uint32_t)ps << 8) |
           (uint32_t)sa;
}

bool j1939_decode_frame(const CanFrame* frame, VehicleTelematicsState* state) {
    if (!frame || !state) return false;

    J1939Header hdr = j1939_parse_id(frame->id);

    switch (hdr.pgn) {
        case J1939_PGN_EEC1: {
            // Byte 3-4: Engine / Motor Speed (Resolution: 0.125 rpm/bit, Offset: 0)
            uint16_t raw_rpm = (uint16_t)(frame->data[3] | (frame->data[4] << 8));
            state->motor_rpm = raw_rpm * 0.125f;

            // Byte 2: Actual Engine Percent Torque (Resolution: 1%/bit, Offset: -125%)
            state->motor_torque_percent = (float)frame->data[2] - 125.0f;
            state->last_update_ms = frame->timestamp;
            return true;
        }

        case J1939_PGN_CCVS: {
            // Byte 1-2: Wheel-Based Vehicle Speed (Resolution: 1/256 km/h per bit, Offset: 0)
            uint16_t raw_speed = (uint16_t)(frame->data[1] | (frame->data[2] << 8));
            state->vehicle_speed_kmh = raw_speed / 256.0f;
            state->last_update_ms = frame->timestamp;
            return true;
        }

        case J1939_PGN_VEP: {
            // Byte 4-5: Battery Potential (Voltage) (Resolution: 0.05 V/bit, Offset: 0)
            uint16_t raw_v = (uint16_t)(frame->data[4] | (frame->data[5] << 8));
            state->battery_voltage = raw_v * 0.05f;

            // Byte 2-3: Battery Current (Resolution: 0.1 A/bit, Offset: -3212.8 A)
            uint16_t raw_i = (uint16_t)(frame->data[2] | (frame->data[3] << 8));
            state->battery_current = (raw_i * 0.1f) - 3212.8f;

            // Byte 1: Battery State of Charge (SOC) (Resolution: 0.4 %/bit, Offset: 0)
            state->state_of_charge = frame->data[1] * 0.4f;
            state->last_update_ms = frame->timestamp;
            return true;
        }

        case J1939_PGN_ET1: {
            // Byte 0: Battery / Engine Coolant Temperature (Resolution: 1 deg C/bit, Offset: -40 C)
            state->battery_temp_c = (float)frame->data[0] - 40.0f;
            state->last_update_ms = frame->timestamp;
            return true;
        }

        case J1939_PGN_DM1: {
            // Diagnostic Trouble Code Frame
            state->high_voltage_interlock = (frame->data[0] & 0x01) == 0; // Check lamp bits
            state->active_dtc_count = 0;

            if (frame->len >= 6) {
                J1939Dtc dtc;
                // SPN is stored across 19 bits: byte 2, byte 3, and top 3 bits of byte 4
                uint32_t spn = frame->data[2] | (frame->data[3] << 8) | ((frame->data[4] & 0xE0) << 11);
                uint8_t fmi = frame->data[4] & 0x1F;
                uint8_t occ = frame->data[5] & 0x7F;

                dtc.spn = spn;
                dtc.fmi = fmi;
                dtc.occurrence = occ;
                dtc.lamp_red = (frame->data[0] & 0x0C) != 0;
                dtc.lamp_amber = (frame->data[0] & 0x03) != 0;

                state->active_dtcs[state->active_dtc_count++] = dtc;
            }
            state->last_update_ms = frame->timestamp;
            return true;
        }

        default:
            return false;
    }
}

const char* j1939_get_spn_description(uint32_t spn) {
    switch (spn) {
        case 110: return "Battery / Engine Coolant Temperature";
        case 190: return "Electric Motor RPM";
        case 84:  return "Wheel-Based Vehicle Speed";
        case 168: return "Battery Main Bus Voltage";
        case 114: return "Battery High-Voltage Net Current";
        case 520201: return "BMS Cell Thermal Runaway Warning";
        default:  return "Proprietary ECU Parameter";
    }
}

const char* j1939_get_fmi_description(uint8_t fmi) {
    switch (fmi) {
        case 0: return "Data Valid But Above Normal Operational Range (Most Severe)";
        case 1: return "Data Valid But Below Normal Operational Range (Most Severe)";
        case 2: return "Data Erratic, Intermittent, Or Incorrect";
        case 3: return "Voltage Above Normal, Or Shorted To High Source";
        case 4: return "Voltage Below Normal, Or Shorted To Low Source";
        case 15: return "Data Valid But Above Normal Range (Least Severe)";
        default: return "Diagnostic Condition Active";
    }
}
