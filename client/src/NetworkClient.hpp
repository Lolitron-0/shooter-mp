#pragma once
#include "Typedefs.hpp"
#include "steam/steamnetworkingtypes.h"
#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <raylib.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>
#include <thread>

using json = nlohmann::json;

namespace smp::network
{

class NetworkClient
{
public:
    NetworkClient();

    ~NetworkClient();

    void Run();

    void SetMessageCallback(const std::function<void(json&&)>& callback);

    auto
    ConnectToGameServer() -> std::future<json>;
    void FindFreeRoom(const std::string& entryPointIp);

    void SendMovement(IdType playerId, Vector2 nextPlayerCoords);
    void SendShoot(IdType shooterId, Vector2 target);

private:
    void SendMessage(const std::string& message);
    [[nodiscard]] auto
    RecieveMessage(HSteamNetConnection connection) -> std::optional<json>;
    void PollIncomingMessages();

    void
    OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

    static NetworkClient* s_CallbackInstance;
    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t* info);

    void PollConnectionStateChanges();

private:
    std::string m_GameServerAddr;

    std::unique_ptr<std::thread> m_PollingThread{ nullptr };
    ISteamNetworkingSockets* m_Interface{ nullptr };
    HSteamNetConnection m_Connection{ k_HSteamNetConnection_Invalid };
    bool m_Alive{ true };
    std::function<void(json&&)> m_MessageCallback{ [](json&&) {} };
};

} // namespace smp::network
