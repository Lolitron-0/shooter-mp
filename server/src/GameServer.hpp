#pragma once
#include "ServerBase.hpp"
#include "SessionOptions.hpp"
#include "Typedefs.hpp"
#include <cassert>
#include <chrono>
#include <entt/entt.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <sw/redis++/redis++.h>
#include <thread>
#include <unordered_map>

namespace smp::server
{

using namespace sw;

class GameServer : public ServerBase
{
public:
    GameServer(const std::string& redisHost, int32_t redisPort,
               game::SessionOptions options);

    virtual ~GameServer();

    void Run(const std::string& addrIpv4) override;
    void Stop();

private:
    void ProcessMessage(json&& messageJson);

    void SendMessageToAllClients(const json& message);

    void UpdateGameState(float frameTime);

    void PollIncomingMessages();

    void OnConnectionStatusChanged(
        SteamNetConnectionStatusChangedCallback_t* info) override;

    void RegisterSelfInRedis();

    void NotifyEntityDestruction(IdType id);

    [[nodiscard]] auto GetCurrentStateJson() const -> json;

private:
    static constexpr Vector2 s_PlayerSpawnPos{ 300, 300 };

    std::unique_ptr<redis::Redis> m_RedisClient;
    std::string m_Name;
    std::string m_Host;
    int32_t m_Port;

    std::unordered_map<HSteamNetConnection, IdType> m_ClientMap;
    HSteamNetPollGroup m_PollGroup{ k_HSteamNetPollGroup_Invalid };

    game::SessionOptions m_SessionOptions;
    entt::basic_registry<IdType> m_Registry;

    std::chrono::steady_clock::time_point m_TickStart;
};

} // namespace smp::server
