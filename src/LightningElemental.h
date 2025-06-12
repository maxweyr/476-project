#include "Enemy.h"
#include "Config.h"
#include "Bezier.h"

class LightningElemental : public Enemy {

    public:
        LightningElemental(const glm::vec3& position, float hitpoints, float moveSpeed, AssimpModel* model, const glm::vec3& scale = glm::vec3(1.0f), const glm::vec3& rotation = glm::vec3(0.0f));

        void moveTowardsPlayer(const glm::vec3& playerPosition, float deltaTime) override;
        void update(Player* player, float deltaTime) override;
        void LightningElemental::tpInterp(float deltaTime);

        glm::vec3 newPosition;
        glm::vec3 startPosition;
        float teleport_timing;
        float teleport_clock;

        };