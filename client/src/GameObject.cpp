#include "GameObject.hpp"
#include "Typedefs.hpp"
#include <unordered_map>

namespace smp::game
{

auto GameObject::GetId() const -> IdType
{
    return m_Id;
}

GameObject::GameObject(IdType id, Scene* parent)
    : m_Scene{ parent },
      m_Id{ id }
{
}

auto GameObject::GetScene() const -> Scene*
{
    return m_Scene;
}
} // namespace smp::game
