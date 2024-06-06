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

Bullet::Bullet(IdType id, Scene* parent, IdType owner,
               Vector2 initialPosition, Vector2 target)
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

void Bullet::AfterCollision()
{
    auto& collider{ GetScene()->GetRegistry()->get<CircleCollider>(GetId()) };
    collider.SetPosition(collider.GetNextPosition());
}

void Bullet::CollideWith(const std::shared_ptr<GameObject>& obj)
{
    auto& thisCollider{ GetScene()->GetRegistry()->get<CircleCollider>(
        GetId()) };
    switch (obj->GetType())
    {
    case GameObjectType::Player:
    {
        if (obj->GetId() == m_OwnerId)
        {
            break;
        }
        auto bulletRadius{ GetScene()->GetOptions().BulletRadius };
        auto playerRadius{ GetScene()->GetOptions().PlayerRadius };
        auto collider{ obj->GetCollider<CircleCollider>() };
        if (CheckCollisionCircles(thisCollider.GetPosition(), bulletRadius,
                                  collider.GetPosition(), collider.GetRadius()))
        {
            GetScene()->HandleEvent(KillEvent{ this, obj.get() });
        }
    }
    case GameObjectType::Wall:
    {
        auto collider{ obj->GetCollider<LineCollider>() };
        auto radius{ GetScene()->GetOptions().BulletRadius };
        if (CheckCollisionCircleLine(thisCollider.GetPosition(), radius,
                                     collider.Start, collider.End))
        {
            GetScene()->RemoveObject(GetId());
        }
        break;
    }
    case GameObjectType::Bullet:
        break;
    }
}
} // namespace smp::game
