#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include "InverterSIM.h"

// ================= Buffer & Sample ==================
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
    std::string apiKey = "NjhhZWIwNDU1ZDdmMzg3MzNiMTQ5YTFjOjY4YWViMDQ1NWQ3ZjM4NzMzYjE0OWExMg==";
    InverterSIM sim(apiKey);

    // Demo: write once
    if (sim.setExportPowerPercent(20)) {
        std::cout<<"Export power set to 20%\n";
    } else {
        std::cerr<<"Failed to set export power percent\n";
    }

    // Demo: dynamic register read (temperature and export power percent)
    std::vector<uint16_t> values;
    if (sim.readRegisters(7, 2, values)) {
        float temperature = values[0] / 10.0f;
        int exportPercent = values[1];
        std::cout << "Temperature: " << temperature << " C\n";
        std::cout << "Export Power Percent: " << exportPercent << " %\n";
    } else {
        std::cerr << "Failed to read temperature and export power percent\n";
    }

    // Demo: dynamic register read (voltage and current)
    std::vector<uint16_t> vc_values;
    if (sim.readRegisters(0, 2, vc_values)) {
        float voltage = vc_values[0] / 10.0f;
        float current = vc_values[1] / 10.0f;
        std::cout << "[Dynamic] Voltage: " << voltage << " V\n";
        std::cout << "[Dynamic] Current: " << current << " A\n";
    } else {
        std::cerr << "Failed to read voltage and current registers dynamically\n";
    }

    // Start polling voltage/current as before
    DataBuffer buffer(30);
    std::thread pollT(pollLoop,std::ref(sim),std::ref(buffer),std::chrono::milliseconds(5000));
    std::thread upT(uploadLoop,std::ref(buffer),std::chrono::milliseconds(30000));
    pollT.join(); upT.join();
    return 0;
}   
