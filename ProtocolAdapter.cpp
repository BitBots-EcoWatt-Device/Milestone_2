#include "ProtocolAdapter.h"
#include <curl/curl.h>
#include <iostream>
#include <vector>
#include <iomanip>

// ========== CURL callback ==========
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// ========== Modbus CRC-16 ===========
uint16_t calculateCRC(const std::vector<uint8_t> &data)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < data.size(); ++i)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// ========== Error Code Handling ==========
std::string modbusExceptionMessage(uint8_t code)
{
    switch (code)
    {
    case 0x01:
        return "Illegal Function (function not supported)";
    case 0x02:
        return "Illegal Data Address (address not valid)";
    case 0x03:
        return "Illegal Data Value (value out of range)";
    case 0x04:
        return "Slave Device Failure";
    case 0x05:
        return "Acknowledge (request accepted, processing delayed)";
    case 0x06:
        return "Slave Device Busy";
    case 0x08:
        return "Memory Parity Error";
    case 0x0A:
        return "Gateway Path Unavailable";
    case 0x0B:
        return "Gateway Target Device Failed to Respond";
    default:
        return "Unknown Modbus Exception";
    }
}

// ========== Post JSON ==========
bool post_json(const std::string &url, const std::string &apiKey,
               const std::string &frameHex, std::string &outFrameHex)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return false;

    std::string readBuffer;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: " + apiKey).c_str());

    std::string body = "{\"frame\":\"" + frameHex + "\"}";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK)
        return false;

    auto pos = readBuffer.find("\"frame\":\"");
    if (pos == std::string::npos)
        return false;
    pos += 9;
    auto end = readBuffer.find("\"", pos);
    if (end == std::string::npos)
        return false;
    outFrameHex = readBuffer.substr(pos, end - pos);

    // CRC check for received frame
    if (outFrameHex.size() < 8)
        return false; // too short for CRC
    std::vector<uint8_t> frameBytes;
    for (size_t i = 0; i < outFrameHex.size(); i += 2)
    {
        frameBytes.push_back(static_cast<uint8_t>(std::stoi(outFrameHex.substr(i, 2), nullptr, 16)));
    }
    if (frameBytes.size() < 4)
        return false;
    uint16_t receivedCRC = frameBytes[frameBytes.size() - 2] | (frameBytes[frameBytes.size() - 1] << 8);
    uint16_t calcCRC = calculateCRC(std::vector<uint8_t>(frameBytes.begin(), frameBytes.end() - 2));
    if (receivedCRC != calcCRC)
    {
        std::cerr << "CRC error: received " << std::hex << receivedCRC << ", calculated " << calcCRC << std::dec << std::endl;
        return false;
    }

    // Modbus error code handling
    if (frameBytes.size() >= 5 && (frameBytes[1] & 0x80))
    {
        uint8_t excCode = frameBytes[2];
        std::cerr << "Modbus Exception: Code 0x" << std::hex << (int)excCode << ": " << modbusExceptionMessage(excCode) << std::dec << std::endl;
        return false;
    }

    return true;
}

// ========== ProtocolAdapter methods ==========
ProtocolAdapter::ProtocolAdapter(const std::string &apiKey) : apiKey_(apiKey) {}

bool ProtocolAdapter::sendReadRequest(const std::string &frameHex, std::string &outFrameHex)
{
    return post_json(readURL, apiKey_, frameHex, outFrameHex);
}

bool ProtocolAdapter::sendWriteRequest(const std::string &frameHex, std::string &outFrameHex)
{
    return post_json(writeURL, apiKey_, frameHex, outFrameHex);
}
