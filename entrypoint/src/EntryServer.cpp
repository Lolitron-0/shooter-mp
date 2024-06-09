#include "EntryServer.hpp"

namespace smp::server
{

EntryServer::EntryServer(const std::string& redisHost, int32_t redisPort)
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
                        std::lock_guard<std::mutex> mtxLock{ m_ServerMapMutex };

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
                            auto dotIdx{ key.find('.') };
                            auto serverName{ key.substr(0, dotIdx) };
                            auto keyType{ key.substr(dotIdx + 1) };

                            auto valueOpt{ m_RedisClient->get(key) };
                            assert(valueOpt.has_value());

                            m_AvailableServersMap[serverName];
                            if (keyType == "player_count")
                            {
                                m_AvailableServersMap[serverName]
                                                     ["player_count"] =
                                                         std::stoi(
                                                             valueOpt.value());
                            }
                            else
                            {
                                m_AvailableServersMap[serverName].merge_patch(
                                    json::parse(valueOpt.value()));
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
EntryServer::~EntryServer()
{
    m_Alive = false;
    m_RedisPollThread->join();
}
void EntryServer::Run(const std::string& addrIpv4)
{
    InitConnection(addrIpv4);

    while (m_Alive)
    {
        PollConnectionStateChanges();
        std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }
}
void EntryServer::OnConnectionStatusChanged(
    SteamNetConnectionStatusChangedCallback_t* info)
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

            auto suitableServerIt{ std::min_element(
                m_AvailableServersMap.begin(), m_AvailableServersMap.end(),
                [](auto a, auto b)
                {
                    std::cout
                        << (a.second["player_count"].template get<int32_t>() <
                            b.second["player_count"].template get<int32_t>());
                    return a.second["player_count"].template get<int32_t>() <
                           b.second["player_count"].template get<int32_t>();
                }) };

            if (suitableServerIt != m_AvailableServersMap.end())
            {
                SendMessageToConnection(info->m_hConn,
                                        suitableServerIt->second);
                m_ServerMapMutex.unlock();
                break;
            }

            m_ServerMapMutex.unlock();

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
} // namespace smp::server
