#include "InverterSIM.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdint>
#include <algorithm>

// ---------------- Modbus helpers ----------------
static uint16_t crc16_modbus(const std::vector<uint8_t>& bytes) {
    uint16_t crc = 0xFFFF;
    for (uint8_t b : bytes) {
        crc ^= b;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001; // polynomial
            } else {
                crc >>= 1;
            }
        }
    }
    return crc; // note: LSB first when appending to frame
}

static std::string bytesToHex(const std::vector<uint8_t>& buf) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    for (uint8_t b : buf) oss << std::setw(2) << (int)b;
    return oss.str();
}

static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> out;
    out.reserve(hex.size()/2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t v = static_cast<uint8_t>(std::stoi(hex.substr(i,2), nullptr, 16));
        out.push_back(v);
    }
    return out;
}

// ------------------------------------------------

InverterSIM::InverterSIM(const std::string& apiKey) : adapter_(apiKey) {}

bool InverterSIM::getVoltageCurrent(float &voltage, float &current) {
    std::string resp;
    // Fixed frame: Read Holding Registers, slave=0x11, start=0x0000, qty=0x0002, CRC=0x9BC6 (little-endian in wire)
    // Hex shown here is the whole RTU frame in big-endian text order.
    const std::string req = "110300000002C69B";

    if (!adapter_.sendReadRequest(req, resp)) return false;
    if (resp.size() < 18) return false; // 11 03 04 VHi VLo IHi ILo CRClo CRChi -> 18 hex chars min

    // Minimal parse (your API returns hex without spaces). Byte count is at byte 2.
    // Voltage at bytes 3..4, Current at bytes 5..6 (Modbus register big-endian).
    // Indexes in hex chars: [0..1]=slave, [2..3]=func, [4..5]=bytecount,
    // voltage: [6..9], current: [10..13].
    try {
        int vRaw = std::stoi(resp.substr(6, 4),  nullptr, 16);
        int iRaw = std::stoi(resp.substr(10, 4), nullptr, 16);
        voltage = vRaw / 10.0f;
        current = iRaw / 10.0f;
    } catch (...) {
        return false;
    }
    return true;
}

bool InverterSIM::setExportPowerPercent(int value) {
    // Guard range (0..100). Out-of-range will likely return Modbus exception 0x03,
    // but we clamp here to make the demo clean.
    if (value < 0)   value = 0;
    if (value > 100) value = 100;

    // Build Modbus RTU Write Single Register (0x06)
    // Slave     = 0x11
    // Function  = 0x06
    // Address   = 0x0008 (Export Power Percent register per API)
    // Value     = 0x0000..0x0064 (0..100)
    std::vector<uint8_t> frame = {
        0x11,       // slave
        0x06,       // function
        0x00, 0x08, // register address
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>( value       & 0xFF)
    };

    // Compute CRC (append LSB then MSB)
    uint16_t crc = crc16_modbus(frame);
    frame.push_back(static_cast<uint8_t>(crc & 0xFF));        // CRC low
    frame.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF)); // CRC high

    const std::string reqHex = bytesToHex(frame);

    std::string respHex;
    if (!adapter_.sendWriteRequest(reqHex, respHex)) {
        std::cerr << "[WRITE] HTTP/transport failed. No response frame.\n";
        return false;
    }

    // Success path: device echoes exactly the same frame.
    if (respHex == reqHex) {
        std::cout << "[WRITE] Export power set to " << value << "% (echo OK)\n";
        return true;
    }

    // Exception path: function code OR'ed with 0x80 (i.e., 0x86), next byte is exception code.
    // Expected minimal exception frame: 11 86 EC CRClo CRChi -> hex length >= 10.
    try {
        if (respHex.size() >= 6) {
            int func = std::stoi(respHex.substr(2, 2), nullptr, 16);
            if ((func & 0x80) == 0x80 && respHex.size() >= 10) {
                int exCode = std::stoi(respHex.substr(4, 2), nullptr, 16);
                std::cerr << "[WRITE] Modbus exception 0x" << std::hex << std::uppercase
                          << exCode << " for value " << std::dec << value << "\n";
                return false;
            }
        }
    } catch (...) {
        // fallthrough to generic log
    }

    // Generic mismatch
    std::cerr << "[WRITE] Response did not echo request. req=" << reqHex
              << " resp=" << respHex << "\n";
    return false;
}
