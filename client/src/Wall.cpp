#include "Wall.hpp"
#include "Components.hpp"
#include "GameObject.hpp"
#include "Scene.hpp"
#include "Typedefs.hpp"
#include <raylib.h>

namespace smp::game
{

Wall::Wall(IdType id, Scene* parent, const LineCollider& collider)
    : GameObject{ id, parent }
{
    GetScene()->GetRegistry()->emplace<LineCollider>(GetId(), collider);
}

void Wall::Update() {}

void Wall::Draw() const
{
    auto collider{ GetScene()->GetRegistry()->get<LineCollider>(GetId()) };
    DrawLineEx(collider.Start, collider.End, 10, BLUE);
}

} // namespace smp::game
