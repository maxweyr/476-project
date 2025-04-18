#pragma once

#include "../seecs.h"
#include "../Components.h"
#include "../Program.h"
#include "../MatrixStack.h"
#include "../GLSL.h"
#include "../Config.h"
#include "ModelSystem.h"
#include <glm/gtc/type_ptr.hpp>

class RenderSystem {
public:
    static void Render(seecs::ECS& ecs);

private:
    static void RenderMeshes(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    static void RenderModels(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    static void RenderParticles(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    static void UpdateParticleBuffer(seecs::ECS& ecs, GLuint vbo);
};