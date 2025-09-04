#include "ProtocolAdapter.h"
#include <curl/curl.h>
#include <iostream>

// ========== CURL callback ==========
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ========== Post JSON ==========
bool post_json(const std::string& url, const std::string& apiKey,
               const std::string& frameHex, std::string& outFrameHex) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string readBuffer;
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: " + apiKey).c_str());

    std::string body = "{\"frame\":\"" + frameHex + "\"}";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    // Set timeout for CURL request (5 seconds)
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << "\n";
        std::cerr << "Request body: " << body << "\n";
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return false;
    }

    auto pos = readBuffer.find("\"frame\":\"");
    if (pos == std::string::npos) {
        std::cerr << "Response missing 'frame' field. Response: " << readBuffer << "\n";
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return false;
    }
    pos += 9;
    auto end = readBuffer.find("\"", pos);
    if (end == std::string::npos) {
        std::cerr << "Malformed 'frame' field in response. Response: " << readBuffer << "\n";
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return false;
    }
    outFrameHex = readBuffer.substr(pos, end - pos);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return true;
}

// ========== ProtocolAdapter methods ==========
ProtocolAdapter::ProtocolAdapter(const std::string& apiKey) : apiKey_(apiKey) {}

bool ProtocolAdapter::sendReadRequest(const std::string& frameHex, std::string& outFrameHex) {
    int attempts = 0;
    while (attempts < 3) {
        if (post_json(readURL, apiKey_, frameHex, outFrameHex)) {
            return true;
        } else {
            std::cerr << "sendReadRequest failed (attempt " << (attempts+1) << ")\n";
        }
        attempts++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // brief backoff
    }
    return false;
}

bool ProtocolAdapter::sendWriteRequest(const std::string& frameHex, std::string& outFrameHex) {
    int attempts = 0;
    while (attempts < 3) {
        if (post_json(writeURL, apiKey_, frameHex, outFrameHex)) {
            return true;
        } else {
            std::cerr << "sendWriteRequest failed (attempt " << (attempts+1) << ")\n";
        }
        attempts++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // brief backoff
    }
    return false;
}
