#include "Scene.hpp"
#include "Bullet.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Player.hpp"
#include "Typedefs.hpp"
#include "Wall.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <string>

namespace smp::game
{

Scene::Scene(std::unique_ptr<network::NetworkClient> networkClient)
    : m_NetworkClient{ std::move(networkClient) }
{
    auto gameStateFuture{ m_NetworkClient->ConnectToGameServer() };

    m_Registry = std::make_shared<Registry>();

    m_NetworkClient->SetMessageCallback(
        [this](json&& message)
        {
            std::scoped_lock<std::mutex> mtxLock{ m_MQMutex };
            m_MessageQueue.push_back(std::move(message));
        });

    json gameStateJson = gameStateFuture.get();
    std::cout << gameStateJson << std::endl;
    gameStateJson = gameStateJson["payload"];

    Vector2 spawnPos{ gameStateJson["player_x"].template get<float>(),
                      gameStateJson["player_y"].template get<float>() };
    AddMainPlayer(gameStateJson["player_id"].template get<IdType>(), spawnPos);
    for (const auto& playerJson : gameStateJson["players"])
    {
        auto id{ playerJson["id"].template get<IdType>() };
        Vector2 spawnPos{
            playerJson["x"].template get<float>(),
            playerJson["y"].template get<float>(),
        };
        AddObject<Player>(id, spawnPos);
    }

    for (const auto& bulletJson : gameStateJson["bullets"])
    {
        auto id{ bulletJson["id"].template get<IdType>() };
        auto shooterId{ bulletJson["shooter_id"].template get<IdType>() };
        Vector2 initialPos{
            bulletJson["x"].template get<float>(),
            bulletJson["y"].template get<float>(),
        };
        Vector2 target{
            bulletJson["target_x"].template get<float>(),
            bulletJson["target_y"].template get<float>(),
        };
        AddObject<Bullet>(id, shooterId, initialPos, target);
    }

    for (const auto& wallJson : gameStateJson["walls"])
    {
        AddObject<Wall>(wallJson["id"].template get<IdType>(),
                        LineCollider{ wallJson });
    }

    m_Options = SessionOptions{ gameStateJson };

    // dot't intercept greeting
    m_NetworkClient->Run();
}

auto Scene::GetOptions() const -> SessionOptions
{
    return m_Options;
}
void Scene::Update()
{
    // important: process queue BEFORE sending new movement to avoid packet
    // overlaps (jitter)
    ProcessMessages();

    m_MainPlayer->Update();

    auto [mainPlayerController, mainPlayerCollider]{
        m_Registry->get<PlayerController, CircleCollider>(m_MainPlayer->GetId())
    };

    mainPlayerController.Update();
    mainPlayerCollider.SetVelocity(mainPlayerController.GetCurrentVelocity());

    m_NetworkClient->SendMovement(m_MainPlayer->GetId(),
                                  mainPlayerCollider.GetVelocity());

    // remove queued objects after all iterations
    for (auto& idToDelete : m_MarkedForDeletion)
    {
        m_Registry->destroy(idToDelete);
        m_Objects.erase(idToDelete);
    }
    m_MarkedForDeletion.clear();
}
void Scene::Draw() const
{
    for (const auto& object : m_Objects)
    {
        object.second->Draw();
    }

    DrawText(("FPS: " + std::to_string(GetFPS())).c_str(), 5, 5, 20, BLACK);
}

void Scene::ProcessMessages()
{
    std::scoped_lock<std::mutex> mtxLock{ m_MQMutex };

    for (auto it{ m_MessageQueue.begin() }; it != m_MessageQueue.end();)
    {
        if (this->ProcessIncomingMessage(*it))
        {
            it = m_MessageQueue.erase(it);
        }
        else
        {
            // we can't process it yet, let it hang
            it++;
        }
    }
}

auto Scene::ProcessIncomingMessage(const json& message) -> bool
{
    // guaranteed to exist
    auto type{ message["type"].template get<std::string>() };
    auto payload = message["payload"]; // nlohmann/json doesn't like universal
                                       // initialization :(

    if (type == "coords")
    {
        auto entityId{ payload["id"].template get<IdType>() };
        if (!m_Registry->valid(entityId))
        {
            // just skip bad coordinates
            return true;
        }

        m_Registry->patch<CircleCollider>(
            entityId,
            [payload](auto& collider)
            {
                collider.SetPosition({ payload["x"].template get<float>(),
                                       payload["y"].template get<float>() });
            });
    }
    else if (type == "connection")
    {
        Vector2 spawnPos{ payload["x"].template get<float>(),
                          payload["y"].template get<float>() };
        AddObject<Player>(payload["id"].template get<IdType>(), spawnPos);
    }
    else if (type == "shoot")
    {
        auto id{ payload["bullet_id"].template get<IdType>() };

        // an ugly way to check if entity exists
        auto enttId{ m_Registry->create(id) };
        if (enttId != id)
        {
            m_Registry->destroy(enttId);
            // need to wait till destruction
            return false;
        }
        m_Registry->destroy(enttId);

        Vector2 targetVec{ payload["target_x"].template get<float>(),
                           payload["target_y"].template get<float>() };
        Vector2 initialPos{ payload["bullet_x"].template get<float>(),
                            payload["bullet_y"].template get<float>() };

        AddObject<Bullet>(id, payload["shooter_id"].template get<IdType>(),
                          initialPos, targetVec);
    }
    else if (type == "destroy")
    {
        auto id{ payload["id"].template get<IdType>() };
        // if (id == m_MainPlayer->GetId())
        // {
        //     m_NetworkClient = nullptr;
        //     *reinterpret_cast<char*>(0); // дружеский прикол
        // }
        RemoveObject(id);
    }
    else if (type == "network_error")
    {
        std::cerr << payload["what"].template get<std::string>() << std::endl;
        m_Alive = false;
    }
    return true;
}

void Scene::HandleEvent(ShootEvent event)
{
    m_NetworkClient->SendShoot(m_MainPlayer->GetId(), event.Target);
}

void Scene::HandleEvent(KillEvent event)
{
    RemoveObject(event.Victim->GetId());
    RemoveObject(event.Projectile->GetId());

    if (event.Victim->GetId() == m_MainPlayer->GetId())
    {
        return;
    }
}

void Scene::RemoveObject(IdType id)
{
    m_MarkedForDeletion.push_back(id);
}
auto Scene::GetRegistry() const -> std::shared_ptr<Registry>
{
    return m_Registry;
}
auto Scene::IsAlive() const -> bool
{
    return m_Alive;
}
} // namespace smp::game
