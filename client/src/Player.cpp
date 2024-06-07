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


auto Player::GetPosition() const -> Vector2
{
    return GetScene()
        ->GetRegistry()
        ->get<CircleCollider>(GetId())
        .GetPosition();
}
} // namespace smp::game
