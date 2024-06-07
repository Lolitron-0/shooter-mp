#include "GameServer.hpp"
#include "Components.hpp"
#include "Typedefs.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <thread>
#include <utility>

namespace smp::server
{

GameServer* GameServer::s_CallbackInstance{ nullptr };

GameServer::GameServer(game::SessionOptions options)
    : m_SessionOptions{ std::move(options) }
{
    for (auto& wall : m_SessionOptions.Walls)
    {
        wall.Id = m_Registry.create();
        m_Registry.emplace<game::LineCollider>(wall.Id, wall.Collider);
    }
}

void GameServer::Run(const std::string& addrIpv4)
{
    m_TickStart = std::chrono::steady_clock::now();
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
    m_PollGroup = m_Interface->CreatePollGroup();
    if (m_PollGroup == k_HSteamNetPollGroup_Invalid)
    {
        std::cerr << "Failed to listen on " << addrIpv4 << '\n';
    }

    std::cout << "Listening on " << addrIpv4 << '\n';

    m_TickThread = std::make_unique<std::thread>(
        [this]()
        {
            while (true)
            {
                auto now{ std::chrono::steady_clock::now() };
                // in seconds
                std::chrono::duration<float> frameTime{ now - m_TickStart };

                // tickrate 60Hz
                if (frameTime.count() < s_TickTimeSeconds)
                {
                    continue;
                }

                m_TickStart = std::chrono::steady_clock::now();

                UpdateGameState(frameTime.count());
            }
        });

    while (true)
    {

        PollIncomingMessages();
        PollConnectionStateChanges();
    }

    std::cout << "Shutting down...\n";

    m_Interface->CloseListenSocket(m_ListenSocket);
    m_ListenSocket = k_HSteamListenSocket_Invalid;

    m_Interface->DestroyPollGroup(m_PollGroup);
    m_PollGroup = k_HSteamNetPollGroup_Invalid;
}
void GameServer::ProcessMessage(json&& messageJson)
{
    auto type{ messageJson["type"].template get<std::string>() };
    auto payload = messageJson["payload"];

    if (type == "coords")
    {
        m_Registry.patch<game::CircleCollider>(
            payload["id"].template get<IdType>(),
            [payload](auto& collider)
            {
                collider.SetPosition({ payload["x"].template get<float>(),
                                       payload["y"].template get<float>() });
            });
        SendMessageToAllClients(messageJson);
    }
    else if (type == "shoot")
    {
        auto bulletId{ m_Registry.create() };

        auto shooterPos{ m_Registry
                             .get<game::CircleCollider>(
                                 payload["shooter_id"].template get<IdType>())
                             .GetPosition() };

        Vector2 targetVec{ payload["target_x"].template get<float>(),
                           payload["target_y"].template get<float>() };
        targetVec = Vector2Normalize(targetVec);
        auto bulletPos{ Vector2Add(
            shooterPos,
            Vector2Scale(targetVec, m_SessionOptions.PlayerRadius)) };
        targetVec = Vector2Scale(targetVec, m_SessionOptions.BulletSpeed);

        auto& newBulletCollider{ m_Registry.emplace<game::CircleCollider>(
            bulletId, bulletPos, m_SessionOptions.BulletRadius) };
        newBulletCollider.SetVelocity(targetVec);

        // send shoot event to everyone
        messageJson["payload"]["bullet_id"] = bulletId;
        // shift initial bullet pos just for fun
        messageJson["payload"]["bullet_x"] = bulletPos.x;
        messageJson["payload"]["bullet_y"] = bulletPos.y;
        SendMessageToAllClients(messageJson);

        // include it in tick cycle only after creation message has been sent
        m_Registry.emplace<game::BulletTag>(bulletId);
    }
}
void GameServer::SendMessageToConnection(HSteamNetConnection connection,
                                         const json& message)
{
    auto messageString{ message.dump() };
    m_Interface->SendMessageToConnection(
        connection, messageString.c_str(), messageString.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);
}
void GameServer::SendMessageToAllClients(const json& message)
{
    for (auto& pair : m_ClientMap)
    {
        SendMessageToConnection(pair.first, message);
    }
}

void GameServer::UpdateGameState(float frameTime)
{
    auto bulletsView{ m_Registry.view<game::BulletTag>() };
    auto wallsView{ m_Registry.view<game::LineCollider>() };
    auto playersView{ m_Registry.view<game::PlayerTag>() };
    json coordsMessage;

    for (const auto& bullet : bulletsView)
    {
        auto& bulletCollider{ m_Registry.get<game::CircleCollider>(bullet) };

        for (const auto& wall : wallsView)
        {
            auto& wallCollider{ m_Registry.get<game::LineCollider>(wall) };
            auto collided{ game::collider::CollideCircleLine(
                bulletCollider, wallCollider, frameTime) };

            if (collided)
            {
                json destroyMessage = { { "type", "destroy" },
                                        { "payload", { { "id", bullet } } } };
                SendMessageToAllClients(destroyMessage);
                m_Registry.destroy(bullet);
                goto skip_iter; // i think goto is cleaner than
                                // break-flag-continue
            }
        }

        for (const auto& player : playersView)
        {
            auto& playerCollider{ m_Registry.get<game::CircleCollider>(
                player) };
            auto collided{ game::collider::CollideCircles(
                playerCollider, bulletCollider, frameTime) };

            if (collided)
            {
                json destroyMessage = { { "type", "destroy" },
                                        { "payload", { { "id", bullet } } } };
                SendMessageToAllClients(destroyMessage);
                destroyMessage = { { "type", "destroy" },
                                   { "payload", { { "id", player } } } };
                SendMessageToAllClients(destroyMessage);

                m_Registry.destroy(bullet);
                m_Registry.destroy(player);
                goto skip_iter;
            }
        }

        bulletCollider.SetPosition(bulletCollider.GetNextPosition(frameTime));
        // send coords message
        coordsMessage = { { "type", "coords" },
                          { "payload",
                            {
                                { "id", bullet },
                                { "x", bulletCollider.GetPosition().x },
                                { "y", bulletCollider.GetPosition().y },
                            } } };
        SendMessageToAllClients(coordsMessage);
    skip_iter:;
    }
}

void GameServer::PollIncomingMessages()
{
    while (true)
    {
        ISteamNetworkingMessage* incomingMessage{ nullptr };
        auto numMessages{ m_Interface->ReceiveMessagesOnPollGroup(
            m_PollGroup, &incomingMessage, 1) };

        if (numMessages == 0)
        {
            break;
        }
        if (numMessages < 0)
        {
            std::cerr << "Error polling message\n";
        }

        assert(numMessages && incomingMessage);

        std::string messageString;
        messageString.assign(static_cast<const char*>(incomingMessage->m_pData),
                             incomingMessage->m_cbSize);

        incomingMessage->Release();

        json messageJson = json::parse(messageString);

        ProcessMessage(std::move(messageJson));
    }
}
void GameServer::OnConnectionStatusChanged(
    SteamNetConnectionStatusChangedCallback_t* info)
{
    switch (info->m_info.m_eState)
    {
    case k_ESteamNetworkingConnectionState_None:
    {
        break;
    }
    case k_ESteamNetworkingConnectionState_ClosedByPeer:
    case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
    {
        m_Registry.destroy(m_ClientMap[info->m_hConn]);
        m_ClientMap.erase(info->m_hConn);
        m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);
        std::cout << "Disconnected this one: "
                  << std::string{ info->m_info.m_szConnectionDescription }
                  << '\n';
        break;
    }
    case k_ESteamNetworkingConnectionState_Connecting:
    {
        assert(m_ClientMap.count(info->m_hConn) == 0);

        std::cout << "Connecting this guy: "
                  << std::string{ info->m_info.m_szConnectionDescription }
                  << '\n';

        if (m_Interface->AcceptConnection(info->m_hConn) != k_EResultOK)
        {
            m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);
            std::cout << "Could not accept connection\n";
            break;
        }

        if (!m_Interface->SetConnectionPollGroup(info->m_hConn, m_PollGroup))
        {
            m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);
            std::cout << "Could not assign to poll group\n";
            break;
        }

        json greetingJson = { { "type", "greeting" },
                              { "payload", GetCurrentStateJson() } };

        auto newPlayerId{ m_Registry.create() };

        greetingJson["payload"]["player_id"] = newPlayerId;
        SendMessageToConnection(info->m_hConn,
                                greetingJson); // we just need id for greeting

        m_Registry.emplace<game::CircleCollider>(newPlayerId, Vector2{ 0, 0 },
                                                 m_SessionOptions.PlayerRadius);
        m_Registry.emplace<game::PlayerTag>(newPlayerId);
        json newConnectionJson = { { "type", "connection" },
                                   { "payload",
                                     {
                                         { "id", newPlayerId },
                                     } } };
        SendMessageToAllClients(newConnectionJson);

        m_ClientMap[info->m_hConn] = newPlayerId;
        std::cout << "Successful connection. Player id: " << newPlayerId
                  << '\n';
        break;
    }
    case k_ESteamNetworkingConnectionState_Connected:
    {
        break;
    }
    }
}
auto GameServer::GetCurrentStateJson() const -> json
{
    auto playersView{ m_Registry.view<game::PlayerTag>() };
    std::vector<json> players;
    for (auto entity : playersView)
    {
        const auto& playerCollider{ m_Registry.get<game::CircleCollider>(
            entity) };
        players.push_back({ { "id", entity },
                            { "x", playerCollider.GetPosition().x },
                            { "y", playerCollider.GetPosition().y } });
    }

    json state = m_SessionOptions.ToJSON();
    state["players"] = players;
    return state;
}
void GameServer::SteamNetConnectionStatusChangedCallback(
    SteamNetConnectionStatusChangedCallback_t* info)
{
    s_CallbackInstance->OnConnectionStatusChanged(info);
}
void GameServer::PollConnectionStateChanges()
{
    s_CallbackInstance = this;
    m_Interface->RunCallbacks();
}
} // namespace smp::server
