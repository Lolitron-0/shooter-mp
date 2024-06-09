#pragma once
#include "Typedefs.hpp"
#include <cmath>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>

namespace smp::game
{
struct LineCollider
{
    LineCollider() = default;
    LineCollider(Vector2 start, Vector2 end)
        : Start{ start },
          End{ end }
    {
    }
    explicit LineCollider(const nlohmann::json& json)
        : LineCollider{ Vector2{
                            json["start"]["x"].template get<float>(),
                            json["start"]["y"].template get<float>(),
                        },
                        Vector2{
                            json["end"]["x"].template get<float>(),
                            json["end"]["y"].template get<float>(),
                        } }
    {
    }

    Vector2 Start;
    Vector2 End;
};

class CircleCollider
{
public:
    CircleCollider(Vector2 position, float radius);

    [[nodiscard]] auto GetPosition() const -> Vector2;
    [[nodiscard]] auto GetRadius() const -> float;
    [[nodiscard]] auto GetVelocity() const -> Vector2;
    // overload for delta time
    [[nodiscard]] auto GetNextPosition(float velScale) const -> Vector2;

    void SetPosition(Vector2 position);
    void SetVelocity(Vector2 velocity);
    void AddVelocity(Vector2 accel);

private:
    Vector2 m_Position;
    Vector2 m_Velocity{ 0, 0 };
    float m_Radius;
};

class PlayerController
{
public:
    explicit PlayerController(float speed);

    void Update();

    [[nodiscard]] auto GetCurrentVelocity() const -> Vector2;

private:
    Vector2 m_CurrentVelocity{ 0, 0 };
    float m_PlayerSpeed;
};

namespace collider
{
auto CollideCircles(CircleCollider& first, CircleCollider& second,
                    float deltaTime) -> bool;
auto CollideCircleLine(CircleCollider& circle, LineCollider& line,
                       float deltaTime) -> bool;
}; // namespace collider

// entt allows filtering eninies by component type (probably ok since all such
// checks are static)
struct PlayerTag
{
};

struct BulletTag
{
    IdType ShooterId; 
};

// inline auto CheckCollisionCircleLine(Vector2 center, float radius,
//                                      Vector2 start, Vector2 end) -> bool
// {
//     auto dir{ Vector2Subtract(end, start) };
//     auto conn{ Vector2Subtract(start, center) };
//
//     float a{ Vector2DotProduct(dir, dir) };
//     float b{ 2.F * Vector2DotProduct(conn, dir) };
//     float c{ Vector2DotProduct(conn, conn) - radius * radius };
//
//     float discriminant{ b * b - 4 * a * c };
//
//     if (discriminant <= 0)
//     {
//         return false;
//     }
//
//     discriminant = std::sqrt(discriminant);
//     float t1 = (-b - discriminant) / (2 * a);
//     float t2 = (-b + discriminant) / (2 * a);
//
//     if (t1 >= 0 && t1 <= 1)
//     {
//         return true;
//     }
//
//     if (t2 >= 0 && t2 <= 1)
//     {
//         return true;
//     }
//     return false;
// }
//
} // namespace smp::game
