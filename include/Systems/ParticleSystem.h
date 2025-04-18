#pragma once

#include "../seecs.h"
#include "../Components.h"
#include "../Config.h"
#include "Random.h"

class ParticleSystem {
public:
    static void Update(seecs::ECS& ecs, float deltaTime);

private:
    static void UpdateParticles(seecs::ECS& ecs, float deltaTime);
    static void UpdateEmitters(seecs::ECS& ecs, float deltaTime);
};