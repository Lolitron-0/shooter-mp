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
    [[nodiscard]] auto GetNextPosition() const -> Vector2;

    void Update() override;
    void Draw() const override;
    void AfterCollision() override;
    void CollideWith(const std::shared_ptr<GameObject>& obj) override;
};

} // namespace smp::game
