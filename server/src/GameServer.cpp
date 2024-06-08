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

GameServer::GameServer(const std::string& redisHost, int32_t redisPort,
                       game::SessionOptions options)
    : m_Name(options.Name),
      m_SessionOptions{ std::move(options) }
{
    try
    {
        redis::ConnectionOptions connOptions{};
        connOptions.host = redisHost;
        connOptions.port = redisPort;
        connOptions.password = "mypassword"; // hehehe

        m_RedisClient = std::make_unique<redis::Redis>(connOptions);
    }
    catch (const redis::Error& error)
    {
        std::cerr << error.what() << std::endl;
    }

    for (auto& wall : m_SessionOptions.Walls)
    {
        wall.Id = m_Registry.create();
        m_Registry.emplace<game::LineCollider>(wall.Id, wall.Collider);
    }
}

GameServer::~GameServer()
{
    m_Alive = false;
    if (m_Interface)
    {
        m_Interface->DestroyPollGroup(m_PollGroup);
    }
    m_PollGroup = k_HSteamNetPollGroup_Invalid;
    m_TickThread->join();

    m_RedisClient->del(m_Name + ".endpoint");
    m_RedisClient->del(m_Name + ".player_count");
}

void GameServer::RegisterSelfInRedis()
{
    json serverInfo = { { "ip", m_Host }, { "port", m_Port } };
    m_RedisClient->set(m_Name + ".endpoint", serverInfo.dump());
    m_RedisClient->set(m_Name + ".player_count", "0");
}

void GameServer::Run(const std::string& addrIpv4)
{
    InitConnection(addrIpv4);

    auto colonIdx{ addrIpv4.find(':') };
    m_Host = addrIpv4.substr(0, colonIdx);
    m_Port = std::stoi(addrIpv4.substr(colonIdx + 1));
    RegisterSelfInRedis();

    m_PollGroup = m_Interface->CreatePollGroup();
    if (m_PollGroup == k_HSteamNetPollGroup_Invalid)
    {
        std::cerr << "Failed to listen on " << addrIpv4 << '\n';
    }

    m_TickStart = std::chrono::steady_clock::now();

    m_TickThread = std::make_unique<std::thread>(
        [this]()
        {
            while (m_Alive)
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

    while (m_Alive)
    {

        PollIncomingMessages();
        PollConnectionStateChanges();
    }
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
                NotifyEntityDestruction(bullet);
                m_Registry.destroy(bullet);
                NotifyEntityDestruction(player);
                // player will be actually destroyed on disconnect
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
    while (m_Alive)
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

void GameServer::NotifyEntityDestruction(IdType id)
{
    json destroyMessage = { { "type", "destroy" },
                            { "payload", { { "id", id } } } };
    SendMessageToAllClients(destroyMessage);
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
        auto playerId{ m_ClientMap[info->m_hConn] };
        m_Registry.destroy(playerId);

        m_ClientMap.erase(info->m_hConn);
        m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);

        m_RedisClient->decr(m_Name + ".player_count");

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
        greetingJson["payload"]["player_x"] = s_PlayerSpawnPos.x;
        greetingJson["payload"]["player_y"] = s_PlayerSpawnPos.y;
        SendMessageToConnection(info->m_hConn,
                                greetingJson); // we just need id for greeting

        // for simplicity spawn is fixed
        m_Registry.emplace<game::CircleCollider>(newPlayerId, s_PlayerSpawnPos,
                                                 m_SessionOptions.PlayerRadius);
        m_Registry.emplace<game::PlayerTag>(newPlayerId);
        json newConnectionJson = { { "type", "connection" },
                                   { "payload",
                                     {
                                         { "id", newPlayerId },
                                         { "x", s_PlayerSpawnPos.x },
                                         { "y", s_PlayerSpawnPos.y },
                                     } } };
        SendMessageToAllClients(newConnectionJson);

        m_ClientMap[info->m_hConn] = newPlayerId;
        m_RedisClient->incr(m_Name + ".player_count");
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
} // namespace smp::server
