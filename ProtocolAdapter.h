#ifndef PROTOCOL_ADAPTER_H
#define PROTOCOL_ADAPTER_H

#include <string>

class ProtocolAdapter {
public:
    ProtocolAdapter(const std::string& apiKey);

    // Send a read frame and return response hex
    bool sendReadRequest(const std::string& frameHex, std::string& outFrameHex);

    // Send a write frame and return response hex
    bool sendWriteRequest(const std::string& frameHex, std::string& outFrameHex);

private:
    std::string apiKey_;
    const std::string readURL  = "http://20.15.114.131:8080/api/inverter/read";
    const std::string writeURL = "http://20.15.114.131:8080/api/inverter/write";
};

// Internal helper (hidden from main)
bool post_json(const std::string& url, const std::string& apiKey,
               const std::string& frameHex, std::string& outFrameHex);

#endif
