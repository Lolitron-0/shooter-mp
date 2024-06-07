#include "Components.hpp"
#include <iostream>
#include <raylib.h>
#include <raymath.h>

namespace smp::game
{

CircleCollider::CircleCollider(Vector2 position, float radius)
    : m_Position(position),
      m_Radius(radius)
{
}
auto CircleCollider::GetPosition() const -> Vector2
{
    return m_Position;
}
auto CircleCollider::GetVelocity() const -> Vector2
{
    return m_Velocity;
};
auto CircleCollider::GetNextPosition(float velScale) const -> Vector2
{
    return Vector2Add(m_Position, Vector2Scale(m_Velocity, velScale));
};

void CircleCollider::SetPosition(Vector2 position)
{
    m_Position = position;
};
void CircleCollider::SetVelocity(Vector2 velocity)
{
    m_Velocity = velocity;
};
void CircleCollider::AddVelocity(Vector2 accel)
{
    m_Velocity = Vector2Add(m_Velocity, accel);
}
auto CircleCollider::GetRadius() const -> float
{
    return m_Radius;
}
auto PlayerController::GetCurrentVelocity() const -> Vector2
{
    return m_CurrentVelocity;
}
void PlayerController::Update()
{
    m_CurrentVelocity = { 0, 0 };
    if (IsKeyDown(KEY_W))
    {
        m_CurrentVelocity.y = -m_PlayerSpeed;
    }
    if (IsKeyDown(KEY_S))
    {
        m_CurrentVelocity.y = m_PlayerSpeed;
    }
    if (IsKeyDown(KEY_D))
    {
        m_CurrentVelocity.x = m_PlayerSpeed;
    }
    if (IsKeyDown(KEY_A))
    {
        m_CurrentVelocity.x = -m_PlayerSpeed;
    }
}
PlayerController::PlayerController(float speed)
    : m_PlayerSpeed{ speed }
{
}
auto collider::CollideCircles(CircleCollider& first, CircleCollider& second,
                              float deltaTime) -> bool
{
    if (CheckCollisionCircles(
            first.GetNextPosition(deltaTime), first.GetRadius(),
            second.GetNextPosition(deltaTime), second.GetRadius()))
    {
        first.SetVelocity({ 0, 0 });
        second.SetVelocity({ 0, 0 });
        return true;
    }
    return false;
}
auto collider::CollideCircleLine(CircleCollider& circle, LineCollider& line,
                                 float deltaTime) -> bool
{
    if (CheckCollisionCircleLine(circle.GetNextPosition(deltaTime),
                                 circle.GetRadius(), line.Start, line.End))
    {
        circle.SetVelocity({ 0, 0 });
        return true;
    }
    return false;
}

} // namespace smp::game
