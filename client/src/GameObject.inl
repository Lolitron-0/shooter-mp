#include "GameObject.hpp"
#include "Scene.hpp"

namespace smp::game
{

template <class T>
auto GameObject::GetCollider() const -> T
{
    // static_assert(std::holds_alternative<T>(m_Collider),
    //               "Wrong collider type");
    return GetScene()->GetRegistry()->get<T>(m_Id);
}

} // namespace smp::game
