#include "Bullet.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Player.hpp"
#include "Scene.hpp"
#include "Typedefs.hpp"
#include <iostream>
#include <raylib.h>
#include <raymath.h>

namespace smp::game
{

Bullet::Bullet(IdType id, Scene* parent, IdType owner, Vector2 initialPosition,
               Vector2 target)
    : GameObject{ id, parent, GameObjectType::Bullet },
      m_OwnerId{ owner }
{
    auto registry{ GetScene()->GetRegistry() };
    auto& collider{ registry->emplace<CircleCollider>(
        GetId(), initialPosition, GetScene()->GetOptions().BulletRadius) };
    collider.SetVelocity(
        Vector2Scale(target, GetScene()->GetOptions().BulletSpeed));
}

void Bullet::Update() {}

void Bullet::Draw() const
{
    auto& collider{ GetScene()->GetRegistry()->get<CircleCollider>(GetId()) };
    DrawCircleV(collider.GetPosition(), GetScene()->GetOptions().BulletRadius,
                BLACK);
    // DrawText((std::to_string(m_Position.x) + ";" +
    // std::to_string(m_Position.y))
    //              .c_str(),
    //          200, 200, 20, BLACK);
}

} // namespace smp::game
