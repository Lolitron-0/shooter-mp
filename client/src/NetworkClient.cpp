#include "NetworkClient.hpp"
#include "Typedefs.hpp"
#include <raylib.h>
#include <raymath.h>

namespace smp::network
{
NetworkClient* NetworkClient::s_CallbackInstance{ nullptr };
NetworkClient::NetworkClient()
    : m_Interface(SteamNetworkingSockets())
{
}
NetworkClient::~NetworkClient()
{
    m_Alive = false;
    m_Interface->CloseConnection(
        m_Connection, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);
    m_PollingThread->join();
}
void NetworkClient::Run()
{
    assert(m_Connection != k_HSteamNetConnection_Invalid);

    m_PollingThread = std::make_unique<std::thread>(
        [this]()
        {
            while (m_Alive)
            {
                PollIncomingMessages();
                PollConnectionStateChanges();
            }
        });
}
void NetworkClient::SetMessageCallback(
    const std::function<void(json&&)>& callback)
{
    m_MessageCallback = callback;
}
auto NetworkClient::ConnectToGameServer()
    -> std::future<json>
{
	assert(m_GameServerAddr != "");

    SteamNetworkingConfigValue_t opt{};

    opt.SetPtr(
        k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
        reinterpret_cast<void*>(SteamNetConnectionStatusChangedCallback));

    SteamNetworkingIPAddr serverAddr{};
    serverAddr.Clear();
    serverAddr.ParseString(m_GameServerAddr.c_str());
    m_Connection = m_Interface->ConnectByIPAddress(serverAddr, 1, &opt);
    if (m_Connection == k_HSteamNetConnection_Invalid)
    {
        throw std::runtime_error{ "Failed to connect to server" };
    }

    auto playerIdFuture{ std::async(
        std::launch::async,
        [this]()
        {
            while (m_Alive)
            {
                auto messageOpt{ RecieveMessage(m_Connection) };

                if (messageOpt.has_value())
                {
                    // ideally we'll need to recieve our ip and check if
                    // this greeting is ours, but we have no client-specific
                    // info
                    auto type{
                        messageOpt.value()["type"].template get<std::string>()
                    };
                    if (type != "greeting")
                    {
                        continue;
                    }

                    return messageOpt.value();
                }
            }
            return json{};
        }) };
    return playerIdFuture;
}

void NetworkClient::FindFreeRoom(const std::string& entryPointIp)
{
    std::cout << "Connecting to entrypoint\n";

    SteamNetworkingIPAddr entryPointAddr{};
    entryPointAddr.Clear();
    entryPointAddr.ParseString(entryPointIp.c_str());

    SteamNetworkingConfigValue_t opt{};
    opt.SetPtr(
        k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
        reinterpret_cast<void*>(SteamNetConnectionStatusChangedCallback));

    auto connection{ m_Interface->ConnectByIPAddress(entryPointAddr, 1, &opt) };
    assert(connection != k_HSteamNetConnection_Invalid);

    std::cout << "Searching for a free room...\n";
    while (m_Alive)
    {
        auto messageOpt{ RecieveMessage(connection) };

        if (!messageOpt.has_value())
        {
            continue;
        }

        std::cout << messageOpt.value() << std::endl;
        auto host{ messageOpt.value()["ip"].template get<std::string>() };
        auto port{ messageOpt.value()["port"].template get<int32_t>() };
        m_GameServerAddr = host + ":" + std::to_string(port);
        m_Interface->CloseConnection(connection, 0, nullptr, false);
        return;
    }
}

void NetworkClient::SendMovement(IdType playerId, Vector2 nextPlayerCoords)
{
    // not quite thread safe, but we are in one for now
    static Vector2 lastPos{ nextPlayerCoords };

    if (Vector2Equals(lastPos, nextPlayerCoords) != 0)
    {
        return;
    }

    lastPos = nextPlayerCoords;

    json data = { { "type", "coords" },
                  { "payload",
                    {
                        { "id", playerId },
                        { "x", nextPlayerCoords.x },
                        { "y", nextPlayerCoords.y },
                    } } };

    SendMessage(data.dump());
}
void NetworkClient::SendShoot(IdType shooterId, Vector2 target)
{
    json data = { { "type", "shoot" },
                  { "payload",
                    {
                        { "shooter_id", shooterId },
                        { "target_x", target.x },
                        { "target_y", target.y },
                    } } };

    SendMessage(data.dump());
}
void NetworkClient::SendMessage(const std::string& message)
{
    m_Interface->SendMessageToConnection(
        m_Connection, message.c_str(), message.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);
}
auto NetworkClient::RecieveMessage(HSteamNetConnection connection)
    -> std::optional<json>
{
    ISteamNetworkingMessage* incomingMessage{ nullptr };
    auto numMessages{ m_Interface->ReceiveMessagesOnConnection(
        connection, &incomingMessage, 1) };

    if (numMessages == 0)
    {
        return std::nullopt;
    }
    if (numMessages < 0)
    {
        throw std::runtime_error{ "Error polling message\n" };
    }

    assert(numMessages && incomingMessage);

    std::string messageString{};
    messageString.assign(static_cast<const char*>(incomingMessage->m_pData),
                         incomingMessage->m_cbSize);
    incomingMessage->Release();

    json messageJson = json::parse(messageString);
    return messageJson;
}
void NetworkClient::PollIncomingMessages()
{
    while (m_Alive /* have pending messages */)
    {
        auto messageOpt{ RecieveMessage(m_Connection) };

        if (!messageOpt.has_value()) // no new messages
        {
            break;
        }

        m_MessageCallback(std::move(messageOpt.value()));
    }
}
void NetworkClient::OnConnectionStatusChanged(
    SteamNetConnectionStatusChangedCallback_t* info)
{
    // todo
}
void NetworkClient::SteamNetConnectionStatusChangedCallback(
    SteamNetConnectionStatusChangedCallback_t* info)
{
    s_CallbackInstance->OnConnectionStatusChanged(info);
}
void NetworkClient::PollConnectionStateChanges()
{
    s_CallbackInstance = this;
    m_Interface->RunCallbacks();
}
} // namespace smp::network
