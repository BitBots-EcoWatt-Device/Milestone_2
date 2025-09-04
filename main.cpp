#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include "InverterSIM.h"
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>

// ================= Buffer & Sample ==================
// ================= Config Reader ==================
std::map<std::string, std::string> readConfig(const std::string& path) {
    std::ifstream f(path);
    std::map<std::string, std::string> cfg;
    std::string line;
    while (std::getline(f, line)) {
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        cfg[line.substr(0, pos)] = line.substr(pos + 1);
    }
    return cfg;
}

struct Sample {
    std::map<int, float> values; // key: register, value: reading
    long long timestamp;
};

class DataBuffer {
public:
    explicit DataBuffer(size_t cap): capacity_(cap) {}
    bool hasSpace(){ std::lock_guard<std::mutex> l(m_); return buf_.size()<capacity_; }
    void append(const Sample& s){ std::lock_guard<std::mutex> l(m_); buf_.push_back(s); }
    std::vector<Sample> flush(){
        std::lock_guard<std::mutex> l(m_);
        auto out=buf_; buf_.clear(); return out;
    }
private:
    std::vector<Sample> buf_; std::mutex m_; size_t capacity_;
};

// ================= Loops ==================
// Helper: parse register list from config string
std::vector<int> parseRegisterList(const std::string& regStr) {
    std::vector<int> regs;
    std::stringstream ss(regStr);
    std::string item;
    while (std::getline(ss, item, ',')) {
        try {
            regs.push_back(std::stoi(item));
        } catch (...) {}
    }
    return regs;
}

// Simulate reading a register (for demo, only voltage/current supported)
bool readRegister(InverterSIM& sim, int reg, float& value) {
    float v, i;
    if (reg == 0 || reg == 1) {
        if (sim.getVoltageCurrent(v, i)) {
            value = (reg == 0) ? v : i;
            return true;
        }
    }
    // Extend here for more registers
    return false;
}

void pollLoop(InverterSIM& sim, DataBuffer& buf,
              std::chrono::milliseconds pollInt,
              const std::vector<int>& regList) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        Sample s;
        s.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        bool ok = true;
        for (int reg : regList) {
            float val;
            if (readRegister(sim, reg, val)) {
                s.values[reg] = val;
            } else {
                ok = false;
                std::cerr << "Poll failed for register " << reg << "\n";
            }
        }
        if (ok && buf.hasSpace()) buf.append(s);
        std::this_thread::sleep_for(pollInt);
    }
}
void uploadLoop(DataBuffer& buf, std::chrono::milliseconds upInt, const std::vector<int>& regList) {
    while (true) {
        std::this_thread::sleep_for(upInt);
        auto data = buf.flush();
        if (!data.empty()) {
            std::cout << "Uploading " << data.size() << " samples\n";
            for (auto& s : data) {
                std::cout << "t=" << s.timestamp << " ms";
                for (int reg : regList) {
                    if (s.values.count(reg)) {
                        std::cout << " R" << reg << "=" << s.values.at(reg);
                    }
                }
                std::cout << "\n";
            }
        } else std::cout << "No data\n";
    }
}

// ================= Main ==================
int main() {
    auto cfg = readConfig("config.txt");
    std::string apiKey = cfg.count("api_key") ? cfg["api_key"] : "";
    int pollInt = cfg.count("poll_interval_ms") ? std::stoi(cfg["poll_interval_ms"]) : 1000;
    int upInt = cfg.count("write_interval_ms") ? std::stoi(cfg["write_interval_ms"]) : 15000;
    int exportPower = cfg.count("export_power_percent") ? std::stoi(cfg["export_power_percent"]) : 20;
    size_t bufCap = 30;
    std::vector<int> regList = cfg.count("registers") ? parseRegisterList(cfg["registers"]) : std::vector<int>{0, 1};

    // Triggered write config
    bool enableWrite = cfg.count("triggered_write_enable") ? (cfg["triggered_write_enable"] == "1") : false;
    int writeInterval = cfg.count("triggered_write_interval_ms") ? std::stoi(cfg["triggered_write_interval_ms"]) : 0;

    InverterSIM sim(apiKey);

    // Demo: write once
    if (!enableWrite && sim.setExportPowerPercent(exportPower)) {
        std::cout << "Export power set to " << exportPower << "%\n";
    }

    DataBuffer buffer(bufCap);
    std::thread pollT(pollLoop, std::ref(sim), std::ref(buffer), std::chrono::milliseconds(pollInt), regList);
    std::thread upT(uploadLoop, std::ref(buffer), std::chrono::milliseconds(upInt), regList);

    // Periodic triggered write
    std::thread writeT;
    if (enableWrite && writeInterval > 0) {
        writeT = std::thread([&sim, exportPower, writeInterval]() {
            while (true) {
                if (sim.setExportPowerPercent(exportPower)) {
                    std::cout << "Triggered export power set to " << exportPower << "%\n";
                } else {
                    std::cerr << "Triggered write failed\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(writeInterval));
            }
        });
    }

    pollT.join();
    upT.join();
    if (writeT.joinable()) writeT.join();
    return 0;
}
