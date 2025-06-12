#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include "Spline.h"
#include "AssimpModel.h"
#include "Config.h"
#include "Texture.h"

// --- Enums ---
enum class BookState {
    ON_SHELF,
    FALLING,
    LANDED,
    OPENING,
    OPENED
};

enum class OrbState {
    SPAWNING,
    LEVITATING,
    IDLE,
    COLLECTED
};

enum class Man_State {
    WALKING,
    IDLE,
    ROLL,
    GRAB_BOOK,
    SHOOT
};

enum class SpellType {
    NONE,
    FIRE,
    ICE,
    LIGHTNING,
    HEAL
};

typedef enum SpellSlot {
    SLOT_LEFT,
    SLOT_RIGHT,
    SLOT_INV_ONE,
    SLOT_INV_TWO,
    SLOT_INV_THREE,
    SLOT_INV_FOUR,
    SLOT_INV_FIVE,
    SLOT_INV_SIX
};

enum class Material {
    purple,
    black,
    eye_white,
    pupil_white,
    bronze,
    silver,
    brown,
    orb_glowing_blue,
	orb_glowing_red,
	orb_glowing_yellow,
    grey,
    wood,
    mini_map,
    defaultMaterial,
    blue_body,
    red_body,
    yellow_body,
    gold,
    key_color
};

// Helper function to map each <Material> to a base color for particles (or fallback white)
inline glm::vec3 materialToColor(Material m) {
    switch (m) {
    case Material::purple:            return glm::vec3(0.3f, 0.1f, 0.4f);
    case Material::black:             return glm::vec3(0.04f);
    case Material::eye_white:         return glm::vec3(0.95f);
    case Material::pupil_white:       return glm::vec3(0.85f, 0.85f, 0.9f);
    case Material::bronze:            return glm::vec3(0.714f, 0.4284f, 0.181f);
    case Material::silver:            return glm::vec3(0.972f, 0.960f, 0.915f);
    case Material::brown:             return glm::vec3(0.25f, 0.15f, 0.08f);
    case Material::orb_glowing_blue:  return glm::vec3(0.1f, 0.2f, 0.5f);
    case Material::orb_glowing_red:   return glm::vec3(0.5f, 0.1f, 0.1f);
    case Material::orb_glowing_yellow:return glm::vec3(0.5f, 0.4f, 0.1f);
    case Material::grey:              return glm::vec3(0.8f);
    case Material::wood:              return glm::vec3(0.65f, 0.45f, 0.25f);
    case Material::mini_map:          return glm::vec3(0.65f, 0.45f, 0.25f);
    case Material::defaultMaterial:   return glm::vec3(0.5f);
    case Material::blue_body:         return glm::vec3(0.35f, 0.4f, 0.914f);
    case Material::red_body:          return glm::vec3(0.914f, 0.0f, 0.0f);
    case Material::yellow_body:       return glm::vec3(0.0f, 0.914f, 0.0f);
    case Material::gold:              return glm::vec3(1.0f, 0.766f, 0.336f);
    case Material::key_color:         return glm::vec3(0.9, 0.9, 0.9); // Example key color
    default:                          return glm::vec3(1.0f); // fallback white
    }
};

// --- Structs ---
struct SpellProjectile {
    glm::vec3 position;
    glm::vec3 direction;
    float speed = 15.0f;
    float lifetime = 2.0f;
    float spawnTime = 0.0f;
    bool active = true;

    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
    glm::mat4 transform;

    glm::vec3 localAABBMin_logical;
    glm::vec3 localAABBMax_logical;

    SpellType spellType = SpellType::FIRE;

    SpellProjectile(glm::vec3 startPos, glm::vec3 dir, float time)
        : position(startPos), direction(normalize(dir)), spawnTime(time), transform(1.0f) {
        float s = 0.2f;
        localAABBMin_logical = glm::vec3(-s, -s, -s);
        localAABBMax_logical = glm::vec3(s, s, s);
        active = true;
    }

    void setLifetime(float time) {
        lifetime = time;
    }
};

struct WallObject {
	float length;
	vec3 position;
	vec3 direction;
	float height;
	float width;
	GLuint WallVAID;
	GLuint BuffObj, NorBuffObj, IndxBuffObj;
	GLuint TexBuffObj;
	int GiboLen;
	std::shared_ptr<Texture> texture; // Texture for the wall
};

struct LibGrndObject {
	float length;
	float width;
	float height;
	vec3 center_pos;
	GLuint VAO;
	GLuint BuffObj, NorBuffObj, IndxBuffObj;
	GLuint TexBuffObj;
	int GiboLen;
	std::shared_ptr<Texture> texture; // Texture for the library
};

struct WallObjKey {
	glm::vec3 position;
	glm::vec3 direction;
	float height;
	bool operator<(const WallObjKey& other) const {
		return std::tie(position.x, position.y, position.z, direction.x, direction.y, direction.z, height) <
			std::tie(other.position.x, other.position.y, other.position.z, other.direction.x, other.direction.y, other.direction.z, other.height);
	}
};

struct LibGrndObjKey {
	glm::vec3 center_pos;
	float height;
	bool operator<(const LibGrndObjKey& other) const {
		return std::tie(center_pos.x, center_pos.y, center_pos.z, height) <
			std::tie(other.center_pos.x, other.center_pos.y, other.center_pos.z, other.height);
	}
};

struct AABB {
    vec3 min;
    vec3 max;
};

struct PawPrint {
    vec2 pos;
    float angle;
    float spawnTime;
};

// --- Classes ---
class Book {
public:
    vec3 initialPosition;
    vec3 position;
    vec3 scale;
    quat orientation;
    BookState state = BookState::ON_SHELF;
    Spline* fallSpline = nullptr;
    float fallStartTime = 0.0f;
    float openAngle = 0.0f;
    float maxOpenAngle = Config::PI;
    float openSpeed = glm::radians(120.0f);
    Material orbColor;
    float orbScale = 0.1f;
    bool orbSpawned = false;
    SpellType spellType = SpellType::FIRE;

    Book(const glm::vec3& pos, const glm::vec3& scl, const glm::quat& orient, SpellType type)
        : initialPosition(pos), position(pos), scale(scl), orientation(orient), spellType(type) {
        switch (spellType) {
        case SpellType::FIRE:
            orbColor = Material::orb_glowing_red;
            break;
        case SpellType::ICE:
            orbColor = Material::orb_glowing_blue;
            break;
        case SpellType::LIGHTNING:
            orbColor = Material::orb_glowing_yellow;
            break;
        case SpellType::HEAL:
            orbColor = Material::red_body; // Example color for healing
            break;
        default:
            orbColor = Material::defaultMaterial;
            break;
        }
    }

    ~Book() {
        delete fallSpline;
    }

    void startFalling(float groundY, const glm::vec3& playerPos) {
        if (state != BookState::ON_SHELF) return;
        state = BookState::FALLING;
        fallStartTime = (float)glfwGetTime();

        glm::vec3 endPosition;
        endPosition.y = groundY + (scale.z * 0.15);
        glm::vec3 dirToBook = playerPos - initialPosition;
        dirToBook.y = 0.0f;
        float distSq = dot(dirToBook, dirToBook);
        if (distSq < 0.01f) {
            dirToBook = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        else {
            dirToBook = normalize(dirToBook);
        }
        float landingDistance = 3.0f;
        float randomSpread = 0.75f;
        endPosition.x = initialPosition.x + dirToBook.x * landingDistance + Config::randFloat(-randomSpread, randomSpread);
        endPosition.z = initialPosition.z + dirToBook.z * landingDistance + Config::randFloat(-randomSpread, randomSpread);

        glm::vec3 controlPoint = (initialPosition + endPosition) * 0.5f;
        controlPoint.y = initialPosition.y + 3.0f;
        controlPoint.x += Config::randFloat(-1.5f, 1.5f);

        float fallDuration = 0.4f;
        delete fallSpline;
        fallSpline = new Spline(initialPosition, controlPoint, endPosition, fallDuration);
    }

    void update(float deltaTime, float groundY) {
        switch (state) {
        case BookState::FALLING:
            if (fallSpline) {
                fallSpline->update(deltaTime);
                position = fallSpline->getPosition();
                if (fallSpline->isDone()) {
                    state = BookState::LANDED;
                    position.y = groundY + (scale.z * 0.15);
                    delete fallSpline;
                    fallSpline = nullptr;
                    orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            break;
        case BookState::LANDED:
            state = BookState::OPENING;
            break;
        case BookState::OPENING:
            openAngle += openSpeed * deltaTime;
            if (openAngle >= maxOpenAngle) {
                openAngle = maxOpenAngle;
                state = BookState::OPENED;
            }
            break;
        case BookState::OPENED:
            break; // Remains open
        case BookState::ON_SHELF:
        default:
            break; // Do nothing
        }
    }
};

class Collectible {
public:
    AssimpModel* model;
    glm::vec3 position;
    float scale;
    glm::vec3 AABBmin;
    glm::vec3 AABBmax;
    bool collected;
    Material color;
    OrbState state = OrbState::SPAWNING;
    SpellType spellType = SpellType::FIRE;
    // KeyState key_state = KeyState::SPAWNING;
    glm::vec3 spawnPosition;
    glm::vec3 idlePosition;
    float levitationHeight = 0.6f;
    float levitationStartTime = 0.0f;
    float levitationDuration = 0.75f;
    bool keyUsed = false; // For keys, to track if they have been used

    Collectible(AssimpModel* mdl, const glm::vec3& spawnPos, float scl, Material clrIn, SpellType type = SpellType::FIRE)
        : model(mdl), position(spawnPos), scale(scl), collected(false),
        state(OrbState::LEVITATING), spellType(type), spawnPosition(spawnPos) {

        // Set color based on spellType
        switch (spellType) {
        case SpellType::FIRE:
            //this->color = glm::vec3(1.0f, 0.4f, 0.1f); // Orange/Red for Fire
            this->color = Material::orb_glowing_red; // Orange/Red for Fire
            break;
        case SpellType::ICE:
            //this->color = glm::vec3(0.2f, 0.7f, 1.0f); // Light Blue for Ice
            this->color = Material::orb_glowing_blue; // Light Blue for Ice
            break;
        case SpellType::LIGHTNING:
            //this->color = glm::vec3(1.0f, 1.0f, 0.3f); // Yellow for Lightning
            this->color = Material::orb_glowing_yellow; // Yellow for Lightning
            break;
        default:
            this->color = clrIn; // Fallback to input color if type is NONE or undefined
            break;
        }

        idlePosition = spawnPosition + glm::vec3(0.0f, levitationHeight, 0.0f);
        levitationStartTime = glfwGetTime();
        updateAABB();
    }

    void updateAABB() {
        if (!model) return;
        glm::vec3 localMin = model->getBoundingBoxMin() * scale;
        glm::vec3 localMax = model->getBoundingBoxMax() * scale;
        AABBmin = localMin + position;
        AABBmax = localMax + position;
    }

    void updateLevitation(float currentTime) {
        if (state == OrbState::LEVITATING) {
            float elapsedTime = currentTime - levitationStartTime;
            float t = glm::clamp(elapsedTime / levitationDuration, 0.0f, 1.0f);
            t = t * t * (3.0f - 2.0f * t); // Smoothstep
            position = glm::mix(spawnPosition, idlePosition, t);
            updateAABB();
            if (t >= 1.0f) {
                state = OrbState::IDLE;
                position = idlePosition;
                updateAABB();
            }
        }
    }
};

enum class GameState {
    TITLE_SCREEN,
    IN_GAME,
};

struct LocksOnDoor {
    glm::vec3 position = glm::vec3(0.0f); // Position of the lock on the door
    float RotX = 0.0f; // Rotation around X-axis
    float RotY = 0.0f; // Rotation around Y-axis
    float RotZ = 0.0f; // Rotation around Z-axis
    bool isLocked = true;
    bool interacted = false; // Whether the lock has been interacted with
    float unlockStartTime = -1.0f; // Start time of the unlock animation
    bool animDone = false; // Whether the unlock animation is done
    int keyIndex = -1; // Index of the key used to unlock the door
    bool playUnlockSound = false; // Whether to play the unlock sound
};