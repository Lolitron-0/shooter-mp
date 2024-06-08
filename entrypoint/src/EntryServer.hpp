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
    explicit EntryServer(const std::string& redisHost, int32_t redisPort)
    {
        try
        {
            redis::ConnectionOptions options{};
            options.host = redisHost;
            options.port = redisPort;
            options.password = "mypassword"; // hehehe

            m_RedisClient = std::make_unique<redis::Redis>(options);
            m_RedisPollThread = std::make_unique<std::thread>(
                [this]()
                {
                    while (m_Alive)
                    {
                        try
                        {
                            std::lock_guard<std::mutex> mtxLock{
                                m_ServerMapMutex
                            };

                            m_AvailableServersMap.clear();

                            std::unordered_set<std::string> keys;
                            auto cursor{ 0LL };

                            while (m_Alive)
                            {
                                cursor = m_RedisClient->scan(
                                    cursor, std::inserter(keys, keys.begin()));

                                if (cursor == 0)
                                {
                                    break;
                                }
                            }

                            for (const auto& key : keys)
                            {
                                if (key.ends_with(".endpoint"))
                                {
                                    auto valueOpt{ m_RedisClient->get(key) };
                                    assert(valueOpt.has_value());

                                    json serverState =
                                        json::parse(valueOpt.value());
                                    m_AvailableServersMap[key] = serverState;
                                }
                            }
                        }
                        catch (const redis::Error& error)
                        {
                            std::cerr << error.what() << std::endl;
                        }

                        std::this_thread::sleep_for(std::chrono::seconds{ 1 });
                    }
                });
        }
        catch (const redis::Error& error)
        {
            std::cerr << error.what() << std::endl;
        }
    }

    virtual ~EntryServer()
    {
        m_Alive = false;
        m_RedisPollThread->join();
    }

    void Run(const std::string& addrIpv4) override
    {
        InitConnection(addrIpv4);

        while (m_Alive)
        {
            PollConnectionStateChanges();
        }
    }

private:
    void OnConnectionStatusChanged(
        SteamNetConnectionStatusChangedCallback_t* info) override
    {
        switch (info->m_info.m_eState)
        {
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        {
            m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);

            std::cout << "Bro left to connect to room: "
                      << std::string{ info->m_info.m_szConnectionDescription }
                      << std::endl;
            break;
        }
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);

            std::cout << "Bro left with no game: "
                      << std::string{ info->m_info.m_szConnectionDescription }
                      << std::endl;
            break;
        }
        case k_ESteamNetworkingConnectionState_Connecting:
        {
            std::cout << "New bro wants to play game: "
                      << std::string{ info->m_info.m_szConnectionDescription }
                      << std::endl;

            if (m_Interface->AcceptConnection(info->m_hConn) != k_EResultOK)
            {
                m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);
                std::cout << "Could not accept connection\n";
                break;
            }

            // find room or die
            while (m_Alive)
            {
                m_ServerMapMutex.lock();

                auto minPlayersIt{ m_AvailableServersMap.end() };
                int32_t minPlayerCount{ std::numeric_limits<int32_t>::max() };

                for (auto it{ m_AvailableServersMap.begin() };
                     it != m_AvailableServersMap.end(); it++)
                {
                    // get the server-name.player_count value
                    auto token{ it->first.substr(0, it->first.find('.')) +
                                ".player_count" };
                    auto opt{ m_RedisClient->get(token) };

                    assert(opt.has_value());

                    auto playerCount{ std::stoi(opt.value()) };
                    if (playerCount < minPlayerCount)

                    {
                        minPlayersIt = it;
                        minPlayerCount = playerCount;
                    }
                }

                m_ServerMapMutex.unlock();

                if (minPlayersIt != m_AvailableServersMap.end())
                {
                    SendMessageToConnection(info->m_hConn,
                                            minPlayersIt->second);
                    break;
                }

                std::cout << "No suitable server :( Retrying...\n";
                std::this_thread::sleep_for(std::chrono::seconds{ 1 });
            }
            break;
        }
        case k_ESteamNetworkingConnectionState_Connected:
        case k_ESteamNetworkingConnectionState_None:
        {
            break;
        }
        }
    }

private:
    std::unique_ptr<std::thread> m_RedisPollThread{ nullptr };
    std::unordered_map<std::string, json> m_AvailableServersMap;
    std::unique_ptr<redis::Redis> m_RedisClient;
    std::mutex m_ServerMapMutex;
};

} // namespace smp::server
