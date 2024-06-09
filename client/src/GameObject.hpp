#pragma once
#include "Components.hpp"
#include "Scene.hpp"
#include "Typedefs.hpp"
#include <cstdint>
#include <memory>
#include <raylib.h>
#include <variant>

namespace smp::game
{

class Player;
class Bullet;

enum class GameObjectType : uint8_t
{
    Player,
    Bullet,
    Wall
};

class GameObject
{
public:
    GameObject(IdType id, Scene* parent, GameObjectType type);

    virtual void Update() {};
    virtual void Draw() const = 0;

    [[nodiscard]] auto GetId() const -> IdType;

    template <class T>
    auto GetCollider() const -> T
    {
        return GetScene()->GetRegistry()->get<T>(m_Id);
    }

    virtual ~GameObject() = default;

protected:
    [[nodiscard]] auto GetScene() const -> Scene*;

private:
    IdType m_Id;
    Scene* m_Scene;
    GameObjectType m_Type;
};


} // namespace smp::game
