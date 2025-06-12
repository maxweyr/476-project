#include "LightningElemental.h"
#include <glm/gtc/random.hpp>

LightningElemental::LightningElemental(const glm::vec3& position, float hitpoints, float moveSpeed, AssimpModel* model, const glm::vec3& scale, const glm::vec3& rotation)
    : Enemy(position, hitpoints, moveSpeed, model, scale, rotation) {
        meleeTimer = Config::LIGHTNING_ELEMENTAL_MELEE_SPEED; // Timer for melee attack cooldown
        meleeRange = Config::LIGHTNING_ELEMENTAL_MELEE_RANGE; // Range for melee attack
        meleeDamage = Config::LIGHTNING_ELEMENTAL_MELEE_DAMAGE; // Damage dealt by melee attack
        aggroRange = Config::LIGHTNING_ELEMENTAL_AGGRO_RANGE; // Range for aggro detection
        sightRange = Config::LIGHTNING_ELEMENTAL_SIGHT_RANGE; // Sight range for lightning elemental
        territoryRadius = Config::LIGHTNING_ELEMENTAL_TERRITORY_RADIUS; // Territory radius for lightning elemental
    }

void LightningElemental::moveTowardsPlayer(const glm::vec3& playerPosition, float deltaTime) {
    
    glm::vec3 direction = glm::normalize(playerPosition - this->getPosition());
    direction.y = 0; // Keep the enemy on the same Y level
    direction = glm::normalize(direction);
    // this->move(direction, deltaTime);

    float static teleportTimer = 0.0f; // Static timer to track teleport intervals

    // Check if enough time has passed since the last teleport

    if (teleportTimer <= 0.0f) {
        // Add random angular deviation
        float angleOffset = glm::radians((float)((rand() % 120) - 60)); // -60 to +60 degrees
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleOffset, glm::vec3(0, 1, 0));
        glm::vec3 warpedDirection = glm::vec3(rotation * glm::vec4(direction, 0.0f));

        // Random teleport distance
        float teleportDistance = glm::linearRand(3.0f, 5.0f);
        this->startPosition = this->getPosition();
        this->newPosition = this->getPosition() + warpedDirection * teleportDistance;
        
        //this->setPosition(newPosition);

        // Add randomness to cooldown
        teleportTimer = Config::LIGHTNING_ELEMENTAL_TP_INTERVAL + glm::linearRand(-0.2f, 0.3f);
        teleport_timing = teleportTimer;
        teleport_clock = teleportTimer;
    }
    teleportTimer -= deltaTime; // Decrease teleport timer

    // printf("teleportTimer: %f\n", teleportTimer);

    /* Slow rotation towards player */
    float targetRotY = atan2(direction.x, direction.z);
    // float currentRotY = this->getRotY();
    this->setRotY(targetRotY);

    // Compute angle difference and interpolate
    // float angleDiff = targetRotY - currentRotY;
    // angleDiff = glm::mod(angleDiff + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
    // float rotationSpeed = Config::LIGHTNING_ELEMENTAL_ROTATION_SPEED;
    // float maxRotation = rotationSpeed * deltaTime;

    // if (fabs(angleDiff) < maxRotation) {
    //     currentRotY = targetRotY; // Snap if close enough
    // } else {
    //     currentRotY += glm::sign(angleDiff) * maxRotation; // Rotate toward player
    // }

    // this->setRotY(currentRotY);
}


void LightningElemental::update(Player* player, float deltaTime) {
    Enemy::update(player, deltaTime); // Call base class update

    // Bobbing animation
    float bobSpeed = 2.25f;
    float bobHeight = 0.25f;
    glm::vec3 currentPos = getPosition();
    setPosition(glm::vec3(currentPos.x, Config::LIGHTNING_ELEMENTAL_TRANS_Y + sin(glfwGetTime() * bobSpeed) * bobHeight, currentPos.z));
    // updateAABB(); // if AABB implemented

    // Move toward player if aggroed
    if (isAggro()) {
        moveTowardsPlayer(player->getPosition(), deltaTime);
        tpInterp(deltaTime);
    }
}

void LightningElemental::tpInterp(float deltaTime) {
    if (teleport_clock >= 0.0f) {
    glm::vec3 midstep = Bezier::quadErp( this->newPosition, this->startPosition, teleport_clock / teleport_timing);
    setPosition(midstep);
    //cout << "tt: " << teleport_timing << endl;
    //cout << "dt: " << deltaTime << endl;
    teleport_clock -= 20 * deltaTime;
    }
}
