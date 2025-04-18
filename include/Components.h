#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <memory>
#include "Texture.h"
#include "Program.h"
#include "Model.h"

// Components for our ECS

// Transform component - position, rotation, scale
struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

// RenderableComponent - holds data needed for rendering
struct RenderableComponent {
    std::shared_ptr<Model> model;
};

// Material component - holds shader and material properties
struct MaterialComponent {
    std::shared_ptr<Program> shader;
    glm::vec3 color = glm::vec3(1.0f);
    std::shared_ptr<Texture> texture;
};

// Camera component
struct CameraComponent {
    float fov = 45.0f;
    float aspectRatio = 1.0f;
    float nearPlane = 0.01f;
    float farPlane = 100.0f;
    glm::mat4 viewMatrix = glm::mat4(1.0f);
};

// Particle component - for the particle system
struct ParticleComponent {
    glm::vec3 velocity = glm::vec3(0.0f);
    float lifespan = 1.0f;
    float remainingLife = 1.0f;
    float mass = 1.0f;
    float charge = 1.0f;
    float damping = 0.0f;
    bool active = true;
};

// Particle emitter component - spawns particles
struct ParticleEmitterComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, -0.01f, 0.0f);
    float spawnRate = 0.01f;
    float timeSinceLastSpawn = 0.0f;
    int maxParticles = 300;
    int activeParticles = 0;

    // Particle properties for new particles
    glm::vec3 initialVelocityMin = glm::vec3(-0.27f, -0.1f, -0.3f);
    glm::vec3 initialVelocityMax = glm::vec3(0.3f, 0.9f, 0.27f);
    float initialLifespanMin = 100.0f;
    float initialLifespanMax = 200.0f;
    float initialScaleMin = 0.2f;
    float initialScaleMax = 1.0f;
};

// Tag component to mark entities for specific roles
struct ParticleSystemComponent {
    GLuint vertexArrayObj = 0;
    GLuint vertexBufferObj = 0;
    float stepSize = 0.01f;
    float timeAccumulator = 0.0f;
};

// Input handler component for controlling the camera or other entities
struct InputComponent {
    bool keys[256] = { false };
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    float camRotation = 0.0f;
};

struct ModelComponent {
    std::shared_ptr<Model> model;
    std::string modelPath;
    std::string fileType;
    int currentAnimation = 0;
    bool animationPlaying = false;
    float animationSpeed = 1.0f;

    // Optional overrides for material properties
    bool overrideMaterial = false;
    glm::vec3 ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specular = glm::vec3(0.5f, 0.5f, 0.5f);
    float shininess = 32.0f;
};

struct HPParticleComponent {
    // GPU buffers
    GLuint particleVAO = 0;
    GLuint positionBuffer = 0;
    GLuint velocityBuffer = 0;
    GLuint lifetimeBuffer = 0;
    GLuint colorBuffer = 0;

    // CPU side data (for initialization and updates)
    std::vector<glm::vec4> positions;    // xyz = position, w = size
    std::vector<glm::vec4> velocities;   // xyz = velocity, w = damping
    std::vector<glm::vec4> colors;       // rgb = color, a = alpha
    std::vector<float> lifetimes;        // x = current lifetime, y = max lifetime

    // Color properties
    glm::vec3 baseColor = glm::vec3(1.0f, 0.7f, 1.0f); // Default: purple
    float colorVariation = 0.1f; // How much to vary the color for each particle

    // System parameters
    int maxParticles = 1000000;
    float spawnRate = 1000.0f;           // particles per second
    float timeSinceLastSpawn = 0.0f;
    glm::vec3 emitterPosition = glm::vec3(0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, -0.01f, 0.0f);

    // Particle properties
    glm::vec3 initialVelocityMin = glm::vec3(-0.27f, -0.1f, -0.3f);
    glm::vec3 initialVelocityMax = glm::vec3(0.3f, 0.9f, 0.27f);
    float initialLifespanMin = 3.0f;
    float initialLifespanMax = 6.0f;
    float initialSizeMin = 0.2f;
    float initialSizeMax = 1.0f;
    float emissionRadius = 1.0f;

    // Statistics
    int activeParticles = 0;
};