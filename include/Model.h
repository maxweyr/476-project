#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "GLSL.h"
#include "Program.h"

class Model {
public:
    Model();
    ~Model();

    bool loadModel(const std::string& path);
    void draw(std::shared_ptr<Program> shader);
    void update(float deltaTime);

    // Get bone transforms for animation
    const std::vector<glm::mat4>& getBoneTransforms() const { return boneTransforms; }
    int getBoneCount() const { return boneTransforms.size(); }

    // Set position and orientation
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setRotation(float angle) { rotationAngle = angle; }
    void setScale(float sc) { scale = sc; }

    // Config for changing animations
    void setAnimation(int animIndex);
    int getAnimationCount() const;
    std::string getAnimationName(int index) const;
    int getCurrentAnimation() const { return currentAnimation; }

    size_t getMeshCount() const {
        return meshes.size();
    }
    
    int getTexturedMeshCount() const {
        int count = 0;
        for (const auto& mesh : meshes) {
            if (mesh.hasTexture && mesh.textureID > 0) {
                count++;
            }
        }
        return count;
    }

    void setMeshMaterial(int meshIndex, const glm::vec3& ambient, const glm::vec3& diffuse,
        const glm::vec3& specular, float shininess);
    bool hasMeshMaterials() const { return !meshMaterials.empty(); }

private:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::ivec4 boneIDs;
        glm::vec4 weights;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        GLuint VAO, VBO, EBO, textureID;
        bool hasTexture;
    };

    struct Bone {
        std::string name;
        int id;
        glm::mat4 offset;
    };

    struct Animation {
        float duration;
        float ticksPerSecond;
        bool loop;
    };

    struct MeshMaterial {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
    };

    std::vector<MeshMaterial> meshMaterials;

    // Process meshes and animations from the loaded scene
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    void loadAnimations(const aiScene* scene);
    void updateBoneTransforms(float time, const aiScene* scene);

    // Member variables
    std::vector<Mesh> meshes;
    std::vector<Bone> bones;
    std::map<std::string, int> boneMap;
    std::vector<Animation> animations;
    std::vector<glm::mat4> boneTransforms;

    int currentAnimation;
    float animationTime;
    glm::vec3 position;
    float rotationAngle;
    float scale;

    // Assimp importer must be kept alive while we're using the scene
    Assimp::Importer importer;
    const aiScene* scene;

    std::string modelDirectory;
    GLuint loadTexture(const std::string& path);
    GLuint loadEmbeddedTexture(const aiTexture* embeddedTexture);
};