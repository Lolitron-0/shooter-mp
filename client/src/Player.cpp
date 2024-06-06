#include "Player.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "Typedefs.hpp"
#include <entt/entt.hpp>
#include <raylib.h>
#include <raymath.h>

namespace smp::game
{

Player::Player(IdType id, Scene* parent)
    : GameObject{ id, parent, GameObjectType::Player }
{
    GetScene()->GetRegistry()->emplace<CircleCollider>(
        GetId(), Vector2{ 300, 300 }, // initial position
        GetScene()->GetOptions().PlayerRadius);
}

void Player::Update()
{
    // auto& collider{ GetScene()->GetRegistry()->get<CircleCollider>(GetId())
    // };
    //
    // collider.SetVelocity({ 0, 0 });
    //
    // if (IsKeyDown(KEY_W))
    // {
    //     collider.SetVelocity({ 0, -GetScene()->GetOptions().PlayerSpeed });
    // }
    // if (IsKeyDown(KEY_S))
    // {
    //     collider.SetVelocity({ 0, GetScene()->GetOptions().PlayerSpeed });
    // }
    // if (IsKeyDown(KEY_D))
    // {
    //     collider.SetVelocity({ GetScene()->GetOptions().PlayerSpeed, 0 });
    // }
    // if (IsKeyDown(KEY_A))
    // {
    //     collider.SetVelocity({ -GetScene()->GetOptions().PlayerSpeed, 0 });
    // }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        auto& collider{ GetScene()->GetRegistry()->get<CircleCollider>(
            GetId()) };
        GetScene()->HandleEvent(
            { .Shooter = this,
              .Target = Vector2Subtract(GetMousePosition(),
                                        collider.GetPosition()) });
    }
}

void Player::Draw() const
{
    auto& collider{ GetScene()->GetRegistry()->get<CircleCollider>(GetId()) };
    DrawCircleV(collider.GetPosition(), GetScene()->GetOptions().PlayerRadius,
                GREEN);
}

void Player::AfterCollision()
{
    auto& collider{ GetScene()->GetRegistry()->get<CircleCollider>(GetId()) };
    collider.SetPosition(collider.GetNextPosition());
}

void Player::CollideWith(const std::shared_ptr<GameObject>& obj)
{
    auto& thisCollider{ GetScene()->GetRegistry()->get<CircleCollider>(
        GetId()) };
    switch (obj->GetType())
    {
    case GameObjectType::Wall:
    {
        auto collider{ obj->GetCollider<LineCollider>() };
        auto radius{ GetScene()->GetOptions().PlayerRadius };
        if (CheckCollisionCircleLine(thisCollider.GetNextPosition(), radius,
                                     collider.Start, collider.End))
        {
            thisCollider.SetVelocity({ 0, 0 });
        }
        break;
    }
    case GameObjectType::Bullet:
    case GameObjectType::Player:
        break;
    }
}
auto Player::GetPosition() const -> Vector2
{
    return GetScene()
        ->GetRegistry()
        ->get<CircleCollider>(GetId())
        .GetPosition();
}
auto Player::GetNextPosition() const -> Vector2
{
    return GetScene()
        ->GetRegistry()
        ->get<CircleCollider>(GetId())
        .GetNextPosition();
}
} // namespace smp::game
