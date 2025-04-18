#include "Systems.h"

void RenderSystem::Render(seecs::ECS& ecs) {
    // Get the camera information
    auto cameraView = ecs.View<CameraComponent, TransformComponent, InputComponent>();
    glm::mat4 viewMatrix(1.0f);
    glm::mat4 projMatrix(1.0f);

    cameraView.ForEach([&](CameraComponent& camera, TransformComponent& transform, InputComponent& input) {
        // Create view matrix
        auto P = std::make_shared<MatrixStack>();
        auto V = std::make_shared<MatrixStack>();

        P->pushMatrix();

        // Use config values if component values aren't set
        float fov = (camera.fov > 0) ? camera.fov : Config::Camera::DEFAULT_FOV;
        float aspectRatio = (camera.aspectRatio > 0) ? camera.aspectRatio : Config::Graphics::ASPECT_RATIO;
        float nearPlane = (camera.nearPlane > 0) ? camera.nearPlane : Config::Camera::DEFAULT_NEAR_PLANE;
        float farPlane = (camera.farPlane > 0) ? camera.farPlane : Config::Camera::DEFAULT_FAR_PLANE;

        P->perspective(fov, aspectRatio, nearPlane, farPlane);
        projMatrix = P->topMatrix();
        P->popMatrix();

        V->pushMatrix();
        V->translate(transform.position);
        V->rotate(input.camRotation, glm::vec3(0, 1, 0));
        viewMatrix = V->topMatrix();
        V->popMatrix();

        // Update the camera component
        camera.viewMatrix = viewMatrix;
    });

    // Use wireframe debug mode if enabled
    if (Config::Graphics::WIREFRAME_DEBUG) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // Render regular mesh entities
    RenderMeshes(ecs, viewMatrix, projMatrix);

    // Render model entities
    RenderModels(ecs, viewMatrix, projMatrix);

    // Render high-performance particles
    HighPerformanceParticleSystem::Render(ecs, viewMatrix, projMatrix);

    // Render particles
    RenderParticles(ecs, viewMatrix, projMatrix);

    // Reset polygon mode if wireframe was enabled
    if (Config::Graphics::WIREFRAME_DEBUG) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void RenderSystem::RenderMeshes(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    auto view = ecs.View<RenderableComponent, TransformComponent, MaterialComponent>();

    view.ForEach([&](RenderableComponent& renderable, TransformComponent& transform, MaterialComponent& material) {
        if (!renderable.model || !material.shader) return;

        material.shader->bind();

        // Set up uniforms like matrices, materials, etc.
        CHECKED_GL_CALL(glUniformMatrix4fv(material.shader->getUniform("P"), 1, GL_FALSE, glm::value_ptr(projMatrix)));
        CHECKED_GL_CALL(glUniformMatrix4fv(material.shader->getUniform("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix)));

        // Apply transform from the TransformComponent to the model
        renderable.model->setPosition(transform.position);
        renderable.model->setRotation(transform.rotation.y);
        renderable.model->setScale(transform.scale.x);

        // Draw the model
        renderable.model->draw(material.shader);

        material.shader->unbind();
        });
}

void RenderSystem::RenderModels(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    auto view = ecs.View<ModelComponent>();

    view.ForEach([&](seecs::EntityID entity, ModelComponent& modelComp) {
        if (!modelComp.model || !ecs.Has<MaterialComponent>(entity)) return;

        auto& material = ecs.Get<MaterialComponent>(entity);
        if (!material.shader) return;

        // Set shader and uniform values
        material.shader->bind();

        // Set matrices
        CHECKED_GL_CALL(glUniformMatrix4fv(material.shader->getUniform("P"), 1, GL_FALSE, glm::value_ptr(projMatrix)));
        CHECKED_GL_CALL(glUniformMatrix4fv(material.shader->getUniform("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix)));

        // Apply material properties
        if (modelComp.overrideMaterial) {
            glUniform3fv(material.shader->getUniform("MatAmb"), 1, glm::value_ptr(modelComp.ambient));
            glUniform3fv(material.shader->getUniform("MatDif"), 1, glm::value_ptr(modelComp.diffuse));
            glUniform3fv(material.shader->getUniform("MatSpec"), 1, glm::value_ptr(modelComp.specular));
            glUniform1f(material.shader->getUniform("MatShine"), modelComp.shininess);
        }
        else {
            // Use default material settings from config
            glUniform3fv(material.shader->getUniform("MatAmb"), 1, glm::value_ptr(Config::Models::Materials::DEFAULT_AMBIENT));
            glUniform3fv(material.shader->getUniform("MatDif"), 1, glm::value_ptr(Config::Models::Materials::DEFAULT_DIFFUSE));
            glUniform3fv(material.shader->getUniform("MatSpec"), 1, glm::value_ptr(Config::Models::Materials::DEFAULT_SPECULAR));
            glUniform1f(material.shader->getUniform("MatShine"), Config::Models::Materials::DEFAULT_SHININESS);
        }

        // Draw the model
        modelComp.model->draw(material.shader);

        material.shader->unbind();
        });
}

void RenderSystem::RenderParticles(seecs::ECS& ecs, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    // Find particle system component
    auto systemView = ecs.View<ParticleSystemComponent>();
    GLuint vao = 0, vbo = 0;
    std::shared_ptr<Program> particleShader;

    systemView.ForEach([&](seecs::EntityID id, ParticleSystemComponent& system) {
        vao = system.vertexArrayObj;
        vbo = system.vertexBufferObj;

        // Find any entity with a material component that has a particle shader
        auto view = ecs.View<MaterialComponent>();
        view.ForEach([&](MaterialComponent& material) {
            if (particleShader) return; // Only use the first one we find
            particleShader = material.shader;
            });
        });

    if (!particleShader || vao == 0 || vbo == 0) return;

    // Get the texture from any material component with a texture
    std::shared_ptr<Texture> texture;
    auto texView = ecs.View<MaterialComponent>();
    texView.ForEach([&](MaterialComponent& material) {
        if (texture) return; // Only use the first one we find
        texture = material.texture;
        });

    // Prepare to draw particles
    particleShader->bind();

    // Set shader uniforms
    CHECKED_GL_CALL(glUniformMatrix4fv(particleShader->getUniform("P"), 1, GL_FALSE, glm::value_ptr(projMatrix)));
    CHECKED_GL_CALL(glUniformMatrix4fv(particleShader->getUniform("V"), 1, GL_FALSE, glm::value_ptr(viewMatrix)));
    CHECKED_GL_CALL(glUniformMatrix4fv(particleShader->getUniform("M"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f))));

    // Set color from config
    CHECKED_GL_CALL(glUniform3fv(particleShader->getUniform("pColor"), 1, glm::value_ptr(Config::Particles::DEFAULT_COLOR)));

    // Bind texture if available
    if (texture) {
        texture->bind(particleShader->getUniform("alphaTexture"));
    }

    // Update particle positions in GPU buffer
    UpdateParticleBuffer(ecs, vbo);

    // Draw particles
    glBindVertexArray(vao);
    int particleCount = 0;

    // Count active particles
    auto particleView = ecs.View<ParticleComponent>();
    particleView.ForEach([&](ParticleComponent& particle) {
        if (particle.active) particleCount++;
        });

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Disable depth writing but keep depth testing
    glDepthMask(GL_FALSE);

    // Set point size from config
    glPointSize(Config::Particles::DEFAULT_SIZE);

    // Draw particles
    if (particleCount > 0) {
        glDrawArraysInstanced(GL_POINTS, 0, 1, particleCount);
    }

    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (texture) {
        texture->unbind();
    }

    particleShader->unbind();
}

void RenderSystem::UpdateParticleBuffer(seecs::ECS& ecs, GLuint vbo) {
    // Use config for max particles
    const int MAX_PARTICLES = Config::Particles::MAX_PARTICLES;
    std::vector<GLfloat> points(Config::Particles::MAX_PARTICLES * 3, 0.0f);
    int index = 0;

    auto view = ecs.View<ParticleComponent, TransformComponent>();

    view.ForEach([&](seecs::EntityID id, ParticleComponent& particle, TransformComponent& transform) {
        if (!particle.active) return;

        // Store position in buffer
        points[index++] = transform.position.x;
        points[index++] = transform.position.y;
        points[index++] = transform.position.z;
        });

    // Update buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * points.size(), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * index, points.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}