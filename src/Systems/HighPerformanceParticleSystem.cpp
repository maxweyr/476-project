#include "Systems/Systems.h"

void HighPerformanceParticleSystem::Initialize(seecs::ECS& ecs, seecs::EntityID particleSysEntity) {
    auto& system = ecs.Get<HPParticleComponent>(particleSysEntity);

    // Initialize CPU-side buffers
    system.positions.resize(system.maxParticles, glm::vec4(0.0f));
    system.velocities.resize(system.maxParticles, glm::vec4(0.0f));
    system.colors.resize(system.maxParticles, glm::vec4(1.0f));
    system.lifetimes.resize(system.maxParticles * 2, 0.0f); // current and max lifetime

    // Initialize all particles as inactive
    for (int i = 0; i < system.maxParticles; i++) {
        system.lifetimes[i * 2] = -1.0f; // Negative lifetime = inactive
    }

    // Create VAO and buffers
    glGenVertexArrays(1, &system.particleVAO);
    glBindVertexArray(system.particleVAO);

    // Position buffer (xyz = position, w = size)
    glGenBuffers(1, &system.positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, system.positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, system.positions.size() * sizeof(glm::vec4), system.positions.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(0, 1); // One per instance

    // Velocity buffer (xyz = velocity, w = damping)
    glGenBuffers(1, &system.velocityBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, system.velocityBuffer);
    glBufferData(GL_ARRAY_BUFFER, system.velocities.size() * sizeof(glm::vec4), system.velocities.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(1, 1);

    // Color buffer (rgba)
    glGenBuffers(1, &system.colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, system.colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, system.colors.size() * sizeof(glm::vec4), system.colors.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(2, 1);

    // Lifetime buffer (x = current, y = max)
    glGenBuffers(1, &system.lifetimeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, system.lifetimeBuffer);
    glBufferData(GL_ARRAY_BUFFER, system.lifetimes.size() * sizeof(float), system.lifetimes.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Emit initial particles
    EmitParticles(system, system.maxParticles / 10); // Start with 10% of max
}

void HighPerformanceParticleSystem::Update(seecs::ECS& ecs, float deltaTime) {
    auto view = ecs.View<HPParticleComponent, TransformComponent>();

    view.ForEach([&](seecs::EntityID entity, HPParticleComponent& system, TransformComponent& transform) {
        // Update emitter position from entity transform
        system.emitterPosition = transform.position;

        // Update spawn timer and emit new particles
        system.timeSinceLastSpawn += deltaTime;
        float spawnInterval = 1.0f / system.spawnRate;

        if (system.timeSinceLastSpawn >= spawnInterval) {
            int particlesToSpawn = static_cast<int>(system.timeSinceLastSpawn / spawnInterval);
            system.timeSinceLastSpawn -= particlesToSpawn * spawnInterval;

            EmitParticles(system, particlesToSpawn);
        }

        // Update all active particles on CPU
        system.activeParticles = 0;

        for (int i = 0; i < system.maxParticles; i++) {
            float& currentLife = system.lifetimes[i * 2];
            float& maxLife = system.lifetimes[i * 2 + 1];

            // Skip inactive particles
            if (currentLife < 0) continue;

            // Update lifetime
            currentLife -= deltaTime;

            // Deactivate expired particles
            if (currentLife <= 0) {
                currentLife = -1.0f;
                continue;
            }

            system.activeParticles++;

            // Update position based on velocity
            glm::vec3& pos = *reinterpret_cast<glm::vec3*>(&system.positions[i]);
            glm::vec3& vel = *reinterpret_cast<glm::vec3*>(&system.velocities[i]);
            float damping = system.velocities[i].w;

            // Update velocity (apply gravity and damping)
            vel += system.gravity * deltaTime;
            vel *= (1.0f - damping * deltaTime);

            // Update position
            pos += vel * deltaTime;

            // Update color alpha based on lifetime
            float lifeRatio = currentLife / maxLife;

            // Fade in during first 10% of life
            if (lifeRatio > 0.9f) {
                system.colors[i].a = (1.0f - lifeRatio) * 10.0f;
            }
            // Fade out during last 30% of life
            else if (lifeRatio < 0.3f) {
                system.colors[i].a = lifeRatio / 0.3f;
            }
            else {
                system.colors[i].a = 1.0f;
            }
        }

        // Update GPU buffers
        UpdateGPUBuffers(system);
        });
}

void HighPerformanceParticleSystem::EmitParticles(HPParticleComponent& system, int count) {
    count = std::min<int>(count, system.maxParticles); // Ensure we don't exceed max particles

    int emitted = 0;
    for (int i = 0; i < system.maxParticles && emitted < count; i++) {
        // Find inactive particles
        if (system.lifetimes[i * 2] < 0) {
            // Position
            float angle = Random::Range(0.0f, glm::two_pi<float>());
            float distance = Random::Range(0.0f, system.emissionRadius);
            glm::vec3 offset = glm::vec3(cos(angle) * distance, 0, sin(angle) * distance);

            system.positions[i] = glm::vec4(
                system.emitterPosition + offset,
                Random::Range(system.initialSizeMin, system.initialSizeMax) // Size in w component
            );

            // Velocity and damping
            system.velocities[i] = glm::vec4(
                Random::Range(system.initialVelocityMin.x, system.initialVelocityMax.x),
                Random::Range(system.initialVelocityMin.y, system.initialVelocityMax.y),
                Random::Range(system.initialVelocityMin.z, system.initialVelocityMax.z),
                0.05f // Damping
            );

            // Color with random variation based on component settings
            float r = system.baseColor.r + Random::Range(-system.colorVariation, system.colorVariation);
            float g = system.baseColor.g + Random::Range(-system.colorVariation, system.colorVariation);
            float b = system.baseColor.b + Random::Range(-system.colorVariation, system.colorVariation);

            // Clamp colors to valid range
            r = std::max<float>(0.0f, std::min<float>(1.0f, r));
            g = std::max<float>(0.0f, std::min<float>(1.0f, g));
            b = std::max<float>(0.0f, std::min<float>(1.0f, b));

            system.colors[i] = glm::vec4(r, g, b, 0.0f); // Start with alpha = 0 for fade-in

            // Lifetime
            float lifetime = Random::Range(system.initialLifespanMin, system.initialLifespanMax);
            system.lifetimes[i * 2] = lifetime;     // Current lifetime
            system.lifetimes[i * 2 + 1] = lifetime; // Max lifetime

            emitted++;
        }
    }
}

void HighPerformanceParticleSystem::UpdateGPUBuffers(HPParticleComponent& system) {
    // Update position buffer
    glBindBuffer(GL_ARRAY_BUFFER, system.positionBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, system.positions.size() * sizeof(glm::vec4), system.positions.data());

    // Update color buffer
    glBindBuffer(GL_ARRAY_BUFFER, system.colorBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, system.colors.size() * sizeof(glm::vec4), system.colors.data());

    // Update lifetime buffer
    glBindBuffer(GL_ARRAY_BUFFER, system.lifetimeBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, system.lifetimes.size() * sizeof(float), system.lifetimes.data());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void HighPerformanceParticleSystem::Render(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    auto view = ecs.View<HPParticleComponent, MaterialComponent>();

    view.ForEach([&](HPParticleComponent& system, MaterialComponent& material) {
        if (!material.shader) return;

        material.shader->bind();

        // Set matrices
        glUniformMatrix4fv(material.shader->getUniform("P"), 1, GL_FALSE, glm::value_ptr(projMatrix));
        glUniformMatrix4fv(material.shader->getUniform("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(material.shader->getUniform("M"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

        // Set texture if available
        if (material.texture) {
            material.texture->bind(material.shader->getUniform("alphaTexture"));
        }

        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        // Disable depth writing but keep depth testing
        glDepthMask(GL_FALSE);

        // Bind VAO and draw
        glBindVertexArray(system.particleVAO);
        glDrawArraysInstanced(GL_POINTS, 0, 1, system.maxParticles);
        glBindVertexArray(0);

        // Restore state
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        material.shader->unbind();
        });
}