#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <string>
#include <random>

// It's often better to use static const or constexpr for typed constants
// instead of #define for better type safety and namespacing

namespace Config {// --- Global Game Settings (Config::<thing you want>)---

    const std::string RESOURCE_DIRECTORY_PREFIX = "../resources"; // Default, can be overridden

    // Debugging --- set all to false for release builds
    constexpr bool DEBUG_ENEMY_MOVEMENT = true;
    constexpr bool DEBUG_ROOM_PLACEMENT = false;
    constexpr bool DEBUG_TEX_LOADING = false;
    constexpr bool DEBUG_PLAYER_AABB = false;
    constexpr bool DEBUG_ORB_PICKUP = false;
    constexpr bool DEBUG_PLAYER_HP = false;
    constexpr bool DEBUG_SHADER = true; // Sets verbose flag for shaders
    constexpr bool DEBUG_SHADER_PARAMS = false; // Debug lighting settings

    // In-Game Debugging (can be toggled real time)
    inline static bool DEBUG_LIGHTING = false;
    inline static bool DEBUG_GEOM = false;
    inline static bool DEBUG_AABBS = false;
    inline static bool WRITE_FBOS = false;

    // Rendering & Shaders
    constexpr int MAX_BONES = 200;
    constexpr float ORTHO_SIZE = 40.0f;
    inline static bool SHADOW = true;
    inline static float EXPOSURE = 1.5f; // Default exposure
    inline static float SATURATION = 1.7f; // Default saturation
	inline static vec3 LIGHT_COLOR = vec3(1.0f, 1.0f, 1.0f); // Default light color
    inline static bool FIRST_PASS = true;
    inline static bool DEFER = false;
    constexpr int MAX_LIGHTS = 100;
    constexpr vec3 CANDELABRA_L_COLOR = vec3(1.0f, 0.58f, 0.25f) * 10.0f;
    constexpr vec3 SHELF_L_COLOR = vec3(1.0f, 0.58f, 0.25f) * 5.0f;

    // Game Elements --- should be set to true to enable drawing of the system
    constexpr bool DRAW_PARTICLES = true;
    constexpr bool DRAW_HEALTHBAR = true;
	constexpr bool DRAW_MINIMAP = true;
    constexpr bool DRAW_PLAYER_DAMAGE = true;
    constexpr bool DRAW_PAW_PRINTS = true;

    // Default Window Dimensions
    constexpr int DEFAULT_WINDOW_WIDTH = 1920;
    constexpr int DEFAULT_WINDOW_HEIGHT = 1080;

    // Player settings
    constexpr int INVENTORY_SIZE = 8;
    constexpr float PLAYER_HP_MAX = 100.0f;
    constexpr float PLAYER_HP_MIN = 0.0f;
    constexpr float PLAYER_MOVE_SPEED = 0.045f;
    constexpr float PLAYER_HIT_DURATION = 0.5f; // Duration of hit effect

    // Enemy settings
    constexpr int NUM_ENEMIES = 3;
    // constexpr float ENEMY_HP_MAX = 200.0f;
    // constexpr float ENEMY_HP_MIN = 0.0f;
    // constexpr float ENEMY_MOVE_SPEED = 0.02f;
    constexpr float ENEMY_ATTACK_DAMAGE = 10.0f;
    constexpr float ENEMY_HIT_DURATION = 0.5f; // Duration of hit effect
    constexpr float ENEMY_HIT_COOLDOWN = 1.0f; // Cooldown before enemy can be hit again

    // Spell settings
    constexpr float ORB_HEAL_AMOUNT = 20.0f; // Amount of health restored by healing orbs

    // Ice Elemental settings
    constexpr float ICE_ELEMENTAL_HP_MAX = 200.0f;
    constexpr float ICE_ELEMENTAL_TRANS_Y = 2.0f;
    constexpr float ICE_ELEMENTAL_MOVE_SPEED = 2.0f;
    constexpr float ICE_ELEMENTAL_ROTATION_SPEED = 1.0f; // Radians per second
    constexpr float ICE_ELEMENTAL_MELEE_DAMAGE = 33.0f;
    constexpr float ICE_ELEMENTAL_MELEE_SPEED = 1.0f;
    constexpr float ICE_ELEMENTAL_MELEE_RANGE = 3.0f;
    constexpr float ICE_ELEMENTAL_AGGRO_RANGE = 4.0f; // Cooldown before enemy can attack again
    constexpr float ICE_ELEMENTAL_SIGHT_RANGE = 10.0f; // scale
    constexpr float ICE_ELEMENTAL_TERRITORY_RADIUS = 20.0f; // Territory radius for ice elemental

    // Fire Elemental settings
    constexpr float FIRE_ELEMENTAL_HP_MAX = 150.0f;
    constexpr float FIRE_ELEMENTAL_TRANS_Y = 2.0f;
    constexpr float FIRE_ELEMENTAL_MOVE_SPEED = 3.2f;
    constexpr float FIRE_ELEMENTAL_ROTATION_SPEED = 1.2f; // Radians per second
    constexpr float FIRE_ELEMENTAL_MELEE_DAMAGE = 20.0f;
    constexpr float FIRE_ELEMENTAL_MELEE_SPEED = 1.2f;
    constexpr float FIRE_ELEMENTAL_MELEE_RANGE = 3.5f;
    constexpr float FIRE_ELEMENTAL_AGGRO_RANGE = 7.0f; // Aggro range for fire elemental
    constexpr float FIRE_ELEMENTAL_SIGHT_RANGE = 14.0f; // Teleport interval for fire elemental
    constexpr float FIRE_ELEMENTAL_TERRITORY_RADIUS = 10.0f; // Teleport interval for fire elemental
    // constexpr float FIRE_ELEMENTAL_SCALE = vec3(1.0f, 1.0f, 1.0f); // scale

    // Lightning Elemental settings
    constexpr float LIGHTNING_ELEMENTAL_HP_MAX = 100.0f;
    constexpr float LIGHTNING_ELEMENTAL_TRANS_Y = 2.0f;
    constexpr float LIGHTNING_ELEMENTAL_MOVE_SPEED = 4.6f;
    constexpr float LIGHTNING_ELEMENTAL_ROTATION_SPEED = 1.8f; // Radians per second
    constexpr float LIGHTNING_ELEMENTAL_MELEE_DAMAGE = 10.0f;
    constexpr float LIGHTNING_ELEMENTAL_MELEE_SPEED = 1.8f;
    constexpr float LIGHTNING_ELEMENTAL_MELEE_RANGE = 5.0f;
    constexpr float LIGHTNING_ELEMENTAL_AGGRO_RANGE = 3.0f; // Aggro range for lightning elemental
    constexpr float LIGHTNING_ELEMENTAL_SIGHT_RANGE = 20.0f; // Sight range for lightning elemental
    constexpr float LIGHTNING_ELEMENTAL_TP_INTERVAL = 1.5f;
    constexpr float LIGHTNING_ELEMENTAL_TERRITORY_RADIUS = 30.0f; // Territory radius for lightning elemental
    // constexpr float LIGHTNING_ELEMENTAL_SCALE = vec3(1.0f, 1.0f, 1.0f); // scale

    // Boss Enemy settings
    constexpr float BOSS_SLAM_DAMAGE = 80.0f; // Damage dealt by boss slam attack
    constexpr float BOSS_SLAM_COOLDOWN = 3.0f; // Cooldown for boss slam attack
    constexpr float BOSS_SLAM_DURATION = 1.5f; // Duration of boss slam attack

    // Projectile settings
    constexpr float PROJECTILE_DAMAGE = 100.0f;

    // Camera settings
    constexpr float CAMERA_DEFAULT_RADIUS = 5.0f;
    constexpr float CAMERA_DEFAULT_THETA_DEGREES = 0.0f;
    constexpr float CAMERA_DEFAULT_PHI_DEGREES = -30.0f;
    constexpr float CAMERA_MOUSE_SENSITIVITY = 0.005f;
    constexpr float CAMERA_SCROLL_SENSITIVITY_DEGREES = 1.3f;
    constexpr float CAMERA_PHI_MIN_DEGREES = -80.0f;
    constexpr float CAMERA_PHI_MAX_DEGREES = -10.0f;

    // Gameplay
    constexpr float INTERACTION_RADIUS = 5.0f;
    constexpr float SPELL_PROJECTILE_SPEED = 20.0f;
    constexpr float SPELL_PROJECTILE_LIFETIME = 2.0f;
    constexpr glm::vec3 SPELL_PROJECTILE_SCALE = glm::vec3(0.05f, 0.05f, 0.6f);
    constexpr float SPELL_DAMAGE_AMOUNT = 25.0f;
    constexpr float BOOK_TIME_LIMIT = 5.0f;

    // Scene
    constexpr float GROUND_SIZE = 20.0f;
    constexpr float GROUND_HEIGHT = 0.0f;
    constexpr vec3 LIB_CENTER = vec3(0.0f, GROUND_HEIGHT, 0.0f);
    constexpr vec3 BOSS_CENTER = vec3(0.0f, GROUND_HEIGHT, 60.0f);

    // Rotation Constants
    constexpr float HALF_PI = glm::half_pi<float>();
    constexpr float PI = glm::pi<float>();

    // Paw Print Settings
    constexpr int PRINTS_MAX = 16;
    constexpr float PRINTS_LIFETIME = 5.0f; // seconds
    inline static bool LEFT_PAW = false;
    constexpr float PAW_SPACING = 0.1f;
    inline static vec2 LAST_PAW_POS;
    constexpr float MIN_PAW_DIST = 0.2f;

    // Sun / Moon Settings
    inline static vec3 sunPos;
    inline static vec3 sunColor = vec3(1.0f, 0.9f, 0.6f);
    inline static float previousAngle = 0.0f;
    inline static vec3 sunOrbitCenter = LIB_CENTER;
    inline static vec3 targetOrbitCenter = LIB_CENTER;
    inline static vec3 shadowTargetCenter = LIB_CENTER;

    // --- Utility Functions ---

    // A random float generator
    inline float randFloat(float l, float h) {
        if (h < l) {
            float t = h;
            h = l;
            l = t;
        }
        static std::mt19937 generator(std::random_device{}());
        std::uniform_real_distribution<float> distribution(l, h);
        return distribution(generator);
    }
}
