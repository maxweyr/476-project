#include "Entity.h"

Entity::Entity(const glm::vec3& position, float hitpoints, float moveSpeed)
        : position(position), hitpoints(hitpoints), moveSpeed(moveSpeed) {};

glm::vec3 Entity::getPosition() const {
    return position;
}

void Entity::setPosition(const glm::vec3 pos) {
    position = pos;
}

void Entity::takeDamage(float damage) {
    hitpoints -= damage;
    if (hitpoints < 0) {
        hitpoints = 0;
    }

    if (!isAlive()) {
        // Handle entity death (e.g., play death animation, remove from game)
    }
}

bool Entity::isAlive() const {
    return hitpoints > 0;
}

void Entity::move(const glm::vec3& direction) {
    position += direction * moveSpeed;
    // Add any additional logic for movement (e.g., collision detection)
}