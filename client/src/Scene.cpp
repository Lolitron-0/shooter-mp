#include "Scene.hpp"
#include "Bullet.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Player.hpp"
#include "Typedefs.hpp"
#include "Wall.hpp"
#include <cassert>
#include <memory>
#include <raylib.h>
#include <string>

namespace smp::game
{

Scene::Scene()
{
    m_NetworkClient = std::make_unique<network::NetworkClient>();
    auto gameStateFuture{ m_NetworkClient->Connect("127.0.0.1:25330") };

    m_Registry = std::make_shared<Registry>();

    m_NetworkClient->SetMessageCallback(
        [this](json&& message)
        { m_MessageQueue.push_front(std::move(message)); });
    // m_NetworkClient->SetMessageCallback(
    //     [this](json&& message) { this->ProcessIncomingMessage(message); });

    json gameStateJson = gameStateFuture.get();
    std::cout << gameStateJson << std::endl;
    gameStateJson = gameStateJson["payload"];

    AddMainPlayer(gameStateJson["player_id"].template get<IdType>());
    for (const auto& playerJson : gameStateJson["players"])
    {
        auto id{ playerJson["id"].template get<IdType>() };
        AddObject<Player>(id);
        m_Registry->get<CircleCollider>(id).SetPosition({
            playerJson["x"].template get<float>(),
            playerJson["y"].template get<float>(),
        });
    }

    for (const auto& wallJson : gameStateJson["walls"])
    {
        AddObject<Wall>(wallJson["id"].template get<IdType>(),
                        LineCollider{ wallJson });
    }

    // dot't intercept greeting
    m_NetworkClient->Run();
}

auto Scene::GetOptions() const -> SessionOptions
{
    return m_Options;
}
void Scene::Update()
{
    m_MainPlayer->Update();

    auto [mainPlayerController, mainPlayerCollider]{
        m_Registry->get<PlayerController, CircleCollider>(m_MainPlayer->GetId())
    };

    mainPlayerController.Update();
    mainPlayerCollider.SetVelocity(mainPlayerController.GetCurrentVelocity());

    auto wallView{ m_Registry->view<LineCollider>() };
    for (auto wall : wallView)
    {
        auto& wallCollider{ m_Registry->get<LineCollider>(wall) };

        collider::CollideCircleLine(mainPlayerCollider, wallCollider,
                                    GetFrameTime());
    }

    // important: process queue BEFORE sending new movement to avoid datagram
    // overlaps (jitter)
    ProcessMessages();

    m_NetworkClient->SendMovement(
        m_MainPlayer->GetId(),
        mainPlayerCollider.GetNextPosition(GetFrameTime()));

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
        AddObject<Player>(payload["id"].template get<IdType>());
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
        if (id == m_MainPlayer->GetId())
        {
			m_NetworkClient = nullptr;
            *reinterpret_cast<char*>(0); // дружеский прикол
        }
        RemoveObject(id);
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

void Scene::HandleServerMessage(json&& message) {}

void Scene::RemoveObject(IdType id)
{
    m_MarkedForDeletion.push_back(id);
}
auto Scene::GetRegistry() const -> std::shared_ptr<Registry>
{
    return m_Registry;
}
} // namespace smp::game
