#include "GameObject.hpp"
#include "Typedefs.hpp"
#include <unordered_map>

namespace smp::game
{

auto GameObject::GetId() const -> IdType
{
    return m_Id;
}

GameObject::GameObject(IdType id, Scene* parent, GameObjectType type)
    : m_Scene{ parent },
      m_Type{ type },
      m_Id{ id }
{
}

auto GameObject::GetScene() const -> Scene*
{
    return m_Scene;
}
auto GameObject::GetType() const -> GameObjectType
{
    return m_Type;
}
} // namespace smp::game
