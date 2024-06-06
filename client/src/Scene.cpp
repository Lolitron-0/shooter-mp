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
        { this->ProcessIncomingMessage(std::move(message)); });

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
    // for (auto& object : m_Objects)
    // {
    //     object.second->Update();
    // }

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

        collider::CollideCircleLine(mainPlayerCollider, wallCollider);
    }

    m_NetworkClient->SendMovement(m_MainPlayer->GetId(),
                                  m_MainPlayer->GetNextPosition());

    // remove objects after collision
    for (auto& idToDelete : m_MarkedForDeletion)
    {
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

void Scene::ProcessIncomingMessage(json&& message)
{
    // guaranteed to exist (right?)
    auto type{ message["type"].template get<std::string>() };
    auto payload = message["payload"]; // nlohmann/json doesn't like universal
                                       // initialization :(

    if (type == "coords")
    {
        auto entityId{ payload["id"].template get<IdType>() };
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
        Vector2 targetVec{ payload["target_x"].template get<float>(),
                           payload["target_y"].template get<float>() };
        Vector2 initialPos{ payload["bullet_x"].template get<float>(),
                            payload["bullet_y"].template get<float>() };

        AddObject<Bullet>(payload["bullet_id"].template get<IdType>(),
                          payload["shooter_id"].template get<IdType>(),
                          initialPos, targetVec);
    }
}

void Scene::HandleEvent(ShootEvent event)
{
    // AddObject<Bullet>(1, event.Shooter, event.Target);
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
