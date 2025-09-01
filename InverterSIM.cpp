#include "InverterSIM.h"
#include <iostream>

InverterSIM::InverterSIM(const std::string& apiKey) : adapter_(apiKey) {}

bool InverterSIM::getVoltageCurrent(float &voltage, float &current) {
    std::string resp;
    std::string req = "110300000002C69B";  // fixed frame: read voltage+current

    if (!adapter_.sendReadRequest(req, resp)) return false;
    if (resp.size() < 18) return false;

    int vRaw = std::stoi(resp.substr(6,4), nullptr, 16);
    int iRaw = std::stoi(resp.substr(10,4), nullptr, 16);
    voltage = vRaw / 10.0f;
    current = iRaw / 10.0f;
    return true;
}

bool InverterSIM::setExportPowerPercent(int value) {
    std::string resp;
    // For milestone demo we use a fixed frame that sets 20%.
    // (Later you can build frame based on "value")
    std::string req = "1106000800149A7C"; 
    if (!adapter_.sendWriteRequest(req, resp)) return false;
    return resp == req;
}
