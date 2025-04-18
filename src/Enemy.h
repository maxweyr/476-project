#ifndef ENEMY_H
#define ENEMY_H

#include "Entity.h"

class Enemy : public Entity {
    public:
        Enemy(const glm::vec3& position, float hitpoints, float moveSpeed);

        void attack();
};

#endif // ENEMY_H