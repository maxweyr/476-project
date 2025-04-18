#include "ModelSystem.h"

void ModelSystem::LoadModel(seecs::ECS& ecs, seecs::EntityID entity, const std::string& modelPath) {
    auto& modelComp = ecs.Get<ModelComponent>(entity);
    modelComp.modelPath = modelPath;
    modelComp.model = std::make_shared<Model>();

    // Extract file extension
    std::string extension = modelPath.substr(modelPath.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Store file type information
    modelComp.fileType = extension;

    if (modelComp.model->loadModel(modelPath)) {
        std::cout << "Successfully loaded " << extension << " model: " << modelPath << std::endl;

        // If the model has animations, set the first one as default
        if (modelComp.model->getAnimationCount() > 0) {
            modelComp.model->setAnimation(0);
            modelComp.currentAnimation = 0;
            modelComp.animationPlaying = true;
            modelComp.animationSpeed = Config::Models::Animation::DEFAULT_ANIMATION_SPEED;
        }

        // Set default scale from config
        if (ecs.Has<TransformComponent>(entity)) {
            auto& transform = ecs.Get<TransformComponent>(entity);
            transform.scale = glm::vec3(Config::Models::DEFAULT_MODEL_SCALE);
        }

        // Set default materials
        modelComp.ambient = Config::Models::Materials::DEFAULT_AMBIENT;
        modelComp.diffuse = Config::Models::Materials::DEFAULT_DIFFUSE;
        modelComp.specular = Config::Models::Materials::DEFAULT_SPECULAR;
        modelComp.shininess = Config::Models::Materials::DEFAULT_SHININESS;
    }
    else {
        std::cerr << "Failed to load model: " << modelPath << std::endl;
    }
}

void ModelSystem::Update(seecs::ECS& ecs, float deltaTime) {
    auto view = ecs.View<ModelComponent, TransformComponent>();

    view.ForEach([&](seecs::EntityID entity, ModelComponent& modelComp, TransformComponent& transform) {
        if (modelComp.model) {
            // Update model transform from entity transform
            modelComp.model->setPosition(transform.position);
            modelComp.model->setRotation(transform.rotation.y); // Assuming Y-up rotation
            modelComp.model->setScale(transform.scale.x);// Assuming uniform scale

            // Update animation if playing
            if (modelComp.animationPlaying) {
                // Use game speed to affect all animations
                float timeScale = Config::App::GAME_SPEED;
                modelComp.model->update(deltaTime * modelComp.animationSpeed * timeScale);
            }
        }
    });
}

void ModelSystem::SetAnimation(seecs::ECS& ecs, seecs::EntityID entity, int animIndex) {
    if (ecs.Has<ModelComponent>(entity)) {
        auto& modelComp = ecs.Get<ModelComponent>(entity);

        if (modelComp.model && animIndex >= 0 && animIndex < modelComp.model->getAnimationCount()) {
            modelComp.model->setAnimation(animIndex);
            modelComp.currentAnimation = animIndex;
            modelComp.animationPlaying = true;
        }
    }
}

void ModelSystem::SetAnimationPlaying(seecs::ECS& ecs, seecs::EntityID entity, bool playing) {
    if (ecs.Has<ModelComponent>(entity)) {
        auto& modelComp = ecs.Get<ModelComponent>(entity);
        modelComp.animationPlaying = playing;
    }
}

void ModelSystem::SetAnimationSpeed(seecs::ECS& ecs, seecs::EntityID entity, float speed) {
    if (ecs.Has<ModelComponent>(entity)) {
        auto& modelComp = ecs.Get<ModelComponent>(entity);
        modelComp.animationSpeed = speed;
    }
}