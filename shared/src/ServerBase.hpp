#pragma once
#include <nlohmann/json.hpp>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

namespace smp::server
{

using json = nlohmann::json;

class ServerBase
{
public:
    ~ServerBase();
    virtual void Run(const std::string& addrIpv4) = 0;

protected:
    void InitConnection(const std::string& addrIpv4);

    void SendMessageToConnection(HSteamNetConnection connection,
                                 const json& message);

    virtual void OnConnectionStatusChanged(
        SteamNetConnectionStatusChangedCallback_t* info) = 0;

    static ServerBase* s_CallbackInstance;
    static void SteamNetConnectionStatusChangedCallback(
        SteamNetConnectionStatusChangedCallback_t* info);

    void PollConnectionStateChanges();

protected:
    ISteamNetworkingSockets* m_Interface{ nullptr };
    HSteamListenSocket m_ListenSocket{ k_HSteamListenSocket_Invalid };
    bool m_Alive{ true };
};

} // namespace smp::server
