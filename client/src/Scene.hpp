#pragma once
#include "Components.hpp"
#include "GameEvents.hpp"
#include "NetworkClient.hpp"
#include "SessionOptions.hpp"
#include "Typedefs.hpp"
#include <cassert>
#include <entt/entt.hpp>
#include <memory>
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
    Scene();

    void Update();
    void Draw() const;

    template <class T, class... Args>
    void AddObject(IdType id, Args&&... args)
    {
        auto ecsId{ m_Registry->create(id) };
        assert(ecsId == id); // all identifiers are in sync with
                             // server, so we should match 

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

    void ProcessIncomingMessage(json&& message);

    void HandleEvent(ShootEvent event);
    void HandleEvent(KillEvent event);

    void HandleServerMessage(json&& message);

    [[nodiscard]] auto GetOptions() const -> SessionOptions;
    [[nodiscard]] auto GetRegistry() const -> std::shared_ptr<Registry>;

private:
    std::unique_ptr<network::NetworkClient> m_NetworkClient;

    ObjectsContainer m_Objects;
    std::shared_ptr<Registry> m_Registry;
    SessionOptions m_Options;
    Player* m_MainPlayer{ nullptr };
    // since we are using unordered_map, they won't invalidate
    std::vector<IdType> m_MarkedForDeletion;
};

} // namespace smp::game
