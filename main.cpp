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
struct Sample { float voltage; float current; long long timestamp; };

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
void pollLoop(InverterSIM& sim, DataBuffer& buf,
              std::chrono::milliseconds pollInt) {
    auto start = std::chrono::steady_clock::now();
    while(true) {
        float v,i;
        if (sim.getVoltageCurrent(v,i)) {
            Sample s;
            s.voltage = v; s.current = i;
            s.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()-start).count();
            if (buf.hasSpace()) buf.append(s);
        } else {
            std::cerr<<"Poll failed\n";
        }
        std::this_thread::sleep_for(pollInt);
    }
}
void uploadLoop(DataBuffer& buf, std::chrono::milliseconds upInt) {
    while(true) {
        std::this_thread::sleep_for(upInt);
        auto data=buf.flush();
        if(!data.empty()){
            std::cout<<"Uploading "<<data.size()<<" samples\n";
            for(auto&s:data){
                std::cout<<"t="<<s.timestamp<<" ms V="<<s.voltage
                         <<" I="<<s.current<<"\n";
            }
        } else std::cout<<"No data\n";
    }
}

// ================= Main ==================
int main(){
    auto cfg = readConfig("config.txt");
    std::string apiKey = cfg.count("api_key") ? cfg["api_key"] : "";
    int pollInt = cfg.count("poll_interval_ms") ? std::stoi(cfg["poll_interval_ms"]) : 1000;
    int upInt = cfg.count("write_interval_ms") ? std::stoi(cfg["write_interval_ms"]) : 15000;
    int exportPower = cfg.count("export_power_percent") ? std::stoi(cfg["export_power_percent"]) : 20;
    size_t bufCap = 30;

    InverterSIM sim(apiKey);

    // Demo: write once
    if (sim.setExportPowerPercent(exportPower)) {
        std::cout << "Export power set to " << exportPower << "%\n";
    }

    DataBuffer buffer(bufCap);
    std::thread pollT(pollLoop, std::ref(sim), std::ref(buffer), std::chrono::milliseconds(pollInt));
    std::thread upT(uploadLoop, std::ref(buffer), std::chrono::milliseconds(upInt));
    pollT.join(); upT.join();
    return 0;
}
