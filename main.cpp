#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include "Inverter.h"

// ================= Buffer & Sample ==================
struct Sample
{
    float voltage;
    float current;
    long long timestamp;
};

class DataBuffer
{
public:
    explicit DataBuffer(size_t cap) : capacity_(cap) {}
    bool hasSpace()
    {
        std::lock_guard<std::mutex> l(m_);
        return buf_.size() < capacity_;
    }
    void append(const Sample &s)
    {
        std::lock_guard<std::mutex> l(m_);
        buf_.push_back(s);
    }
    std::vector<Sample> flush()
    {
        std::lock_guard<std::mutex> l(m_);
        auto out = buf_;
        buf_.clear();
        return out;
    }

private:
    std::vector<Sample> buf_;
    std::mutex m_;
    size_t capacity_;
};

// ================= Loops ==================
void pollLoop(Inverter &inverter, DataBuffer &buf,
              std::chrono::milliseconds pollInt)
{
    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        float v, i;
        if (inverter.getACVoltage(v) && inverter.getACCurrent(i))
        {
            Sample s;
            s.voltage = v;
            s.current = i;
            s.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start)
                              .count();
            if (buf.hasSpace())
                buf.append(s);
        }
        else
        {
            std::cerr << "Poll failed\n";
        }
        std::this_thread::sleep_for(pollInt);
    }
}
void uploadLoop(DataBuffer &buf, std::chrono::milliseconds upInt)
{
    while (true)
    {
        std::this_thread::sleep_for(upInt);
        auto data = buf.flush();
        if (!data.empty())
        {
            std::cout << "Uploading " << data.size() << " samples\n";
            for (auto &s : data)
            {
                std::cout << "t=" << s.timestamp << " ms V=" << s.voltage
                          << " I=" << s.current << "\n";
            }
        }
        else
            std::cout << "No data\n";
    }
}

// ================= Main ==================
int main()
{
    std::cout << "=== Inverter Communication Demo ===\n";

    // Create Inverter instance (configuration loaded automatically)
    Inverter inverter;

    // Demo: write once
    if (inverter.setExportPowerPercent(20))
    {
        std::cout << "Export power set to 20%\n";
    }
    else
    {
        std::cerr << "Failed to set export power percent\n";
    }

    // Demo: dynamic register read (temperature and export power percent)
    float temperature;
    int exportPercent;
    if (inverter.getTemperature(temperature) && inverter.getExportPowerPercent(exportPercent))
    {
        std::cout << "Temperature: " << temperature << " C\n";
        std::cout << "Export Power Percent: " << exportPercent << " %\n";
    }
    else
    {
        std::cerr << "Failed to read temperature and export power percent\n";
    }

    // Demo: comprehensive AC measurements
    float acVoltage, acCurrent, acFrequency;
    if (inverter.getACMeasurements(acVoltage, acCurrent, acFrequency))
    {
        std::cout << "AC Measurements - Voltage: " << acVoltage << " V, Current: " << acCurrent << " A, Frequency: " << acFrequency << " Hz\n";
    }
    else
    {
        std::cerr << "Failed to read AC measurements\n";
    }

    // Demo: PV input measurements
    float pv1Voltage, pv2Voltage, pv1Current, pv2Current;
    if (inverter.getPVMeasurements(pv1Voltage, pv2Voltage, pv1Current, pv2Current))
    {
        std::cout << "PV1 - Voltage: " << pv1Voltage << " V, Current: " << pv1Current << " A\n";
        std::cout << "PV2 - Voltage: " << pv2Voltage << " V, Current: " << pv2Current << " A\n";
    }
    else
    {
        std::cerr << "Failed to read PV measurements\n";
    }

    // Demo: system status
    int outputPower;
    if (inverter.getSystemStatus(temperature, exportPercent, outputPower))
    {
        std::cout << "System Status - Temperature: " << temperature << " C, Export: " << exportPercent << " %, Output Power: " << outputPower << " W\n";
    }
    else
    {
        std::cerr << "Failed to read system status\n";
    }

    // Demo: dynamic register read (voltage and current)
    float voltage, current;
    if (inverter.getACVoltage(voltage) && inverter.getACCurrent(current))
    {
        std::cout << "[Dynamic] Voltage: " << voltage << " V\n";
        std::cout << "[Dynamic] Current: " << current << " A\n";
    }
    else
    {
        std::cerr << "Failed to read voltage and current registers dynamically\n";
    }

    // Start polling voltage/current
    DataBuffer buffer(30);
    std::thread pollT(pollLoop, std::ref(inverter), std::ref(buffer), std::chrono::milliseconds(5000));
    std::thread upT(uploadLoop, std::ref(buffer), std::chrono::milliseconds(30000));
    pollT.join();
    upT.join();
    return 0;
}
