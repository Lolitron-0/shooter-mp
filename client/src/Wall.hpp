#pragma once
#include "Components.hpp"
#include "GameObject.hpp"
#include "Typedefs.hpp"
#include <raylib.h>

namespace smp::game
{

class Wall : public GameObject
{
public:
    Wall(IdType id, Scene* parent, const LineCollider& collider);

    void Update() override;
    void Draw() const override;

private:
    Vector2 m_Start;
    Vector2 m_Finish;
};

} // namespace smp::game
