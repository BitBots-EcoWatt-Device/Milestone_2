#ifndef INVERTER_SIM_H
#define INVERTER_SIM_H

#include <cstdint>
#include "ProtocolAdapter.h"
#include <vector>

class InverterSIM
{
public:
    InverterSIM(const std::string &apiKey);

    bool getVoltageCurrent(float &voltage, float &current);
    bool setExportPowerPercent(int value);
    bool readRegisters(uint16_t startAddr, uint16_t numRegs, std::vector<uint16_t> &values);

private:
    ProtocolAdapter adapter_;
};

#endif
