#pragma once

#include "../seecs.h"
#include "../Components.h"
#include "../Model.h"
#include "../GLSL.h"
#include "../Config.h"
#include <iostream>

class ModelSystem {
public:
    // Load a model and attach it to the entity
    static void LoadModel(seecs::ECS& ecs, seecs::EntityID entity, const std::string& modelPath);

    // Update all models (animations, etc.)
    static void Update(seecs::ECS& ecs, float deltaTime);

    // Animation control methods
    static void SetAnimation(seecs::ECS& ecs, seecs::EntityID entity, int animIndex);
    static void SetAnimationPlaying(seecs::ECS& ecs, seecs::EntityID entity, bool playing);
    static void SetAnimationSpeed(seecs::ECS& ecs, seecs::EntityID entity, float speed);
};