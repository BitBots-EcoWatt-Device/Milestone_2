#ifndef INVERTER_SIM_H
#define INVERTER_SIM_H

#include "ProtocolAdapter.h"

class InverterSIM {
public:
    InverterSIM(const std::string& apiKey);

    bool getVoltageCurrent(float &voltage, float &current);
    bool setExportPowerPercent(int value);

private:
    ProtocolAdapter adapter_;
};

#endif
