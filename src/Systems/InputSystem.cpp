#include "InputSystem.h"

void InputSystem::Update(seecs::ECS& ecs, WindowManager* windowManager) {
    auto view = ecs.View<InputComponent>();

    view.ForEach([&](InputComponent& input) {
        // Use camera rotation speed from config
        float rotationSpeed = Config::Camera::CAMERA_ROTATION_SPEED;
        if (input.keys[GLFW_KEY_A]) {
            input.camRotation -= rotationSpeed;
        }
        if (input.keys[GLFW_KEY_D]) {
            input.camRotation += rotationSpeed;
        }
    });
}

void InputSystem::HandleKeyEvent(seecs::ECS& ecs, int key, int action) {
    if (key < 0 || key >= 256) return;

    auto view = ecs.View<InputComponent>();
    view.ForEach([&](InputComponent& input) {
        if (action == GLFW_PRESS) {
            input.keys[key] = true;

            // Handle key press events
            if (key == Config::Input::Keybinds::WIREFRAME_TOGGLE && Config::App::DEBUG_MODE) {
                // Toggle wireframe mode for debugging
                Config::Graphics::WIREFRAME_DEBUG = !Config::Graphics::WIREFRAME_DEBUG;
            }
            else if (key == Config::Input::Keybinds::RESET_PARTICLE_SYSTEM) {
                // Reset the particle system when F5 is pressed
                ResetParticleSystem(ecs);
            }
        }
        else if (action == GLFW_RELEASE) {
            input.keys[key] = false;

            // Could handle key release events here
        }
    });
}

void InputSystem::ResetParticleSystem(seecs::ECS& ecs) {
    auto view = ecs.View<ParticleEmitterComponent>();

    view.ForEach([&](seecs::EntityID emitterId, ParticleEmitterComponent& emitter) {
        auto particleView = ecs.View<ParticleComponent, TransformComponent>();

        particleView.ForEach([&](seecs::EntityID particleId, ParticleComponent& particle, TransformComponent& transform) {
            // Reset the particle to its initial state
            float lifeMin = (emitter.initialLifespanMin > 0) ?
                emitter.initialLifespanMin :
                Config::Particles::DEFAULT_LIFESPAN * 0.5f;

            float lifeMax = (emitter.initialLifespanMax > 0) ?
                emitter.initialLifespanMax :
                Config::Particles::DEFAULT_LIFESPAN;

            particle.remainingLife = Random::Range(lifeMin, lifeMax);
            particle.active = true;

            transform.position = emitter.position;

            // Set velocity using config values if component values aren't set
            glm::vec3 velMin = (glm::length(emitter.initialVelocityMin) > 0) ?
                emitter.initialVelocityMin :
                glm::vec3(-Config::Particles::Emitter::MIN_VELOCITY);

            glm::vec3 velMax = (glm::length(emitter.initialVelocityMax) > 0) ?
                emitter.initialVelocityMax :
                glm::vec3(Config::Particles::Emitter::MAX_VELOCITY);

            particle.velocity.x = Random::Range(velMin.x, velMax.x);
            particle.velocity.y = Random::Range(velMin.y, velMax.y);
            particle.velocity.z = Random::Range(velMin.z, velMax.z);

            // Set scale using config values if component values aren't set
            float scaleMin = (emitter.initialScaleMin > 0) ?
                emitter.initialScaleMin :
                Config::Particles::Emitter::MIN_SCALE;

            float scaleMax = (emitter.initialScaleMax > 0) ?
                emitter.initialScaleMax :
                Config::Particles::Emitter::MAX_SCALE;

            transform.scale = glm::vec3(Random::Range(scaleMin, scaleMax));
        });
    });
}