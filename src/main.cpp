#include <iostream>
#include <algorithm>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Model.h"
#include "Texture.h"
#include "WindowManager.h"

// ECS includes
#include "seecs.h"
#include "Components.h"
#include "Systems/Systems.h"
#include "Config.h"

using namespace std;
using namespace glm;

class Application : public EventCallbacks {
public:
    WindowManager* windowManager = nullptr;

    // ECS instance
    seecs::ECS ecs;

    // Entity IDs
    seecs::EntityID cameraEntity = seecs::NULL_ENTITY;
    seecs::EntityID particleSystemEntity = seecs::NULL_ENTITY;
    seecs::EntityID sphereEntity = seecs::NULL_ENTITY;
    seecs::EntityID emitterEntity = seecs::NULL_ENTITY;

    // Time management
    float lastTime = 0.0f;

    // Resource paths
    string resourceDir;

    // Initialize the application
    void init() {
        // Set up OpenGlstate
        initOpenGL();

		// Initialize the random number generator
        Random::Init();

        // Create entities
        createEntities();

        // Initialize subsytems
		initShaders();
		initGeometry();
		initTexture();
		initParticleSystem();
		initHighPerformanceParticles();

        // get current time for delta time computation
        lastTime = glfwGetTime();
    }

    // Main render/update loop
	void render() {
		// Compute delta time
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		if (deltaTime <= 0.0f) {
			deltaTime = 0.01f; // Avoid division by zero
		}

        // Get current frame buffer size
        int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

        // Clear both color and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update systems
        InputSystem::Update(ecs, windowManager);
		ModelSystem::Update(ecs, deltaTime);
		ParticleSystem::Update(ecs, deltaTime);
        HighPerformanceParticleSystem::Update(ecs, deltaTime);
		
		// Render the scene
		RenderSystem::Render(ecs);
	}

    // Shutdown and cleanup
    void shutdown() {
		// Any neccessary cleanup before exiting
    }

    // GLFW Callbacks
    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // Pass to input system
        InputSystem::HandleKeyEvent(ecs, key, action);
    }

    void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
        // Could implement zoom here
    }

    void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
        double posX, posY;

        if (action == GLFW_PRESS) {
            glfwGetCursorPos(window, &posX, &posY);
            cout << "Pos X " << posX << " Pos Y " << posY << endl;
        }
    }

    void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        // Could implement mouse look here
    }

    void resizeCallback(GLFWwindow* window, int width, int height) {
        CHECKED_GL_CALL(glViewport(0, 0, width, height));

        // Update camera aspect ratio
        if (cameraEntity != seecs::NULL_ENTITY) {
            auto& camera = ecs.Get<CameraComponent>(cameraEntity);
            camera.aspectRatio = width / (float)height;
        }
    }
private:
    std::shared_ptr<Program> hpParticleShaderProgram;
    std::shared_ptr<Texture> particleTexture;
    // Initialize OpenGl state
	void initOpenGL() {
        int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		GLSL::checkVersion();

        // Set background color from config
		CHECKED_GL_CALL(glClearColor(
			Config::Graphics::CLEAR_COLOR.r,
			Config::Graphics::CLEAR_COLOR.g,
			Config::Graphics::CLEAR_COLOR.b,
            1.0f
		));
		
        // Enable depth testing
        CHECKED_GL_CALL(glEnable(GL_DEPTH_TEST));
        CHECKED_GL_CALL(glEnable(GL_BLEND));
        CHECKED_GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        CHECKED_GL_CALL(glPointSize(Config::Particles::DEFAULT_SIZE));
	}

    // Create entities
    void createEntities() {
        // Create main entities
        cameraEntity = ecs.CreateEntity("MainCamera");
        particleSystemEntity = ecs.CreateEntity("ParticleSystem");
        sphereEntity = ecs.CreateEntity("Sphere");
        emitterEntity = ecs.CreateEntity("ParticleEmitter");

        // Setup camera entity
        setupCamera();

        // Setup sphere entity
        setupSphere();

        // Setup particle entities
        setupParticleEntities();
    }

    void setupCamera() {
        // Add camera component
        auto& camera = ecs.Add<CameraComponent>(cameraEntity);
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        camera.aspectRatio = width / (float)height;
        camera.fov = Config::Camera::DEFAULT_FOV;
        camera.nearPlane = Config::Camera::DEFAULT_NEAR_PLANE;
        camera.farPlane = Config::Camera::DEFAULT_FAR_PLANE;

        // Add transform to camera
        auto& cameraTransform = ecs.Add<TransformComponent>(cameraEntity);
        cameraTransform.position = Config::Camera::DEFAULT_POSITION;

        // Add input component
        ecs.Add<InputComponent>(cameraEntity);
    }

    void setupSphere() {
        // Add transform to sphere
        auto& sphereTransform = ecs.Add<TransformComponent>(sphereEntity);
        sphereTransform.position = vec3(0, 0, 0);
        sphereTransform.scale = vec3(0.5f);
    }

    void setupParticleEntities() {
        // Add particle system component
        ecs.Add<ParticleSystemComponent>(particleSystemEntity);

        // Add particle emitter component and configure it
        auto& emitter = ecs.Add<ParticleEmitterComponent>(emitterEntity);
        emitter.position = vec3(0, 0, 0);
        emitter.gravity = Config::Particles::DEFAULT_GRAVITY;
        emitter.spawnRate = Config::Particles::SPAWN_RATE;
        emitter.maxParticles = Config::Particles::MAX_PARTICLES;

        // Set default emitter values
        emitter.initialLifespanMin = Config::Particles::DEFAULT_LIFESPAN * 0.5f;
        emitter.initialLifespanMax = Config::Particles::DEFAULT_LIFESPAN;
        emitter.initialVelocityMin = vec3(-Config::Particles::Emitter::MIN_VELOCITY);
        emitter.initialVelocityMax = vec3(Config::Particles::Emitter::MAX_VELOCITY);
        emitter.initialScaleMin = Config::Particles::Emitter::MIN_SCALE;
        emitter.initialScaleMax = Config::Particles::Emitter::MAX_SCALE;
    }

    void initShaders() {
        // Particle shader
        auto particleShader = make_shared<Program>();
        particleShader->setVerbose(true);
		string shaderDir = resourceDir + "/shaders"; // Ensure this points to the shader directory
        particleShader->setShaderNames(shaderDir + "/particle_vert.glsl",
            shaderDir + "/particle_frag.glsl");
        if (!particleShader->init()) {
            cerr << "Failed to initialize particle shader program" << endl;
            exit(1);
        }
        particleShader->addUniform("P");
        particleShader->addUniform("M");
        particleShader->addUniform("V");
        particleShader->addUniform("pColor");
        particleShader->addUniform("alphaTexture");
        particleShader->addAttribute("vertPos");

        // Mesh shader
        auto meshShader = make_shared<Program>();
        meshShader->setVerbose(true);
        meshShader->setShaderNames(shaderDir + "/simple_vert.glsl",
            shaderDir + "/simple_frag.glsl");
        if (!meshShader->init()) {
            cerr << "Failed to initialize mesh shader program" << endl;
            exit(1);
        }
        // Basic matrices
        meshShader->addUniform("P");
        meshShader->addUniform("M");
        meshShader->addUniform("V");

        // Material properties
        meshShader->addUniform("MatAmb");
        meshShader->addUniform("MatDif");
        meshShader->addUniform("MatSpec");
        meshShader->addUniform("MatShine");

        // Texture support
        meshShader->addUniform("hasTexture");
        meshShader->addUniform("textureSampler");

        // Basic vertex attributes
        meshShader->addAttribute("vertPos");
        meshShader->addAttribute("vertNor");
        meshShader->addAttribute("vertTex");

        // Create material components with these shaders
        auto& particleMaterial = ecs.Add<MaterialComponent>(particleSystemEntity);
        particleMaterial.shader = particleShader;

        auto& sphereMaterial = ecs.Add<MaterialComponent>(sphereEntity);
        sphereMaterial.shader = meshShader;
    }

    void initTexture() {
		// Ensure the texture path is correct
		string textureDir = resourceDir + "/textures";
        // Create a texture
        auto texture = make_shared<Texture>();
        texture->setFilename(textureDir + "/alpha.bmp");
        texture->init();
        texture->setUnit(0);
        texture->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        // Add texture to particle material
        auto& material = ecs.Get<MaterialComponent>(particleSystemEntity);
        material.texture = texture;
    }

    void initGeometry() {
        // Load sphere mesh using the Model class directly
        auto sphere = make_shared<Model>();
		string modelDir = resourceDir + "/models";
        if (sphere->loadModel(modelDir + "/SmoothSphere.obj")) {
            // Add renderable component to sphere entity
            auto& renderable = ecs.Add<RenderableComponent>(sphereEntity);
            renderable.model = sphere;
        }
        else {
            cerr << "Failed to load sphere model" << endl;
        }
    }

    void initParticleSystem() {
        // Set up the particle system
        auto& particleSystem = ecs.Get<ParticleSystemComponent>(particleSystemEntity);
		auto& emitter = ecs.Get<ParticleEmitterComponent>(emitterEntity);

        // Generate VAO and VBO for particles
        glGenVertexArrays(1, &particleSystem.vertexArrayObj);
        glBindVertexArray(particleSystem.vertexArrayObj);

        glGenBuffers(1, &particleSystem.vertexBufferObj);
        glBindBuffer(GL_ARRAY_BUFFER, particleSystem.vertexBufferObj);

        // Buffer will be updated each frame, just initialize it here
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * emitter.maxParticles * 3, nullptr, GL_STREAM_DRAW);

        // Set up vertex attributes
        GLuint posAttrib = 0; // Using 0 by convention, also matches the shader "location = 0"
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribDivisor(posAttrib, 1); // For instanced rendering

        glBindVertexArray(0);

        // Create all particles at once
        for (int i = 0; i < emitter.maxParticles; i++) {
            seecs::EntityID particleEntity = ecs.CreateEntity("Particle_" + to_string(i));

            // Add transform component
            auto& transform = ecs.Add<TransformComponent>(particleEntity);
            transform.position = emitter.position;
            transform.scale = vec3(Random::Range(emitter.initialScaleMin, emitter.initialScaleMax));

            // Add particle component
            auto& particle = ecs.Add<ParticleComponent>(particleEntity);
            particle.velocity.x = Random::Range(emitter.initialVelocityMin.x, emitter.initialVelocityMax.x);
            particle.velocity.y = Random::Range(emitter.initialVelocityMin.y, emitter.initialVelocityMax.y);
            particle.velocity.z = Random::Range(emitter.initialVelocityMin.z, emitter.initialVelocityMax.z);
            particle.lifespan = Random::Range(emitter.initialLifespanMin, emitter.initialLifespanMax);
            particle.remainingLife = particle.lifespan;
            particle.active = true;
        }
    }

    void initHighPerformanceParticles() {
        // Initialize the high performance particle shader
        initHighPerformanceParticleShader();

        // Make sure to use the same texture as regular particles
        particleTexture = ecs.Get<MaterialComponent>(particleSystemEntity).texture;

        // If it's not set yet, create it
        if (!particleTexture) {
            particleTexture = make_shared<Texture>();
            particleTexture->setFilename(resourceDir + "/textures/alpha.bmp");
            particleTexture->init();
            particleTexture->setUnit(0);
            particleTexture->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        }

        // Create an entity for the particle system
        seecs::EntityID particleSysEntity = ecs.CreateEntity("HighPerfParticleSystem");

        // Add transform component
        auto& transform = ecs.Add<TransformComponent>(particleSysEntity);
		transform.position = glm::vec3(0.0f, 0.0f, 0.0f); // Position at the origin

        // Add material component with the shader
        auto& material = ecs.Add<MaterialComponent>(particleSysEntity);
        material.shader = hpParticleShaderProgram;
        material.texture = particleTexture; // Make sure this is initialized

        // Add high-performance particle component
        auto& system = ecs.Add<HPParticleComponent>(particleSysEntity);
        system.maxParticles = 100000;
        system.spawnRate = 2000.0f; // 2000 particles per second
        system.emitterPosition = transform.position;
        system.gravity = glm::vec3(0.0f, -0.01f, 0.0f);
        system.initialLifespanMin = 3.0f;
        system.initialLifespanMax = 6.0f;
        system.initialSizeMin = 0.05f;
        system.initialSizeMax = 0.2f;
        system.emissionRadius = 0.5f;
        system.initialVelocityMin = glm::vec3(-0.5f, 0.1f, -0.5f);
        system.initialVelocityMax = glm::vec3(0.5f, 2.0f, 0.5f);

        // Initialize the particle system
        HighPerformanceParticleSystem::Initialize(ecs, particleSysEntity);
    }

    void initHighPerformanceParticleShader() {
        // Create the shader program
        auto hpParticleShader = make_shared<Program>();
        hpParticleShader->setVerbose(true);
        hpParticleShader->setShaderNames(
            resourceDir + "/shaders/high_perf_part_vert.glsl",
            resourceDir + "/shaders/high_perf_part_frag.glsl"
        );

        // Compile and link the shader
        if (!hpParticleShader->init()) {
            cerr << "Failed to initialize high-performance particle shader program" << endl;
            exit(1);
        }

        // Add uniforms
        hpParticleShader->addUniform("P");
        hpParticleShader->addUniform("V");
        hpParticleShader->addUniform("M");
        hpParticleShader->addUniform("alphaTexture");

        // Add attributes (though these are defined by layout in the shader)
        hpParticleShader->addAttribute("particlePos");
        hpParticleShader->addAttribute("particleVel");
        hpParticleShader->addAttribute("particleCol");
        hpParticleShader->addAttribute("particleLife");

        // Store the shader for later use
        // (you could store it in a class member, global variable, or pass it to relevant systems)
        hpParticleShaderProgram = hpParticleShader;
    }
};

int main(int argc, char** argv) {
    // Where the resources are loaded from
    std::string resourceDir = "../resources";

    if (argc >= 2) {
        resourceDir = argv[1];
    }

    Application* application = new Application();
    application->resourceDir = resourceDir;

    // Initialize window management
    WindowManager* windowManager = new WindowManager();
    windowManager->init(Config::Graphics::WINDOW_WIDTH, Config::Graphics::WINDOW_HEIGHT);
    windowManager->setEventCallbacks(application);
    application->windowManager = windowManager;

    // Initialize the application
    application->init();

    // Main loop
    while (!glfwWindowShouldClose(windowManager->getHandle())) {
        // Render scene
        application->render();

        // Swap buffers
        glfwSwapBuffers(windowManager->getHandle());

        // Poll for events
        glfwPollEvents();
    }

    // Clean up
	application->shutdown();
    windowManager->shutdown();
    delete application;

    return 0;
}