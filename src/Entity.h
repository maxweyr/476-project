#ifndef ENTITY_H
#define ENTITY_H

#include "AssimpModel.h"
#include <glm/glm.hpp>
#include "Texture.h"

class Entity {
    private:
        glm::vec3 position;
        float hitpoints;
        float moveSpeed;

    public:
        Entity(const glm::vec3& position, float hitpoints, float moveSpeed);

        glm::vec3 getPosition() const;
        void setPosition(const glm::vec3 pos);
        void takeDamage(float damage);
        void move(const glm::vec3& direction);
        bool isAlive() const;

};

#endif // ENTITY_H