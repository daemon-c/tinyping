/**
* @author Christian Valdez
* @file tinyping.cpp
* @brief portable c++ uptime notification program. 
*/

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include "icmplib.h"
#include <string>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>

/**
* @brief macro definition for os check. In the event user compiles for their given OS.
* @return will return file path for given OS.
*/

// adding support for windows now but the library handling icmp notes errors for windows. I might open a pull request for this.
// https://github.com/markondej/cpp-icmplib/blob/master/LICENSE

/**
 * @brief macros to determine OS compiled for. 
 * @return returns proper path for OS
 */
std::filesystem::path getDataDirectory()
{
    #if defined(_WIN32)
        const char* programData = std::getenv("ProgramData");

        if (programData == nullptr || programData[0] == '\0')
            throw std::runtime_error("ProgramData is not set");

        return std::filesystem::path(programData) / "tinyping";

    #elif defined(__APPLE__)
        return "/Library/Application Support/tinyping";

    #elif defined(__linux__)
        return "/var/opt/tinyping";
    #else 
        #error Unsupported operating system
    #endif
}

void sendNtfyHTTP(const std::string& topic, const std::string& message, const std::string& title = "")
{
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        std::string url = "https://ntfy.sh/" + topic;

        // set target URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // set post data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message.c_str());

        // skipping optional headers like title and priority

        // perform the request
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "Error: CURL failed to perform the request: " << curl_easy_strerror(res) << std::endl;
        }

        // clean up required by curl
        curl_easy_cleanup(curl);
        // no header contents to cleanup
    }
    curl_global_cleanup();
}

void pingRunner(std::unordered_map<std::string, bool> hostStatus, std::vector<std::string> hosts, int TTL, std::string topic)
{
    for (const std::string& host : hosts)
    {
        bool& lastPingGood = hostStatus.at(host);

        // sends out ping, can use its response to do other things

        auto ping = icmplib::Ping(host, ICMPLIB_TIMEOUT_1S, 1, TTL);
        
        switch (ping.response)
        {
            case icmplib::PingResult::ResponseType::TimeExceeded:
                if (lastPingGood)
                {
                    sendNtfyHTTP(topic, "Host " + host + " is offline.", "tinyping: Alert");
                    lastPingGood = false;
                }
                break;
            case icmplib::PingResult::ResponseType::Success:
                if (lastPingGood == true)
                {
                    // do nothing as last ping was good. since still good, do nothing
                    break;
                }
                else
                {
                    // last ping was not good, send update notification
                    sendNtfyHTTP(topic, "Host " + host + " is back online.", "tinyping: Recovery");
                    lastPingGood = true;
                }
                break;
            default:
                if (lastPingGood)
                {
                    // blanket case, set hosts last ping to bad
                    sendNtfyHTTP(topic, "Host " + host + " is unreachable (Unknown Error).", "tinyping: Alert");
                    lastPingGood = false;
                }
                break;
        }
    }

    // keeps cpu from maxing out, sends out pings every # of sec defined
    // default 10 sec
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

void configurationCheck(const auto filePath)
{
    std::ofstream file(filePath, std::ios::app);

    if (!file)
    {
        throw std::runtime_error(
            "Could not open or create: " + filePath.string()
        );
    }

    if (std::filesystem::is_empty(filePath))
    {
        file << "# Enter IP addresses or hostnames below.\n"
            << "# One IP or hostname per line.\n"
            << "# Set your ntfy topic after the > symbol.\n"
            << "> enter_ntfy_topic_here\n"
            << "8.8.8.8\n";
    }
    
}

std::ifstream fileHelper(const auto filePath)
{
    std::ifstream file(filePath);
    if (!file)
    {
        throw std::runtime_error(
            "Could not read: " + filePath.string()
        );
        EXIT_FAILURE;
    }
    return file;
}

auto trim = [](const std::string& text) -> std::string
{
    const auto first = text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
    {
        return "";
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
};

void fileTrim(std::ifstream &file, std::string line, std::string topic, std::vector<std::string> hosts)
{
    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        if (line[0] == '>')
        {
            topic = trim(line.substr(1));
        }
        else
        {
            hosts.push_back(line);
        }
    }
}

int main()
{
    // dr.google says 64 is normal ttl for linux
    int TTL = 64;

    // upon first run, check for config file in path
    const auto dataDirectory = getDataDirectory();
    std::filesystem::create_directories(dataDirectory);

    const auto filePath = dataDirectory / "tinyping.conf";

    // create configuration if it does not exist
    configurationCheck(filePath);
    std::ifstream file = fileHelper(filePath);
    std::vector<std::string> hosts;
    std::string topic;
    std::string line;

    // trim the file down. look for unexpected white spaces
    fileTrim(file, line, topic, hosts);
    

    
    if (topic.empty() || topic == "enter_ntfy_topic_here")
    {
        std::cerr << "Set your ntfy topic in: " << filePath << '\n';
        return 1;
    }

    if (hosts.empty())
    {
        std::cerr << "No hosts configured in: " << filePath << '\n';
        return 1;
    }

    // using unordered map to set status for each host prior to main loop.
    // causes incorrect ping messages to be sent out at start of program
    std::unordered_map<std::string, bool> hostStatus;
    for (const auto& host : hosts)
    {
        hostStatus[host] = true;
    }
    

    // loop over ping targets in config file
    while (true)
    {
        pingRunner(hostStatus, hosts, TTL, topic);
    }
}
