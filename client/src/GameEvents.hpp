#pragma once
#include <raylib.h>

namespace smp::game
{

class Player;
class Bullet;
class GameObject;

struct ShootEvent
{
    Player* Shooter;
    Vector2 Target;
};

struct KillEvent
{
    Bullet* Projectile;
    GameObject* Victim;
};
} // namespace smp::game
