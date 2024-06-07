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
    explicit Player(IdType id, Scene* parent);

    [[nodiscard]] auto GetPosition() const -> Vector2;

    void Update() override;
    void Draw() const override;
};

} // namespace smp::game
