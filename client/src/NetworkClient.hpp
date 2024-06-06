#pragma once
#include "Typedefs.hpp"
#include "entt/entity/entity.hpp"
#include "steam/steamnetworkingtypes.h"
#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <raylib.h>
#include <stdexcept>
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

    auto Connect(const std::string& addrString) -> std::future<json>;

    void SendMovement(IdType playerId, Vector2 nextPlayerCoords);
    void SendShoot(IdType shooterId, Vector2 target);

private:
    void SendMessage(const std::string& message);
    auto RecieveMessage() -> std::optional<json>;
    void PollIncomingMessages();

    void
    OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

    static NetworkClient* s_CallbackInstance;
    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t* info);

    void PollConnectionStateChanges();

private:
    std::unique_ptr<std::thread> m_PollingThread{ nullptr };
    ISteamNetworkingSockets* m_Interface{ nullptr };
    HSteamNetConnection m_Connection{ k_HSteamNetConnection_Invalid };
    bool m_Alive{ false };
    std::function<void(json&&)> m_MessageCallback{ [](json&&) {} };
};

} // namespace smp::network
