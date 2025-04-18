#include "ParticleSystem.h"

void ParticleSystem::Update(seecs::ECS& ecs, float deltaTime) {
    UpdateParticles(ecs, deltaTime);
    UpdateEmitters(ecs, deltaTime);
}

void ParticleSystem::UpdateParticles(seecs::ECS& ecs, float deltaTime) {
    auto emitterView = ecs.View<ParticleEmitterComponent>();
	glm::vec3 gravityForce = Config::Particles::DEFAULT_GRAVITY; // Default gravitu if no emitter is found

    // Find first emitter to get gravity
    emitterView.ForEach([&](ParticleEmitterComponent& emitter) {
        gravityForce = emitter.gravity;
        });

    auto particleView = ecs.View<ParticleComponent, TransformComponent>();

    particleView.ForEach([&](seecs::EntityID id, ParticleComponent& particle, TransformComponent& transform) {
        // Skip inactive particles
        if (!particle.active) return;

        // Decrease remaining lifetime
        particle.remainingLife -= deltaTime;

        // Check if particle has expired
        if (particle.remainingLife <= 0.0f) {
            particle.active = false;
            return;
        }

        // Simple physics update
        transform.position += particle.velocity * deltaTime;
        particle.velocity += gravityForce * deltaTime;
        });
}

void ParticleSystem::UpdateEmitters(seecs::ECS& ecs, float deltaTime) {
    auto view = ecs.View<ParticleEmitterComponent>();

    view.ForEach([&](seecs::EntityID emitterId, ParticleEmitterComponent& emitter) {
        emitter.timeSinceLastSpawn += deltaTime;

        // Use config spawn rate if emitter's rate is not set
        float effectiveSpawnRate = (emitter.spawnRate > 0) ?
            emitter.spawnRate :
            Config::Particles::SPAWN_RATE;

        // Find inactive particles to reactivate
        if (emitter.timeSinceLastSpawn >= effectiveSpawnRate) {
            auto particleView = ecs.View<ParticleComponent, TransformComponent>();
            bool particleReactivated = false;

            particleView.ForEach([&](seecs::EntityID particleId, ParticleComponent& particle, TransformComponent& transform) {
                // If we already found a particle to reactivate, skip the rest
                if (particleReactivated) return;

                // Look for an inactive particle
                if (!particle.active) {
                    // Reactivate this particle
                    particle.active = true;

                    // Use config values for defaults if component values aren't set
                    float lifeMin = (emitter.initialLifespanMin > 0) ?
                        emitter.initialLifespanMin :
                        Config::Particles::DEFAULT_LIFESPAN * 0.5f;

                    float lifeMax = (emitter.initialLifespanMax > 0) ?
                        emitter.initialLifespanMax :
                        Config::Particles::DEFAULT_LIFESPAN;

                    particle.remainingLife = Random::Range(lifeMin, lifeMax);

                    // Position particles within emission radius if set
                    if (Config::Particles::Emitter::EMISSION_RADIUS > 0) {
                        float angle = Random::Range(0, 2 * glm::pi<float>());
                        float distance = Random::Range(0, Config::Particles::Emitter::EMISSION_RADIUS);
                        transform.position = emitter.position +
                            glm::vec3(cos(angle) * distance, 0, sin(angle) * distance);
                    }
                    else {
                        transform.position = emitter.position;
                    }

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

                    particleReactivated = true;
                }
                });

            // Reset the spawn timer if we found a particle to reactivate
            if (particleReactivated) {
                emitter.timeSinceLastSpawn = 0.0f;
            }
        }
    });
}