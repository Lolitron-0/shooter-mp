#pragma once
#include "SessionOptions.hpp"
#include "Typedefs.hpp"
#include "steam/steamnetworkingtypes.h"
#include <cassert>
#include <chrono>
#include <entt/entt.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>
#include <string>
#include <thread>
#include <unordered_map>

using json = nlohmann::json;

namespace smp::server
{

class GameServer
{
public:
    explicit GameServer(game::SessionOptions options);

    void Run(const std::string& addrIpv4);

private:
    void ProcessMessage(json&& messageJson);

    void SendMessageToConnection(HSteamNetConnection connection,
                                 const json& message);

    void SendMessageToAllClients(const json& message);

    void UpdateGameState(float frameTime);

    void PollIncomingMessages();

    void
    OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

    [[nodiscard]] auto GetCurrentStateJson() const -> json;

    static GameServer* s_CallbackInstance;
    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t* info);

    void PollConnectionStateChanges();

private:
    static constexpr float s_TickTimeSeconds{ 0.01667 };

    std::unordered_map<HSteamNetConnection, IdType> m_ClientMap;
    ISteamNetworkingSockets* m_Interface{ nullptr };
    HSteamListenSocket m_ListenSocket{ k_HSteamListenSocket_Invalid };
    HSteamNetPollGroup m_PollGroup{ k_HSteamNetPollGroup_Invalid };

    game::SessionOptions m_SessionOptions;
    entt::basic_registry<IdType> m_Registry;

    std::chrono::steady_clock::time_point m_TickStart;
    std::unique_ptr<std::thread> m_TickThread;
};

} // namespace smp::server
