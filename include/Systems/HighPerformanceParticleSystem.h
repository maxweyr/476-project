#pragma once

#include "seecs.h"
#include "../Components.h"
#include "../Config.h"
#include "Random.h"
#include <vector>
#include <glad/glad.h>

class HighPerformanceParticleSystem {
public:
    static void Initialize(seecs::ECS& ecs, seecs::EntityID particleSysEntity);
    static void Update(seecs::ECS& ecs, float deltaTime);
    static void Render(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

private:
    static void EmitParticles(HPParticleComponent& system, int count);
    static void UpdateGPUBuffers(HPParticleComponent& system);
};