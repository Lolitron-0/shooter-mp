#pragma once
#include "Components.hpp"
#include "GameEvents.hpp"
#include "NetworkClient.hpp"
#include "SessionOptions.hpp"
#include "Typedefs.hpp"
#include <cassert>
#include <entt/entt.hpp>
#include <memory>
#include <queue>
#include <raylib.h>
#include <unordered_map>

using json = nlohmann::json;

namespace smp::network
{
class NetworkClient;
} // namespace smp::network

namespace smp::game
{

class GameObject;

class Scene
{
    using ObjectsContainer =
        std::unordered_map<IdType, std::shared_ptr<GameObject>>;
    using Registry = entt::basic_registry<IdType>;

public:
    explicit Scene(std::unique_ptr<network::NetworkClient> networkClient);

    void Update();
    void Draw() const;

    auto IsAlive() const -> bool;

    template <class T, class... Args>
    void AddObject(IdType id, Args&&... args)
    {
        auto ecsId{ m_Registry->create(id) };
        assert(ecsId == id);

        auto ptr{ std::make_unique<T>(id, this, std::forward<Args>(args)...) };
        m_Objects[ptr->GetId()] = std::move(ptr);
    }

    template <class... Args>
    void AddMainPlayer(IdType id, Args&&... args)
    {
        auto ecsId{ m_Registry->create(id) };
        assert(ecsId == id);
        m_Registry->emplace<PlayerController>(ecsId, m_Options.PlayerSpeed);

        auto ptr{ std::make_unique<Player>(id, this,
                                           std::forward<Args>(args)...) };
        m_MainPlayer = ptr.get();
        m_Objects[ptr->GetId()] = std::move(ptr);
    }

    void RemoveObject(IdType id);

    void HandleEvent(ShootEvent event);
    void HandleEvent(KillEvent event);

    [[nodiscard]] auto GetOptions() const -> SessionOptions;
    [[nodiscard]] auto GetRegistry() const -> std::shared_ptr<Registry>;

private:
    auto ProcessIncomingMessage(const json& message) -> bool;
    void ProcessMessages();


private:
    std::unique_ptr<network::NetworkClient> m_NetworkClient;
    bool m_Alive{ true };

    ObjectsContainer m_Objects;
    std::shared_ptr<Registry> m_Registry;

    SessionOptions m_Options;
    Player* m_MainPlayer{ nullptr };
    // since we are using unordered_map, they won't invalidate
    std::vector<IdType> m_MarkedForDeletion;

    std::list<nlohmann::json> m_MessageQueue;
	std::mutex m_MQMutex;
};

} // namespace smp::game
