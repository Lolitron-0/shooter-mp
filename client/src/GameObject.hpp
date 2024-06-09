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

class GameObject
{
public:
    GameObject(IdType id, Scene* parent);

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
};

} // namespace smp::game
