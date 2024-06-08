#pragma once
#include "GameObject.hpp"
#include "Typedefs.hpp"
#include <memory>
#include <raylib.h>

namespace smp::game
{
class Scene;

class Player : public GameObject
{
public:
    Player(IdType id, Scene* parent, Vector2 initialPos);

    [[nodiscard]] auto GetPosition() const -> Vector2;

    void Update() override;
    void Draw() const override;
};

} // namespace smp::game
