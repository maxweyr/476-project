#pragma once

#include <string>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Use inline for variables (C++17 and later)
// This prevents multiple definition errors when included in multiple translation units

namespace Config {
    // ===========================
    // Global application settings
    // ===========================
    namespace App {
        inline std::string RESOURCE_DIR = "../resources";
        inline bool DEBUG_MODE = false;
        inline float TIME_STEP = 1.0f / 60.0f;
        inline float GAME_SPEED = 1.0f;
    }

    // ============================
    // Window and graphics settings
	// ============================
    namespace Graphics {
        inline int WINDOW_WIDTH = 1280;
        inline int WINDOW_HEIGHT = 720;
        inline float ASPECT_RATIO = WINDOW_WIDTH / static_cast<float>(WINDOW_HEIGHT);
        inline bool VSYNC_ENABLED = true;
        inline glm::vec3 CLEAR_COLOR = glm::vec3(0.12f, 0.34f, 0.56f);
        inline bool WIREFRAME_DEBUG = false;

        // Lighting
        namespace Lighting {
            inline glm::vec3 AMBIENT_LIGHT = glm::vec3(0.0f, 1.0f, 0.0f);
            inline float SPECULAR_INTENSITY = 0.5f;
            inline float DEFAULT_SHININESS = 32.0f;
        }

        // Shadows
        namespace Shadows {
            inline int SHADOW_MAP_RESOLUTION = 1024;
            inline float SHADOW_BIAS = 0.005f;
            inline bool SHADOWS_ENABLED = true;
        }
    }

    // ===============
    // Camera settings
	// ===============
    namespace Camera {
        inline float DEFAULT_FOV = 45.0f;
        inline float DEFAULT_NEAR_PLANE = 0.1f;
        inline float DEFAULT_FAR_PLANE = 100.0f;
        inline float CAMERA_DAMPING = 0.9f;
        inline float CAMERA_ROTATION_SPEED = 0.05f;
        inline float CAMERA_MOVEMENT_SPEED = 0.1f;
        inline float MOUSE_SENSITIVITY = 0.003f;
        inline float ZOOM_SPEED = 0.1f;
        inline glm::vec3 DEFAULT_POSITION = glm::vec3(0.0f, 0.0f, -5.0f);
        inline glm::vec3 DEFAULT_LOOK_AT = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    // ============================
    // Model and animation settings
	// ============================
    namespace Models {
        inline float DEFAULT_MODEL_SCALE = 1.0f;
        inline int MAX_BONES = 100;

        namespace Animation {
            inline float DEFAULT_ANIMATION_SPEED = 1.0f;
            inline bool LOOP_ANIMATIONS = true;
            inline float BLEND_TIME = 0.2f;  // Time to blend between animations
        }

        namespace Materials {
            inline glm::vec3 DEFAULT_AMBIENT = glm::vec3(0.2f, 0.2f, 0.2f);
            inline glm::vec3 DEFAULT_DIFFUSE = glm::vec3(0.8f, 0.8f, 0.8f);
            inline glm::vec3 DEFAULT_SPECULAR = glm::vec3(0.5f, 0.5f, 0.5f);
            inline float DEFAULT_SHININESS = 32.0f;
        }
    }

    // ========================
    // Particle system settings
    // ========================
    namespace Particles {
        inline int MAX_PARTICLES = 300;
        inline float DEFAULT_SIZE = 15.0f;
        inline float DEFAULT_LIFESPAN = 5.0f;
        inline float SPAWN_RATE = 0.01f;
        inline float FADE_RATE = 0.05f;
        inline glm::vec3 DEFAULT_COLOR = glm::vec3(0.9f, 0.7f, 0.7f);
        inline glm::vec3 DEFAULT_GRAVITY = glm::vec3(0.0f, -0.01f, 0.0f);

        namespace Emitter {
            inline float MIN_VELOCITY = 0.01f;
            inline float MAX_VELOCITY = 0.1f;
            inline float MIN_SCALE = 0.5f;
            inline float MAX_SCALE = 1.5f;
            inline float EMISSION_RADIUS = 0.2f;
        }
    }

    // ==============
    // Input settings
    // ==============
    namespace Input {
        inline float KEY_REPEAT_DELAY = 0.2f;
        inline float DOUBLE_CLICK_TIME = 0.3f;

        namespace Keybinds {
			inline int WIREFRAME_TOGGLE = GLFW_KEY_F1; // Toggle wireframe mode
			inline int RESET_PARTICLE_SYSTEM = GLFW_KEY_F5; // Reset the particle system
        }
    }
}