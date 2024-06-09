#pragma once
#include "ServerBase.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <sw/redis++/redis++.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace smp::server
{

using namespace sw;

class EntryServer : public ServerBase
{
public:
    EntryServer(const std::string& redisHost, int32_t redisPort);

    virtual ~EntryServer();

    void Run(const std::string& addrIpv4) override;

private:
    void OnConnectionStatusChanged(
        SteamNetConnectionStatusChangedCallback_t* info) override;

private:
    std::unique_ptr<std::thread> m_RedisPollThread{ nullptr };
    std::unordered_map<std::string, json> m_AvailableServersMap;
    std::unique_ptr<redis::Redis> m_RedisClient;
    std::mutex m_ServerMapMutex;
};

} // namespace smp::server
