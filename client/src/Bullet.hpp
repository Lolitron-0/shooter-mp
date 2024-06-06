#pragma once
#include "GameObject.hpp"
#include "Typedefs.hpp"
#include <memory>
#include <raylib.h>
namespace smp::game
{

class Scene;
class Player;

class Bullet : public GameObject
{
public:
    Bullet(IdType id, Scene* parent, IdType owner, Vector2 initialPosition,
           Vector2 target);

    void Update() override;
    void Draw() const override;
    void AfterCollision() override;

    void CollideWith(const std::shared_ptr<GameObject>& obj) override;

private:
    IdType m_OwnerId;
};

} // namespace smp::game
