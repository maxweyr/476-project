#include "Model.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"

Model::Model() :
    currentAnimation(0),
    animationTime(0.0f),
    position(0.0f),
    rotationAngle(0.0f),
    scale(1.0f),
    scene(nullptr) {
}

Model::~Model() {
    // Clean up OpenGL resources
    for (auto& mesh : meshes) {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }
}

bool Model::loadModel(const std::string& path) {
    modelDirectory = path.substr(0, path.find_last_of("/\\"));
    // Import the FBX file
    scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }

    // Process all meshes
    processNode(scene->mRootNode, scene);

    // Load animations if available
    loadAnimations(scene);

    // Pre-allocate bone transforms array
    boneTransforms.resize(bones.size(), glm::mat4(1.0f));

    std::cout << "Loaded model: " << path << std::endl;
    std::cout << "  Meshes: " << meshes.size() << std::endl;
    std::cout << "  Bones: " << bones.size() << std::endl;
    std::cout << "  Animations: " << animations.size() << std::endl;

    return true;
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // Process all meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    // Process all child nodes recursively
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Model::Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    Mesh result;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // Position
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        // Normal
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        // Default bone indices and weights
        vertex.boneIDs = glm::ivec4(-1, -1, -1, -1);
        vertex.weights = glm::vec4(0.0f);

        result.vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            result.indices.push_back(face.mIndices[j]);
        }
    }

    // Process bones
    if (mesh->HasBones()) {
        for (unsigned int i = 0; i < mesh->mNumBones; i++) {
            aiBone* bone = mesh->mBones[i];
            std::string boneName = bone->mName.C_Str();
            int boneID = 0;

            // Check if bone exists
            if (boneMap.find(boneName) == boneMap.end()) {
                // New bone
                boneID = bones.size();
                Bone newBone;
                newBone.name = boneName;
                newBone.id = boneID;

                // Convert from assimp matrix to glm
                aiMatrix4x4 offset = bone->mOffsetMatrix;
                newBone.offset = glm::mat4(
                    offset.a1, offset.b1, offset.c1, offset.d1,
                    offset.a2, offset.b2, offset.c2, offset.d2,
                    offset.a3, offset.b3, offset.c3, offset.d3,
                    offset.a4, offset.b4, offset.c4, offset.d4
                );

                bones.push_back(newBone);
                boneMap[boneName] = boneID;
            }
            else {
                boneID = boneMap[boneName];
            }

            // Add weights to vertices
            for (unsigned int j = 0; j < bone->mNumWeights; j++) {
                aiVertexWeight weight = bone->mWeights[j];
                unsigned int vertexID = weight.mVertexId;
                float weightValue = weight.mWeight;

                // Add to first available slot in vertex
                for (int k = 0; k < 4; k++) {
                    if (result.vertices[vertexID].weights[k] == 0.0f) {
                        result.vertices[vertexID].boneIDs[k] = boneID;
                        result.vertices[vertexID].weights[k] = weightValue;
                        break;
                    }
                }
            }
        }

        // Normalize weights
        for (auto& vertex : result.vertices) {
            float sum = vertex.weights.x + vertex.weights.y + vertex.weights.z + vertex.weights.w;
            if (sum > 0.0f) {
                vertex.weights /= sum;
            }
        }
    }

    // Process materials and textures
    result.hasTexture = false;
    result.textureID = 0;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::cout << "Material " << mesh->mMaterialIndex << " has "
            << material->GetTextureCount(aiTextureType_DIFFUSE)
            << " diffuse textures" << std::endl;

        // If any diffuse textures exist
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                std::cout << "Looking for texture: " << texturePath.C_Str() << std::endl;

                // Try to see if there's an embedded texture
                const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(texturePath.C_Str());
                if (embeddedTexture) {
                    std::cout << "Found embedded texture!" << std::endl;
                }
                else {
                    std::cout << "No embedded texture!" << std::endl;
                }
            }
        }
    }

    // Create OpenGL buffers
    glGenVertexArrays(1, &result.VAO);
    glGenBuffers(1, &result.VBO);
    glGenBuffers(1, &result.EBO);

    glBindVertexArray(result.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, result.VBO);
    glBufferData(GL_ARRAY_BUFFER, result.vertices.size() * sizeof(Vertex), &result.vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, result.indices.size() * sizeof(unsigned int), &result.indices[0], GL_STATIC_DRAW);

    // Vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    // Vertex bone IDs
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));

    // Vertex weights
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

    glBindVertexArray(0);

    return result;
}

void Model::loadAnimations(const aiScene* scene) {
    if (!scene->HasAnimations()) {
        return;
    }

    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];
        Animation animation;

        animation.duration = anim->mDuration;
        animation.ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0f;
        animation.loop = true;

        // Print animation details for debugging
        std::cout << "Found animation " << i << ": "
            << anim->mName.C_Str()
            << " (Duration: " << animation.duration
            << ", TPS: " << animation.ticksPerSecond
            << ", Channels: " << anim->mNumChannels << ")" << std::endl;

        animations.push_back(animation);
    }
}

void Model::update(float deltaTime) {
    if (animations.empty() || !scene || !scene->HasAnimations()) {
        return;
    }

    Animation& animation = animations[currentAnimation];

    // Update animation time
    animationTime += deltaTime * animation.ticksPerSecond;

    // Handle looping
    if (animation.loop) {
        while (animationTime >= animation.duration) {
            animationTime -= animation.duration;
        }
    }
    else if (animationTime > animation.duration) {
        animationTime = animation.duration;
    }

    // Update bone transforms
    updateBoneTransforms(animationTime, scene);
}

void Model::updateBoneTransforms(float time, const aiScene* scene) {
    if (!scene->HasAnimations() || currentAnimation >= scene->mNumAnimations) {
        return;
    }

    aiAnimation* animation = scene->mAnimations[currentAnimation];

    // Create a map of node transformations
    std::map<std::string, aiMatrix4x4> nodeTransforms;

    // Helper function to find animation for a node
    auto findNodeAnim = [animation](const std::string& nodeName) -> const aiNodeAnim* {
        for (unsigned int i = 0; i < animation->mNumChannels; i++) {
            if (animation->mChannels[i]->mNodeName.C_Str() == nodeName) {
                return animation->mChannels[i];
            }
        }
        return nullptr;
        };

    // Helper function to interpolate position
    auto interpolatePosition = [](float time, const aiNodeAnim* nodeAnim) -> aiVector3D {
        if (nodeAnim->mNumPositionKeys == 1) {
            return nodeAnim->mPositionKeys[0].mValue;
        }

        unsigned int index = 0;
        for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys - 1; i++) {
            if (time < nodeAnim->mPositionKeys[i + 1].mTime) {
                index = i;
                break;
            }
        }

        unsigned int nextIndex = index + 1;
        float t = (time - nodeAnim->mPositionKeys[index].mTime) /
            (nodeAnim->mPositionKeys[nextIndex].mTime - nodeAnim->mPositionKeys[index].mTime);

        const aiVector3D& start = nodeAnim->mPositionKeys[index].mValue;
        const aiVector3D& end = nodeAnim->mPositionKeys[nextIndex].mValue;

        return start + (end - start) * t;
        };

    // Helper function to interpolate rotation
    auto interpolateRotation = [](float time, const aiNodeAnim* nodeAnim) -> aiQuaternion {
        if (nodeAnim->mNumRotationKeys == 1) {
            return nodeAnim->mRotationKeys[0].mValue;
        }

        unsigned int index = 0;
        for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys - 1; i++) {
            if (time < nodeAnim->mRotationKeys[i + 1].mTime) {
                index = i;
                break;
            }
        }

        unsigned int nextIndex = index + 1;
        float t = (time - nodeAnim->mRotationKeys[index].mTime) /
            (nodeAnim->mRotationKeys[nextIndex].mTime - nodeAnim->mRotationKeys[index].mTime);

        const aiQuaternion& start = nodeAnim->mRotationKeys[index].mValue;
        const aiQuaternion& end = nodeAnim->mRotationKeys[nextIndex].mValue;

        aiQuaternion result;
        aiQuaternion::Interpolate(result, start, end, t);
        return result.Normalize();
        };

    // Helper function to interpolate scaling
    auto interpolateScaling = [](float time, const aiNodeAnim* nodeAnim) -> aiVector3D {
        if (nodeAnim->mNumScalingKeys == 1) {
            return nodeAnim->mScalingKeys[0].mValue;
        }

        unsigned int index = 0;
        for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys - 1; i++) {
            if (time < nodeAnim->mScalingKeys[i + 1].mTime) {
                index = i;
                break;
            }
        }

        unsigned int nextIndex = index + 1;
        float t = (time - nodeAnim->mScalingKeys[index].mTime) /
            (nodeAnim->mScalingKeys[nextIndex].mTime - nodeAnim->mScalingKeys[index].mTime);

        const aiVector3D& start = nodeAnim->mScalingKeys[index].mValue;
        const aiVector3D& end = nodeAnim->mScalingKeys[nextIndex].mValue;

        return start + (end - start) * t;
        };

    // Helper function to calculate node transformation
    std::function<void(aiNode*, const aiMatrix4x4&)> calculateNodeTransform;
    calculateNodeTransform = [&](aiNode* node, const aiMatrix4x4& parentTransform) {
        std::string nodeName = node->mName.C_Str();
        aiMatrix4x4 nodeTransformation = node->mTransformation;

        // Find animation channel for this node
        const aiNodeAnim* nodeAnim = findNodeAnim(nodeName);
        if (nodeAnim) {
            // Interpolate transformation
            aiVector3D position = interpolatePosition(time, nodeAnim);
            aiQuaternion rotation = interpolateRotation(time, nodeAnim);
            aiVector3D scaling = interpolateScaling(time, nodeAnim);

            // Create transformation matrix
            aiMatrix4x4 positionMat;
            aiMatrix4x4::Translation(position, positionMat);

            aiMatrix4x4 rotationMat = aiMatrix4x4(rotation.GetMatrix());

            aiMatrix4x4 scalingMat;
            aiMatrix4x4::Scaling(scaling, scalingMat);

            // Combine transformations
            nodeTransformation = positionMat * rotationMat * scalingMat;
        }

        // Calculate global transformation
        aiMatrix4x4 globalTransform = parentTransform * nodeTransformation;
        nodeTransforms[nodeName] = globalTransform;

        // Process children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            calculateNodeTransform(node->mChildren[i], globalTransform);
        }
        };

    // Start from the root node
    aiMatrix4x4 identity;
    calculateNodeTransform(scene->mRootNode, identity);

    // Update bone transforms
    for (const auto& bone : bones) {
        if (nodeTransforms.find(bone.name) != nodeTransforms.end()) {
            aiMatrix4x4 globalTransform = nodeTransforms[bone.name];

            // Convert from Assimp matrix to GLM
            glm::mat4 offset = bone.offset;
            glm::mat4 globalTransformGLM = glm::mat4(
                globalTransform.a1, globalTransform.b1, globalTransform.c1, globalTransform.d1,
                globalTransform.a2, globalTransform.b2, globalTransform.c2, globalTransform.d2,
                globalTransform.a3, globalTransform.b3, globalTransform.c3, globalTransform.d3,
                globalTransform.a4, globalTransform.b4, globalTransform.c4, globalTransform.d4
            );

            // Final bone transform
            boneTransforms[bone.id] = globalTransformGLM * offset;
        }
    }
}

void Model::setAnimation(int animIndex) {
    if (animIndex >= 0 && animIndex < animations.size()) {
        currentAnimation = animIndex;
        animationTime = 0.0f; // Reset animation time when switching
        std::cout << "Switched to animation " << animIndex;
        if (scene && scene->HasAnimations() && animIndex < scene->mNumAnimations) {
            std::cout << " (" << scene->mAnimations[animIndex]->mName.C_Str() << ")";
        }
        std::cout << std::endl;
    }
}

int Model::getAnimationCount() const {
    return animations.size();
}

std::string Model::getAnimationName(int index) const {
    if (scene && scene->HasAnimations() && index >= 0 && index < scene->mNumAnimations) {
        return scene->mAnimations[index]->mName.C_Str();
    }
    return "Unknown";
}

GLuint Model::loadTexture(const std::string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            format = GL_RGB;
            std::cout << "Unknown image format with " << nrComponents << " components" << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

GLuint Model::loadEmbeddedTexture(const aiTexture* embeddedTexture) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data;

    // Check if texture is compressed
    if (embeddedTexture->mHeight == 0) {
        // Compressed texture
        data = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(embeddedTexture->pcData),
            embeddedTexture->mWidth,
            &width, &height, &nrComponents, 0);
    }
    else {
        // Uncompressed texture
        width = embeddedTexture->mWidth;
        height = embeddedTexture->mHeight;
        nrComponents = 4; // Assuming RGBA
        data = reinterpret_cast<unsigned char*>(embeddedTexture->pcData);
    }

    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            format = GL_RGB;
            std::cout << "Unknown embedded texture format with " << nrComponents << " components" << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Only free if it was a compressed texture loaded by stbi
        if (embeddedTexture->mHeight == 0) {
            stbi_image_free(data);
        }
    }
    else {
        std::cout << "Embedded texture failed to load" << std::endl;
        return 0;
    }

    return textureID;
}

void Model::setMeshMaterial(int meshIndex, const glm::vec3& ambient, const glm::vec3& diffuse,
    const glm::vec3& specular, float shininess) {
    // Resize the materials vector if needed
    if (meshMaterials.size() <= meshIndex) {
        meshMaterials.resize(meshIndex + 1);
    }

    // Set the material properties
    meshMaterials[meshIndex].ambient = ambient;
    meshMaterials[meshIndex].diffuse = diffuse;
    meshMaterials[meshIndex].specular = specular;
    meshMaterials[meshIndex].shininess = shininess;
}

void Model::draw(std::shared_ptr<Program> shader) {
    // Set model matrix in shader
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(scale));

    // Check if M uniform exists
    int mLoc = shader->getUniform("M");
    if (mLoc != -1) {
        glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(model));
    }

    // Set bone transforms only if the shader supports them
    for (unsigned int i = 0; i < boneTransforms.size(); i++) {
        std::string uniformName = "boneTransforms[" + std::to_string(i) + "]";
        int boneLoc = shader->getUniform(uniformName);
        if (boneLoc != -1) {
            glUniformMatrix4fv(boneLoc, 1, GL_FALSE, glm::value_ptr(boneTransforms[i]));
        }
    }

    // Check if hasTexture uniform exists before setting it
    int hasTextureLoc = shader->getUniform("hasTexture");
    if (hasTextureLoc != -1) {
        glUniform1i(hasTextureLoc, 0);
    }

    // Draw meshes
    for (size_t i = 0; i < meshes.size(); i++) {
        const auto& mesh = meshes[i];

        // Apply per-mesh material if available
        if (i < meshMaterials.size()) {
            int ambLoc = shader->getUniform("MatAmb");
            int difLoc = shader->getUniform("MatDif");
            int specLoc = shader->getUniform("MatSpec");
            int shineLoc = shader->getUniform("MatShine");

            if (ambLoc != -1) {
                glUniform3fv(ambLoc, 1, glm::value_ptr(meshMaterials[i].ambient));
            }
            if (difLoc != -1) {
                glUniform3fv(difLoc, 1, glm::value_ptr(meshMaterials[i].diffuse));
            }
            if (specLoc != -1) {
                glUniform3fv(specLoc, 1, glm::value_ptr(meshMaterials[i].specular));
            }
            if (shineLoc != -1) {
                glUniform1f(shineLoc, meshMaterials[i].shininess);
            }
        }

        // Check if mesh has texture
        if (mesh.hasTexture && mesh.textureID > 0 && hasTextureLoc != -1) {
            glUniform1i(hasTextureLoc, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);

            int samplerLoc = shader->getUniform("textureSampler");
            if (samplerLoc != -1) {
                glUniform1i(samplerLoc, 0);
            }
        }
        else if (hasTextureLoc != -1) {
            glUniform1i(hasTextureLoc, 0);
        }

        // Draw the mesh
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}