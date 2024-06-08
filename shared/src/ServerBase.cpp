#include "ServerBase.hpp"
#include "steam/steamnetworkingtypes.h"
#include <iostream>

namespace smp::server
{

ServerBase* ServerBase::s_CallbackInstance{ nullptr };

void ServerBase::InitConnection(const std::string& addrIpv4)
{
    m_Interface = SteamNetworkingSockets();

    SteamNetworkingIPAddr serverLocalAddr{};
    serverLocalAddr.Clear();
    serverLocalAddr.ParseString(addrIpv4.c_str());
    SteamNetworkingConfigValue_t opt{};

    opt.SetPtr(
        k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
        reinterpret_cast<void*>(SteamNetConnectionStatusChangedCallback));

    m_ListenSocket =
        m_Interface->CreateListenSocketIP(serverLocalAddr, 1, &opt);

    if (m_ListenSocket == k_HSteamListenSocket_Invalid)
    {
        std::cerr << "Failed to listen on " << addrIpv4 << '\n';
    }

    std::cout << "Listening on " << addrIpv4 << '\n';
}

void ServerBase::SendMessageToConnection(HSteamNetConnection connection,
                                         const json& message)
{
    auto messageString{ message.dump() };
    m_Interface->SendMessageToConnection(
        connection, messageString.c_str(), messageString.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);
}

void ServerBase::SteamNetConnectionStatusChangedCallback(
    SteamNetConnectionStatusChangedCallback_t* info)
{
    s_CallbackInstance->OnConnectionStatusChanged(info);
}
void ServerBase::PollConnectionStateChanges()
{
    s_CallbackInstance = this;
    m_Interface->RunCallbacks();
}
ServerBase::~ServerBase()
{
    std::cout << "Shutting down...\n"; // logging this way is not the best i
                                       // guess, but enough for such scale

    if (m_Interface != nullptr)
    {
        m_Interface->CloseListenSocket(m_ListenSocket);
    }
    m_ListenSocket = k_HSteamListenSocket_Invalid;
}
} // namespace smp::server
