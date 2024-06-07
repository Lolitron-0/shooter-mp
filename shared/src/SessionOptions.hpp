#pragma once
#include "Components.hpp"
#include "Typedefs.hpp"
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <utility>

namespace smp::game
{

struct WallEntitiy
{
    LineCollider Collider;
    IdType Id{ 0 };
};

// Options for current session, recieved from server on connection
struct SessionOptions
{
    SessionOptions() = default;
    explicit SessionOptions(const nlohmann::json& json)
        : PlayerRadius(json["player_radius"].template get<float>()),
          PlayerSpeed(json["player_speed"].template get<float>()),
          BulletRadius(json["bullet_radius"].template get<float>()),
          BulletSpeed(json["bullet_speed"].template get<float>())
    {
        for (const auto& wall : json["walls"])
        {
            Walls.push_back({ LineCollider{ wall } });
        }

        // add bounding walls
        Walls.push_back(
            { LineCollider{ Vector2{ 0, 0 }, Vector2{ WorldWidth, 0 } } });
        Walls.push_back({ LineCollider{ Vector2{ WorldWidth, 0 },
                                        Vector2{ WorldWidth, WorldHeight } } });
        Walls.push_back({ LineCollider{ Vector2{ WorldWidth, WorldHeight },
                                        Vector2{ 0, WorldHeight } } });
        Walls.push_back(
            { LineCollider{ Vector2{ 0, WorldHeight }, Vector2{ 0, 0 } } });
    }

    [[nodiscard]] auto ToJSON() const -> nlohmann::json
    {
        nlohmann::json res = { { "player_radius", PlayerRadius },
                               { "player_speed", PlayerSpeed },
                               { "bullet_radius", BulletRadius },
                               { "bullet_speed", BulletSpeed } };
        for (auto wall : Walls)
        {
            nlohmann::json wallJson = { { "id", wall.Id },
                                        { "start",

                                          { { "x", wall.Collider.Start.x },
                                            { "y", wall.Collider.Start.y } } },
                                        { "end",
                                          { { "x", wall.Collider.End.x },
                                            { "y", wall.Collider.End.y } } } };

            res["walls"].push_back(wallJson);
        }
        return res;
    }

    float PlayerRadius{ 30.F };
    float PlayerSpeed{ 300.F };
    float BulletRadius{ 5.F };
    float BulletSpeed{ 500.F };
    std::vector<WallEntitiy> Walls;

public:
    static constexpr uint32_t WorldWidth{ 860 };
    static constexpr uint32_t WorldHeight{ 600 };
};

} // namespace smp::game
