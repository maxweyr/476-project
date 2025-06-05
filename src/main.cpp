#define MINIAUDIO_IMPLEMENTATION
#include "../ext/miniaudio.h"
//========================================
// Main (GOD FILE) for the Wizard Library
//========================================

#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <glad/glad.h>
#include <chrono>
#include <thread>
#include <set>
#include <algorithm>
#include <limits>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "Spline.h"
#include "stb_image.h"
#include "AssimpModel.h"
#include "Animator.h"
#include "LightTrail.h"
#include "LibraryGen.h"
// #include "Grid.h"
#include "Enemy.h"
#include "IceElemental.h"
#include "FireElemental.h"
#include "LightningElemental.h"
#include "Player.h"
#include "BossRoomGen.h"
#include "FrustumCulling.h"
#include "BossEnemy.h"
#include "Config.h"
#include "GameObjectTypes.h"
#include "../particles/particleGen.h"
#include "TextureManager.h"
#include "Quadtree.h"
#include "Freetype.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#define USE_INSTANCING 0

using namespace std;
using namespace glm;

ma_engine engine;
ma_sound sound;

class Application : public EventCallbacks {
public:
	std::shared_ptr<Player> player;
	WindowManager * windowManager = nullptr;

	bool windowMaximized = false;
	int window_width = Config::DEFAULT_WINDOW_WIDTH;
	int window_height = Config::DEFAULT_WINDOW_HEIGHT;

	// Our shader programs
	shared_ptr<Program> particleProg;
	shared_ptr<Program> DepthProg;
	shared_ptr<Program> DepthProgDebug;
	shared_ptr<Program> ShadowProg;
	shared_ptr<Program> DebugProg;
	shared_ptr<Program> hudProg;
	shared_ptr<Program> redFlashProg;
	shared_ptr<Program> SkyboxProg;
	shared_ptr<Program> debugLineProg;
	shared_ptr<Program> textProg;

	// ground data - Reused for all flat ground planes
	GLuint GrndBuffObj = 0, GrndNorBuffObj = 0, GIndxBuffObj = 0; // Initialize to 0
	int g_GiboLen = 0;
	GLuint GroundVertexArrayID = 0; // Initialize to 0
	float groundSize = 20.0f; // Half-size of the main library ground square
	float groundY = Config::GROUND_HEIGHT;     // Y level for all ground planes

	float exposure = 1.0f;
	float saturation = 1.0f;

	//Timeout for F Key
	float fTimeout;

	// Textures
	shared_ptr<Texture> borderWallTex;
	shared_ptr<Texture> libraryGroundTex;
	shared_ptr<Texture> carpetTex;
	shared_ptr<Texture> particleAlphaTex;
	shared_ptr<Texture> pawTex;

	// Skybox texture and VAO/VBO
	GLuint skyboxCubemap;
	GLuint skyboxCubeVAO = 0;
	GLuint skyboxCubeVBO = 0;

	// AABB VAO/VBO/EBO
	GLuint aabbVAO = 0, aabbVBO = 0, aabbEBO = 0;

	vector<WallObject> borderWalls;
	std::set<WallObjKey> borderWallKeys; // Set to track unique keys
	vector<LibGrndObject> libraryGrounds;
	std::set<LibGrndObjKey> libraryGroundKeys; // Set to track unique keys

	// Scene layout parameters
	vec3 libraryCenter = vec3(0.0f, groundY, 0.0f);
	vec3 bossAreaCenter = vec3(0.0f, groundY, 60.0f); // Further away
	vec3 doorPosition = vec3(0.0f, 1.5f, groundSize); // Center of door at library edge
	vec3 doorScale = vec3(1.5f, 3.0f, 0.2f); // Width, Height, Thickness
	float pathWidth = 4.0f; // Width of the path connecting areas

	// setup collectibles vector
	std::vector<Collectible> orbCollectibles;
	std::unordered_map<SpellType, int> spellCounts;
	int orbsCollectedCount = 0;
	std::vector<Enemy*> enemies;

	// --- Spell Projectiles ---
	std::vector<SpellProjectile> activeSpells;
	std::shared_ptr<particleGen> particleSystem;
	shared_ptr<AABB> sphereBB;

	// -- Boss Enemy Spell Projectiles --
	std::vector<SpellProjectile> bossActiveSpells;

	// character bounding box
	shared_ptr<AABB> playerBB;

	AssimpModel *book_shelf1, *book_shelf2;
	AssimpModel *candelabra, *chest, *library_bench, *low_poly_bookshelf, *table_chairs1, *table_chairs2, *grandfather_clock, *bookstand, *door;
	AssimpModel *healthBar;
	AssimpModel *cube, *sphere;
	AssimpModel *border, *lock, *lockHandle, *key;
	AssimpModel *bookCover, *bookPaper;
	AssimpModel *stoneGolem;

	//key collectibles
	std::vector<Collectible> keyCollectibles;
	int keysCollectedCount = 0;
	 //bool keyAlreadyExists = false;
	 //bool enemyLastPos = false;

	vector<Book> books; // vector of books to be drawn

	AssimpModel* player_rig;
	Animation *player_walk, *player_idle;
	Animator *catwizard_animator;

	AssimpModel *CatWizard;

	AssimpModel *iceElemental;
	AssimpModel *fireElemental;
	AssimpModel *lightningElemental;

	BossEnemy *bossEnemy;

	float AnimDeltaTime = 0.0f;
	float AnimLastFrame = 0.0f;

	int change_mat = 0;

	// vec3 characterMovement = vec3(0, 0, 0);
	glm::vec3 manScale = glm::vec3(0.01f, 0.01f, 0.01f);
	glm::vec3 manMoveDir = glm::vec3(sin(radians(0.0f)), 0, cos(radians(0.0f)));

	// initial position of light cycles
	glm::vec3 start_lightcycle1_pos = glm::vec3(-384, -11, 31);
	glm::vec3 start_lightcycle2_pos = glm::vec3(-365, -11, 9.1);

	float theta = glm::radians(Config::CAMERA_DEFAULT_THETA_DEGREES); // controls yaw
	float phi = glm::radians(Config::CAMERA_DEFAULT_PHI_DEGREES); // controls pitch
	float radius = Config::CAMERA_DEFAULT_RADIUS;

	float wasd_sens = 0.5f;

	glm::vec3 eye = glm::vec3(-6, 1.03, 0); /*MINI MAP*/
	glm::vec3 lookAt = glm::vec3(0, 0, 0); /*MINI MAP*/
	glm::vec3 up = glm::vec3(0, 1, 0);
	bool CULL = false;

	vec3 right = normalize(cross(manMoveDir, up));

	bool mouseIntialized = false;
	double lastX, lastY;

	int debug = 0;
	int debug_pos = 0;

	bool debug_shelf = false;

	bool cursor_visable = true;

	//Movement Variables (Maybe move?)
	bool movingForward = false;
	bool movingBackward = false;
	bool movingLeft = false;
	bool movingRight = false;

	//unlock bool
	bool unlock = false;
	float lTheta = 0;

	float characterRotation = 0.0f;

	//Debug Camera
	bool debugCamera = false;
	vec3 debugEye = vec3(0.0f, 0.0f, 0.0f);
	float debugMovementSpeed = 0.2f;

	bool enemyActive = true;
	bool playerActive = true;

	Man_State manState = Man_State::IDLE;

	LibraryGen *library = new LibraryGen();
	Grid<LibraryGen::Cell> grid;
	ivec2 gridSize = glm::ivec2(30, 30); // Size of the grid (number of cells in each dimension)

	BossRoomGen *bossRoom = new BossRoomGen();
	Grid<BossRoomGen::Cell> bossGrid;
	ivec2 bossGridSize = glm::ivec2(40, 40); // Size of the grid (number of cells in each dimension)

	ivec2 bossEntranceDir = glm::ivec2(0, 1); // Direction of the boss entrance (relative to the library grid)

	glm::vec4 planes[6]; // Frustum planes

	// Flags for game state
	bool canFightboss = false; // Flag to check if the player can fight the boss
	bool allEnemiesDead = false; // Flag to check if all enemies are dead
	bool restartGen = false;
	bool bossfightstarted = false;
	bool bossfightended = false;

	// -- Camera Occlusion Query --
	GLuint occlusionQueryID = 0; // Occlusion query ID
	GLuint visible = 0;
	GLuint occlusionBoxVAO = 0;
	GLuint occlusionBoxVBO = 0;
	const int VISIBILITY_HISTORY_LENGTH = 5;
	std::deque<bool> visibilityHistory; // History of visibility statese

	float cameraVisibleCooldown = 0.0f; // Cooldown for camera visibility check
	bool wasVisibleLastFrame = true;

	int nextSpellTypeIndex = 1; // Used to cycle spell types for new orbs: 1=FIRE, 2=ICE, 3=LIGHTNING

	// Shadows
	GLuint depthMapFBO;
	const GLuint S_WIDTH = 2048, S_HEIGHT = 2048;
	GLuint depthMap;

	// Geometry for texture render
	GLuint quad_VertexArrayID;
	GLuint quad_vertexbuffer;

	Quadtree *libraryQuadTree;
	Quadtree *bossRoomQuadTree;

	std::vector<glm::mat4> book_shelf1Matrices;
	std::vector<glm::mat4> book_shelf2Matrices;
	std::vector<glm::mat4> bookstandMatrices;
	std::vector<glm::mat4> table_chairs1Matrices;
	std::vector<glm::mat4> table_chairs2Matrices;
	std::vector<glm::mat4> chestMatrices;
	std::vector<glm::mat4> candelabraMatrices;
	std::vector<glm::mat4> clockMatrices;
	std::vector<glm::mat4> doorMatrices;

	std::vector<glm::mat4> vbook_shelf1Matrices;
	std::vector<glm::mat4> vbook_shelf2Matrices;
	std::vector<glm::mat4> vbookstandMatrices;
	std::vector<glm::mat4> vtable_chairs1Matrices;
	std::vector<glm::mat4> vtable_chairs2Matrices;
	std::vector<glm::mat4> vchestMatrices;
	std::vector<glm::mat4> vcandelabraMatrices;
	std::vector<glm::mat4> vclockMatrices;

	std::vector<glm::mat4> circularBookShelfMatrices;
	glm::vec3 bossEntrancePos;
	float bossEntranceRot;

	std::vector<glm::mat4> vCircularBookShelfMatrices;
	int activeEnemiesCount = 0; // Count of active enemies in the scene
	int keysneededToCollect = 0; // Total number of keys to collect in the scene

	SpellType spellSlots[4] = {
		SpellType::LIGHTNING,
		SpellType::FIRE,
		SpellType::ICE,
		SpellType::HEAL
	};

	int currentSpellSlotIndex = 1; // Current spell type in use
	SpellType currentPlayerSpellType = spellSlots[currentSpellSlotIndex]; // Player starts with Fire spell by default

	// --- Paw Prints ---
	// CPU: record and upload a list of paw prints
	// maintain up to a maximum and replace the oldest when adding a new print
	// GPU: handles everything else
	deque<PawPrint> prints;
	void onStep(vec3 worldPos, float facingAngle) {
		vec2 cur{ worldPos.x, worldPos.z };

		vec2 moveDelta = cur - Config::LAST_PAW_POS;
		if (glm::length(moveDelta) < Config::MIN_PAW_DIST)
			return;  // not far enough to step

		glm::vec2 forward2D = glm::normalize(moveDelta);

		glm::vec2 lateral = glm::vec2(-forward2D.y, forward2D.x);

		float side = Config::LEFT_PAW ? +1.0f : -1.0f;
		Config::LEFT_PAW = !Config::LEFT_PAW; // alternate left/right

		glm::vec2 footPos = cur + lateral * (side * Config::PAW_SPACING);

		prints.push_back({ footPos, facingAngle, float(glfwGetTime()) }); // record it
		if (prints.size() > Config::PRINTS_MAX) prints.pop_front(); // remove oldest print
		Config::LAST_PAW_POS = cur;
	}

	void initAABBWireframe() {
		// corner positions
		static const glm::vec3 corners[8] = {
		  {-1,-1,-1},{+1,-1,-1},{+1,+1,-1},{-1,+1,-1},
		  {-1,-1,+1},{+1,-1,+1},{+1,+1,+1},{-1,+1,+1}
		};
		// edges: 12 segments -> 24 indices
		static const GLuint edges[24] = {
		  0,1, 1,2, 2,3, 3,0,
		  4,5, 5,6, 6,7, 7,4,
		  0,4, 1,5, 2,6, 3,7
		};

		glGenVertexArrays(1, &aabbVAO);
		glGenBuffers(1, &aabbVBO);
		glGenBuffers(1, &aabbEBO);

		glBindVertexArray(aabbVAO);
		// positions
		glBindBuffer(GL_ARRAY_BUFFER, aabbVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
		// edges
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aabbEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(edges), edges, GL_STATIC_DRAW);
		// attrib 0 = position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glBindVertexArray(0);
	}

	// Set up the FBO for storing the light's depth map
	void initShadow() {
		glGenFramebuffers(1, &depthMapFBO); // Generate FBO for shadow depth
		glGenTextures(1, &depthMap); // Generate texture for shadow depth
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, S_WIDTH, S_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO); // bind with framebuffer's depth buffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0); // attach the texture to the framebuffer
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind the framebuffer
	}

	void initSkyboxCube() {
		static const float skyboxVertices[] = {
		// back face
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		// front face
		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
		// left face
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		// right face
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		// bottom face
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		// top face
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f
		};

		glGenVertexArrays(1, &skyboxCubeVAO);
		glGenBuffers(1, &skyboxCubeVBO);

		glBindVertexArray(skyboxCubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, skyboxCubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glBindVertexArray(0);
	}

	void initSkyboxTex(string resourceDirectory) {
		glGenTextures(1, &skyboxCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);

		vector<string> faces = {
			resourceDirectory + "/skyboxTex/px.png",
			resourceDirectory + "/skyboxTex/nx.png",
			resourceDirectory + "/skyboxTex/py.png",
			resourceDirectory + "/skyboxTex/ny.png",
			resourceDirectory + "/skyboxTex/pz.png",
			resourceDirectory + "/skyboxTex/nz.png"
		};

		for (GLuint i = 0; i < faces.size(); i++) {
			int w, h, n;
			unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &n, 0);
			if (data) {
				GLenum format = (n == 4 ? GL_RGBA : GL_RGB);
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, format, w, h, 0,
					format, GL_UNSIGNED_BYTE, data
				);
				stbi_image_free(data);
			}
			else {
				std::cerr << "Failed to load skybox face " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}

		// Set filtering and wraping
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void updateCameraVectors() {
		if (!debugCamera) {
			//Activate Player Camera
			// vec3 front;
			// front.x = radius * cos(phi) * cos(theta);
			// front.y = radius * sin(phi);
			// front.z = radius * cos(phi) * cos((pi<float>()/2) - theta);

			// eye = player->getPosition() - front;
			// lookAt = player->getPosition();

			// // manRot.y = theta + radians(-90.0f);
			// // manRot.y = - manRot.y;
			// // manRot.x = phi;

			// player->setRotY(-(theta + radians(-90.0f)));
			// player->setRotX(phi);

			// // cout << "Theta: " << theta << " Phi: " << phi << endl;
			// manMoveDir = vec3(sin(player->getRotY()), 0, cos(player->getRotY()));
			// right = normalize(cross(manMoveDir, up));
			// 1. Compute the desired front vector
			glm::vec3 front;
			front.x = cos(phi) * cos(theta);
			front.y = sin(phi);
			front.z = cos(phi) * cos((pi<float>() / 2) - theta);

			// 2. Store current desired eye (before occlusion check)
			glm::vec3 playerPos = glm::vec3(player->getPosition().x, player->getPosition().y + 1.0f, player->getPosition().z);
			glm::vec3 desiredEye = playerPos - front * radius;


			// 3. Adjust radius if needed
			float desiredRadius = Config::CAMERA_DEFAULT_RADIUS;
			float minRadius = 0.5f;
			float step = 0.45f;
			float testRadius = desiredRadius;
			float finalRadius = radius;
			const float cooldownTime = 1.0f; // Cooldown time in seconds
			cameraVisibleCooldown -= AnimDeltaTime;

			// if (visible == 0) {
			// 	// std::cout << "Camera Occluded" << std::endl;
			// 	radius = glm::max(minRadius, radius - step);
			// } else {
			// 	radius = glm::min(desiredRadius, radius + step);
			// }

			checkCameraCollision();

			if (visible == 0) {
				radius = glm::max(minRadius, radius - step);
				cameraVisibleCooldown = cooldownTime; // Reset cooldown
				wasVisibleLastFrame = false; // Mark as not visible
			}
			else if (cameraVisibleCooldown <= 0.0f) {
				// Only expand if cooldown is over
				radius = glm::min(desiredRadius, radius + step);
				wasVisibleLastFrame = true; // Mark as visible
			}
			// updateVisibilityHistory(visible, radius, minRadius, step, desiredRadius, cooldownTime);

			radius = glm::mix(radius, finalRadius, 0.15f); // Smoothly interpolate radius

			// 4. Recalculate final eye based on adjusted radius
			eye = playerPos - front * radius;
			lookAt = playerPos;

			// 5. Update player rotation
			player->setRotY(-(theta + radians(-90.0f)));
			player->setRotX(phi);

			manMoveDir = vec3(sin(player->getRotY()), 0, cos(player->getRotY()));
			right = normalize(cross(manMoveDir, up));

				// lookAt = eye + front;
		}
		else {
			//Activate Debug Camera
			float radius = 1.0;
			float x = radius * cos(phi) * cos(theta);
			float y = radius * sin(phi);
			float z = radius * cos(phi) * sin(theta);
			// Defined above Globally- eyePos = vec3(0.0, 0.0, 0.0);
			vec3 targetPos = vec3(x, y, z);
			vec3 viewVec = normalize(targetPos);

			if (movingForward) {
				debugEye += debugMovementSpeed * viewVec;
			}
			if (movingBackward) {
				debugEye -= debugMovementSpeed * viewVec;
			}
			if (movingLeft) {
				debugEye -= debugMovementSpeed * normalize(cross(targetPos, up));
			}
			if (movingRight) {
				debugEye += debugMovementSpeed * normalize(cross(targetPos, up));
			}

			eye = debugEye;
			lookAt = debugEye + targetPos;
		}

	}

	void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			if (spellCounts[currentPlayerSpellType] > 0) {
				shootSpell(); // Changed from shootSpell
			}
		}
	}

	void resizeCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}


	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color and enable z-buffer test
		glClearColor(.12f, .34f, .56f, 1.0f);
		glEnable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Initialize GLSL programs for shadow mapping
		DepthProg = make_shared<Program>();
		DepthProg->setVerbose(Config::DEBUG_SHADER);
		DepthProg->setShaderNames(resourceDirectory + "/depth_vert.glsl", resourceDirectory + "/depth_frag.glsl");
		DepthProg->init();

		DepthProgDebug = make_shared<Program>();
		DepthProgDebug->setVerbose(Config::DEBUG_SHADER);
		DepthProgDebug->setShaderNames(resourceDirectory + "/depth_vertDebug.glsl", resourceDirectory + "/depth_fragDebug.glsl");
		DepthProgDebug->init();

		ShadowProg = make_shared<Program>();
		ShadowProg->setVerbose(Config::DEBUG_SHADER);
		ShadowProg->setShaderNames(resourceDirectory + "/shadow_vert.glsl", resourceDirectory + "/shadow_frag.glsl");
		ShadowProg->init();

		DebugProg = make_shared<Program>();
		DebugProg->setVerbose(Config::DEBUG_SHADER);
		DebugProg->setShaderNames(resourceDirectory + "/pass_vert.glsl", resourceDirectory + "/pass_texfrag.glsl");
		DebugProg->init();

		SkyboxProg = make_shared<Program>();
		SkyboxProg->setVerbose(Config::DEBUG_SHADER);
		SkyboxProg->setShaderNames(resourceDirectory + "/skybox_vert.glsl", resourceDirectory + "/skybox_frag.glsl");
		SkyboxProg->init();

		debugLineProg = make_shared<Program>();
		debugLineProg->setVerbose(Config::DEBUG_SHADER);
		debugLineProg->setShaderNames(resourceDirectory + "/line_vert.glsl", resourceDirectory + "/line_frag.glsl");
		debugLineProg->init();

		// Add unfigorm and attrubutes to each of the programs
		DepthProg->addUniform("LP");
		DepthProg->addUniform("LV");
		DepthProg->addUniform("M");
		DebugProg->addUniform("texBuf");
		DepthProg->addAttribute("vertPos");

		DepthProgDebug->addUniform("LP");
		DepthProgDebug->addUniform("LV");
		DepthProgDebug->addUniform("M");
		DepthProgDebug->addAttribute("vertPos");

		ShadowProg->addUniform("P");
		ShadowProg->addUniform("V");
		ShadowProg->addUniform("M");
		ShadowProg->addUniform("LV");
		ShadowProg->addUniform("lightDir");
		ShadowProg->addUniform("lightColor");
		ShadowProg->addUniform("cameraPos");
		ShadowProg->addAttribute("vertPos");
		ShadowProg->addAttribute("vertNor");
		ShadowProg->addAttribute("vertTex");
		ShadowProg->addAttribute("InstancedOffset");
		ShadowProg->addUniform("uMaps");
		ShadowProg->addUniform("shadowDepth");
		ShadowProg->addUniform("hasMaterial");
		ShadowProg->addUniform("hasBones");
		ShadowProg->addUniform("hasInstancing");
		ShadowProg->addUniform("MatAlbedo");
		ShadowProg->addUniform("MatRough");
		ShadowProg->addUniform("MatMetal");
		ShadowProg->addUniform("MatEmit");
		ShadowProg->addUniform("enemyAlpha");
		ShadowProg->addUniform("texOnly");
		ShadowProg->addUniform("exposure");
		ShadowProg->addUniform("saturation");
		for (int i = 0; i < Config::MAX_BONES; i++) ShadowProg->addUniform("finalBonesMatrices[" + to_string(i) + "]");
		ShadowProg->addUniform("pawCount");
		ShadowProg->addUniform("pawData");
		ShadowProg->addUniform("pawTex");
		ShadowProg->addUniform("curTime");
		ShadowProg->addAttribute("vertPos");
		ShadowProg->addAttribute("vertNor");
		ShadowProg->addAttribute("vertTex");
		ShadowProg->bind();
		GLint loc = ShadowProg->getUniform("uMaps");
		GLint units[6] = { 0,1,2,3,4,5 };
		glUniform1iv(loc, 6, units);
		ShadowProg->unbind();

		initShadow();

		hudProg = make_shared<Program>();
		hudProg->setVerbose(true);
		hudProg->setShaderNames(resourceDirectory + "/hud_vert.glsl", resourceDirectory + "/hud_frag.glsl");
		hudProg->init();
		hudProg->addUniform("projection");
		hudProg->addUniform("model");
		hudProg->addUniform("healthPercent");
		hudProg->addUniform("BarStartX");
		hudProg->addUniform("BarWidth");

		// Initialize the particle program
		particleProg = make_shared<Program>();
		particleProg->setVerbose(true);
		particleProg->setShaderNames(resourceDirectory + "/particle_vert.glsl", resourceDirectory + "/particle_frag.glsl");
		particleProg->init();
		particleProg->addUniform("P");
		particleProg->addUniform("V");
		particleProg->addUniform("M");
		particleProg->addUniform("alphaTexture");
		particleProg->addAttribute("vertPos");
		particleProg->addAttribute("vertColor");
		particleProg->addAttribute("vertScale");

		redFlashProg = make_shared<Program>();
		redFlashProg->setVerbose(true);
		redFlashProg->setShaderNames(resourceDirectory + "/red_flash_vert.glsl", resourceDirectory + "/red_flash_frag.glsl");
		redFlashProg->init();
		redFlashProg->addUniform("projection");
		redFlashProg->addUniform("model");
		redFlashProg->addUniform("alpha");

		SkyboxProg->addUniform("P");
		SkyboxProg->addUniform("V");
		SkyboxProg->addUniform("skyTex");

		debugLineProg->addUniform("P");
		debugLineProg->addUniform("V");
		debugLineProg->addUniform("M");
		debugLineProg->addUniform("color");

		textProg = make_shared<Program>();
		textProg->setVerbose(true);
		textProg->setShaderNames(resourceDirectory + "/textVert.glsl", resourceDirectory + "/textFrag.glsl");
		textProg->init();
		textProg->addAttribute("vertex");
		textProg->addUniform("projection");
		textProg->addUniform("textTex");
		textProg->addUniform("textColor");

		updateCameraVectors();

		pawTex = make_shared<Texture>();
		pawTex->setFilename(resourceDirectory + "/paw_print.png");
		pawTex->init();
		pawTex->setUnit(11);
		pawTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		borderWallTex = make_shared<Texture>();
		borderWallTex->setFilename(resourceDirectory + "/Wall/textures/mossCastle.png");
		borderWallTex->init();
		borderWallTex->setUnit(0);
		borderWallTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		libraryGroundTex = make_shared<Texture>();
		libraryGroundTex->setFilename(resourceDirectory + "/book_shelf/textures/wood_texture.png");
		libraryGroundTex->init();
		libraryGroundTex->setUnit(0);
		libraryGroundTex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		carpetTex = make_shared<Texture>();
		carpetTex->setFilename(resourceDirectory + "/cluster_assets/carpet_texture1.png");
		carpetTex->init();
		carpetTex->setUnit(0);
		carpetTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		// Initialize particle alpha texture
		particleAlphaTex = make_shared<Texture>();
		particleAlphaTex->setFilename(resourceDirectory + "/alpha.bmp");
		particleAlphaTex->init();
		particleAlphaTex->setUnit(0);
		particleAlphaTex->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		// Initialize particle system
		particleSystem = make_shared<particleGen>(vec3(0.0f), 0.0f, 0.2f, 0.6f, 0.8f, 0.8f, 1.0f, 0.1f, 0.2f);
		particleSystem->gpuSetup();

		unsigned char white[3] = { 255,255,255 };
		unsigned char flatN[3] = { 128,128,255 };
		unsigned char black[3] = { 0,0,0 };
		GLuint blackTex = genSolidTexture(black, GL_RGB);
		GLuint whiteTex = genSolidTexture(white, GL_RGB);
		GLuint normalTex = genSolidTexture(flatN, GL_RGB);

		TextureManager::initFallbacks(whiteTex, normalTex, blackTex);

		int fError = initFont(resourceDirectory);
		cout << "Font error? " << fError << endl;
	}

	GLuint genSolidTexture(const unsigned char* pixel, GLenum format) {
		GLuint id;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, format, 1, 1, 0, format, GL_UNSIGNED_BYTE, pixel);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return id;
	}

	void initMapGen()
	{
		library->generate(gridSize, glm::vec3(0, 0, 0), player->getPosition(), bossEntranceDir);
		grid = library->getGrid();

		if (bossEntranceDir.y > 0) {
			addWall(gridSize.x * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(0)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.x - 3, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.x - 1, vec3(library->mapGridXtoWorldX((gridSize.x - 1) / 2), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.y * 2, vec3(library->mapGridXtoWorldX(0), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
			addWall(gridSize.y * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
		}
		else if (bossEntranceDir.y < 0) {
			addWall(gridSize.x * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.x - 3, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(0)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.x - 1, vec3(library->mapGridXtoWorldX((gridSize.x - 1) / 2), 0, library->mapGridYtoWorldZ(0)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.y * 2, vec3(library->mapGridXtoWorldX(0), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
			addWall(gridSize.y * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
		}
		else if (bossEntranceDir.x > 0) {
			addWall(gridSize.x * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.x * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(0)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.y - 3, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
			addWall(gridSize.y - 1, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ((gridSize.y - 1) / 2)), vec3(0, 0, -1), 10.0f, borderWallTex);
			addWall(gridSize.y * 2, vec3(library->mapGridXtoWorldX(0) , 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
		}
		else if (bossEntranceDir.x < 0) {
			addWall(gridSize.x * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.x * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(0)), vec3(-1, 0, 0), 10.0f, borderWallTex);
			addWall(gridSize.y - 3, vec3(library->mapGridXtoWorldX(0), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
			addWall(gridSize.y - 1, vec3(library->mapGridXtoWorldX(0), 0, library->mapGridYtoWorldZ((gridSize.y - 1) / 2)), vec3(0, 0, -1), 10.0f, borderWallTex);
			addWall(gridSize.y * 2, vec3(library->mapGridXtoWorldX(gridSize.x - 1), 0, library->mapGridYtoWorldZ(gridSize.y - 1)), vec3(0, 0, -1), 10.0f, borderWallTex);
		}

		addLibGrnd(gridSize.x * 2, gridSize.y * 2, 0.0f, vec3(0, 0, 0), libraryGroundTex);

		bossRoom->generate(bossGridSize, gridSize, glm::vec3(0, 0, 0), bossEntranceDir);
		bossGrid = bossRoom->getGrid();
		addLibGrnd(bossGridSize.x * 2, bossGridSize.y * 2, 0.0f, bossRoom->getWorldOrigin(), libraryGroundTex);
	}

	void initInstancingMatrices() {
		// Clear previous frame data
		book_shelf1Matrices.clear();
		book_shelf2Matrices.clear();
		bookstandMatrices.clear();
		table_chairs1Matrices.clear();
		table_chairs2Matrices.clear();
		chestMatrices.clear();
		candelabraMatrices.clear();
		clockMatrices.clear();
		doorMatrices.clear();


		for (int z = 0; z < grid.getSize().y; ++z) {
			for (int x = 0; x < grid.getSize().x; ++x) {
				glm::ivec2 gridPos(x, z);
				if (grid[gridPos].type != LibraryGen::CellType::CLUSTER)
					continue;

				float i = library->mapGridXtoWorldX(x);
				float j = library->mapGridYtoWorldZ(z);
				glm::vec3 pos(i, libraryCenter.y, j);
				float rotation = grid[gridPos].transformData.rotation;
				glm::vec3 scale = grid[gridPos].transformData.scale;
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, pos);


				auto addInstance = [&](std::vector<glm::mat4>& container) {
					glm::mat4 instModel = model;
					instModel = glm::rotate(instModel, rotation, glm::vec3(0, 1, 0));
					instModel = glm::scale(instModel, scale);
					container.push_back(instModel);
				};

				using CT = LibraryGen::ClusterType;
				using OT = LibraryGen::CellObjType;

				switch (grid[gridPos].clusterType) {
					case CT::SHELF1: addInstance(book_shelf1Matrices); break;
					case CT::SHELF2: addInstance(book_shelf1Matrices); break;
					case CT::SHELF3: addInstance(book_shelf1Matrices); break;
					case CT::ONLY_CANDELABRA: addInstance(candelabraMatrices); break;
					case CT::ONLY_CHEST: addInstance(chestMatrices); break;
					case CT::ONLY_TABLE:
						addInstance(table_chairs1Matrices);
						addLibGrnd(5.0f, 5.0f, 1.0f, vec3(i, libraryCenter.y + 0.1f, j), carpetTex);
						break;
					case CT::ONLY_CLOCK: addInstance(clockMatrices); break;
					case CT::ONLY_BOOKSTAND: addInstance(bookstandMatrices); break;

					case CT::LAYOUT1:
						switch (grid[gridPos].objectType) {
							case OT::BOOKSHELF: addInstance(book_shelf1Matrices); break;
							case OT::ROTATED_BOOKSHELF: addInstance(book_shelf1Matrices); break;
							case OT::TABLE_AND_CHAIR1:
							case OT::TABLE_AND_CHAIR2:
								addInstance(table_chairs1Matrices);
								addLibGrnd(5.0f, 5.0f, 1.0f, vec3(i, libraryCenter.y + 0.1f, j), carpetTex);
								break;
							case OT::CHEST: addInstance(chestMatrices); break;
							case OT::CANDELABRA: addInstance(candelabraMatrices); break;
							case OT::GRANDFATHER_CLOCK: addInstance(clockMatrices); break;
							default: break;
						}
						break;

					case CT::GLOWING_SHELF1:
						switch (grid[gridPos].objectType) {
							case OT::SHELF_WITH_ABILITY: addInstance(book_shelf2Matrices); break;
							case OT::BOOKSHELF: addInstance(book_shelf1Matrices); break;
							default: break;
						}
						break;

					case CT::GLOWING_SHELF2:
						switch (grid[gridPos].objectType) {
							case OT::SHELF_WITH_ABILITY_ROTATED:
								addInstance(book_shelf2Matrices); break;
							case OT::ROTATED_BOOKSHELF:
								addInstance(book_shelf1Matrices); break;
							default: break;
						}
						break;

					default:
						break;
				}
			}
		}

		// -- Append boss room objects to instancing arrays --
		for (int z = 0; z < bossGrid.getSize().y; ++z) {
			for (int x = 0; x < bossGrid.getSize().x; ++x) {
				glm::ivec2 gridPos(x, z);

				float i = bossRoom->mapGridXtoWorldX(x);
				float j = bossRoom->mapGridYtoWorldZ(z);
				if (bossGrid[gridPos].type == BossRoomGen::CellType::BORDER) continue; // Skip borders for instancing already initialized in another function
				glm::vec3 pos(i, libraryCenter.y, j);
				float rotation = bossGrid[gridPos].transformData.rotation;
				glm::vec3 scale = bossGrid[gridPos].transformData.scale;
				glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
				model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 1, 0));
				model = glm::scale(model, scale);

				auto addInstance = [&](std::vector<glm::mat4>& container) {
					container.push_back(model);
				};

				using CT = BossRoomGen::CellType;
				using BT = BossRoomGen::BorderType;
				using OT = BossRoomGen::CellObjType;

				const auto& cell = bossGrid[gridPos];

				switch (cell.type) {
					case CT::BORDER:
						addInstance(book_shelf1Matrices);
						break;

					case CT::ENTRANCE:
						if (cell.borderType == BT::ENTRANCE_SIDE) {
							addInstance(book_shelf1Matrices);
						} else if (cell.borderType == BT::ENTRANCE_MIDDLE) {
							addInstance(doorMatrices);
						}
						break;

					case CT::EXIT:
						if (cell.borderType == BT::EXIT_SIDE) {
							addInstance(book_shelf1Matrices);
						} else if (cell.borderType == BT::EXIT_MIDDLE) {
							addInstance(doorMatrices);
						}
						break;

					case CT::CLUSTER:
						if (cell.clusterType == BossRoomGen::ClusterType::SHELF1) {
							if (cell.objectType == OT::GLOWING_SHELF) {
								addInstance(book_shelf2Matrices);
							}
						}
						break;

					default:
						break;
				}
			}
		}

		book_shelf1->InitializeInstancing(book_shelf1Matrices);
		book_shelf2->InitializeInstancing(book_shelf2Matrices);
		bookstand->InitializeInstancing(bookstandMatrices);
		table_chairs1->InitializeInstancing(table_chairs1Matrices);
		table_chairs2->InitializeInstancing(table_chairs2Matrices);
		chest->InitializeInstancing(chestMatrices);
		candelabra->InitializeInstancing(candelabraMatrices);
		grandfather_clock->InitializeInstancing(clockMatrices);

	}


	void initQuadTree() {
		int count = 0;
		float cellSize = 2.0f; // Assuming square cells
		libraryQuadTree = new Quadtree(glm::vec2(0, 0), glm::vec2(grid.getSize().x * cellSize * 0.5f, grid.getSize().y * cellSize * 0.5f));
		for (int z = 0; z < grid.getSize().y; ++z) {
			for (int x = 0; x < grid.getSize().x; ++x) {
				glm::ivec2 cellPos(x, z);
				if (!grid.inBounds(cellPos)) continue; // Skip out-of-bounds cells
				LibraryGen::Cell cell = grid[cellPos];
				if ((cell.type != LibraryGen::CellType::CLUSTER) && (cell.type != LibraryGen::CellType::BORDER)) {
					continue; // Skip non-cluster and non-border cells
				}
				// if (cell.type != LibraryGen::CellType::CLUSTER) continue; // Skip non-cluster cells

				float i = library->mapGridXtoWorldX(x);
				float j = library->mapGridYtoWorldZ(z);
				glm::vec3 clusterCenter = glm::vec3(i, libraryCenter.y, j);
				glm::vec3 clusterBboxMin, clusterBboxMax;

				if (grid[cellPos].type == LibraryGen::CellType::CLUSTER) {
						switch (cell.objectType) {
							case LibraryGen::CellObjType::CANDELABRA:
								clusterBboxMin = candelabra->getBoundingBoxMin();
								clusterBboxMax = candelabra->getBoundingBoxMax();
								break;
							case LibraryGen::CellObjType::CHEST:
								clusterBboxMin = chest->getBoundingBoxMin();
								clusterBboxMax = chest->getBoundingBoxMax();
								break;
							case LibraryGen::CellObjType::GRANDFATHER_CLOCK:
								clusterBboxMin = grandfather_clock->getBoundingBoxMin();
								clusterBboxMax = grandfather_clock->getBoundingBoxMax();
								break;
							case LibraryGen::CellObjType::ROTATED_BOOKSHELF:
							case LibraryGen::CellObjType::BOOKSHELF:
								clusterBboxMin = book_shelf1->getBoundingBoxMin();
								clusterBboxMax = book_shelf1->getBoundingBoxMax();
								break;
							case LibraryGen::CellObjType::TABLE_AND_CHAIR1:
								clusterBboxMin = table_chairs1->getBoundingBoxMin();
								clusterBboxMax = table_chairs1->getBoundingBoxMax();
								break;
							case LibraryGen::CellObjType::TABLE_AND_CHAIR2:
								clusterBboxMin = table_chairs2->getBoundingBoxMin();
								clusterBboxMax = table_chairs2->getBoundingBoxMax();
								break;
							case LibraryGen::CellObjType::SHELF_WITH_ABILITY:
							case LibraryGen::CellObjType::SHELF_WITH_ABILITY_ROTATED:
								clusterBboxMin = book_shelf2->getBoundingBoxMin();
								clusterBboxMax = book_shelf2->getBoundingBoxMax();
								count++;
								break;
							case LibraryGen::CellObjType::BOOKSTAND:
								clusterBboxMin = bookstand->getBoundingBoxMin();
								clusterBboxMax = bookstand->getBoundingBoxMax();
								break;
							default:
								continue; // Skip unknown object types
						}
					} else if (cell.type == LibraryGen::CellType::BORDER) {
						switch (cell.borderType) {
							case LibraryGen::BorderType::TOP_BORDER:
							case LibraryGen::BorderType::BOTTOM_BORDER:
								clusterBboxMin = glm::vec3(-2.0f, 0.0f, -0.25f);
								clusterBboxMax = glm::vec3(2.0f, 2.0f, 0.25f);
								break;
							case LibraryGen::BorderType::LEFT_BORDER:
							case LibraryGen::BorderType::RIGHT_BORDER:
								clusterBboxMin = glm::vec3(-0.25f, 0.0f, -2.0f);
								clusterBboxMax = glm::vec3(0.25f, 2.0f, 2.0f);
								break;
							case LibraryGen::BorderType::LEFT_OF_BOSS_ENTRANCE:
								if (bossEntranceDir.y > 0) {
									clusterBboxMin = glm::vec3(-1.0f, 0.0f, -0.25f);
									clusterBboxMax = glm::vec3(1.0f, 2.0f, 0.25f);
								} else if (bossEntranceDir.y < 0) {
									clusterBboxMin = glm::vec3(-1.0f, 0.0f, -0.25f);
									clusterBboxMax = glm::vec3(1.0f, 2.0f, 0.25f);
								} else if (bossEntranceDir.x > 0) {
									clusterBboxMin = glm::vec3(-0.25f, 0.0f, -1.5f);
									clusterBboxMax = glm::vec3(0.25f, 2.0f, 1.5f);
								} else if (bossEntranceDir.x < 0) {
									clusterBboxMin = glm::vec3(-0.25f, 0.0f, -1.5f);
									clusterBboxMax = glm::vec3(0.25f, 2.0f, 1.5f);
								} else {
									continue; // Skip if boss entrance is not at the top
								}
								break;
							case LibraryGen::BorderType::RIGHT_OF_BOSS_ENTRANCE:
								if (bossEntranceDir.x > 0) {
									clusterBboxMin = glm::vec3(-0.25f, 0.0f, -0.40f);
									clusterBboxMax = glm::vec3(0.25f, 2.0f, 0.75f);
								} else if (bossEntranceDir.x < 0) {
									clusterBboxMin = glm::vec3(-0.25f, 0.0f, -0.40f);
									clusterBboxMax = glm::vec3(0.25f, 2.0f, 0.75f);
								}
								else {
									continue; // Skip if boss entrance is not at the top
								}
								break;
							default:
								// // For borders, we can use a simple bounding box
								// clusterBboxMin = glm::vec3(-1.0f, 0.0f, -1.0f);
								// clusterBboxMax = glm::vec3(1.0f, 2.0f, 1.0f);
								// break;
								continue; // Skip unknown border types
						}

					} else {
						continue; // Skip non-cluster cells
					}

				// Calculate the world bounding box for the object
				glm::mat4 objectTransform = glm::translate(glm::mat4(1.0f), clusterCenter);
				objectTransform = glm::rotate(objectTransform, cell.transformData.rotation, glm::vec3(0, 1, 0));
				objectTransform = glm::scale(objectTransform, cell.transformData.scale);

				glm::vec3 clusterWorldMin, clusterWorldMax;
				updateBoundingBox(clusterBboxMin, clusterBboxMax, objectTransform, clusterWorldMin, clusterWorldMax);

				// Add the bounding box to the quadtree
				int id = z * gridSize.x + x; // Unique ID for the cell
				glm::vec2 quadaabb_center = glm::vec2(clusterCenter.x, clusterCenter.z);
				QuadElement element(id, quadaabb_center, cellPos, clusterWorldMin, clusterWorldMax);
				libraryQuadTree->insert(element, 5);
				// std::cout << "Inserted element with ID: " << id << " at position: (" << clusterCenter.x << ", " << clusterCenter.z << ")" << std::endl;
			}
		}
		std::cout << "GLOWING SHELF COUNT: " << count << std::endl;
		std::cout << "Library Quadtree initialized with " << libraryQuadTree->getElementCount() << " elements." << std::endl;
		std::cout << "Subdivisions: " << libraryQuadTree->getMaxSubdivisions() << std::endl;

		bossRoomQuadTree = new Quadtree(glm::vec2(bossRoom->getWorldOrigin().x, bossRoom->getWorldOrigin().z), glm::vec2(bossGrid.getSize().x * cellSize * 0.5f, bossGrid.getSize().y * cellSize * 0.5f));
		for (int z = 0; z < bossGridSize.y; ++z) {
			for (int x = 0; x < bossGridSize.x; ++x) {
				glm::ivec2 cellPos = glm::ivec2(x, z);
				if (!bossGrid.inBounds(cellPos)) continue; // Skip out-of-bounds cells

				const auto& cell = bossGrid[cellPos];
				if (cell.type == BossRoomGen::CellType::NONE) continue;
				// if (bossfightstarted && !bossRoom->isInsideBossArea(cellPos)) return true;

				glm::vec3 clusterBboxMin;
				glm::vec3 clusterBboxMax;
				glm::vec3 clusterCenter = glm::vec3(bossRoom->mapGridXtoWorldX(cellPos.x), libraryCenter.y, bossRoom->mapGridYtoWorldZ(cellPos.y));

				switch (cell.objectType) {
					case BossRoomGen::CellObjType::BOOKSHELF:
						clusterBboxMin = book_shelf1->getBoundingBoxMin();
						clusterBboxMax = book_shelf1->getBoundingBoxMax();
						break;
					case BossRoomGen::CellObjType::GLOWING_SHELF:
						clusterBboxMin = book_shelf2->getBoundingBoxMin();
						clusterBboxMax = book_shelf2->getBoundingBoxMax();
						break;
					case BossRoomGen::CellObjType::DOOR:
						clusterBboxMin = door->getBoundingBoxMin();
						clusterBboxMax = door->getBoundingBoxMax();
						break;
					default:
						continue; // Skip unknown object types
				}

				glm::mat4 objectTransform = glm::translate(glm::mat4(1.0f), clusterCenter);
				objectTransform = glm::rotate(objectTransform, cell.transformData.rotation, glm::vec3(0, 1, 0));
				objectTransform = glm::scale(objectTransform, cell.transformData.scale);
				glm::vec3 clusterWorldMin, clusterWorldMax;
				updateBoundingBox(clusterBboxMin, clusterBboxMax, objectTransform, clusterWorldMin, clusterWorldMax);

				int id = z * bossGridSize.x + x; // Unique ID for the cell
				glm::vec2 quadaabb_center = glm::vec2(clusterCenter.x, clusterCenter.z);
				QuadElement element(id, quadaabb_center, cellPos, clusterWorldMin, clusterWorldMax);
				bossRoomQuadTree->insert(element, 10);
				// std::cout << "Inserted boss room element with ID: " << id << " at position: (" << clusterCenter.x << ", " << clusterCenter.z << ")" << std::endl;
			}
		}

		std::cout << "Boss Room Quadtree initialized with " << bossRoomQuadTree->getElementCount() << " elements." << std::endl;

	}

	void initGeom(const std::string& resourceDirectory) { // NOTE: PROBLEMS GETTING ANIMATION FROM "Fixed" FBX
		string errStr;

		// load the walking character moded
		player_rig = new AssimpModel(resourceDirectory + "/CatWizard/CatWizardAnimationRig2.fbx");
		player_rig->assignTexture("texture_diffuse", resourceDirectory + "/CatWizard/textures/ImphenziaPalette02-Albedo.png");
		//PROBLEM GETTING ANIMATION FROM "Fixed" FBX
		player_walk = new Animation(resourceDirectory + "/CatWizard/CatWizardAnimationRig2.fbx", player_rig, 1);
		player_idle = new Animation(resourceDirectory + "/CatWizard/CatWizardAnimationRig2.fbx", player_rig, 2);
		//player_idle = new Animation(resourceDirectory + "/Vanguard/Vanguard.fbx", player_rig, 1);

		//TEST Load the cat
		//CatWizard = new AssimpModel(resourceDirectory + "/CatWizard/BlendWalkFix.fbx");
		playerBB = make_shared<AABB>();
		playerBB->min = player_rig->getBoundingBoxMin() * manScale.x;
		playerBB->max = player_rig->getBoundingBoxMax() * manScale.x;
		playerBB->max.y += 2.0f;

		catwizard_animator = new Animator(player_walk);

		cube = new AssimpModel(resourceDirectory + "/cube.obj");

		bookCover = new AssimpModel(resourceDirectory + "/cornerCube/sideCube.fbx");
		bookCover->assignTexture("texture_diffuse", resourceDirectory + "/cornerCube/brown-leather-tex/brown-leather_albedo.png");
		bookCover->assignTexture("texture_roughness", resourceDirectory + "/cornerCube/brown-leather-tex/brown-leather_roughness.png");
		bookCover->assignTexture("texture_metalness", resourceDirectory + "/cornerCube/brown-leather-tex/brown-leather_metallic.png");
		bookCover->assignTexture("texture_normal", resourceDirectory + "/cornerCube/brown-leather-tex/brown-leather_normal-ogl.png");

		bookPaper = new AssimpModel(resourceDirectory + "/cornerCube/sideCube.fbx");
		bookPaper->assignTexture("texture_diffuse", resourceDirectory + "/cornerCube/wrinkled-paper-tex/wrinkled-paper-albedo.png");
		bookPaper->assignTexture("texture_roughness", resourceDirectory + "/cornerCube/wrinkled-paper-tex/wrinkled-paper-roughness.png");
		bookPaper->assignTexture("texture_metalness", resourceDirectory + "/cornerCube/wrinkled-paper-tex/wrinkled-paper-metalness.png");
		bookPaper->assignTexture("texture_normal", resourceDirectory + "/cornerCube/wrinkled-paper-tex/wrinkled-paper-normal-ogl.png");

		book_shelf1 = new AssimpModel(resourceDirectory + "/cluster_assets/bookshelf_texture2.obj");
		book_shelf1->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/darker_bookshelf_diffuse.png");

		book_shelf2 = new AssimpModel(resourceDirectory + "/cluster_assets/bookshelf_texture2.obj");
		book_shelf2->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/glowing_bookshelf_bake_diffuse.png");

		candelabra = new AssimpModel(resourceDirectory + "/cluster_assets/candelabrum/Candelabrum.obj");
		candelabra->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/candelabrum/textures/defaultobject_gloss.png");
		candelabra->assignTexture("texture_specular", resourceDirectory + "/cluster_assets/candelabrum/textures/defaultobject_specular.png");
		candelabra->assignTexture("texture_normal", resourceDirectory + "/cluster_assets/candelabrum/textures/defaultobject_normal.png");

		chest = new AssimpModel(resourceDirectory + "/cluster_assets/chest/Chest.obj");
		chest->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/chest/textures/TreasureChestDiffuse_2.png");
		chest->assignTexture("texture_roughness", resourceDirectory + "/cluster_assets/chest/textures/TreasureChestRoughness_2.png");
		chest->assignTexture("texture_metalness", resourceDirectory + "/cluster_assets/chest/textures/TreasureChestMetal_2.png");
		chest->assignTexture("texture_normal", resourceDirectory + "/cluster_assets/chest/textures/TreasureChestNormal_2.png");

		library_bench = new AssimpModel(resourceDirectory + "/cluster_assets/library_bench/library_bench.obj");
		library_bench->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/library_bench/textures/bench_diffuse.png");

		table_chairs1 = new AssimpModel(resourceDirectory + "/cluster_assets/table_chairs/table_chairs_3.obj");
		table_chairs1->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/table_chairs/textures/table_chairs_3_diffuse.png");

		table_chairs2 = new AssimpModel(resourceDirectory + "/cluster_assets/table_chairs/table_chairs_4.obj");
		table_chairs2->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/table_chairs/textures/table_chairs_4_diffuse.png");

		grandfather_clock = new AssimpModel(resourceDirectory + "/cluster_assets/grandfather_clock/grandfather_clock.obj");
		grandfather_clock->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/grandfather_clock/textures/Clock_L_lambert1_BaseColor.tga.png");
		grandfather_clock->assignTexture("texture_metalness", resourceDirectory + "/cluster_assets/grandfather_clock/textures/Clock_L_lambert1_Metallic.tga.png");
		grandfather_clock->assignTexture("texture_roughness", resourceDirectory + "/cluster_assets/grandfather_clock/textures/Clock_L_lambert1_Roughness.tga.png");
		grandfather_clock->assignTexture("texture_normal", resourceDirectory + "/cluster_assets/grandfather_clock/textures/Clock_L_lambert1_Normal.tga.jpg");

		bookstand = new AssimpModel(resourceDirectory + "/cluster_assets/bookstand/bookstand.obj");
		bookstand->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/bookstand/textures/bookstand_diffuse.png");

		door = new AssimpModel(resourceDirectory + "/cluster_assets/door/door.obj");
		door->assignTexture("texture_diffuse", resourceDirectory + "/cluster_assets/door/Door_diffuse.png");

		sphere = new AssimpModel(resourceDirectory + "/SmoothSphere.obj");
		sphereBB = make_shared<AABB>();
		sphereBB->min = sphere->getBoundingBoxMin();
		sphereBB->max = sphere->getBoundingBoxMax();

		iceElemental = new AssimpModel(resourceDirectory + "/IceElemental/IceElem.fbx");
		fireElemental = new AssimpModel(resourceDirectory + "/FireElemental/FireElem.fbx");
		lightningElemental = new AssimpModel(resourceDirectory + "/LightningElemental/LightningElem.fbx");

		healthBar = new AssimpModel(resourceDirectory + "/Quad/hud_quad.obj");
		healthBar->assignTexture("texture_diffuse", resourceDirectory + "/healthbar.bmp");

		stoneGolem = new AssimpModel(resourceDirectory + "/StoneGolem/Stone.obj");
		stoneGolem->assignTexture("texture_diffuse", resourceDirectory + "/StoneGolem/textures/diffuso.tif");

		/*
		* KEY COLLECTIBLE IS BROKEN. THIS IS THE COMMENTED OUT PROGRESS OF MADILINE SINCE PROJECT DOESN'T COMPILE WITH IT
		//key
		key = new AssimpModel(resourceDirectory + "/Key_and_Lock/key.obj");

		// Collectible key1 = Collectible(key, vec3(0.0, 2.0, 0.0), 0.1f,  vec3(0.9, 0.9, 0.9), SpellType::NONE);
		// keyCollectibles.push_back(key1);


		Collectible key1 = Collectible(key, vec3(0.0, 2.0, 0.0), 0.1f,  vec3(0.9, 0.9, 0.9), SpellType::NONE);
		keyCollectibles.push_back(key1);
		*/
		//lock

		key = new AssimpModel(resourceDirectory + "/Key_and_Lock/key.obj");

		lock = new AssimpModel(resourceDirectory + "/Key_and_Lock/lockCopy.obj");
		lockHandle = new AssimpModel(resourceDirectory + "/Key_and_Lock/lockHandle.obj");

		cout << "[DEBUG] Stored Base Sphere Local AABB." << endl;

		vec3 bossSpawnPos = bossRoom->getWorldOrigin();



		initEnemies();
		bossEnemy = new BossEnemy(bossSpawnPos, BOSS_HP_MAX, stoneGolem, vec3(1.3f, 0.8f, 1.0f), vec3(0, 1, 0), BOSS_SPECIAL_ATTACK_COOLDOWN, SpellType::ICE);

		initTextQuad();
	}

	void SetMaterial(shared_ptr<Program> shader, Material color) {
		/*
		Albedo(Base Color) :
		Never use pure black (0,0,0) or pure white (1,1,1)
		Realistic materials range from about 0.04 to 0.95
		For metals, this is the actual metal color

		Roughness:
		0.0 = perfectly smooth (mirror-like)
		1.0 = completely rough (diffuse)
		Most materials fall between 0.2-0.8

		Metalness:
		0.0 = non-metallic (dielectric)
		1.0 = metallic
		Should almost always be 0 or 1, rarely in between

		Emission:
		Only for materials that emit light
		Values can exceed 1.0 for strong emission
		Most materials have (0,0,0) emission

		Good reference values can be found at physicallybased.info.
		*/

		if (!shader->hasUniform("hasMaterial")) return;

		glUniform1i(shader->getUniform("hasMaterial"), GL_TRUE);

		switch (color) {
		case Material::purple:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.3f, 0.1f, 0.4f);
				glUniform1f(shader->getUniform("MatRough"), 0.7f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::black:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.04f, 0.04f, 0.04f);
				glUniform1f(shader->getUniform("MatRough"), 0.8f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::eye_white:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.95f, 0.95f, 0.95f);
				glUniform1f(shader->getUniform("MatRough"), 0.2f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::pupil_white:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.85f, 0.85f, 0.9f);
				glUniform1f(shader->getUniform("MatRough"), 0.1f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::bronze:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.714f, 0.4284f, 0.181f);
				glUniform1f(shader->getUniform("MatRough"), 0.4f);
				glUniform1f(shader->getUniform("MatMetal"), 1.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::silver:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.972f, 0.960f, 0.915f);
				glUniform1f(shader->getUniform("MatRough"), 0.2f);
				glUniform1f(shader->getUniform("MatMetal"), 1.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::brown:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.25f, 0.15f, 0.08f);
				glUniform1f(shader->getUniform("MatRough"), 0.7f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::orb_glowing_blue:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.1f, 0.2f, 0.5f);
				glUniform1f(shader->getUniform("MatRough"), 0.7f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.1f, 0.2f, 1.0f);
				break;
			case Material::orb_glowing_red:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.5f, 0.1f, 0.1f);
				glUniform1f(shader->getUniform("MatRough"), 1.0f);
				glUniform1f(shader->getUniform("MatMetal"), 1.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.9f, 0.3f, 0.2f);
				break;
			case Material::orb_glowing_yellow:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.5f, 0.4f, 0.1f);
				glUniform1f(shader->getUniform("MatRough"), 1.0f);
				glUniform1f(shader->getUniform("MatMetal"), 1.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.9f, 0.8f, 0.2f);
				break;
			case Material::grey:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.8f, 0.8f, 0.8f);
				glUniform1f(shader->getUniform("MatRough"), 0.6f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::wood:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.65f, 0.45f, 0.25f);
				glUniform1f(shader->getUniform("MatRough"), 0.8f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::mini_map:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.65f, 0.45f, 0.25f);
				glUniform1f(shader->getUniform("MatRough"), 0.0f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 1.0f, 1.0f, 1.0f);
				break;
			case Material::defaultMaterial:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.5f, 0.5f, 0.5f);
				glUniform1f(shader->getUniform("MatRough"), 0.0f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::blue_body:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.35f, 0.4f, 0.914f);
				glUniform1f(shader->getUniform("MatRough"), 0.8f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::red_body:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.914f, 0.35f, 0.4f);
				glUniform1f(shader->getUniform("MatRough"), 0.8f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::yellow_body:
				glUniform3f(shader->getUniform("MatAlbedo"), 0.914f, 0.914f, 0.35f);
				glUniform1f(shader->getUniform("MatRough"), 0.8f);
				glUniform1f(shader->getUniform("MatMetal"), 0.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
			case Material::gold:
				glUniform3f(shader->getUniform("MatAlbedo"), 1.0f, 0.766f, 0.336f);
				glUniform1f(shader->getUniform("MatRough"), 0.2f);
				glUniform1f(shader->getUniform("MatMetal"), 1.0f);
				glUniform3f(shader->getUniform("MatEmit"), 0.0f, 0.0f, 0.0f);
				break;
		}
	}

	/* helper for sending top of the matrix strack to GPU */
	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
	}

	/* helper function to set model trasnforms */
	void setModel(shared_ptr<Program> curS, vec3 trans, float rotY, float rotX, float sc) {
		mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
		mat4 RotX = glm::rotate(glm::mat4(1.0f), rotX, vec3(1, 0, 0));
		mat4 RotY = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
		mat4 ctm = Trans * RotX * RotY * ScaleS;
		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	void updateBoundingBox(const glm::vec3& localMin, const glm::vec3& localMax, const glm::mat4& transform, glm::vec3& outWorldMin, glm::vec3& outWorldMax) {
		// Initialize with extreme values
		outWorldMin = glm::vec3(std::numeric_limits<float>::max());
		outWorldMax = glm::vec3(-std::numeric_limits<float>::max());

		// Get the 8 corners of the bounding box
		glm::vec3 corners[8] = {
			{localMin.x, localMin.y, localMin.z},
			{localMax.x, localMin.y, localMin.z},
			{localMin.x, localMax.y, localMin.z},
			{localMax.x, localMax.y, localMin.z},
			{localMin.x, localMin.y, localMax.z},
			{localMax.x, localMin.y, localMax.z},
			{localMin.x, localMax.y, localMax.z},
			{localMax.x, localMax.y, localMax.z}
		};

		// Transform corners and update min/max
		for (int i = 0; i < 8; ++i) {
			glm::vec4 transformed = transform * glm::vec4(corners[i], 1.0f);
			outWorldMin = glm::min(outWorldMin, glm::vec3(transformed));
			outWorldMax = glm::max(outWorldMax, glm::vec3(transformed));
		}
	}

	void initGround() {
		// Check if already initialized
		if (GroundVertexArrayID != 0) {
			cout << "Warning: initGround() called more than once." << endl;
			return;
		}

		// Ground plane from -groundSize to +groundSize in X and Z at groundY
		float groundSize = Config::GROUND_SIZE;
		float groundY = Config::GROUND_HEIGHT;

		float GrndPos[] = {
			-groundSize, groundY, -groundSize, // top-left
			-groundSize, groundY,  groundSize, // bottom-left
			 groundSize, groundY,  groundSize, // bottom-right
			 groundSize, groundY, -groundSize  // top-right
		};

		// Normals point straight up
		float GrndNorm[] = {
			0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0
		};

		// Indices for two triangles covering the quad
		unsigned short idx[] = { 0, 1, 2,   0, 2, 3 };
		g_GiboLen = 6; // Number of indices

		// Generate VAO
		glGenVertexArrays(1, &GroundVertexArrayID);
		glBindVertexArray(GroundVertexArrayID);

		// Position buffer (Attribute 0)
		glGenBuffers(1, &GrndBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Normal buffer (Attribute 1)
		glGenBuffers(1, &GrndNorBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Index buffer
		glGenBuffers(1, &GIndxBuffObj);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

		// Unbind VAO and buffers (good practice)
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		cout << "Ground Initialized: VAO ID " << GroundVertexArrayID << endl;
	}

	// Draw the ground sections (library, boss area, path)
	void drawGroundSections(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model || GroundVertexArrayID == 0) { // Check if ground is initialized
			// cerr << "Error: Cannot draw ground sections - shader, model, or ground VAO invalid." << endl;
			return;
		}

		shader->bind(); // Bind the simple shader

		glBindVertexArray(GroundVertexArrayID); // Bind ground VAO

		// 1. Draw Library Ground
		// Model->pushMatrix();
		// Model->loadIdentity();
		// Model->translate(libraryCenter); // Center the ground plane
		// No scaling needed if initGround used groundSize correctly relative to its vertices
		// setModel(shader, Model);
		// SetMaterialMan(shader, 6); // brown material
		// glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
		// Model->popMatrix();

		// 2. Draw Boss Area Ground
		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(bossAreaCenter); // Position the boss ground plane
		setModel(shader, Model);

		SetMaterial(shader, Material::bronze); // Bronze material

		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
		Model->popMatrix();

		// 3. Draw Path (Scaled ground geometry)
		// Model->pushMatrix();
		// Model->loadIdentity();
		// // Calculate path dimensions and position
		// float pathLength = bossAreaCenter.z - libraryCenter.z - 2 * groundSize;
		// if (pathLength < 0) pathLength = 0; // Avoid negative length if areas overlap
		// float pathCenterZ = libraryCenter.z + groundSize + pathLength * 0.5f;
		// // Calculate scaling factors based on the original ground quad size (groundSize * 2)
		// float scaleX = pathWidth / (groundSize * 2.0f);
		// float scaleZ = pathLength / (groundSize * 2.0f);

		// Model->translate(vec3(libraryCenter.x, groundY, pathCenterZ)); // Center the path segment
		// Model->scale(vec3(scaleX, 1.0f, scaleZ)); // Scale ground quad to path dimensions
		// setModel(shader, Model);
		// SetMaterialMan(shader, 4); // Dark white material
		// glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
		// Model->popMatrix();

		// Unbind VAO after drawing all ground parts
		glBindVertexArray(0);

		shader->unbind(); // Unbind the simple shader
	}

	void initLibGrnd(float length, float width, float height, vec3 center_pos,
		GLuint& LibGrndVertexArrayID, GLuint& LibGrndBuffObj, GLuint& LibGrndNormBuffObj, GLuint& LibGrndIndxBuffObj, GLuint& LibGrndTexBuffObj, int& g_GiboLen) {
		// Define vertices for the library ground
		float LibGrndPos[] = {
			center_pos.x - length / 2, center_pos.y, center_pos.z - width / 2,
			center_pos.x - length / 2, center_pos.y, center_pos.z + width / 2,
			center_pos.x + length / 2, center_pos.y, center_pos.z + width / 2,
			center_pos.x + length / 2, center_pos.y, center_pos.z - width / 2
		};

		// Normals point straight up
		float LibGrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
		};

		float LibGrndTex[] = {
			0, 0,
			0, 1,
			1, 1,
			1, 0,
		};

		// Indices for two triangles covering the quad
		unsigned short idx[] = { 0, 1, 2, 0, 2, 3 };
		g_GiboLen = 6; // Number of indices

		// Generate VAO
		glGenVertexArrays(1, &LibGrndVertexArrayID);
		glBindVertexArray(LibGrndVertexArrayID);

		// Position buffer (Attribute 0)
		glGenBuffers(1, &LibGrndBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, LibGrndBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(LibGrndPos), LibGrndPos, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Normal buffer (Attribute 1)
		glGenBuffers(1, &LibGrndNormBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, LibGrndNormBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(LibGrndNorm), LibGrndNorm, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Texture buffer (Attribute 2)
		glGenBuffers(1, &LibGrndTexBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, LibGrndTexBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(LibGrndTex), LibGrndTex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Index buffer
		glGenBuffers(1, &LibGrndIndxBuffObj);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, LibGrndIndxBuffObj);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

		// Unbind VAO and buffers (good practice)
		glBindVertexArray(0);
	}

	void addLibGrnd(float length, float width, float height, vec3 center_pos, shared_ptr<Texture> tex) {
		LibGrndObjKey key{ center_pos, height };

		// Check if already initialized
		if (libraryGroundKeys.count(key)) {
			return;
		}

		LibGrndObject newLibGrnd;
		newLibGrnd.length = length;
		newLibGrnd.width = width;
		newLibGrnd.height = height;
		newLibGrnd.center_pos = center_pos;
		newLibGrnd.texture = tex;

		initLibGrnd(length, width, height, center_pos,
			newLibGrnd.VAO, newLibGrnd.BuffObj, newLibGrnd.NorBuffObj,
			newLibGrnd.IndxBuffObj, newLibGrnd.TexBuffObj, newLibGrnd.GiboLen);

		libraryGrounds.push_back(newLibGrnd);
		libraryGroundKeys.insert(key); // Add key to set to avoid duplicates
	}

	void drawLibGrnd(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model) {
			cerr << "Error: Null pointer in drawLibGrnd." << endl;
			return;
		}

		shader->bind(); // Bind the simple shader

		if (shader == ShadowProg && Config::DRAW_PAW_PRINTS) {
			int num = (int)prints.size(); // only draw as many paw prints as we have left (removed on timer)
			vec4 pawArr[Config::PRINTS_MAX];
			for (int i = 0; i < num; ++i) {
				pawArr[i].x = prints[i].pos.x;
				pawArr[i].y = prints[i].pos.y;
				pawArr[i].z = prints[i].angle;
				pawArr[i].w = prints[i].spawnTime;
			}

			glUniform1i(shader->getUniform("pawCount"), num);
			glUniform4fv(shader->getUniform("pawData"), num, value_ptr(pawArr[0]));
			glUniform1f(shader->getUniform("curTime"), (float)glfwGetTime());

			glActiveTexture(GL_TEXTURE0 + pawTex->getUnit());
			glBindTexture(GL_TEXTURE_2D, pawTex->getID());
			glUniform1i(shader->getUniform("pawTex"), pawTex->getUnit());
		}

		for (const auto& libGrnd : libraryGrounds) {
			glBindVertexArray(libGrnd.VAO); // Bind each library ground VAO

			if (shader == ShadowProg) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, libGrnd.texture->getID());
			}

			Model->pushMatrix();
			Model->loadIdentity();
			setModel(shader, Model);

			glUniformMatrix4fv(shader->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
			SetMaterial(shader, Material::wood);

			glDrawElements(GL_TRIANGLES, libGrnd.GiboLen, GL_UNSIGNED_SHORT, 0);
			Model->popMatrix();

			if (shader == ShadowProg) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, 0);
				libGrnd.texture->unbind(); // Unbind the texture after drawing each border
			}
		}

		if (shader->hasUniform("pawCount")) glUniform1i(shader->getUniform("pawCount"), 0);
		glBindVertexArray(0); // Unbind VAO after drawing all library grounds

		shader->unbind(); // Unbind the simple shader
	}

	void initWall(float length, vec3 pos, vec3 dir, float height,
		GLuint& WallVertexArrayID, GLuint& WallBuffObj, GLuint& WallNormBuffObj, GLuint& WIndxBuffObj, GLuint& WallTexBuffObj, int& w_GiboLen) {
		vec3 dirNorm = normalize(dir);

		// Define border vertices
		// positioned relative to the bottom-left corner of the border
		float WallPos[] = {
			pos.x, pos.y, pos.z, // bottom-left
			pos.x + dirNorm.x * length, pos.y, pos.z + dirNorm.z * length, // bottom-right
			pos.x + dirNorm.x * length, pos.y + height, pos.z + dirNorm.z * length, // top-right
			pos.x, pos.y + height, pos.z // top-left
		};

		// Normals face outward
		float WallNorm[] = {
			0, 0, 1,
			0, 0, 1,
			0, 0, 1,
			0, 0, 1
		};

		// float WallTex[] = {
		// 	0, 0,
		// 	1, 0,
		// 	1, 1,
		// 	0, 1
		// };

		// Repeating wall texture
		float texRepeatPerUnit = 0.2f;

		// Compute number of times to repeat the texture
		float texRepeatX = length * texRepeatPerUnit;
		float texRepeatY = height * texRepeatPerUnit;

		float WallTex[] = {
			0.0f,         0.0f,
			texRepeatX,   0.0f,
			texRepeatX,   texRepeatY,
			0.0f,         texRepeatY
		};

		unsigned short idx[] = { 0, 1, 2, 0, 2, 3 };
		w_GiboLen = 6; // Number of indices

		// Generate VAO
		glGenVertexArrays(1, &WallVertexArrayID);
		glBindVertexArray(WallVertexArrayID);

		// Position buffer (Attribute 0)
		glGenBuffers(1, &WallBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, WallBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(WallPos), WallPos, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Normal buffer (Attribute 1)
		glGenBuffers(1, &WallNormBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, WallNormBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(WallNorm), WallNorm, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Texture buffer (Attribute 2)
		glGenBuffers(1, &WallTexBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, WallTexBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(WallTex), WallTex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Index buffer
		glGenBuffers(1, &WIndxBuffObj);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WIndxBuffObj);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

		// Unbind VAO and buffers (good practice)
		glBindVertexArray(0);
	}

	void addWall(float length, vec3 pos, vec3 dir, float height, shared_ptr<Texture> tex) {
		WallObjKey posKey{ pos, dir, height };
		if (borderWallKeys.count(posKey)) {
			return;
		}

		WallObject newBorder;
		newBorder.length = length;
		newBorder.position = pos;
		newBorder.direction = dir;
		newBorder.height = height;
		newBorder.texture = tex;

		initWall(length, pos, dir, height,
			newBorder.WallVAID, newBorder.BuffObj, newBorder.NorBuffObj,
			newBorder.IndxBuffObj, newBorder.TexBuffObj, newBorder.GiboLen);

		borderWalls.push_back(newBorder);
		borderWallKeys.insert(posKey); // Add key to set to avoid duplicates
	}

	void initEnemies() {
		// if (enemies.size() == 0) {
		// 	std::vector<vec3> enemySpawnPositions = library->getEnemySpawnPositions();

		// 	for (auto e = enemies.begin(); e != enemies.end(); ++e) {
		// 		enemies.erase(e);
		// 	}

		// 	for (const auto& spawnPos : enemySpawnPositions) {
		// 		enemies.push_back(new IceElemental(vec3(spawnPos.x, Config::ICE_ELEMENTAL_TRANS_Y, spawnPos.z), ENEMY_HP_MAX, 2.0f, iceElemental, vec3(1.0f), vec3(0.0f)));
		// 		// cout << " Enemy placed at: (" << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")" << endl;
		// 	}
		// }
		std::vector<vec3> enemySpawnPositions = library->getEnemySpawnPositions();

			for (Enemy* enemy : enemies) {
				delete enemy; // Free heap memory
			}
			enemies.clear();

		for (const auto& spawnPos : enemySpawnPositions) {
			// enemies.push_back(new IceElemental(vec3(spawnPos.x, Config::ICE_ELEMENTAL_TRANS_Y, spawnPos.z), ENEMY_HP_MAX, 2.0f, iceElemental, vec3(1.0f), vec3(0.0f)));
			keysneededToCollect++; // Increment the key count for each enemy spawned

			// cout << " Enemy placed at: (" << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ")" << endl;

				// randomize enemy type, but make sure there is at least one of each type
				static bool guaranteed[3] = {false, false, false};
				static int guaranteedCount = 0;

				int enemyType;
				if (guaranteedCount < 3) {
					// Assign each type once
					for (int t = 0; t < 3; ++t) {
						if (!guaranteed[t]) {
							enemyType = t;
							guaranteed[t] = true;
							guaranteedCount++;
							break;
						}
					}
				} else {
					enemyType = rand() % 3;
				}

				if (enemyType == 0) {
					enemies.push_back(new IceElemental(vec3(spawnPos.x, Config::ICE_ELEMENTAL_TRANS_Y, spawnPos.z), Config::ICE_ELEMENTAL_HP_MAX, Config::ICE_ELEMENTAL_MOVE_SPEED, iceElemental, vec3(1.0f, 1.0f, 1.0f), vec3(glm::radians(90.0f), 0.0f, 0.0f)));
				}
				else if (enemyType == 1) {
					enemies.push_back(new FireElemental(vec3(spawnPos.x, Config::FIRE_ELEMENTAL_TRANS_Y, spawnPos.z), Config::FIRE_ELEMENTAL_HP_MAX, Config::FIRE_ELEMENTAL_MOVE_SPEED, fireElemental, vec3(0.01f, 0.01f, 0.01f), vec3(glm::radians(90.0f), 0.0f, 0.0f)));
				}
				else if (enemyType == 2) {
					enemies.push_back(new LightningElemental(vec3(spawnPos.x, Config::LIGHTNING_ELEMENTAL_TRANS_Y, spawnPos.z), Config::LIGHTNING_ELEMENTAL_HP_MAX, Config::LIGHTNING_ELEMENTAL_MOVE_SPEED, lightningElemental, vec3(0.01f, 0.01f, 0.01f), vec3(glm::radians(90.0f), 0.0f, glm::radians(90.0f))));
				}
		}
	}

	void drawBorderWalls(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model) {
			cerr << "Error: Null pointer in drawBorderWalls." << endl;
			return;
		}

		shader->bind(); // Bind the simple shader

		for (const auto& border : borderWalls) {
			glBindVertexArray(border.WallVAID); // Bind each border VAO

			if (shader == ShadowProg) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, border.texture->getID());
			}

			Model->pushMatrix();
			Model->loadIdentity();
			setModel(shader, Model);
			// SetMaterialMan(shader, 3); // Black material
			glDrawElements(GL_TRIANGLES, border.GiboLen, GL_UNSIGNED_SHORT, 0);
			Model->popMatrix();

			if (shader == ShadowProg) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, 0);
				border.texture->unbind(); // Unbind the texture after drawing each border
			}
		}

		glBindVertexArray(0); // Unbind VAO after drawing all borders

		shader->unbind(); // Unbind the simple shader
	}

	void drawPlayer(shared_ptr<Program> curS, shared_ptr<MatrixStack> Model, float animTime) {
		if (!curS || !Model || !player_rig || !catwizard_animator || !player_walk || !player_idle) {
			cerr << "Error: Null pointer in drawPlayer." << endl;
			return;
		}
		curS->bind();

		if (movingBackward || movingForward || movingLeft || movingRight) {
			manState = Man_State::WALKING;
		}
		else {
			manState = Man_State::IDLE;
		}

		if (animTime != 0.0) {
			catwizard_animator->UpdateAnimation(1.5f * animTime);
		}

		// Animation update
		if (manState == Man_State::WALKING) {
			catwizard_animator->SetCurrentAnimation(player_walk);
		}
		else if (manState == Man_State::IDLE){
			catwizard_animator->SetCurrentAnimation(player_idle);
		}

		// Update bone matrices

		vector<glm::mat4> transforms = catwizard_animator->GetFinalBoneMatrices();


		if (curS->hasUniform("finalBonesMatrices[0]")) {
			int numBones = std::min((int)transforms.size(), Config::MAX_BONES);
			for (int i = 0; i < numBones; ++i) {
				string uniformName = "finalBonesMatrices[" + std::to_string(i) + "]";
				glUniformMatrix4fv(curS->getUniform(uniformName), 1, GL_FALSE, value_ptr(transforms[i]));
			}
		}

		// Model matrix setup
		Model->pushMatrix();
		Model->loadIdentity();
		// Model->translate(characterMovement); // Use final player position
		Model->translate(player->getPosition());
		// *** USE CAMERA ROTATION FOR MODEL ***


		Model->rotate((player->getRotY()), vec3(0, 1, 0)); // <<-- FIXED ROTATION

		Model->scale(0.01f);

		// Update VISUAL bounding box (can be different from collision box if needed)
		// Using the same AABB calculation logic as before for consistency
		glm::mat4 manTransform = Model->topMatrix();
		updateBoundingBox(player_rig->getBoundingBoxMin(),
			player_rig->getBoundingBoxMax(),
			manTransform,
			playerBB->min,
			playerBB->max);

		// Set uniforms and draw
		if (curS->hasUniform("texOnly")) glUniform1i(curS->getUniform("texOnly"), GL_TRUE);
		if (curS->hasUniform("hasBones")) glUniform1i(curS->getUniform("hasBones"), GL_TRUE);
		setModel(curS, Model);
		if (curS->hasUniform("texOnly")) glUniform1i(curS->getUniform("texOnly"), GL_FALSE);
		player_rig->Draw(curS);
		if (curS->hasUniform("hasBones")) glUniform1i(curS->getUniform("hasBones"), GL_FALSE);
		curS->unbind();
		Model->popMatrix();
	}

	void drawBooks(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		shader->bind();
		if (shader->hasUniform("texOnly")) glUniform1i(shader->getUniform("texOnly"), GL_TRUE);
		for (const auto& book : books) {
			// Common values for book halves
			float bookThickness = book.scale.z * 0.15f;
			vec3 coverScale = vec3(book.scale.x * 0.3, book.scale.y * 0.35, bookThickness);
			vec3 pageScale = vec3(book.scale.x * 0.28f, book.scale.y * 0.33f, 0.01f);

			// BACK COVER (flat)
			Model->pushMatrix(); {
				Model->translate(book.position);
				Model->multMatrix(mat4_cast(book.orientation));
				Model->rotate(-Config::HALF_PI, vec3(1, 0, 0));
				Model->translate(vec3(0, 0, -bookThickness * 0.5f));
				Model->scale(coverScale);
				setModel(shader, Model);
				bookCover->Draw(shader);
			} Model->popMatrix();

			// BACK PAGE(S) (flat)
			Model->pushMatrix(); {
				Model->translate(book.position);
				Model->multMatrix(mat4_cast(book.orientation));
				Model->rotate(-Config::HALF_PI, vec3(1, 0, 0));
				Model->translate(vec3(0, 0, +(bookThickness * 0.25f)));
				Model->scale(vec3(pageScale.x, pageScale.y, pageScale.z * 2.0f)); // offset pages so they are visible
				setModel(shader, Model);
				bookPaper->Draw(shader);
			} Model->popMatrix();

			// PAGE 1 (delayed hinge opening)
			Model->pushMatrix(); {
				Model->translate(book.position);
				Model->multMatrix(glm::mat4_cast(book.orientation));
				Model->rotate(-Config::HALF_PI, vec3(1, 0, 0));
				if (book.state == BookState::OPENING || book.state == BookState::OPENED) { // hinge around the �spine� (world-Y after flatten)
					constexpr float startDelay = radians(20.0f); // no page turn until cover > 20 degrees
					float coverA = book.openAngle;
					float pageAngle;
					if (coverA <= startDelay) {
						pageAngle = 0.0f;
					}
					else {
						float t = (coverA - startDelay) / (book.maxOpenAngle - startDelay);
						pageAngle = t * t * book.maxOpenAngle;
					}
					Model->rotate(pageAngle, vec3(0, 1, 0));
				}
				Model->translate(vec3(0, 0, -(bookThickness * 0.25f))); // offset pages so they are visible
				Model->scale(pageScale);
				setModel(shader, Model);
				bookPaper->Draw(shader);
			} Model->popMatrix();

			// PAGE 2 (extra delayed hinge opening)
			Model->pushMatrix(); {
				Model->translate(book.position);
				Model->multMatrix(glm::mat4_cast(book.orientation));
				Model->rotate(-Config::HALF_PI, vec3(1, 0, 0));
				if (book.state == BookState::OPENING || book.state == BookState::OPENED) { // hinge around the �spine� (world-Y after flatten)
					constexpr float startDelay = radians(40.0f); // no page turn until cover > 40 degrees
					float coverA = book.openAngle;
					float pageAngle;
					if (coverA <= startDelay) {
						pageAngle = 0.0f;
					}
					else {
						float t = (coverA - startDelay) / (book.maxOpenAngle - startDelay);
						pageAngle = t * t * book.maxOpenAngle;
					}
					Model->rotate(pageAngle, vec3(0, 1, 0));
				}
				Model->translate(vec3(0, 0, -(bookThickness * 0.25f))); // offset pages so they are visible
				Model->scale(pageScale);
				setModel(shader, Model);
				bookPaper->Draw(shader);
			} Model->popMatrix();

			// FRONT COVER (hinges outwards)
			Model->pushMatrix(); {
				Model->translate(book.position);
				Model->multMatrix(glm::mat4_cast(book.orientation));
				Model->rotate(-Config::HALF_PI, glm::vec3(1, 0, 0));
				if (book.state == BookState::OPENING || book.state == BookState::OPENED) { // hinge around the spine (world-Y after flatten)
					Model->rotate(book.openAngle, glm::vec3(0, 1, 0));
				}
				Model->translate(glm::vec3(0, 0, +bookThickness * 0.5f));
				Model->scale(coverScale);
				setModel(shader, Model);
				bookCover->Draw(shader);
			} Model->popMatrix();
		} // END draw books loop
		if (shader->hasUniform("texOnly")) glUniform1i(shader->getUniform("texOnly"), GL_FALSE);
		shader->unbind();
	}

	void drawSkybox(shared_ptr<Program> shader, const shared_ptr<MatrixStack>& Projection, const shared_ptr<MatrixStack>& View) {
		// disable depth writes and set test
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);

		shader->bind();
		glUniformMatrix4fv(shader->getUniform("P"),
			1, GL_FALSE,
			glm::value_ptr(Projection->topMatrix()));

		glm::mat4 view = View->topMatrix();
		view[3] = glm::vec4(0, 0, 0, 1);
		glUniformMatrix4fv(shader->getUniform("V"),
			1, GL_FALSE,
			glm::value_ptr(view));

		// bind cubemap
		glActiveTexture(GL_TEXTURE0 + 12);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
		glUniform1i(shader->getUniform("skyTex"), 12);

		// draw cube VAO
		glBindVertexArray(skyboxCubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		shader->unbind();

		// restore depth state
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
	}


	//TODO: Add particle effects to orbs
	void drawOrbs(shared_ptr<Program> simpleShader, shared_ptr<MatrixStack> Model) {
		// --- Collision Check Logic ---
		for (auto& orb : orbCollectibles) {
			// Perform collision check ONLY if not collected AND in the IDLE state
			if (!orb.collected && orb.state == OrbState::IDLE && // <<<--- ADD STATE CHECK
				checkAABBCollision(playerBB->min, playerBB->max, orb.AABBmin, orb.AABBmax)) {
				orb.collected = true;
				// orb.state = OrbState::COLLECTED; // Optionally set state

				// for (int i = 0; i < 4; i++) {
				// 	if (orb.spellType == spellSlots[i]) {
				// 		currentSpellSlotIndex = i;
				// 		break;
				// 	}
				// }

				// currentPlayerSpellType = spellSlots[currentSpellSlotIndex]; // Equip the collected spell type
				// orbsCollectedCount++; // This might now just mean "spell charges" or be repurposed
				spellCounts[orb.spellType]++; // Increment the count for the specific spell type
				orbsCollectedCount++; // Increment the total orbs collected count

				// Debug output for spell type equipped
				std::string spellTypeName = "NONE";
				if (currentPlayerSpellType == SpellType::FIRE) spellTypeName = "FIRE";
				else if (currentPlayerSpellType == SpellType::ICE) spellTypeName = "ICE";
				else if (currentPlayerSpellType == SpellType::LIGHTNING) spellTypeName = "LIGHTNING";
				else if (currentPlayerSpellType == SpellType::HEAL) spellTypeName = "HEAL";

				std::cout << "Collected a Spell Orb! Equipped: " << spellTypeName << " Spell. Orbs available: " << orbsCollectedCount << std::endl;
			}
		}

		// --- Drawing Logic ---
		simpleShader->bind();

		int collectedOrbDrawIndex = 0;
		std::map<SpellType, float> upOffsets = {
				{ SpellType::FIRE, 0.0f },
				{ SpellType::ICE, 0.0f },
				{ SpellType::LIGHTNING, 0.0f },
				{ SpellType::HEAL, 0.0f } // No scale offset for HEAL
		};
		for (auto& orb : orbCollectibles) {
            // Particle emission for uncollected, idle orbs
            if (!orb.collected && orb.state == OrbState::IDLE && particleSystem) {
                float current_particle_system_time = particleSystem->getCurrentTime();

                float p_speed_min = 0.05f;
                float p_speed_max = 0.1f;
                float p_spread = 1.5f;
                // lifespans  short so they die quickly and are recycled for other effects
                float p_lifespan_min = 0.6f;
                float p_lifespan_max = 1.2f;

                // Base particle color (TODO: can be tweaked, maybe slightly transparent)
				vec3 base = materialToColor(orb.color);
                vec4 p_color_start = vec4(base, 0.7f);
                vec4 p_color_end = vec4(base, 0.2f);
                float p_scale_min = 0.1f;
                float p_scale_max = 0.25f;

                int current_particles_to_spawn = 15; // Set a fixed number of particles for all orbs
                // Customize particle aura based on spell type
                switch (orb.spellType) {
                    case SpellType::FIRE:
                        // current_particles_to_spawn = 15; // Increased for density with short life
                        p_color_start = glm::vec4(1.0f, 0.5f, 0.1f, 0.8f);
                        p_color_end = glm::vec4(0.9f, 0.2f, 0.0f, 0.3f);
                        p_scale_min = 0.25f;
                        p_scale_max = 0.45f;
                        break;
                    case SpellType::ICE:
                        // current_particles_to_spawn = 15; // Increased for density
                        p_color_start = glm::vec4(0.5f, 0.8f, 1.0f, 0.8f);
                        p_color_end = glm::vec4(0.2f, 0.5f, 0.8f, 0.3f);
                        p_scale_min = 0.25f;
                        p_scale_max = 0.45f;
                        break;
                    case SpellType::LIGHTNING:
                        // current_particles_to_spawn = 15; // Increased for density
                        p_color_start = glm::vec4(1.0f, 1.0f, 0.5f, 0.8f);
                        p_color_end = glm::vec4(0.8f, 0.8f, 0.2f, 0.3f);
                        p_scale_min = 0.25f;
                        p_scale_max = 0.45f;
                        break;
                    default:
                        // current_particles_to_spawn is 15 (standardized)
                        // p_color_start and p_color_end use orb.color
                        // p_lifespan_min/max are standardized
                        // Make scales consistent with other types:
                        p_scale_min = 0.25f;
                        p_scale_max = 0.45f;
                        break;
                }

                particleSystem->spawnParticleBurst(orb.position, // Emit from orb center
                                                 glm::vec3(0,1,0), // Emit upwards slowly or randomly
                                                 current_particles_to_spawn,
                                                 current_particle_system_time,
                                                 p_speed_min, p_speed_max,
                                                 p_spread,
                                                 p_lifespan_min, p_lifespan_max,
                                                 p_color_start, p_color_end,
                                                 p_scale_min, p_scale_max);
            }

			glm::vec3 currentDrawPosition;
			float currentDrawScale = orb.scale; // Use base scale

			float fireSideOffset = 0.15f;
			float iceSideOffset = 0.30f;
			float lightningSideOffset = 0.0f;
			float healSideOffset = 0.4f;

			static std::map<SpellType, float> sideOffsets = {
				{ SpellType::FIRE, fireSideOffset },
				{ SpellType::ICE, iceSideOffset },
				{ SpellType::LIGHTNING, lightningSideOffset },
				{ SpellType::HEAL, healSideOffset} // No side offset for HEAL
			};

			if (orb.collected && spellCounts[orb.spellType] > 0) {
				// Calculate position behind the player (same logic as before)
				float backOffset = 0.4f;
				float upOffsetBase = 0.6f;
				float stackOffset = orb.scale * 2.5f;
				float sideOffset = sideOffsets[orb.spellType];
				glm::vec3 playerForward = normalize(manMoveDir);
				glm::vec3 playerUp = glm::vec3(0.0f, 1.0f, 0.0f);
				glm::vec3 playerRight = normalize(cross(playerForward, playerUp));
				// float currentUpOffset = upOffsetBase + (collectedOrbDrawIndex * stackOffset);
				float currentUpOffset = upOffsetBase + upOffsets[orb.spellType];
				upOffsets[orb.spellType] += stackOffset; // Increment up offset for next orb of the same type
				// float currentSideOffset = (collectedOrbDrawIndex % 2 == 0 ? -sideOffset : sideOffset);

				currentDrawPosition = charMove() - playerForward * backOffset
					+ playerUp * currentUpOffset
					+ playerRight * sideOffset;
				collectedOrbDrawIndex++;

			}
			else {
				currentDrawPosition = orb.position;
			}

			// --- Set up transformations ---
			Model->pushMatrix(); {
				Model->loadIdentity();
				Model->translate(currentDrawPosition);
				Model->scale(currentDrawScale); // Use current scale
				SetMaterial(simpleShader, orb.color);
				setModel(simpleShader, Model);
				orb.model->Draw(simpleShader);
			} Model->popMatrix();
		} // End drawing loop
		simpleShader->unbind();
	}

	void drawCat(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!CatWizard) return; //Need Cat Model
		shader->bind();
		Model->pushMatrix(); {
			Model->loadIdentity();
			Model->translate(vec3(0.0f, 0.0f, 0.0f)); // Position at origin
			//Model->scale(vec3(0.25f));
			Model->rotate(glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
			setModel(shader, Model);
			if (shader == ShadowProg) glUniform1i(shader->getUniform("texOnly"), GL_TRUE);
			CatWizard->Draw(shader);
			if (shader == ShadowProg) glUniform1i(shader->getUniform("texOnly"), GL_FALSE);
		} Model->popMatrix();
		shader->unbind();
	}

	void checkAllEnemies() {
		// if (canFightboss) return; // Already set to true
		// allEnemiesDead = true; // Assume all are dead unless we find one alive
		activeEnemiesCount = 0; // Reset active enemies count
		for (const auto* enemy : enemies) {
			if (enemy && enemy->isAlive()) {
				// allEnemiesDead = false; // Found at least one alive enemy
				// break;
				activeEnemiesCount++;
			}
		}
		// if (allEnemiesDead) {
		// 	canFightboss = true; // All enemies are dead, boss can be fought
		// }
	}

	void checkBossfight() {
		if (canFightboss && !bossfightended && bossEnemy->isAlive()) {
			int i = bossRoom->mapXtoGridX(player->getPosition().x);
			int j = bossRoom->mapZtoGridY(player->getPosition().z);
			if (bossGrid.inBounds(glm::ivec2(i, j))) {

				if (bossRoom->isInsideBossArea(glm::ivec2(i, j))) {
					bossfightstarted = true; // Player is in the boss area
				}
			}
		}
		else if (canFightboss && bossfightstarted && !bossEnemy->isAlive()) {
			bossfightstarted = false; // Player is no longer in the boss area
			bossfightended = true; // Boss fight ended
			bossActiveSpells.clear(); // Clear active spells
			canFightboss = false; // Reset boss fight flag
		}
	}

	void restartGeneration() {
		if (restartGen) {
			restartGen = false; // Reset flag after reinitialization
			canFightboss = false; // Reset boss fight flag
			allEnemiesDead = false; // Reset enemy status
			player->setPosition(vec3(0.0f, 0.0f, 0.0f)); // Reset player position
			books.clear();
			libraryGrounds.clear();
			libraryGroundKeys.clear();
			borderWalls.clear();
			borderWallKeys.clear();
			orbCollectibles.clear();
			keyCollectibles.clear();
			if (!player->isAlive()) {
				player->resetHitpoints();
				player->setAlive(); // Reset player status to alive if had died
				canFightboss = false; // Flag to check if the player can fight the boss
				allEnemiesDead = false; // Flag to check if all enemies are dead
				restartGen = false;
				bossfightstarted = false;
				bossfightended = false;
			}
			bossEnemy->setAlive(); // Reset boss status to alive
			initMapGen();
			libraryQuadTree->cleanup(); // Clean up the quad tree
			bossRoomQuadTree->cleanup(); // Clean up the boss room quad tree
			initQuadTree(); // Reinitialize the quad tree
			initEnemies(); // Reinitialize enemies
			bossActiveSpells.clear();
			bossEnemy->resetPhase();
			// enemies.push_back(new Enemy(libraryCenter + vec3(-5.0f, 0.8f, 8.0f), 50.0f, 2.0f, sphere, glm::vec3(0.5f, 1.28f, 0.5f), vec3(0.0f))); // <<-- Pass sphere and scale
			activeSpells.clear(); // Clear active spells
			unlock = false;
			keyCollectibles.clear(); // Clear key collectibles
			#if USE_INSTANCING
			initInstancingMatrices();
			#endif
			initCircularBorder();
		}
	}

	void drawEnemies(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		for (auto* enemy : enemies) {

			if (!enemy || !enemy->isAlive()) {
				// Ensure a key is added only once per dead enemy if not already present
                // This simple check assumes positions are unique enough for dead enemies.
                // A more robust way would be to tag enemies that have already dropped a key.



                // bool keyAlreadyExists = false; //some local bools and need some global bools
				// bool enemyLastPos = false;
                // for (const auto& key : keyCollectibles) {
                //     // Approximate check, ideally use a unique ID from the enemy
                //     if (glm::distance(key.position, enemy->getPosition()) < 0.1f) {
                //         keyAlreadyExists = true;
                //         break;
                //     }
                // }
				// 	glm:: vec3 keyPos = enemy->getPosition();

                // if (!keyAlreadyExists ) { //&& !enemyLastPos

				// 	//get enemy pos once, then don't change it until change pick it up?
				// 	keyPos.y = keyPos.y - 1.5f;
				// 	//std::cout << "key position " << keyPos.x << " " << keyPos.y << " " << keyPos.z << " " << std::endl;

                //     keyCollectibles.emplace_back(key, keyPos, 0.1f, Material::key_color, SpellType::NONE);
				// 	drawKey(shader, Model );
				// 	//enemyLastPos = true;
                // }

				if (!enemy->isDropSpawned()) {
					glm::vec3 keyPos = enemy->getPosition();
					keyPos.y -= 1.5f; // Adjust height for key position
					keyCollectibles.emplace_back(key, keyPos, 0.1f, Material::gold, SpellType::NONE);
					enemy->setDropSpawned(true); // Mark that the key has been spawned
				}

				continue; // Skip null or dead enemies
			}

			// Get enemy material based on type
			Material enemyMaterial = Material::black;
			glm::vec3 enemyScale = vec3(1.0f, 1.0f, 1.0f); // Default scale
			if (dynamic_cast<const IceElemental*>(enemy)) {
				// cout << "Ice Elemental" << endl;
				enemyMaterial = Material::blue_body; // Ice Elemental
				enemyScale = vec3(1.0f, 1.0f, 1.0f);
			} else if (dynamic_cast<const FireElemental*>(enemy)) {
				// cout << "Fire Elemental" << endl;
				enemyMaterial = Material::red_body; // Fire Elemental
				enemyScale = vec3(0.01f);
			} else if (dynamic_cast<const LightningElemental*>(enemy)) {
				// cout << "Lightning Elemental" << endl;
				enemyMaterial = Material::yellow_body; // Lightning Elemental
				enemyScale = vec3(0.005f);
			}

			shader->bind();
			Model->pushMatrix(); {
				Model->translate(enemy->getPosition());
				Model->scale(enemyScale); // Scale the enemy model
				Model->rotate(enemy->getRotY(), glm::vec3(0, 1, 0));

				if (dynamic_cast<const IceElemental*>(enemy)) {
					Model->rotate(glm::radians(-90.0f), glm::vec3(1, 0, 0)); // Rotate Ice Elemental
				}
				SetMaterial(shader, enemyMaterial); // Set body material
				if (shader->hasUniform("enemyAlpha")) glUniform1f(shader->getUniform("enemyAlpha"), enemy->getDamageTimer() / Config::ENEMY_HIT_DURATION);
				setModel(shader, Model);
				(enemy->getModel())->Draw(shader); // Draw the scaled sphere as the body
			} Model->popMatrix();
			shader->unbind();
		} // End loop through enemies
	}

	void drawLibInstancing(shared_ptr<Program> shader, bool cullFlag) {
		vbook_shelf1Matrices.clear(); // Clear matrices for the next draw call
		vbook_shelf2Matrices.clear();
		vbookstandMatrices.clear();
		vtable_chairs2Matrices.clear();
		vtable_chairs1Matrices.clear();
		vchestMatrices.clear();
		vcandelabraMatrices.clear();
		vclockMatrices.clear();

		if (!shader || !book_shelf1 || grid.getSize().x == 0 || grid.getSize().y == 0) return; // Safety checks
		shader->bind();
		if (shader->hasUniform("hasInstancing")) glUniform1i(shader->getUniform("hasInstancing"), GL_TRUE);

		for (unsigned int i = 0; i < book_shelf1Matrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(book_shelf1Matrices[i][3][0],
				book_shelf1Matrices[i][3][1],
				book_shelf1Matrices[i][3][2]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vbook_shelf1Matrices.push_back(book_shelf1Matrices[i]);
			}
		}

		for (unsigned int i = 0; i < book_shelf2Matrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(book_shelf2Matrices[i][3]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vbook_shelf2Matrices.push_back(book_shelf2Matrices[i]);
			}
		}

		for (unsigned int i = 0; i < bookstandMatrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(bookstandMatrices[i][3]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vbookstandMatrices.push_back(bookstandMatrices[i]);
			}
		}

		for (unsigned int i = 0; i < table_chairs1Matrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(table_chairs1Matrices[i][3]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vtable_chairs1Matrices.push_back(table_chairs1Matrices[i]);
			}
		}

		for (unsigned int i = 0; i < table_chairs2Matrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(table_chairs2Matrices[i][3]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vtable_chairs2Matrices.push_back(table_chairs2Matrices[i]);
			}
		}

		for (unsigned int i = 0; i < chestMatrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(chestMatrices[i][3]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vchestMatrices.push_back(chestMatrices[i]);
			}
		}

		for (unsigned int i = 0; i < candelabraMatrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(candelabraMatrices[i][3]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vcandelabraMatrices.push_back(candelabraMatrices[i]);
			}
		}

		for (unsigned int i = 0; i < clockMatrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(clockMatrices[i][3]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vclockMatrices.push_back(clockMatrices[i]);
			}
		}


		book_shelf1->updateInstancingOffsetVBO(vbook_shelf1Matrices);
		book_shelf2->updateInstancingOffsetVBO(vbook_shelf2Matrices);
		bookstand->updateInstancingOffsetVBO(vbookstandMatrices);
		table_chairs2->updateInstancingOffsetVBO(vtable_chairs2Matrices);
		table_chairs1->updateInstancingOffsetVBO(vtable_chairs1Matrices);
		chest->updateInstancingOffsetVBO(vchestMatrices);
		candelabra->updateInstancingOffsetVBO(vcandelabraMatrices);
		grandfather_clock->updateInstancingOffsetVBO(vclockMatrices);

		book_shelf1->DrawInstanced(vbook_shelf1Matrices);
		book_shelf2->DrawInstanced(vbook_shelf2Matrices);
		bookstand->DrawInstanced(vbookstandMatrices);
		table_chairs2->DrawInstanced(vtable_chairs2Matrices);
		table_chairs1->DrawInstanced(vtable_chairs1Matrices);
		chest->DrawInstanced(vchestMatrices);
		candelabra->DrawInstanced(vcandelabraMatrices);
		grandfather_clock->DrawInstanced(vclockMatrices);

		// for (unsigned int i = 0; i < book_shelf1->meshes.size(); ++i) {
		// 	glBindVertexArray(book_shelf1->meshes[i].VAO);
		// 	glDrawElementsInstanced(GL_TRIANGLES,
		// 		static_cast<unsigned int>(book_shelf1->meshes[i].indices.size()),
		// 		GL_UNSIGNED_INT,
		// 		0,
		// 		vbook_shelf1Matrices.size());
		// 	glBindVertexArray(0);
		// }

		if (shader->hasUniform("hasInstancing")) glUniform1i(shader->getUniform("hasInstancing"), GL_FALSE);

		// Entrance door logic
		if (!unlock) {
			glm::mat4 doorentranceMatrix = doorMatrices[0];
			glUniformMatrix4fv(shader->getUniform("M"), 1, GL_FALSE, value_ptr(doorentranceMatrix));
			door->Draw(shader);
		}

		glm::mat4 doorexitMatrix = doorMatrices[1];
		glUniformMatrix4fv(shader->getUniform("M"), 1, GL_FALSE, value_ptr(doorexitMatrix));
		door->Draw(shader); // Draw the exit door
		shader->unbind();
	}

	void drawLibrary(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model, bool cullFlag) {
		if (!shader || !Model || !book_shelf1 || grid.getSize().x == 0 || grid.getSize().y == 0) return; // Safety checks

		shader->bind();
		//if (shader == ShadowProg) {
		//	glUniform1i(shader->getUniform("hasMaterial"), 0); // Bookshelves should use texture
		//}

		float groundSize = Config::GROUND_SIZE;

		float gridWorldWidth = groundSize * 2.0f; // The world space the grid should occupy (library floor width)
		float gridWorldDepth = groundSize * 2.0f; // The world space the grid should occupy (library floor depth)
		float cellWidth = gridWorldWidth / (float)grid.getSize().x;
		float cellDepth = gridWorldDepth / (float)grid.getSize().y;
		float shelfScaleFactor = 1.8f; // Adjust scale of the bookshelf model itself

		for (int z = 0; z < grid.getSize().y; ++z) {
			for (int x = 0; x < grid.getSize().x; ++x) {
				glm::ivec2 gridPos(x, z);
				float i = library->mapGridXtoWorldX(x); // Center the shelf in the cell
				float j = library->mapGridYtoWorldZ(z); // Center the shelf in the cell
				if (!cullFlag || !ViewFrustCull(glm::vec3(i, 0, j), 2.0f, planes)) {
					if (grid[gridPos].type == LibraryGen::CellType::CLUSTER) {
						if (grid[gridPos].clusterType == LibraryGen::ClusterType::SHELF1) {
							Model->pushMatrix();
							Model->loadIdentity();
							// Model->translate(vec3(worldX, libraryCenter.y, worldZ)); // Position shelf at cell center on ground
							Model->translate(vec3(i, libraryCenter.y, j)); // Position wall at cell center on ground
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							book_shelf1->Draw(shader);
							Model->popMatrix();
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::SHELF2) {
							// Calculate world position based on grid cell, centering the grid on libraryCenter

							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, libraryCenter.y, j)); // Position wall at cell center on ground
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							book_shelf1->Draw(shader);
							Model->popMatrix();
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::SHELF3) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, libraryCenter.y, j)); // Position wall at cell center on ground
							Model->rotate(glm::radians(90.0f), vec3(0, 1, 0)); // Rotate for left/right walls
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							book_shelf1->Draw(shader);
							Model->popMatrix();
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::ONLY_CANDELABRA) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, libraryCenter.y, j)); // Position wall at cell center on ground
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							candelabra->Draw(shader);
							Model->popMatrix();
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::ONLY_CHEST) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, libraryCenter.y, j)); // Position wall at cell center on ground
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							chest->Draw(shader);
							Model->popMatrix();
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::ONLY_TABLE) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, libraryCenter.y, j)); // Position wall at cell center on ground
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							table_chairs1->Draw(shader);
							Model->popMatrix();

							addLibGrnd(5.0f, 5.0f, 1.0f, vec3(i, libraryCenter.y + 0.1f, j), carpetTex);
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::ONLY_CLOCK) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, libraryCenter.y, j)); // Position wall at cell center on ground
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							grandfather_clock->Draw(shader);
							Model->popMatrix();
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::LAYOUT1) {
							if (grid[gridPos].objectType == LibraryGen::CellObjType::BOOKSHELF) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								book_shelf1->Draw(shader);
								Model->popMatrix();
							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::ROTATED_BOOKSHELF) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->rotate(glm::radians(90.0f), vec3(0, 1, 0)); // Rotate for left/right walls
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								book_shelf1->Draw(shader);
								Model->popMatrix();
							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::TABLE_AND_CHAIR2) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								table_chairs1->Draw(shader);
								Model->popMatrix();

								addLibGrnd(5.0f, 5.0f, 1.0f, vec3(i, libraryCenter.y + 0.1f, j), carpetTex);

							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::TABLE_AND_CHAIR1) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								table_chairs1->Draw(shader);
								Model->popMatrix();

								addLibGrnd(5.0f, 5.0f, 1.0f, vec3(i, libraryCenter.y + 0.1f, j), carpetTex);
							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::CANDELABRA) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								candelabra->Draw(shader);
								Model->popMatrix();
							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::GRANDFATHER_CLOCK) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								grandfather_clock->Draw(shader);
								Model->popMatrix();
							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::CHEST) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								chest->Draw(shader);
								Model->popMatrix();
							}
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::ONLY_BOOKSTAND) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
							Model->scale(grid[gridPos].transformData.scale);
							setModel(shader, Model);
							bookstand->Draw(shader);
							Model->popMatrix();
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::GLOWING_SHELF1) {
							if (grid[gridPos].objectType == LibraryGen::CellObjType::SHELF_WITH_ABILITY) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								book_shelf2->Draw(shader);
								Model->popMatrix();
							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::BOOKSHELF) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								book_shelf1->Draw(shader);
								Model->popMatrix();
							}
						}
						else if (grid[gridPos].clusterType == LibraryGen::ClusterType::GLOWING_SHELF2) {
							if (grid[gridPos].objectType == LibraryGen::CellObjType::SHELF_WITH_ABILITY_ROTATED) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->rotate(glm::radians(90.0f), vec3(0, 1, 0)); // Rotate for left/right walls
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								book_shelf2->Draw(shader);
								Model->popMatrix();
							}
							else if (grid[gridPos].objectType == LibraryGen::CellObjType::ROTATED_BOOKSHELF) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->rotate(glm::radians(90.0f), vec3(0, 1, 0)); // Rotate for left/right walls
								Model->scale(grid[gridPos].transformData.scale);
								setModel(shader, Model);
								book_shelf1->Draw(shader);
								Model->popMatrix();
							}
						}
					}
				}
			}
		}
		shader->unbind();
	}

	void drawBossRoom(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model, bool cullFlag) {
		if (!shader || !Model) return;
		shader->bind();

		/*if (shader == ShadowProg) {
			glUniform1i(shader->getUniform("hasMaterial"), 0);
		}*/

		for (int z = 0; z < bossGrid.getSize().y; ++z) {
			for (int x = 0; x < bossGrid.getSize().x; ++x) {
				glm::ivec2 gridPos(x, z);
				float i = bossRoom->mapGridXtoWorldX(x); // Center the shelf in the cell
				float j = bossRoom->mapGridYtoWorldZ(z); // Center the shelf in the cell
				if (!cullFlag || !ViewFrustCull(glm::vec3(i, 0, j), 2.0f, planes)) {
					if (bossGrid[gridPos].type == BossRoomGen::CellType::BORDER) {
						// int test = bossRoom->mapXtoGridX(i);
						// int test2 = bossRoom->mapZtoGridY(j);
						// Model->pushMatrix();
						// Model->loadIdentity();
						// Model->translate(vec3(i, libraryCenter.y, j)); // Position set in class members
						// Model->rotate(glm::radians(bossGrid[gridPos].transformData.rotation), vec3(0, 1, 0)); // Rotate for left/right walls
						// Model->scale(bossGrid[gridPos].transformData.scale); // Scale set in class members
						// setModel(shader, Model);
						// book_shelf1->Draw(shader); // Use the bookshelf model for the border
						// Model->popMatrix();
					}
					else if (bossGrid[gridPos].type == BossRoomGen::CellType::ENTRANCE) {
						if (bossGrid[gridPos].borderType == BossRoomGen::BorderType::ENTRANCE_MIDDLE) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, 0, j));
							Model->rotate(glm::radians(bossGrid[gridPos].transformData.rotation), vec3(0, 1, 0)); // Rotate for left/right walls
							Model->scale(bossGrid[gridPos].transformData.scale); // Scale set in class members
							setModel(shader, Model);
							if (unlock == false) {
								door->Draw(shader); // Use the door model for the entrance
							}

							Model->popMatrix();
						}
						else if (bossGrid[gridPos].borderType == BossRoomGen::BorderType::ENTRANCE_SIDE) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, 0, j));
							Model->rotate(glm::radians(bossGrid[gridPos].transformData.rotation), vec3(0, 1, 0)); // Rotate for left/right walls
							Model->scale(bossGrid[gridPos].transformData.scale); // Scale set in class members
							setModel(shader, Model);
							book_shelf1->Draw(shader); // Use the door model for the entrance
							Model->popMatrix();
						}
					}
					else if (bossGrid[gridPos].type == BossRoomGen::CellType::EXIT) {
						if (bossGrid[gridPos].borderType == BossRoomGen::BorderType::EXIT_MIDDLE) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, 0, j));
							Model->rotate(glm::radians(bossGrid[gridPos].transformData.rotation), vec3(0, 1, 0)); // Rotate for left/right walls
							Model->scale(bossGrid[gridPos].transformData.scale); // Scale set in class members
							setModel(shader, Model);
							door->Draw(shader); // Use the door model for the entrance
							Model->popMatrix();
						}
						else if (bossGrid[gridPos].borderType == BossRoomGen::BorderType::EXIT_SIDE) {
							Model->pushMatrix();
							Model->loadIdentity();
							Model->translate(vec3(i, 0, j));
							Model->rotate(glm::radians(bossGrid[gridPos].transformData.rotation), vec3(0, 1, 0)); // Rotate for left/right walls
							Model->scale(bossGrid[gridPos].transformData.scale); // Scale set in class members
							setModel(shader, Model);
							book_shelf1->Draw(shader); // Use the door model for the entrance
							Model->popMatrix();
						}
					} else if (bossGrid[gridPos].type == BossRoomGen::CellType::CLUSTER) {
						if (bossGrid[gridPos].clusterType == BossRoomGen::ClusterType::SHELF1) {
							if (bossGrid[gridPos].objectType == BossRoomGen::CellObjType::GLOWING_SHELF) {
								Model->pushMatrix();
								Model->loadIdentity();
								Model->translate(vec3(i, libraryCenter.y, j)); // Position shelf at cell center on ground
								Model->rotate(glm::radians(bossGrid[gridPos].transformData.rotation), vec3(0, 1, 0)); // Rotate for left/right walls
								Model->scale(bossGrid[gridPos].transformData.scale); // Scale set in class members
								setModel(shader, Model);
								book_shelf2->Draw(shader);
								Model->popMatrix();
							}
						}
					}
				}
			}
		}
		shader->unbind();
	}

	void drawBossEnemy(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model || !bossEnemy) return; // Need boss enemy model

		shader->bind(); // Use prog2 for simple colored shapes

		// --- Material Settings ---
		glm::vec3 bodyColor = glm::vec3(0.6f, 0.2f, 0.8f); // Purple-ish body
		glm::vec3 eyeWhiteColor = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 eyePupilColor = glm::vec3(0.1f, 0.1f, 0.1f);

		// --- Common Eye Parameters ---
		float bodyBaseScaleY = 0.8f; // Base height factor before pill stretch
		glm::vec3 eyeOffsetBase = glm::vec3(0.0f, bodyBaseScaleY * 0.4f, 0.45f); // Y up, Z forward from body center
		float eyeSeparation = 0.25f; // Distance between eye centers
		float whiteScale = 0.18f;
		float pupilScale = 0.1f;
		float pupilOffsetForward = 0.02f; // Push pupil slightly in front of white

		if (bossEnemy->isAlive() && unlock) {
			bossEnemy->lookAtPlayer(player->getPosition()); // Make the boss look at the player
			glm::vec3 bossPos = bossEnemy->getPosition() + glm::vec3(0, 0, 0); // Position the boss slightly above the ground
			glm::vec3 bossRotation = bossEnemy->getRotation(); // Get rotation from the enemy object
			float bossRotY = bossEnemy->getRotY();

			Model->pushMatrix();
			{
				Model->loadIdentity(); // Reset the model matrix
				Model->translate(bossPos);
				Model->rotate(bossRotY, bossRotation); // Rotate the body to match the boss's rotation
				Model->scale(vec3(1.3f, 0.8f, 1.0f));
				// --- Draw Main Body (Pill Shape) ---
				Model->pushMatrix();
				{
					// Model->translate(bossPos);
					// Scale for pill shape ( taller in Y, squished in X/Z )
					Model->scale(glm::vec3(0.5f, bodyBaseScaleY * 1.6f, 0.5f)); // Adjust scale factors as needed

					// Set body material
					SetMaterial(shader, Material::purple);

					setModel(shader, Model);
					// sphere->Draw(shader); // Draw the scaled sphere as the body
					stoneGolem->Draw(shader); // Use the stone golem model for the body
				}
				Model->popMatrix();


				// --- Draw Eyes (Relative to Enemy Center) ---

				// Set Eye Materials Once
				// White Material Setup (done inside loop per part for clarity now)
				// Black Material Setup (done inside loop per part for clarity now)

				// // Left Eye
				// Model->pushMatrix();
				// {
				// 	// Go to enemy center, then offset to eye position
				// 	// Model->translate(bossPos);
				// 	Model->translate(eyeOffsetBase + glm::vec3(-eyeSeparation, 0, 0));

				// 	// White Part
				// 	Model->pushMatrix();
				// 	{
				// 		Model->scale(glm::vec3(whiteScale));
				// 		// Set white material
				// 		SetMaterial(shader, Material::eye_white);
				// 		setModel(shader, Model);
				// 		sphere->Draw(shader);
				// 	}
				// 	Model->popMatrix(); // Pop white scale

				// 	// Pupil Part
				// 	Model->pushMatrix();
				// 	{
				// 		// Move slightly forward from white surface and scale down
				// 		Model->translate(glm::vec3(0, 0, whiteScale * 0.5f + pupilOffsetForward)); // Offset relative to white scale
				// 		Model->scale(glm::vec3(pupilScale));
				// 		// Set black material
				// 		SetMaterial(shader, Material::black);
				// 		setModel(shader, Model);
				// 		sphere->Draw(shader);
				// 	}
				// 	Model->popMatrix(); // Pop pupil transform
				// }
				// Model->popMatrix(); // Pop left eye transform


				// // Right Eye (Similar to Left)
				// Model->pushMatrix();
				// {
				// 	// Model->translate(bossPos);
				// 	Model->translate(eyeOffsetBase + glm::vec3(+eyeSeparation, 0, 0)); // Offset to the right

				// 	// White Part
				// 	Model->pushMatrix();
				// 	{
				// 		Model->scale(glm::vec3(whiteScale));
				// 		// Set white material
				// 		SetMaterial(shader, Material::eye_white);
				// 		setModel(shader, Model);
				// 		sphere->Draw(shader);
				// 	}
				// 	Model->popMatrix();

				// 	// Pupil Part
				// 	Model->pushMatrix();
				// 	{
				// 		Model->translate(glm::vec3(0, 0, whiteScale * 0.5f + pupilOffsetForward));
				// 		Model->scale(glm::vec3(pupilScale));
				// 		// Set black material
				// 		SetMaterial(shader, Material::black);
				// 		setModel(shader, Model);
				// 		sphere->Draw(shader);
				// 	}
				// 	Model->popMatrix();
				// }
				// Model->popMatrix(); // Pop right eye transform
			}
			Model->popMatrix(); // Pop boss body transform
		}
		shader->unbind();
	}

	void drawDoor(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model || !cube) return; // Need cube model

		shader->bind();

		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(doorPosition); // Position set in class members
		Model->scale(doorScale);      // Scale set in class members

		SetMaterial(shader, Material::wood); // Use Wood material

		setModel(shader, Model);
		cube->Draw(shader);

		Model->popMatrix();
		shader->unbind();
	}

	bool checkAABBCollision(const glm::vec3& minA, const glm::vec3& maxA,
		const glm::vec3& minB, const glm::vec3& maxB)
	{
		return (minA.x <= maxB.x && maxA.x >= minB.x) &&
			(minA.y <= maxB.y && maxA.y >= minB.y) &&
			(minA.z <= maxB.z && maxA.z >= minB.z);
	}

	bool checkSphereCollision(const glm::vec3& spherePos, float sphereRadius,
		const glm::vec3& boxMin, const glm::vec3& boxMax)
	{
		glm::vec3 closestPoint = glm::clamp(spherePos, boxMin, boxMax);
		glm::vec3 distanceVec = spherePos - closestPoint;
		return glm::length(distanceVec) <= sphereRadius;
	}

	void updateBooks(float deltaTime) { // deltaTime might not be needed if using glfwGetTime()
		for (auto& book : books) {
			book.update(deltaTime, 0.0f);

			if (book.state == BookState::OPENED && !book.orbSpawned) {
				glm::mat4 baseRotation = glm::mat4_cast(book.orientation);
				// Spawn slightly above the book center to avoid immediate ground collision?
				glm::vec3 orbOffset = glm::vec3(0.0f, book.scale.y * 0.1f + 0.05f, 0.0f); // Small initial Y offset
				glm::vec3 orbSpawnPos = book.position + glm::vec3(baseRotation * glm::vec4(orbOffset, 0.0f));

				// Constructor handles setting state to LEVITATING and calculating idlePosition
				// Orb color is now set by the Book's spellType
				orbCollectibles.emplace_back(sphere, orbSpawnPos, book.orbScale, book.orbColor, book.spellType);

				book.orbSpawned = true;
				cout << "Orb Spawned! Type: " << static_cast<int>(book.spellType) << " State: LEVITATING" << endl;
			}
		}
	}

	void interactWithBooks() {
		float interactionRadius = 5.0f;
		float interactionRadiusSq = interactionRadius * interactionRadius;

		float groundSize = Config::GROUND_SIZE;
		float groundY = Config::GROUND_HEIGHT;

		float gridWorldWidth = groundSize * 2.0f;
		float gridWorldDepth = groundSize * 2.0f;
		float cellWidth = gridWorldWidth / (float)grid.getSize().x;
		float cellDepth = gridWorldDepth / (float)grid.getSize().y;

		bool interacted = false;

		int gridX = library->mapXtoGridX(player->getPosition().x);
		int gridZ = library->mapZtoGridY(player->getPosition().z);

		float gridInteractionRadius = 1.5f;
		int radiusInCells = static_cast<int>(std::ceil(gridInteractionRadius / cellWidth));

		if (!bossfightstarted) {
			std::vector<const QuadElement*> bookElements;
			libraryQuadTree->query(glm::vec2(player->getPosition().x, player->getPosition().z), glm::vec2(gridInteractionRadius), bookElements);
			for (int i = 0; i < bookElements.size() && !interacted; ++i) {
				const QuadElement* e = bookElements[i];
				// int gridx = library->mapXtoGridX(e->center.x);
				// int gridZ = library->mapZtoGridY(e->center.y);
				// glm::ivec2 gridPos(gridx, gridZ);
				// if (!grid.inBounds(gridPos)) continue; // Skip out-of-bounds cells
				LibraryGen::Cell cell = grid[e->grid_position];
				std::cout << "Checking cell at (" << e->grid_position.x << ", " << e->grid_position.y << ") with object type: " << static_cast<int>(cell.objectType) << std::endl;
				if (cell.objectType == LibraryGen::CellObjType::SHELF_WITH_ABILITY || cell.objectType == LibraryGen::CellObjType::SHELF_WITH_ABILITY_ROTATED) {
					// float shelfWorldX = libraryCenter.x - gridWorldWidth * 0.5f + (x + 0.5f) * cellWidth;
					// float shelfWorldZ = libraryCenter.z - gridWorldDepth * 0.5f + (z + 0.5f) * cellDepth;
					float shelfWorldX = e->center.x; // Center the shelf in the cell
					float shelfWorldZ = e->center.y; // Center the shelf in the cell
					glm::vec3 shelfCenterPos = glm::vec3(shelfWorldX, groundY + 1.0f, shelfWorldZ);

					// glm::vec3 diff = shelfCenterPos - characterMovement;
					glm::vec3 diff = shelfCenterPos - player->getPosition();
					diff.y = 0.0f; // Ignore Y difference for interaction distance
					float distSq = dot(diff, diff); // Use dot product for squared distance

					if (distSq <= interactionRadiusSq) {

						// --- ADJUST Spawn Height ---
						float minSpawnHeight = 1.8f; // Minimum height above groundY
						float maxSpawnHeight = 2.8f; // Maximum height above groundY
						float spawnHeight = groundY + Config::randFloat(minSpawnHeight, maxSpawnHeight); // <-- ADJUSTED height range

						glm::vec3 spawnPos = glm::vec3(shelfWorldX, spawnHeight, shelfWorldZ);

						glm::vec3 bookScale = glm::vec3(0.7f, 0.9f, 0.2f);
						glm::quat bookOrientation = glm::angleAxis(glm::radians(Config::randFloat(-10.f, 10.f)), glm::vec3(0, 1, 0));
						// glm::vec3 orbColor = glm::vec3(Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f)); // Color now set by book

						// Cycle through spell types for newly spawned books/orbs
						// static int nextSpellTypeIndex = 1; // Start with FIRE (index 1 in SpellType enum)
						SpellType newSpellType = static_cast<SpellType>(nextSpellTypeIndex);
						nextSpellTypeIndex++;
						if (nextSpellTypeIndex > 4) { // Assuming 4 spell types: FIRE, ICE, LIGHTNING, HEAL
							nextSpellTypeIndex = 1; // Cycle back to FIRE
						}

						books.emplace_back(spawnPos, bookScale, bookOrientation, newSpellType);
		//books.emplace_back(spawnPos, bookScale, bookOrientation, newSpellType);

						Book& newBook = books.back();

						// --- PASS Player Position to startFalling ---
						// newBook.startFalling(groundY, characterMovement); // <<-- MODIFIED call
						newBook.startFalling(groundY, player->getPosition());

						interacted = true;

					}
				}
			}
		}

		if (bossfightstarted) {
			std::vector<const QuadElement*> bookElements;
			bossRoomQuadTree->query(glm::vec2(player->getPosition().x, player->getPosition().z), glm::vec2(gridInteractionRadius), bookElements);
			for (int i = 0; i < bookElements.size() && !interacted; ++i) {
				const QuadElement* e = bookElements[i];
				// int gridx = bossRoom->mapXtoGridX(e->center.x);
				// int gridZ = bossRoom->mapZtoGridY(e->center.y);
				// glm::ivec2 gridPos(gridx, gridZ);
				// if (!bossGrid.inBounds(gridPos)) continue; // Skip out-of-bounds cells
				BossRoomGen::Cell cell = bossGrid[e->grid_position];
				if (cell.objectType == BossRoomGen::CellObjType::GLOWING_SHELF) {
					// float shelfWorldX = libraryCenter.x - gridWorldWidth * 0.5f + (x + 0.5f) * cellWidth;
					// float shelfWorldZ = libraryCenter.z - gridWorldDepth * 0.5f + (z + 0.5f) * cellDepth;
					float shelfWorldX = e->center.x; // Center the shelf in the cell
					float shelfWorldZ = e->center.y; // Center the shelf in the cell
					glm::vec3 shelfCenterPos = glm::vec3(shelfWorldX, groundY + 1.0f, shelfWorldZ);

					// glm::vec3 diff = shelfCenterPos - characterMovement;
					glm::vec3 diff = shelfCenterPos - player->getPosition();
					diff.y = 0.0f; // Ignore Y difference for interaction distance
					float distSq = dot(diff, diff); // Use dot product for squared distance

					if (distSq <= interactionRadiusSq) {

						// --- ADJUST Spawn Height ---
						float minSpawnHeight = 1.8f; // Minimum height above groundY
						float maxSpawnHeight = 2.8f; // Maximum height above groundY
						float spawnHeight = groundY + Config::randFloat(minSpawnHeight, maxSpawnHeight); // <-- ADJUSTED height range

						glm::vec3 spawnPos = glm::vec3(shelfWorldX, spawnHeight, shelfWorldZ);

						glm::vec3 bookScale = glm::vec3(0.7f, 0.9f, 0.2f);
						glm::quat bookOrientation = glm::angleAxis(glm::radians(Config::randFloat(-10.f, 10.f)), glm::vec3(0, 1, 0));
						// glm::vec3 orbColor = glm::vec3(Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f)); // Color now set by book

						// Cycle through spell types for newly spawned books/orbs
						// static int nextSpellTypeIndex = 1; // Start with FIRE (index 1 in SpellType enum)
						SpellType newSpellType = static_cast<SpellType>(nextSpellTypeIndex);
						nextSpellTypeIndex++;
						if (nextSpellTypeIndex > 3) { // Assuming 3 spell types: FIRE, ICE, LIGHTNING
							nextSpellTypeIndex = 1; // Cycle back to FIRE
						}

						books.emplace_back(spawnPos, bookScale, bookOrientation, newSpellType);
		//books.emplace_back(spawnPos, bookScale, bookOrientation, newSpellType);

						Book& newBook = books.back();

						// --- PASS Player Position to startFalling ---
						// newBook.startFalling(groundY, characterMovement); // <<-- MODIFIED call
						newBook.startFalling(groundY, player->getPosition());

						interacted = true;

					}
				}
			}
		}

		// if (!bossfightstarted) {
		// 	for (int dz = -radiusInCells; dz <= radiusInCells && !interacted; ++dz) {
		// 		for (int dx = -radiusInCells; dx <= radiusInCells && !interacted; ++dx) {
		// 			glm::ivec2 gridPos(gridX + dx, gridZ + dz);

		// 			if (!grid.inBounds(gridPos)) continue; // Skip out-of-bounds cells

		// 			if (grid[gridPos].objectType == LibraryGen::CellObjType::SHELF_WITH_ABILITY || grid[gridPos].objectType == LibraryGen::CellObjType::SHELF_WITH_ABILITY_ROTATED) {
		// 				// float shelfWorldX = libraryCenter.x - gridWorldWidth * 0.5f + (x + 0.5f) * cellWidth;
		// 				// float shelfWorldZ = libraryCenter.z - gridWorldDepth * 0.5f + (z + 0.5f) * cellDepth;
		// 				float shelfWorldX = library->mapGridXtoWorldX(gridX); // Center the shelf in the cell
		// 				float shelfWorldZ = library->mapGridYtoWorldZ(gridZ); // Center the shelf in the cell
		// 				glm::vec3 shelfCenterPos = glm::vec3(shelfWorldX, groundY + 1.0f, shelfWorldZ);

		// 				// glm::vec3 diff = shelfCenterPos - characterMovement;
		// 				glm::vec3 diff = shelfCenterPos - player->getPosition();
		// 				diff.y = 0.0f; // Ignore Y difference for interaction distance
		// 				float distSq = dot(diff, diff); // Use dot product for squared distance

		// 				if (distSq <= interactionRadiusSq) {

		// 					// --- ADJUST Spawn Height ---
		// 					float minSpawnHeight = 1.8f; // Minimum height above groundY
		// 					float maxSpawnHeight = 2.8f; // Maximum height above groundY
		// 					float spawnHeight = groundY + Config::randFloat(minSpawnHeight, maxSpawnHeight); // <-- ADJUSTED height range

		// 					glm::vec3 spawnPos = glm::vec3(shelfWorldX, spawnHeight, shelfWorldZ);

		// 					glm::vec3 bookScale = glm::vec3(0.7f, 0.9f, 0.2f);
		// 					glm::quat bookOrientation = glm::angleAxis(glm::radians(Config::randFloat(-10.f, 10.f)), glm::vec3(0, 1, 0));
		// 					// glm::vec3 orbColor = glm::vec3(Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f)); // Color now set by book

		// 					// Cycle through spell types for newly spawned books/orbs
		// 					// static int nextSpellTypeIndex = 1; // Start with FIRE (index 1 in SpellType enum)
		// 					SpellType newSpellType = static_cast<SpellType>(nextSpellTypeIndex);
		// 					nextSpellTypeIndex++;
		// 					if (nextSpellTypeIndex > 3) { // Assuming 3 spell types: FIRE, ICE, LIGHTNING
		// 						nextSpellTypeIndex = 1; // Cycle back to FIRE
		// 					}

		// 					books.emplace_back(spawnPos, bookScale, bookOrientation, newSpellType);
        //       //books.emplace_back(spawnPos, bookScale, bookOrientation, newSpellType);

		// 					Book& newBook = books.back();

		// 					// --- PASS Player Position to startFalling ---
		// 					// newBook.startFalling(groundY, characterMovement); // <<-- MODIFIED call
		// 					newBook.startFalling(groundY, player->getPosition());

		// 					interacted = true;
		// 				}
		// 			}
		// 		}
		// 	}
		// }
		// if (bossfightstarted) {
		// 	gridX = bossRoom->mapXtoGridX(player->getPosition().x);
		// 	gridZ = bossRoom->mapZtoGridY(player->getPosition().z);

		// 	for (int dz = -radiusInCells; dz <= radiusInCells && !interacted; ++dz) {
		// 		for (int dx = -radiusInCells; dx <= radiusInCells && !interacted; ++dx) {
		// 			glm::ivec2 gridPos(gridX + dx, gridZ + dz);

		// 			if (!bossGrid.inBounds(gridPos)) continue; // Skip out-of-bounds cells

		// 			if (bossGrid[gridPos].objectType == BossRoomGen::CellObjType::GLOWING_SHELF) {
		// 				// float shelfWorldX = libraryCenter.x - gridWorldWidth * 0.5f + (x + 0.5f) * cellWidth;
		// 				// float shelfWorldZ = libraryCenter.z - gridWorldDepth * 0.5f + (z + 0.5f) * cellDepth;
		// 				float shelfWorldX = bossRoom->mapGridXtoWorldX(gridX); // Center the shelf in the cell
		// 				float shelfWorldZ = bossRoom->mapGridYtoWorldZ(gridZ); // Center the shelf in the cell
		// 				glm::vec3 shelfCenterPos = glm::vec3(shelfWorldX, groundY + 1.0f, shelfWorldZ);

		// 				// glm::vec3 diff = shelfCenterPos - characterMovement;
		// 				glm::vec3 diff = shelfCenterPos - player->getPosition();
		// 				diff.y = 0.0f; // Ignore Y difference for interaction distance
		// 				float distSq = dot(diff, diff); // Use dot product for squared distance

		// 				if (distSq <= interactionRadiusSq) {

		// 					// --- ADJUST Spawn Height ---
		// 					float minSpawnHeight = 1.8f; // Minimum height above groundY
		// 					float maxSpawnHeight = 2.8f; // Maximum height above groundY
		// 					float spawnHeight = groundY + Config::randFloat(minSpawnHeight, maxSpawnHeight); // <-- ADJUSTED height range

		// 					glm::vec3 spawnPos = glm::vec3(shelfWorldX, spawnHeight, shelfWorldZ);

		// 					glm::vec3 bookScale = glm::vec3(0.7f, 0.9f, 0.2f);
		// 					glm::quat bookOrientation = glm::angleAxis(glm::radians(Config::randFloat(-10.f, 10.f)), glm::vec3(0, 1, 0));
		// 					// glm::vec3 orbColor = glm::vec3(Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f), Config::randFloat(0.2f, 1.0f)); // Color now set by book

		// 					// Cycle through spell types for newly spawned books/orbs
		// 					// static int nextSpellTypeIndex = 1; // Start with FIRE (index 1 in SpellType enum)
		// 					SpellType newSpellType = static_cast<SpellType>(nextSpellTypeIndex);
		// 					nextSpellTypeIndex++;
		// 					if (nextSpellTypeIndex > 3) { // Assuming 3 spell types: FIRE, ICE, LIGHTNING
		// 						nextSpellTypeIndex = 1; // Cycle back to FIRE
		// 					}

		// 					books.emplace_back(spawnPos, bookScale, bookOrientation, newSpellType);

		// 					Book& newBook = books.back();

		// 					// --- PASS Player Position to startFalling ---
		// 					// newBook.startFalling(groundY, characterMovement); // <<-- MODIFIED call
		// 					newBook.startFalling(groundY, player->getPosition());

		// 					interacted = true;
		// 				}
		// 			}
		// 		}
		// 	}
		// }


		if (interacted) {
			cout << "Book spawned and falling." << endl;
		}
		else {
			cout << "No shelf nearby to interact with." << endl;
		}
	}

	void updateOrbs(float currentTime) {
		for (auto& orb : orbCollectibles) {
			// Update levitation only if not already collected
			if (!orb.collected) {
				orb.updateLevitation(currentTime);
			}
		}
	}

	void updateEnemies(float deltaTime) {
		int screenWidth, screenHeight;
		glfwGetFramebufferSize(windowManager->getHandle(), &screenWidth, &screenHeight);
		// TODO: Add enemy movement, AI, attack logic later
		for (auto* enemy : enemies) {
			enemy->update(player.get(), deltaTime);
		}
	}

	// --- Collision Checking Helper ---
	bool checkCollisionAt(const glm::vec3& checkPos, const glm::quat& playerOrientation) {
		if (!book_shelf1 || grid.getSize().x == 0) return false; // Need data

		// 1. Calculate Player's World AABB at checkPos
		glm::mat4 playerTransform = glm::translate(glm::mat4(1.0f), checkPos) * glm::mat4_cast(playerOrientation);
		// Note: We use the PRE-SCALED local AABB calculated earlier
		glm::vec3 playerWorldMin, playerWorldMax;
		updateBoundingBox(playerBB->min, playerBB->max, playerTransform, playerWorldMin, playerWorldMax);

		// spatial detection for library grid

		float gridCollisionRadius = 1.0f;
		float cellSize = 2.0f; // Assuming square cells
		int radiusInCells = static_cast<int>(std::ceil(gridCollisionRadius / cellSize));

		int gridX = library->mapXtoGridX(checkPos.x);
		int gridZ = library->mapZtoGridY(checkPos.z);

		std::vector<const QuadElement*> nearby_elements;
		libraryQuadTree->query(glm::vec2(checkPos.x, checkPos.z), glm::vec2(gridCollisionRadius, gridCollisionRadius), nearby_elements);
		for (const auto* e : nearby_elements) {
			if (checkSphereCollision(checkPos, 0.25f, e->aabb_min, e->aabb_max)) {
				// std::cout << "[DEBUG] Collision DETECTED with shelf at grid (" << gridX << "," << gridZ << ")" << std::endl;
				// int gridx = library->mapXtoGridX(e->center.x);
				// int gridZ = library->mapZtoGridY(e->center.y);
				// glm::ivec2 gridPos = glm::ivec2(gridx, gridZ);
				LibraryGen::Cell cell = grid[e->grid_position];
				std::cout << "Checking cell at (" << e->grid_position.x << ", " << e->grid_position.y << ") with object type: " << static_cast<int>(cell.objectType) << std::endl;
				return true; // Collision found
			}
		}

		std::vector<const QuadElement*> nearby_boss_elements;
		bossRoomQuadTree->query(glm::vec2(checkPos.x, checkPos.z), glm::vec2(gridCollisionRadius, gridCollisionRadius), nearby_boss_elements);
		for (const auto* e : nearby_boss_elements) {
			// int bossGridX = bossRoom->mapXtoGridX(e->center.x);
			// int bossGridZ = bossRoom->mapZtoGridY(e->center.y);
			// glm::ivec2 bossGridPos = glm::ivec2(bossGridX, bossGridZ);
			BossRoomGen::Cell cell = bossGrid[e->grid_position];

			glm::vec3 clusterWorldMin = e->aabb_min;
			glm::vec3 clusterWorldMax = e->aabb_max;

			// Checks collision with the side shelves
			if (cell.borderType == BossRoomGen::BorderType::ENTRANCE_SIDE) {
				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
					std::cout << "[DEBUG] Collision DETECTED with shelf at grid (" << e->grid_position.x << "," << e->grid_position.y << ")" << std::endl;
					return true; // Collision found
				}
			}
			// Prevents entering the boss room until canFightboss is true
			else if (cell.borderType == BossRoomGen::BorderType::ENTRANCE_MIDDLE && !canFightboss) {
				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
					std::cout << "[DEBUG] Collision DETECTED with shelf at grid (" << e->grid_position.x << "," << e->grid_position.y << ")" << std::endl;
					return true; // Collision found
				}
			} // for when done with the boss fight
			else if (cell.borderType == BossRoomGen::BorderType::EXIT_MIDDLE && bossfightended && !bossEnemy->isAlive()) {
				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
					bossfightended = false;
					restartGen = true;
					return false;
				}
			}
			// these two are to prevent leaving the boss area once the fight has started
			else if (cell.borderType == BossRoomGen::BorderType::ENTRANCE_MIDDLE && bossfightstarted) {
				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
					return true;
				}
			}
			else if (cell.borderType == BossRoomGen::BorderType::EXIT_MIDDLE && bossfightstarted) {
				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
					return true;
				}
			} else if (bossfightstarted || bossfightended) {
				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
					return true;
				}
			}
			// checks general collision with shelves inside boss area
			else if (cell.type == BossRoomGen::CellType::CLUSTER) {
				if (checkSphereCollision(checkPos, 0.5f, clusterWorldMin, clusterWorldMax)) {
					return true;
				}
			}

		}

		// gridX = bossRoom->mapXtoGridX(checkPos.x);
		// gridZ = bossRoom->mapZtoGridY(checkPos.z);

		// glm::ivec2 gridPos = glm::ivec2(gridX, gridZ);

		// if (bossGrid.inBounds(gridPos)) {
		// 	for (int dz = -radiusInCells; dz <= radiusInCells; ++dz) {
		// 		for (int dx = -radiusInCells; dx <= radiusInCells; ++dx) {
		// 			glm::ivec2 cellPos = glm::ivec2(gridX + dx, gridZ + dz);
		// 			if (!bossGrid.inBounds(cellPos)) continue; // Skip out-of-bounds cells

		// 			const auto& cell = bossGrid[cellPos];
		// 			if (cell.type == BossRoomGen::CellType::NONE) continue;
		// 			// if (bossfightstarted && !bossRoom->isInsideBossArea(cellPos)) return true;

		// 			glm::vec3 clusterBboxMin;
		// 			glm::vec3 clusterBboxMax;
		// 			glm::vec3 clusterCenter = glm::vec3(bossRoom->mapGridXtoWorldX(cellPos.x), libraryCenter.y, bossRoom->mapGridYtoWorldZ(cellPos.y));

		// 			switch (cell.objectType) {
		// 				case BossRoomGen::CellObjType::BOOKSHELF:
		// 					clusterBboxMin = book_shelf1->getBoundingBoxMin();
		// 					clusterBboxMax = book_shelf1->getBoundingBoxMax();
		// 					break;
		// 				case BossRoomGen::CellObjType::GLOWING_SHELF:
		// 					clusterBboxMin = book_shelf2->getBoundingBoxMin();
		// 					clusterBboxMax = book_shelf2->getBoundingBoxMax();
		// 					break;
		// 				case BossRoomGen::CellObjType::DOOR:
		// 					clusterBboxMin = door->getBoundingBoxMin();
		// 					clusterBboxMax = door->getBoundingBoxMax();
		// 					break;
		// 				default:
		// 					continue; // Skip unknown object types
		// 				}

		// 			glm::mat4 clusterTransform = glm::translate(glm::mat4(1.0f), clusterCenter);
		// 			clusterTransform = glm::rotate(clusterTransform, cell.transformData.rotation, glm::vec3(0, 1, 0));
		// 			clusterTransform = glm::scale(clusterTransform, cell.transformData.scale);
		// 			glm::vec3 clusterWorldMin, clusterWorldMax;
		// 			updateBoundingBox(clusterBboxMin, clusterBboxMax, clusterTransform, clusterWorldMin, clusterWorldMax);

		// 			// Checks collision with the side shelves
		// 			if (cell.borderType == BossRoomGen::BorderType::ENTRANCE_SIDE) {
		// 				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
		// 					std::cout << "[DEBUG] Collision DETECTED with shelf at grid (" << gridX << "," << gridZ << ")" << std::endl;
		// 					return true; // Collision found
		// 				}
		// 			}
		// 			// Prevents entering the boss room until canFightboss is true
		// 			else if (cell.borderType == BossRoomGen::BorderType::ENTRANCE_MIDDLE && !canFightboss) {
		// 				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
		// 					std::cout << "[DEBUG] Collision DETECTED with shelf at grid (" << gridX << "," << gridZ << ")" << std::endl;
		// 					return true; // Collision found
		// 				}
		// 			} // for when done with the boss fight
		// 			else if (cell.borderType == BossRoomGen::BorderType::EXIT_MIDDLE && bossfightended && !bossEnemy->isAlive()) {
		// 				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
		// 					bossfightended = false;
		// 					restartGen = true;
		// 					return false;
		// 				}
		// 			}
		// 			// these two are to prevent leaving the boss area once the fight has started
		// 			else if (cell.borderType == BossRoomGen::BorderType::CIRCULAR_BORDER && bossfightstarted) {
		// 				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
		// 					return true;
		// 				}
		// 			}
		// 			else if (cell.borderType == BossRoomGen::BorderType::ENTRANCE_MIDDLE && bossfightstarted) {
		// 				if (checkSphereCollision(checkPos, 0.25f, clusterWorldMin, clusterWorldMax)) {
		// 					return true;
		// 				}
		// 			}
		// 			// checks general collision with shelves inside boss area
		// 			else if (cell.type == BossRoomGen::CellType::CLUSTER) {
		// 				if (checkSphereCollision(checkPos, 0.5f, clusterWorldMin, clusterWorldMax)) {
		// 					return true;
		// 				}
		// 			}
		// 		}
		// 	}
		// }

		return false; // No collision found
	}

	// --- Modified charMove ---
	vec3 charMove() {
		float moveSpeed = 4.5f * AnimDeltaTime; // Use frame-rate independent speed
		vec3 desiredMoveDelta = vec3(0.0f);

		// Calculate desired movement direction based on input
		if (movingForward)  desiredMoveDelta += manMoveDir;
		if (movingBackward) desiredMoveDelta -= manMoveDir;
		if (movingLeft)     desiredMoveDelta -= right;
		if (movingRight)    desiredMoveDelta += right;

		// Normalize and scale movement delta
		float moveLength = length(desiredMoveDelta);
		if (moveLength > 0.0f) {
			desiredMoveDelta = (desiredMoveDelta / moveLength) * moveSpeed;
		}
		else {
			// return characterMovement; // No movement input, stay put
			return player->getPosition();
		}

		// --- Collision Detection and Resolution ---
		// vec3 currentPos = characterMovement;
		// vec3 nextPos = currentPos + desiredMoveDelta;
		// nextPos.y = groundY; // Keep player on the ground plane

		vec3 currentPos = player->getPosition();
		vec3 nextPos = currentPos + desiredMoveDelta;
		nextPos.y = groundY; // Keep player on the ground plane

		// Player orientation for AABB calculation
		// glm::quat playerOrientation = glm::angleAxis(manRot.y, glm::vec3(0, 1, 0));
		glm::quat playerOrientation = glm::angleAxis(player->getRotY(), glm::vec3(0, 1, 0));

		// --- Simple Stop Method ---
		/*
		if (checkCollisionAt(nextPos, playerOrientation)) {
			 // Don't update characterMovement, effectively stopping before collision
			 cout << "[DEBUG] Collision prevented movement." << endl;
			 return currentPos; // Return current position
		} else {
			 // No collision detected, allow full movement
			 characterMovement = nextPos;
		}
		*/

		// --- Sliding Method (Separate Axes) ---
		vec3 allowedPos = currentPos; // Start with current position

		// Try moving along X only
		vec3 nextPosX = vec3(nextPos.x, currentPos.y, currentPos.z);
		if (!checkCollisionAt(nextPosX, playerOrientation)) {
			allowedPos.x = nextPos.x; // Allow X movement
		}
		else {
			cout << "[DEBUG] X-Collision prevented." << endl;
		}


		// Try moving along Z only (starting from potentially updated X)
		vec3 nextPosZ = vec3(allowedPos.x, currentPos.y, nextPos.z); // Use allowedPos.x
		if (!checkCollisionAt(nextPosZ, playerOrientation)) {
			allowedPos.z = nextPos.z; // Allow Z movement
		}
		else {
			cout << "[DEBUG] Z-Collision prevented." << endl;
		}

		// Final position is the allowed position after checking both axes
		// characterMovement = allowedPos;
		// characterMovement.y = groundY; // Ensure Y stays correct

		player->setPosition(vec3(allowedPos.x, groundY, allowedPos.z)); // Update player position

		// Update camera based on final position (done in render)
		// return characterMovement; // Return the final, potentially adjusted, position
		return player->getPosition(); // Return the final position
	}

	// --- Shooting Function ---
	void shootSpell() {

		cout << "[DEBUG] shootSpell() called. Orbs: " << orbsCollectedCount << endl;
		if (orbsCollectedCount <= 0 && !debugCamera) { // Allow shooting in debug camera without orbs
			cout << "[DEBUG] Cannot shoot: No orbs." << endl;
			return;
		}

		// Consume an orb if not in debug mode
		if (!debugCamera) {
			// Remove visual orb logic... (find first collected orb and erase)
			for (auto it = orbCollectibles.begin(); it != orbCollectibles.end(); ++it) {
				if (it->collected && it->spellType == currentPlayerSpellType) {
					spellCounts[it->spellType]--; // Increment spell count for the type being shot
					cout << "Spells remaining of this type: " << spellCounts[it->spellType] << endl;
					orbsCollectedCount--;
					orbCollectibles.erase(it);
					break;
				}
			}
		}

		vec3 shootDir = manMoveDir;
		vec3 playerRight = normalize(cross(manMoveDir, vec3(0.0f, 1.0f, 0.0f)));

		float forwardOffset = 0.5f;
		float upOffset = 0.8f;
		float rightOffset = 0.2f;

		vec3 spawnPos = player->getPosition()
			+ vec3(0.0f, upOffset, 0.0f)
			+ shootDir * forwardOffset
			+ playerRight * rightOffset;

		activeSpells.emplace_back(spawnPos, shootDir, (float)glfwGetTime());
		SpellProjectile& newProj = activeSpells.back();

		if (particleSystem && spellCounts[currentPlayerSpellType] > 0) {
			float current_particle_system_time = particleSystem->getCurrentTime();
			int particles_to_spawn = 10;

			float p_speed_min = newProj.speed * 0.2f;
			float p_speed_max = newProj.speed * 0.5f;
			float p_spread = 0.6f;
			float p_lifespan_min = 0.4f;
			float p_lifespan_max = 0.8f;
			glm::vec4 p_color_start = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 p_color_end = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
			float p_scale_min = 0.2f;
			float p_scale_max = 0.4f;

			// Use currentPlayerSpellType to determine visuals
			std::string spellTypeName = "NONE";
			switch (currentPlayerSpellType) {
			case SpellType::FIRE:
				spellTypeName = "FIRE";
				particles_to_spawn = 40; // Increased count
				p_color_start = glm::vec4(1.0f, 0.6f, 0.1f, 1.0f);
				p_color_end = glm::vec4(0.9f, 0.2f, 0.0f, 0.5f);
				p_scale_min = 0.45f; // Increased size
				p_scale_max = 0.85f;
				break;
			case SpellType::ICE:
				spellTypeName = "ICE";
				particles_to_spawn = 40;
				p_color_start = glm::vec4(0.5f, 0.8f, 1.0f, 1.0f);
				p_color_end = glm::vec4(0.2f, 0.5f, 0.8f, 0.3f);
				p_scale_min = 0.4f; // Increased size
				p_scale_max = 0.75f;
				newProj.speed = 12.0f; // Slower ice projectile
				break;
			case SpellType::LIGHTNING:
				spellTypeName = "LIGHTNING";
				particles_to_spawn = 50; // More particles for lightning
				p_color_start = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
				p_color_end = glm::vec4(0.8f, 0.8f, 0.2f, 0.3f);
				p_scale_min = 0.35f; // Slightly smaller but more numerous for lightning
				p_scale_max = 0.6f;
				newProj.speed = 20.0f; // Faster lightning projectile
				break;
			case SpellType::HEAL:
				spellTypeName = "HEAL";
				particles_to_spawn = 30; // Fewer particles for healing
				p_color_start = glm::vec4(0.2f, 1.0f, 0.2f, 1.0f);
				p_color_end = glm::vec4(0.2, 1.0f, 0.2f, 1.0f);
				p_scale_min = 0.35f; // Slightly smaller but more numerous for lightning
				p_scale_max = 0.6f;
				newProj.speed = 0.0f; // No speed for healing, just a visual effect
				player->setHitpoints(player->getHitpoints() + Config::ORB_HEAL_AMOUNT); // Heal the player
				break;
			case SpellType::NONE:
			default:
				cout << "[DEBUG] Cannot shoot: No valid spell type selected." << endl;
				if (!activeSpells.empty()) activeSpells.pop_back();
				if (!debugCamera) orbsCollectedCount++; // Refund orb if not in debug mode
				return;
			}
			cout << "[DEBUG] Firing " << spellTypeName << " spell." << endl;

			particleSystem->spawnParticleBurst(spawnPos,       // Use initial spawnPos for particles
				shootDir,       // Use initial shootDir for particles
				particles_to_spawn,
				current_particle_system_time,
				p_speed_min, p_speed_max,
				p_spread,
				p_lifespan_min, p_lifespan_max,
				p_color_start, p_color_end,
				p_scale_min, p_scale_max);
		}

		cout << "[DEBUG] Spell Fired! Start:(" << spawnPos.x << "," << spawnPos.y << "," << spawnPos.z
			<< ") Dir: (" << shootDir.x << "," << shootDir.y << "," << shootDir.z
			<< "). Active spells: " << activeSpells.size() << endl;
	}

	// --- updateProjectiles ---
	void updateProjectiles(float deltaTime) {

		float damageAmount = Config::PROJECTILE_DAMAGE;

		for (int i = 0; i < activeSpells.size(); i++) {
			if (!activeSpells[i].active) {
				i++;
				continue;
			}

			SpellProjectile& proj = activeSpells[i];

			if (glfwGetTime() - proj.spawnTime > proj.lifetime) {
				proj.active = false;
				activeSpells.erase(activeSpells.begin() + i);
				continue;
			}

			proj.position += proj.direction * proj.speed * deltaTime;
			proj.transform = glm::translate(glm::mat4(1.0f), proj.position);

			this->updateBoundingBox(proj.localAABBMin_logical, proj.localAABBMax_logical, proj.transform, proj.aabbMin, proj.aabbMax);

			bool hitSomething = false;
			for (auto* enemy : enemies) {
				if (!enemy || !enemy->isAlive()) continue;

				glm::vec3 enemyMin = glm::vec3(enemy->getAABBMin().x, enemy->getAABBMin().y - 1.0f, enemy->getAABBMin().z);
				glm::vec3 enemyMax = glm::vec3(enemy->getAABBMax().x, enemy->getAABBMax().y + 1.0f, enemy->getAABBMax().z);
				if (checkAABBCollision(proj.aabbMin, proj.aabbMax, enemyMin, enemyMax)) {
					cout << "[DEBUG] Fireball HIT enemy!" << endl;
					enemy->takeDamage(damageAmount);
					proj.active = false;
					hitSomething = true;
					break;
				}
			}
			if (hitSomething) {
				activeSpells.erase(activeSpells.begin() + i);
				continue;
			}

			if (canFightboss && bossEnemy && bossEnemy->isAlive()) {
				if (checkAABBCollision(proj.aabbMin, proj.aabbMax, bossEnemy->getAABBMin(), bossEnemy->getAABBMax())) {
					cout << "[DEBUG] Fireball HIT boss!" << endl;
					float bossDamage = 100.0f;
					bossEnemy->takeDamage(bossDamage);
					proj.active = false;
					activeSpells.erase(activeSpells.begin() + i);
					continue;
				}
			}
		}
	}

	void drawProjectiles(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		// This function is now empty as particles handle visuals.
		// This function is now empty as particles handle visuals for boss fireballs too.
		if (!shader || !Model || !sphere) return; // Need shader, stack, model

		shader->bind();
		// Set material for projectiles (e.g., bright yellow/white, maybe emissive if shader supports)
		SetMaterial(shader, Material::gold);
		// Optional: Emissive properties if shader supports them
		// if(shader->hasUniform("hasEmittance")) glUniform1i(shader->getUniform("hasEmittance"), 1);
		// if(shader->hasUniform("MatEmitt")) glUniform3f(shader->getUniform("MatEmitt"), 1.0f, 1.0f, 0.8f);

		for (const auto& proj : activeSpells) {
			if (!proj.active) continue;
			float current_particle_system_time = particleSystem->getCurrentTime();

			float p_speed_min = 0.05f;
			float p_speed_max = 0.1f;
			float p_spread = 1.5f;
			// lifespans  short so they die quickly and are recycled for other effects
			float p_lifespan_min = 0.2f;
			float p_lifespan_max = 0.6f;

			// Base particle color (TODO: can be tweaked, maybe slightly transparent)
			glm::vec4 p_color_start;
			glm::vec4 p_color_end;
			float p_scale_min = 0.1f;
			float p_scale_max = 0.25f;

			int current_particles_to_spawn = 2; // Set a fixed number of particles for all orbs
			// Customize particle aura based on spell type
			switch (currentPlayerSpellType) {
				case SpellType::FIRE:
					// current_particles_to_spawn = 15; // Increased for density with short life
					p_color_start = glm::vec4(1.0f, 0.5f, 0.1f, 0.8f);
					p_color_end = glm::vec4(0.9f, 0.2f, 0.0f, 0.3f);
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
				case SpellType::ICE:
					// current_particles_to_spawn = 15; // Increased for density
					p_color_start = glm::vec4(0.5f, 0.8f, 1.0f, 0.8f);
					p_color_end = glm::vec4(0.2f, 0.5f, 0.8f, 0.3f);
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
				case SpellType::LIGHTNING:
					// current_particles_to_spawn = 15; // Increased for density
					p_color_start = glm::vec4(1.0f, 1.0f, 0.5f, 0.8f);
					p_color_end = glm::vec4(0.8f, 0.8f, 0.2f, 0.3f);
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
				default:
					// current_particles_to_spawn is 15 (standardized)
					// p_color_start and p_color_end use orb.color
					// p_lifespan_min/max are standardized
					// Make scales consistent with other types:
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
			}
			particleSystem->spawnParticleBurst(proj.position, // Emit from orb center
												glm::vec3(0,1,0), // Emit upwards slowly or randomly
												current_particles_to_spawn,
												current_particle_system_time,
												p_speed_min, p_speed_max,
												p_spread,
												p_lifespan_min, p_lifespan_max,
												p_color_start, p_color_end,
												p_scale_min, p_scale_max);

			Model->pushMatrix();
			Model->loadIdentity(); // Start from identity for projectile

			// Use the pre-calculated transform from updateAABB
			Model->multMatrix(proj.transform);
			Model->scale(0.15f);

			setModel(shader, Model);
			sphere->Draw(shader); // Draw the sphere model

			Model->popMatrix();
		}

		shader->unbind();
	}

	/* boss projectiles */
	void drawBossProjectiles(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		// This function is now empty as particles handle visuals for boss fireballs too.
		if (!shader || !Model || !sphere) return; // Need shader, stack, model

		shader->bind();
		// Set material for projectiles (e.g., bright yellow/white, maybe emissive if shader supports)
		SetMaterial(shader, Material::gold);
		// Optional: Emissive properties if shader supports them
		// if(shader->hasUniform("hasEmittance")) glUniform1i(shader->getUniform("hasEmittance"), 1);
		// if(shader->hasUniform("MatEmitt")) glUniform3f(shader->getUniform("MatEmitt"), 1.0f, 1.0f, 0.8f);

		for (const auto& proj : bossActiveSpells) {
			if (!proj.active) continue;
			float current_particle_system_time = particleSystem->getCurrentTime();

			float p_speed_min = 0.05f;
			float p_speed_max = 0.1f;
			float p_spread = 1.5f;
			// lifespans  short so they die quickly and are recycled for other effects
			float p_lifespan_min = 0.3f;
			float p_lifespan_max = 0.6f;

			// Base particle color (TODO: can be tweaked, maybe slightly transparent)
			glm::vec4 p_color_start;
			glm::vec4 p_color_end;
			float p_scale_min = 0.1f;
			float p_scale_max = 0.25f;

			int current_particles_to_spawn = 3; // Set a fixed number of particles for all orbs
			// Customize particle aura based on spell type
			switch (bossEnemy->getBossSpellType()) {
				case SpellType::FIRE:
					// current_particles_to_spawn = 15; // Increased for density with short life
					p_color_start = glm::vec4(1.0f, 0.5f, 0.1f, 0.8f);
					p_color_end = glm::vec4(0.9f, 0.2f, 0.0f, 0.3f);
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
				case SpellType::ICE:
					// current_particles_to_spawn = 15; // Increased for density
					p_color_start = glm::vec4(0.5f, 0.8f, 1.0f, 0.8f);
					p_color_end = glm::vec4(0.2f, 0.5f, 0.8f, 0.3f);
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
				case SpellType::LIGHTNING:
					// current_particles_to_spawn = 15; // Increased for density
					p_color_start = glm::vec4(1.0f, 1.0f, 0.5f, 0.8f);
					p_color_end = glm::vec4(0.8f, 0.8f, 0.2f, 0.3f);
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
				default:
					// current_particles_to_spawn is 15 (standardized)
					// p_color_start and p_color_end use orb.color
					// p_lifespan_min/max are standardized
					// Make scales consistent with other types:
					p_scale_min = 0.25f;
					p_scale_max = 0.45f;
					break;
			}
			particleSystem->spawnParticleBurst(proj.position, // Emit from orb center
												glm::vec3(0,1,0), // Emit upwards slowly or randomly
												current_particles_to_spawn,
												current_particle_system_time,
												p_speed_min, p_speed_max,
												p_spread,
												p_lifespan_min, p_lifespan_max,
												p_color_start, p_color_end,
												p_scale_min, p_scale_max);

			Model->pushMatrix();
			Model->loadIdentity(); // Start from identity for projectile

			// Use the pre-calculated transform from updateAABB
			Model->multMatrix(proj.transform);
			Model->scale(0.15f);

			setModel(shader, Model);
			sphere->Draw(shader); // Draw the sphere model

			Model->popMatrix();
		}

		shader->unbind();

	}

	void updateBossProjectiles(float deltaTime) {
		// if (!sphereAABBCalculated) return; // Not needed for logical projectiles

		float damageAmount = 25.0f; // Damage from boss fireball

		for (int i = 0; i < bossActiveSpells.size(); ) {
			if (!bossActiveSpells[i].active) {
				i++;
				continue;
			}

			SpellProjectile& proj = bossActiveSpells[i];
			proj.setLifetime(8.0f);

			if (glfwGetTime() - proj.spawnTime > proj.lifetime) {
				proj.active = false;
				bossActiveSpells.erase(bossActiveSpells.begin() + i);
				continue;
			}

			proj.position += proj.direction * proj.speed * deltaTime;

			proj.transform = glm::translate(glm::mat4(1.0f), proj.position);

			this->updateBoundingBox(proj.localAABBMin_logical, proj.localAABBMax_logical, proj.transform, proj.aabbMin, proj.aabbMax);

			bool hitSomething = false;

			glm::vec3 playerCenter = player->getPosition() + glm::vec3(0, 1.0f, 0); // Approx player center
			float playerRadius = 0.5f; // Approx player radius

			if (checkSphereCollision(player->getPosition(), 1.5f, proj.aabbMin, proj.aabbMax)) { // Simple sphere check: proj vs player
				cout << "[DEBUG] Boss Spell HIT player!" << endl;
				player->takeDamage(damageAmount);
				proj.active = false;
				hitSomething = true;
				continue;
			}

			std::vector<const QuadElement*> nearby_objects;
			bossRoomQuadTree->query(glm::vec2(proj.position.x, proj.position.z), glm::vec2(0.5f, 0.5f), nearby_objects);
			for (const auto* e : nearby_objects) {
				if (checkSphereCollision(proj.position, 0.5f, e->aabb_min, e->aabb_max)) {
					cout << "[DEBUG] Boss Spell HIT shelf!" << endl;
					proj.active = false;
					hitSomething = true;
					break; // Stop checking after first hit
				}
			}

			if (hitSomething) {
				bossActiveSpells.erase(bossActiveSpells.begin() + i);
				continue;
			}
			i++;
		}
	}

	void shootBossSpell() {
		// vec3 shootDir = bossEnemy->getBossDirection();
		float upOffset = 7.0f;      // Height relative to player base (groundY)	

		// make projectile aim towards player
		vec3 shootDir = normalize(player->getPosition() - vec3(bossEnemy->getPosition().x, bossEnemy->getPosition().y + upOffset, bossEnemy->getPosition().z));

		vec3 bossRight = normalize(cross(shootDir, vec3(0.0f, 1.0f, 0.0f)));

		float forwardOffset = 0.5f; // How far in front of player center
		
		float rightOffset = 0.0f;   // Offset to the side (e.g., right hand)

		vec3 spawnPos = bossEnemy->getPosition()
			+ vec3(0.0f, upOffset, 0.0f) // Vertical offset from base
			+ shootDir * forwardOffset   // Forward offset along character's facing direction
			+ bossRight * rightOffset; // Sideways offset along character's right

		// Create and add projectile (now uses the 3-argument constructor)
		bossActiveSpells.emplace_back(spawnPos, shootDir, (float)glfwGetTime());
		SpellProjectile& newProj = bossActiveSpells.back();

		if (particleSystem) {
            float current_particle_system_time = particleSystem->getCurrentTime();
            int particles_to_spawn = 5;

            float p_speed_min = newProj.speed * 0.2f;
            float p_speed_max = newProj.speed * 0.5f;
            float p_spread = 0.6f;
            float p_lifespan_min = 0.2f;
            float p_lifespan_max = 0.8f;
            glm::vec4 p_color_start = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            glm::vec4 p_color_end = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            float p_scale_min = 0.2f;
            float p_scale_max = 0.4f;

            // Use bossEnemy->getBossSpellType() to determine visuals
            std::string spellTypeName = "NONE";
            switch (bossEnemy->getBossSpellType()) {
                case SpellType::FIRE:
                    spellTypeName = "FIRE";
                    particles_to_spawn = 40; // Increased count
                    p_color_start = glm::vec4(1.0f, 0.6f, 0.1f, 1.0f);
                    p_color_end = glm::vec4(0.9f, 0.2f, 0.0f, 0.5f);
                    p_scale_min = 0.45f; // Increased size
                    p_scale_max = 0.85f;
                    break;
                case SpellType::ICE:
                    spellTypeName = "ICE";
                    particles_to_spawn = 40;
                    p_color_start = glm::vec4(0.5f, 0.8f, 1.0f, 1.0f);
                    p_color_end = glm::vec4(0.2f, 0.5f, 0.8f, 0.3f);
                    p_scale_min = 0.4f; // Increased size
                    p_scale_max = 0.75f;
                    newProj.speed = 12.0f; // Slower ice projectile
                    break;
                case SpellType::LIGHTNING:
                    spellTypeName = "LIGHTNING";
                    particles_to_spawn = 50; // More particles for lightning
                    p_color_start = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
                    p_color_end = glm::vec4(0.8f, 0.8f, 0.2f, 0.3f);
                    p_scale_min = 0.35f; // Slightly smaller but more numerous for lightning
                    p_scale_max = 0.6f;
                    newProj.speed = 20.0f; // Faster lightning projectile
                    break;
                case SpellType::NONE:
                default:
                    cout << "[DEBUG] Cannot shoot: No valid spell type selected." << endl;
                    if (!bossActiveSpells.empty()) bossActiveSpells.pop_back();
                    if (!debugCamera) orbsCollectedCount++; // Refund orb if not in debug mode
                    return;
            }
            cout << "[DEBUG] Firing " << spellTypeName << " spell." << endl;

            particleSystem->spawnParticleBurst(spawnPos,       // Use initial spawnPos for particles
                                             shootDir,       // Use initial shootDir for particles
                                             particles_to_spawn,
                                             current_particle_system_time,
                                             p_speed_min, p_speed_max,
                                             p_spread,
                                             p_lifespan_min, p_lifespan_max,
                                             p_color_start, p_color_end,
                                             p_scale_min, p_scale_max);
        }

		cout << "[DEBUG] Spell Fired! Start:(" << spawnPos.x << "," << spawnPos.y << "," << spawnPos.z
			<< ") Dir: (" << shootDir.x << "," << shootDir.y << "," << shootDir.z
			<< "). Active spells: " << bossActiveSpells.size() << endl;
	}

// Boss slam attack logic
	struct slamState {
		bool active = false;
		float elapsed = 0.0f;
		float duration = 2.0f;
		float aoeRadius = 5.0f;
		glm::vec3 startPos;
		glm::vec3 targetPos;
	};

	struct slamState slamState; // Global state for the slam attack

	void bossSlamAttack() {
		if (!bossEnemy || !bossEnemy->isAlive()) return;

		// Set slam state
		slamState.active = true;
		slamState.elapsed = 0.0f;
		slamState.duration = 2.0f;
		slamState.aoeRadius = 5.0f;

		slamState.startPos = bossEnemy->getPosition();

		// Reset slam logic
		slamState.targetPos = player->getPosition();
		bossEnemy->setSlamDuration(slamState.duration);
		bossEnemy->setSlamCooldown(0.0f);
		cout << "[DEBUG] Boss slam attack initiated." << endl;
	}

	void updateBossSlamAttack(float deltaTime) {
	if (!slamState.active || !bossEnemy || !bossEnemy->isAlive()) return;

	slamState.elapsed += deltaTime;

	float t = glm::clamp(slamState.elapsed / slamState.duration, 0.0f, 1.0f);

	// Parabolic trajectory
	glm::vec3 horizontal = glm::mix(slamState.startPos, slamState.targetPos, t);
	float peakHeight = 6.0f;
	float heightOffset = peakHeight * sin(glm::pi<float>() * t);
	glm::vec3 newPos = horizontal + glm::vec3(0.0f, heightOffset, 0.0f);
	bossEnemy->setPosition(newPos);

	// End of attack
	if (slamState.elapsed >= slamState.duration) {
		glm::vec3 playerPos = player->getPosition();
		float dist = glm::distance(playerPos, slamState.targetPos);

		if (dist < slamState.aoeRadius) {
			player->takeDamage(Config::BOSS_SLAM_DAMAGE); // AOE damage to player
			cout << "[DEBUG] Player hit by boss slam!" << endl;
		} else {
			cout << "[DEBUG] Player dodged boss slam." << endl;
		}

		// Reset slam state
		slamState.active = false;
		bossEnemy->setSlamCooldown(glfwGetTime());
	}
}



	void BossEnemyAttacks(float deltaTime) {
		if (bossEnemy && bossfightstarted && !bossfightended && bossEnemy->isAlive()) {
			bossEnemy->changePhase(); // Update boss phase logic
			switch (bossEnemy->getPhase()) {
				case BossEnemy::BossPhase::PHASE_1:
					// shooting spell at player
					if (glfwGetTime() - bossEnemy->getSpecialAttackCooldown() > 2.0f) {
						bossEnemy->setSpecialAttackCooldown(glfwGetTime());
						shootBossSpell();
					}
					updateBossProjectiles(deltaTime);
					break;
				case BossEnemy::BossPhase::PHASE_2:
					// Phase 2 logic, e.g., spawning minions
					if (glfwGetTime() - bossEnemy->getAttack1Cooldown() > 4.0f) {
						glm::vec2 spawnPos = bossRoom->getOpenPosinBossRoom();
						bossEnemy->setAttack1Cooldown(glfwGetTime());

						if (activeEnemiesCount <= 5) {
							IceElemental* minion = new IceElemental(vec3(spawnPos.x, Config::ICE_ELEMENTAL_TRANS_Y, spawnPos.y), ENEMY_HP_MAX, 2.0f, iceElemental, vec3(0.65f), vec3(0.0f));
							minion->setAggro(true);
							minion->setSightRange(20.0f);
							enemies.push_back(minion);
						}
					}
					updateBossProjectiles(deltaTime);
					break;
				case BossEnemy::BossPhase::PHASE_3:
					// Phase 3 logic, phase 1 and 2 combined but more aggressive, add slam attack
					if (glfwGetTime() - bossEnemy->getSpecialAttackCooldown() > 1.0f) {
						bossEnemy->setSpecialAttackCooldown(glfwGetTime());
						shootBossSpell();
					}
					if (glfwGetTime() - bossEnemy->getAttack1Cooldown() > 3.0f) {
						glm::vec2 spawnPos = bossRoom->getOpenPosinBossRoom();
						bossEnemy->setAttack1Cooldown(glfwGetTime());

						if (activeEnemiesCount <= 5) {
							enemies.push_back(new IceElemental(vec3(spawnPos.x, Config::ICE_ELEMENTAL_TRANS_Y, spawnPos.y), ENEMY_HP_MAX, 2.0f, iceElemental, vec3(0.65f), vec3(0.0f)));
						}
					}
					if (!slamState.active && (glfwGetTime() - bossEnemy->getSlamCooldown() > Config::BOSS_SLAM_COOLDOWN)) {
						bossSlamAttack();
					}

					updateBossProjectiles(deltaTime);
					updateBossSlamAttack(deltaTime);
					break;
				default:
					cout << "[DEBUG] Boss phase not recognized or unsupported." << endl;
					break;
			}


		}

		if (bossfightended) {
			enemies.clear(); // Clear all enemies when boss fight ends
		}
	}

	/* top down camera view  */
	mat4 SetTopView(shared_ptr<Program> curShade) { /*MINI MAP*/
		mat4 Cam = glm::lookAt(eye + vec3(0, 12, 0), eye, lookAt - eye);
		glUniformMatrix4fv(curShade->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
		return Cam;
	}

	mat4 SetOrthoMatrix(shared_ptr<Program> curShade) {/*MINI MAP*/
		float wS = 1.5;
		mat4 ortho = glm::ortho(-15.0f * wS, 15.0f * wS, -15.0f * wS, 15.0f * wS, 2.1f, 100.f);
		glUniformMatrix4fv(curShade->getUniform("P"), 1, GL_FALSE, value_ptr(ortho));
		return ortho;
  }

	void drawMiniPlayer(shared_ptr<Program> curS, shared_ptr<MatrixStack> Model) { /*MINI MAP*/
		//sphere->Draw(shader);
		curS->bind();

		// Model matrix setup
		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(player->getPosition()); // Use final player position
		// *** USE CAMERA ROTATION FOR MODEL ***
		// Model->rotate(manRot.y, vec3(0, 1, 0)); // <<-- FIXED ROTATION
		Model->scale(1.0);

		// Update VISUAL bounding box (can be different from collision box if needed)
		// Using the same AABB calculation logic as before for consistency
		glm::mat4 manTransform = Model->topMatrix();
		updateBoundingBox(player_rig->getBoundingBoxMin(),
			player_rig->getBoundingBoxMax(),
			manTransform,
			playerBB->min, // This is the visual/interaction AABB
			playerBB->max);

		// Set uniforms and draw
		//glUniform1i(curS->getUniform("hasTexture"), 1); //0.6f, 0.2f, 0.8f
		//0.8f, 0.4f, 0.2f
		// 0.95, 0.78, 0.14
		/*glUniform3f(curS->getUniform("MatAmb"), 0.95f, 0.78f, 0.14f);
		glUniform3f(curS->getUniform("MatDif"), 0.95f, 0.78f, 0.14f);
		glUniform3f(curS->getUniform("MatSpec"), 0.3f, 0.3f, 0.3f);
		glUniform1f(curS->getUniform("MatShine"), 8.0f);*/
		SetMaterial(curS, Material::gold);
		setModel(curS, Model);
		//player_rig->Draw(curS);
		sphere->Draw(curS);

		Model->popMatrix();
		curS->unbind();
	}

	void drawHealthBar() {
		float heatlhBarWidth = 350.0f;
		float healthBarHeight = 25.0f;
		float healthBarStartX = 100.0f;
		float healthBarStartY = 100.0f;
		int screenWidth, screenHeight;
		glfwGetFramebufferSize(windowManager->getHandle(), &screenWidth, &screenHeight);

		glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);

		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(healthBarStartX, healthBarStartY, 0.0f));  // HUD position
		model = glm::scale(model, glm::vec3(heatlhBarWidth, healthBarHeight, 1.0f));                          // HUD size

		hudProg->bind();
		glUniformMatrix4fv(hudProg->getUniform("projection"), 1, GL_FALSE, value_ptr(projection));
		glUniformMatrix4fv(hudProg->getUniform("model"), 1, GL_FALSE, value_ptr(model));
		glUniform1f(hudProg->getUniform("healthPercent"), player->getHitpoints() / Config::PLAYER_HP_MAX); // Pass health value
		glUniform1f(hudProg->getUniform("BarStartX"), healthBarStartX); // Pass max health value
		glUniform1f(hudProg->getUniform("BarWidth"), heatlhBarWidth); // Pass max health value

		healthBar->Draw(hudProg);
		hudProg->unbind();
	}

	// Draw particles
	void drawParticles(shared_ptr<particleGen> gen, shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		// Model->pushMatrix(); // Original push
		shader->bind();
		particleAlphaTex->bind(shader->getUniform("alphaTexture"));

		// glEnable(GL_BLEND); // gen->drawMe() handles its own GL state (blend, depth)
		// glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Original: GL_ONE. gen->drawMe() uses GL_SRC_ALPHA, GL_ONE

		// Disable depth writing but keep depth testing; gen->drawMe() handles this
		//glDepthMask(GL_FALSE);

		// Set Model matrix to identity for world-space particles
		// The Model stack is passed in, so push, load identity, then pop to keep it clean for the stack
		Model->pushMatrix(); {
			Model->loadIdentity();
			glUniformMatrix4fv(shader->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix())); // M is now identity
			gen->drawMe(shader); // gen->drawMe will set its own blend/depth states and draw
		} Model->popMatrix(); // Restore original Model stack state
		// Restore state --- gen->drawMe() handles its own GL state restoration
		//glDepthMask(GL_TRUE);
		// glDisable(GL_BLEND); // gen->drawMe() handles this

		particleAlphaTex->unbind();
		shader->unbind();
		// Model->popMatrix(); // Original pop
	}

	void drawEnemyHealthBars(glm::mat4 viewMatrix, glm::mat4 projMatrix) {
		float healthBarWidth = 100.0f;
		float healthBarHeight = 10.0f;
		float healthBarOffsetY = 15.0f;  // Offset above enemy head

		int screenWidth, screenHeight;
		glfwGetFramebufferSize(windowManager->getHandle(), &screenWidth, &screenHeight);

		glm::mat4 hudProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);

		for (auto* enemy : enemies) {
			if (!enemy || !enemy->isAlive() || (!enemy->isHit())) continue;

			glm::vec3 enemyWorldPos = enemy->getAABBMax(); // Top position in world coordinates

			// Transform enemy position to clip space
			glm::vec4 clipSpacePos = projMatrix * viewMatrix * glm::vec4(enemyWorldPos, 1.0f);

			// If enemy is behind camera, skip
			if (clipSpacePos.w <= 0) continue;

			// Perspective divide (NDC)
			glm::vec3 ndcPos = glm::vec3(clipSpacePos) / clipSpacePos.w;

			// Convert NDC (-1 to 1) to screen coordinates
			glm::vec2 screenPos;
			screenPos.x = (ndcPos.x * 0.5f + 0.5f) * screenWidth;
			screenPos.y = (ndcPos.y * 0.5f + 0.5f) * screenHeight;

			// Offset above enemy's head
			screenPos.y += healthBarOffsetY;

			// Set HUD Model matrix
			glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(screenPos.x - (healthBarWidth / 2.0f), screenPos.y, 0.0f));
			model = glm::scale(model, glm::vec3(healthBarWidth, healthBarHeight, 1.0f));

			hudProg->bind();
			glUniformMatrix4fv(hudProg->getUniform("projection"), 1, GL_FALSE, glm::value_ptr(hudProjection));
			glUniformMatrix4fv(hudProg->getUniform("model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform1f(hudProg->getUniform("healthPercent"), enemy->getHitpoints() / ENEMY_HP_MAX);
			glUniform1f(hudProg->getUniform("BarStartX"), screenPos.x - (healthBarWidth / 2.0f));
			glUniform1f(hudProg->getUniform("BarWidth"), healthBarWidth);

			healthBar->Draw(hudProg);
			hudProg->unbind();
		}
	}

	void drawLock(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		//need models
		shader->bind();


		//top lock

		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 2.5f, 38.5f));
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 2.5f, bossEntrancePos.z));  //doorPosition
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->scale(0.1f);
			SetMaterial(shader, Material::gold); //gold
			setModel(shader, Model);
			lock->Draw(shader);
			lockHandle->Draw(shader);
		Model->popMatrix();

		//middle lock
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 1.5f, 38.5f));  //doorPosition
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 1.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->scale(0.1f);
			SetMaterial(shader, Material::gold); //gold
			setModel(shader, Model);
			lock->Draw(shader);
			lockHandle->Draw(shader);
		Model->popMatrix();

		//lower lock
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 0.5f, 38.5f));
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 0.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->scale(0.1f);
			SetMaterial(shader, Material::gold); //gold
			setModel(shader, Model);
			lock->Draw(shader);
			lockHandle->Draw(shader);
		Model->popMatrix();

		shader->unbind();


	}

	void updateLock(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model){
		//unlock one of the locks if have a key
		//for now unlock all

		shader->bind();

		/*


		Model->pushMatrix();
			Model->loadIdentity();
			Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			//Model->scale(0.1f);
			SetMaterialMan(shader, 5); //gold

			//top lock
			Model->pushMatrix();
				Model->translate(vec3(0.0f, 2.5f, 38.5f));
				Model->scale(0.1f);
				setModel(shader, Model);
				lock->Draw(shader);
			Model->popMatrix();
			//top handle
			Model->pushMatrix();
				Model->translate(vec3(0.0f, 2.5f, 38.5f));
				Model->scale(0.1f);
				Model->rotate( 1* glm::radians(15.0) + lTheta , vec3(0.0f, 0.0f, 1.0f)); //max -30?
				setModel(shader, Model);
				lockHandle->Draw(shader);
			Model->popMatrix();

			//middle lock
			Model->pushMatrix();
				Model->translate(vec3(0.0f, 1.5f, 38.5f));
				Model->scale(0.1f);
				setModel(shader, Model);
				lock->Draw(shader);
			Model->popMatrix();

			//middle handle
			Model->pushMatrix();
				Model->translate(vec3(0.0f, 1.5f, 38.5f));
				Model->scale(0.1f);
				Model->rotate( 1* glm::radians(15.0) + lTheta , vec3(0.0f, 0.0f, 1.0f));
				setModel(shader, Model);
				lockHandle->Draw(shader);
			Model->popMatrix();

			//bottom lock
			Model->pushMatrix();
				Model->translate(vec3(0.0f, 0.5f, 38.5f));
				Model->scale(0.1f);
				setModel(shader, Model);
				lock->Draw(shader);
			Model->popMatrix();

			//bottom handle
			Model->pushMatrix();
				Model->translate(vec3(0.0f, 0.5f, 38.5f));
				Model->scale(0.1f);
				Model->rotate( 1* glm::radians(15.0) + lTheta , vec3(0.0f, 0.0f, 1.0f));
				setModel(shader, Model);
				lockHandle->Draw(shader);
			Model->popMatrix();


		Model->popMatrix();

		*/


		//top lock
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 2.5f, 38.5f));  //doorPosition
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 2.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->scale(0.1f);
			SetMaterial(shader, Material::gold); //gold
			setModel(shader, Model);
			lock->Draw(shader);
		Model->popMatrix();

		//top handle
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 2.5f, 38.5f));
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 2.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->rotate(1 * glm::radians(15.0) + lTheta, vec3(0.0f, 0.0f, 1.0f)); //max -30?
			Model->scale(0.1f);
			// Model->rotate(  glm::radians(90.0) , vec3(0.0f, 1.0f, 0.0f)); //max -30
			SetMaterial(shader, Material::brown); //brown
			setModel(shader, Model);
			lockHandle->Draw(shader);
		Model->popMatrix();

		//middle lock
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 1.5f, 38.5f));
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 1.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->scale(0.1f);
			SetMaterial(shader, Material::gold); //gold
			setModel(shader, Model);
			lock->Draw(shader);
		Model->popMatrix();

		//midle handle
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 1.5f, 38.5f));
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 1.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->rotate(1 * glm::radians(15.0) + lTheta, vec3(0.0f, 0.0f, 1.0f)); //max -30?
			Model->scale(0.1f);
			// Model->rotate(  glm::radians(90.0) , vec3(0.0f, 1.0f, 0.0f)); //max -30
			SetMaterial(shader, Material::brown); //brown
			setModel(shader, Model);
			lockHandle->Draw(shader);
		Model->popMatrix();

		// lower lock
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 0.5f, 38.5f));
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 0.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->scale(0.1f);
			SetMaterial(shader, Material::gold); //gold
			setModel(shader, Model);
			lock->Draw(shader);
		Model->popMatrix();

		//lower handle
		Model->pushMatrix();
			Model->loadIdentity();
			// Model->translate(vec3(0.0f, 0.5f, 38.5f));
			Model->translate(vec3(bossEntrancePos.x, bossEntrancePos.y + 0.5f, bossEntrancePos.z));
			Model->rotate(glm::radians(bossEntranceRot), vec3(0.0f, 1.0f, 0.0f));
			// Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
			Model->rotate(1 * glm::radians(15.0) + lTheta, vec3(0.0f, 0.0f, 1.0f)); //max -30?
			Model->scale(0.1f);
			// Model->rotate(  glm::radians(90.0) , vec3(0.0f, 1.0f, 0.0f)); //max -30
			SetMaterial(shader, Material::brown); //brown
			setModel(shader, Model);
			lockHandle->Draw(shader);
		Model->popMatrix();

		// if(lTheta < 30.0){
		// 	lTheta+= 0.1;
		// lTheta = sin(glfwGetTime());
		// }

	// Model->pushMatrix();
	// 	Model->loadIdentity();
	// 	Model->translate(vec3(0.0f, 0.5f, 38.5f));  //doorPosition
	// 	Model->rotate(glm::radians(180.0f), vec3(0.0f, 1.0f, 0.0f));
	// 	Model->rotate( glm::radians(lTheta) , vec3(0.0f, 1.0f, 0.0f)); //max -30?
	// 	Model->scale(0.1f);
	// 	SetMaterialMan(shader, 6); //brown
	// 	setModel(shader, Model);
	// 	lockHandle->Draw(shader);
	// Model->popMatrix();

		shader->unbind();
	}

	//drawOrb, draw book , updateBooks, updateOrb, shootSpell

	//glm::vec3 enemyPos = enemy->getPosition();
	//enemy->isAlive() == false

	/* keyCollect */
	void drawKey(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {

		// --- Collision Check Logic ---
		for (auto& key : keyCollectibles) {
			// Perform collision check ONLY if not collected AND in the IDLE state
			if (!key.collected && key.state == OrbState::IDLE && // <<<--- ADD STATE CHECK
				//checkAABBCollision(manAABBmin, manAABBmax, key.AABBmin, key.AABBmax)
				checkSphereCollision(player->getPosition(), 4.0f, key.AABBmin, key.AABBmax)
				//checkSphereCollision(player->getPosition(), 2.0f, orb.AABBmin, orb.AABBmax)

				) {
				key.collected = true;
				// key.state = OrbState::COLLECTED; // Optionally set state
				keysCollectedCount++;
				std::cout << "Collected a key! (" << keysCollectedCount << ")\n";
			}
		}

		int collectedKeyDrawIndex = 0;
		shader->bind();
		if (!unlock) {
			for (auto& key : keyCollectibles) {
				glm::vec3 currentDrawPosition;
				//float currentDrawScale = key.scale; // Use base scale
				if (key.collected) {
					// Calculate position behind the player (same logic as before)
					float backOffset = 0.4f;
					float upOffsetBase = 0.6f;
					float stackOffset = key.scale * 2.5f;
					float sideOffset = 0.15f;
					glm::vec3 playerForward = normalize(manMoveDir);
					glm::vec3 playerUp = glm::vec3(0.0f, 1.0f, 0.0f);
					glm::vec3 playerRight = normalize(cross(playerForward, playerUp));
					float currentUpOffset = upOffsetBase + (collectedKeyDrawIndex * stackOffset);
					float currentSideOffset = (collectedKeyDrawIndex % 2 == 0 ? -sideOffset : sideOffset);
					currentDrawPosition = charMove() - playerForward * backOffset
						+ playerUp * currentUpOffset
						+ playerRight * currentSideOffset;
					collectedKeyDrawIndex++;
				}
				else {
					currentDrawPosition = key.position; // Use the orb's current position (potentially animated by updateOrbs)
				}

				// std::cout << "key position " << key.position.x << " " << key.position.y << " " << key.position.z << " " << std::endl;

				// --- Set up transformations ---
				Model->pushMatrix(); {
					Model->loadIdentity();
					Model->translate(currentDrawPosition); //last enemy pos
					Model->rotate(glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
					Model->rotate(glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
					Model->scale(2.0f);


					// --- Set Material & Draw ---
					SetMaterial(shader, Material::gold); //gold
					setModel(shader, Model);
					key.model->Draw(shader);
				} Model->popMatrix();
			} // End drawing loop
		}
		shader->unbind();
	}

	void updateKeys(float currentTime) {
		// for (auto* enemy : enemies) {
		// 	if (!enemy->isAlive() && !enemy->dropSpawned) {
		// 		glm::vec3 keyPos = enemy->getPosition();
		// 		// keyPos.y -= 1.5f; // Adjust height for key position
		// 		keyCollectibles.emplace_back(key, keyPos, 0.1f, Material::key_color, SpellType::NONE);
		// 		enemy->setDropSpawned(true); // Mark that the key has been spawned
		// 	}
		// }
		for (auto& key : keyCollectibles) {
			// Update levitation only if not already collected
			if (!key.collected) {
				key.updateLevitation(currentTime);
			}
		}
	}

	void removeKeys(){
		if(keysCollectedCount <= 0){
			cout << "[DEBUG] Cannot remove: No keys." << endl;
			return;
		}

		keysCollectedCount--;
		for(auto it = keyCollectibles.begin(); it != keyCollectibles.end(); ++it){
			if (it->collected){
				keyCollectibles.erase(it);
				break;
			}
		}

	}

	void drawBossHealthBar(glm::mat4 viewMatrix, glm::mat4 projMatrix) {
		float healthBarWidth = 200.0f;
		float healthBarHeight = 20.0f;
		float healthBarOffsetY = 25.0f;  // Offset above enemy head

		int screenWidth, screenHeight;
		glfwGetFramebufferSize(windowManager->getHandle(), &screenWidth, &screenHeight);

		glm::mat4 hudProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);

		if (bossEnemy && bossEnemy->isAlive()) {
			glm::vec3 enemyWorldPos = bossEnemy->getAABBMax(); // Top position in world coordinates

			// Transform enemy position to clip space
			glm::vec4 clipSpacePos = projMatrix * viewMatrix * glm::vec4(enemyWorldPos, 1.0f);

			// If enemy is behind camera, skip
			if (clipSpacePos.w <= 0) return;

			// Perspective divide (NDC)
			glm::vec3 ndcPos = glm::vec3(clipSpacePos) / clipSpacePos.w;

			// Convert NDC (-1 to 1) to screen coordinates
			glm::vec2 screenPos;
			screenPos.x = (ndcPos.x * 0.5f + 0.5f) * screenWidth;
			screenPos.y = (ndcPos.y * 0.5f + 0.5f) * screenHeight;

			// Offset above enemy's head
			screenPos.y += healthBarOffsetY;

			// Set HUD Model matrix
			glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(screenPos.x - (healthBarWidth / 2.0f), screenPos.y, 0.0f));
			model = glm::scale(model, glm::vec3(healthBarWidth, healthBarHeight, 1.0f));

			hudProg->bind();
			glUniformMatrix4fv(hudProg->getUniform("projection"), 1, GL_FALSE, glm::value_ptr(hudProjection));
			glUniformMatrix4fv(hudProg->getUniform("model"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform1f(hudProg->getUniform("healthPercent"), bossEnemy->getHitpoints() / BOSS_HP_MAX);
			glUniform1f(hudProg->getUniform("BarStartX"), screenPos.x - (healthBarWidth / 2.0f));
			glUniform1f(hudProg->getUniform("BarWidth"), healthBarWidth);
			healthBar->Draw(hudProg);
			hudProg->unbind();
		}
	}

	void drawDamageIndicator(float alpha) {
		int screenWidth, screenHeight;
		glfwGetFramebufferSize(windowManager->getHandle(), &screenWidth, &screenHeight);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// glDisable(GL_DEPTH_TEST);
		redFlashProg->bind();

		glm::mat4 proj = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
		model = glm::scale(model, glm::vec3(screenWidth, screenHeight, 1.0f));
		glUniformMatrix4fv(redFlashProg->getUniform("projection"), 1, GL_FALSE, value_ptr(proj));
		glUniformMatrix4fv(redFlashProg->getUniform("model"), 1, GL_FALSE, value_ptr(model));
		glUniform1f(redFlashProg->getUniform("alpha"), alpha); // Red color with alpha

		healthBar->Draw(redFlashProg);
		redFlashProg->unbind();
	}

	void updateFTimeout(float deltaTime) {
		if (fTimeout > 0) {
			fTimeout -= deltaTime;
		}
	}

	void drawOcclusionBoxAtPlayer(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model || !sphere) return; // Need shader, stack, model

		shader->bind();
		Model->pushMatrix();
		Model->loadIdentity(); // Start from identity for projectile

		Model->translate(player->getPosition() + glm::vec3(0, 1.0f, 0));
		Model->scale(0.25f);

		setModel(shader, Model);
		sphere->Draw(shader); // Draw the sphere model

		Model->popMatrix();
		shader->unbind();
	}

	void setCameraProjectionFromStack(shared_ptr<Program> curShade, shared_ptr<MatrixStack> projStack) {
		curShade->bind();
		glUniformMatrix4fv(curShade->getUniform("P"), 1, GL_FALSE, value_ptr(projStack->topMatrix()));
	}

	void setCameraViewFromStack(shared_ptr<Program> curShade, shared_ptr<MatrixStack> viewStack) {
		curShade->bind();
		glUniformMatrix4fv(curShade->getUniform("V"), 1, GL_FALSE, value_ptr(viewStack->topMatrix()));
	}

	void drawAABB(const vec3& min, const vec3& max,
		const shared_ptr<Program>& shader,
		const shared_ptr<MatrixStack>& Projection,
		const shared_ptr<MatrixStack>& View,
		const vec3& color = { 1,0,0 }) {
		shader->bind();

		glUniformMatrix4fv(shader->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));

		// build model: translate to center, then scale to half‐size
		vec3 center = (min + max) * 0.5f;
		vec3 half = (max - min) * 0.5f;
		mat4 M = translate(mat4(1.0f), center) * scale(mat4(1.0f), half);
		glUniformMatrix4fv(shader->getUniform("M"), 1, GL_FALSE, value_ptr(M));

		glUniform3fv(shader->getUniform("color"), 1, value_ptr(color)); // color

		// draw lines
		glBindVertexArray(aabbVAO);
		glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		shader->unbind();
	}

	// Draw the scene for shadow map generation (Draw only shadow-casting objects) (First Pass)
	void drawSceneForShadowMap(shared_ptr<Program>& prog) {
		auto Model = make_shared<MatrixStack>();
		drawBorderWalls(prog, Model); // Draw the borders

		drawLibGrnd(prog, Model); // Draw the library ground

		#if USE_INSTANCING
		drawLibInstancing(prog, CULL);
		#else
		drawCircularBorder(prog, CULL); // Draw the circular library shelves
		// 2. Draw the Static Library Shelves
		drawLibrary(prog, Model, CULL);

		drawBossRoom(prog, Model, CULL); // Draw the boss room
		#endif

		drawPlayer(prog, Model, 0.0);

		// 4. Draw Falling/Interactable Books
		drawBooks(prog, Model);

		// 5. Draw Enemies
		drawEnemies(prog, Model);

		// 6. Draw Collectible Orbs
		drawOrbs(prog, Model);

		drawKey(prog, Model);

		drawProjectiles(prog, Model);

		drawBossProjectiles(prog, Model);

		//Test drawing cat model
		//drawCat(assimptexProg, Model);

		//testing drawing lock and key
		if (unlock) {
			updateLock(prog, Model);
		}
		else {
			drawLock(prog, Model);
		}

		// orbCollectibles.emplace_back(sphere, orbSpawnPos, book.orbScale, book.orbColor);
		// drawKey(prog2, Model);



		drawBossEnemy(prog, Model);
	}

	// Draw the scene with shadows (Second Pass)
	void drawMainScene(const shared_ptr<Program>& prog, shared_ptr<MatrixStack>& Model, float animTime) {
		drawBorderWalls(prog, Model); // Draw the borders

		drawLibGrnd(prog, Model); // Draw the library ground

		#if USE_INSTANCING
		drawLibInstancing(prog, false); // Draw the library shelves without culling
		#else
		drawCircularBorder(prog, false); // Draw the circular library shelves

		// 2. Draw the Static Library Shelves
		drawLibrary(prog, Model, true);

		drawBossRoom(prog, Model, true); // Draw the boss room
		#endif

		// disable color writes
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// disable depth writes
		glDepthMask(GL_FALSE);

		// begin occlusion query
		glBeginQuery(GL_ANY_SAMPLES_PASSED, occlusionQueryID);

		// Draw a small sphere at the player's position
		drawOcclusionBoxAtPlayer(prog, Model);

		glEndQuery(GL_ANY_SAMPLES_PASSED);

		GLuint resultofQuery = 0;
		glGetQueryObjectuiv(occlusionQueryID, GL_QUERY_RESULT, &resultofQuery);
		visible = resultofQuery;

		// re-enable color writes and depth writes
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);


		drawPlayer(prog, Model, animTime);

		// 4. Draw Falling/Interactable Books
		drawBooks(prog, Model);

		// 5. Draw Enemies
		drawEnemies(prog, Model);

		// 6. Draw Collectible Orbs
		drawOrbs(prog, Model);

		drawKey(prog, Model);

		drawProjectiles(prog, Model);

		drawBossProjectiles(prog, Model);

		//Test drawing cat model
		//drawCat(assimptexProg, Model);

		//testing drawing lock and key
		if (unlock) {
			updateLock(prog, Model);
		}
		else {
			drawLock(prog, Model);
		}
		//drawKey(prog2, Model);


		drawBossEnemy(prog, Model);
	}

	void occlusionQuery(const shared_ptr<Program>& shader, shared_ptr<MatrixStack>& Model) {
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable color writes
		glDepthMask(GL_FALSE); // disable depth writes
		glBeginQuery(GL_ANY_SAMPLES_PASSED, occlusionQueryID); // begin occlusion query
		drawOcclusionBoxAtPlayer(shader, Model);
		glEndQuery(GL_ANY_SAMPLES_PASSED);

		GLuint resultofQuery = 0;
		glGetQueryObjectuiv(occlusionQueryID, GL_QUERY_RESULT, &resultofQuery);
		visible = resultofQuery;

		// re-enable color writes and depth writes
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
	}

	void debugMessages() {
		if (Config::DEBUG_SHADER_PARAMS) {
			cout << "Shader Parameters:" << endl;
			cout << "Exposure: " << Config::EXPOSURE << endl;
			cout << "Saturation: " << Config::SATURATION << endl;
		}
		if (Config::DEBUG_PLAYER_HP) cout << "Player HP (%): " << player->getHitpoints() / Config::PLAYER_HP_MAX << endl;
	}

	void checkCameraCollision() {
		if (!libraryQuadTree || !bossRoomQuadTree) return; // Ensure trees are initialized
		float cameraBoundingRadius = 1.0f;
		if (!bossfightstarted) {
			std::vector<const QuadElement*> objectElements;
			libraryQuadTree->query(glm::vec2(eye.x, eye.z), glm::vec2(cameraBoundingRadius), objectElements);
			for (const auto* element : objectElements) {
				if (checkSphereCollision(eye, 0.25f, element->aabb_min, element->aabb_max)) {
					visible = 0;
				}
			}
		} else if (bossfightstarted) {
			std::vector<const QuadElement*> objectElements;
			bossRoomQuadTree->query(glm::vec2(eye.x, eye.z), glm::vec2(cameraBoundingRadius), objectElements);
			for (const auto* element : objectElements) {
				if (checkSphereCollision(eye, 0.25f, element->aabb_min, element->aabb_max)) {
					visible = 0;
				}
			}
		} else {
			visible = 1; // Default to visible if no collision detected
		}

	}

	void initCircularBorder() {
		circularBookShelfMatrices.clear();

		for (int z = 0; z < bossGrid.getSize().y; ++z) {
			for (int x = 0; x < bossGrid.getSize().x; ++x) {
				glm::ivec2 gridPos(x, z);

				float i = bossRoom->mapGridXtoWorldX(x);
				float j = bossRoom->mapGridYtoWorldZ(z);
				if ((bossGrid[gridPos].type != BossRoomGen::CellType::BORDER) &&
					(bossGrid[gridPos].type != BossRoomGen::CellType::ENTRANCE)) {
					continue; // Skip non-border cells
				}
				glm::vec3 pos(i, libraryCenter.y, j);
				float rotation = bossGrid[gridPos].transformData.rotation;
				glm::vec3 scale = bossGrid[gridPos].transformData.scale;
				glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
				model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 1, 0));
				model = glm::scale(model, scale);

				auto addInstance = [&](std::vector<glm::mat4>& container) {
					container.push_back(model);
				};

				using CT = BossRoomGen::CellType;
				using BT = BossRoomGen::BorderType;
				using OT = BossRoomGen::CellObjType;

				const auto& cell = bossGrid[gridPos];

				switch (cell.type) {
					case CT::BORDER:
						addInstance(circularBookShelfMatrices);
						break;

					case CT::ENTRANCE:
						if (cell.borderType == BT::ENTRANCE_MIDDLE) {
							bossEntrancePos = pos;
							bossEntranceRot = rotation + 180.0f; // Adjust rotation for entrance
						}
						break;

					default:
						break;
				}
			}
		}

		book_shelf1->InitializeInstancing(circularBookShelfMatrices);

	}

	void drawCircularBorder(shared_ptr<Program> shader, bool cullFlag) {
		vCircularBookShelfMatrices.clear(); // Clear matrices for the next draw call


		if (!shader || !book_shelf1 || grid.getSize().x == 0 || grid.getSize().y == 0) return; // Safety checks
		shader->bind();
		if (shader->hasUniform("hasInstancing")) glUniform1i(shader->getUniform("hasInstancing"), GL_TRUE);

		for (unsigned int i = 0; i < circularBookShelfMatrices.size(); ++i) {
			glm::vec3 pos = glm::vec3(circularBookShelfMatrices[i][3][0],
				circularBookShelfMatrices[i][3][1],
				circularBookShelfMatrices[i][3][2]);
			if (!cullFlag || !ViewFrustCull(pos, 2.0f, planes)) {
				vCircularBookShelfMatrices.push_back(circularBookShelfMatrices[i]);
			}
		}


		book_shelf1->updateInstancingOffsetVBO(vCircularBookShelfMatrices);
		book_shelf1->DrawInstanced(vCircularBookShelfMatrices);

		if (shader->hasUniform("hasInstancing")) glUniform1i(shader->getUniform("hasInstancing"), GL_FALSE);
		shader->unbind();


	}

	void render(float frametime, float animTime) {
		// Get current frame buffer size
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;

		// --- Update Game Logic ---
		if (playerActive) { charMove(); }
		updateCameraVectors();
		updateBooks(frametime);
		updateOrbs((float)glfwGetTime());
		updateKeys((float)glfwGetTime());
		if (enemyActive) { updateEnemies(frametime); }
		updateProjectiles(frametime);
		updateFTimeout(frametime);
		particleSystem->update(frametime); // Update particles
		checkAllEnemies();
		checkBossfight();
		BossEnemyAttacks(frametime);
		restartGeneration();
		//debugMessages();

		// Create the matrix stacks
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		vec3 lightPos = vec3(10); // Fixed light position above the scene
		vec3 lightTarget = libraryCenter; // Light looks at library center
		vec3 lightDir = normalize(lightPos - lightTarget); // Light direction
		vec3 lightUp = vec3(0, 1, 0);
		vec3 lc = Config::LIGHT_COLOR;
		mat4 LO, LV, LSpace;

		glDisable(GL_BLEND); // Disable blending for depth map rendering

		// ========================================================================
		// First Pass: Render scene from light's perspective to generate depth map
		// ========================================================================
		if (Config::SHADOW) {
			glViewport(0, 0, S_WIDTH, S_HEIGHT); // Set viewport for shadow map
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO); // Bind shadow framebuffer
			glClear(GL_DEPTH_BUFFER_BIT); // Clear depth buffer
			glCullFace(GL_FRONT); // Cull front faces for shadow map

			DepthProg->bind(); // Setup shadow shader and draw the scene

			// Create a stable orthographic projection that covers the scene
			float size = Config::ORTHO_SIZE;
			LO = glm::ortho(-size, size, -size, size, 1.0f, 200.0f);
			glUniformMatrix4fv(DepthProg->getUniform("LP"), 1, GL_FALSE, value_ptr(LO));

			// Create a stable light view matrix
			LV = glm::lookAt(lightPos, lightTarget, lightUp);
			glUniformMatrix4fv(DepthProg->getUniform("LV"), 1, GL_FALSE, value_ptr(LV));

			CULL = false;
			drawSceneForShadowMap(DepthProg); // Draw the scene from the lights perspective
			CULL = true;

			DepthProg->unbind();
			glCullFace(GL_BACK); // Reset culling to default
			glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind shadow framebuffer (hard coded 0 is the screen)
		}

		// ===================================================
		// Prepare for Second Pass (Main Rendering to Screen)
		// ===================================================
		glViewport(0, 0, width, height); // Return viewport to screen size
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear framebuffer
		drawSkybox(SkyboxProg, Projection, View);

		// Setup Camera
		Projection->pushMatrix();
		Projection->perspective(radians(45.0f), aspect, 0.1f, 1000.0f); // Adjusted near/far
		View->pushMatrix();
		View->loadIdentity();
		View->lookAt(eye, lookAt, up); // Use updated eye/lookAt

		ExtractVFPlanes(Projection->topMatrix(), View->topMatrix(), planes); // Update frustum planes

		// ==============================
		// Second Pass: Render to Screen
		// ==============================
		if (Config::DEBUG_LIGHTING) { // Debugging light view from lights perspective
			if (Config::DEBUG_GEOM) {
				DepthProgDebug->bind();
				glUniformMatrix4fv(DepthProg->getUniform("LP"), 1, GL_FALSE, value_ptr(LO));
				glUniformMatrix4fv(DepthProg->getUniform("LV"), 1, GL_FALSE, value_ptr(LV));
				drawSceneForShadowMap(DepthProgDebug); // Draw the scene from the lights perspective for debugging
				DepthProgDebug->unbind();
			}
			else { // Draw the depth map texture to a quad for visualization
				DebugProg->bind();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, depthMap);
				glUniform1i(DebugProg->getUniform("texBuf"), 0);
				glEnableVertexAttribArray(0); // Now we actually draw the quad
				glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glDisableVertexAttribArray(0);
				DebugProg->unbind();
			}
		}
		else { // Render the scene like normal with shadow mapping
			ShadowProg->bind();
			// Setup shadow mapping
			glActiveTexture(GL_TEXTURE10);
			glBindTexture(GL_TEXTURE_2D, depthMap); // Bind shadow map texture
			glUniform1i(ShadowProg->getUniform("shadowDepth"), 10); // Set uniform for shadow map
			// Set light and camera uniforms
			glUniform3f(ShadowProg->getUniform("lightDir"), lightDir.x, lightDir.y, lightDir.z); // Set light direction
			glUniform3f(ShadowProg->getUniform("lightColor"), lc.x, lc.y, lc.z);
			glUniform3fv(ShadowProg->getUniform("cameraPos"), 1, glm::value_ptr(eye));
			glUniform1f(ShadowProg->getUniform("exposure"), Config::EXPOSURE);
			glUniform1f(ShadowProg->getUniform("saturation"), Config::SATURATION * (player->getHitpoints() / Config::PLAYER_HP_MAX));
			setCameraProjectionFromStack(ShadowProg, Projection);
			setCameraViewFromStack(ShadowProg, View);
			LSpace = LO * LV;
			glUniformMatrix4fv(ShadowProg->getUniform("LV"), 1, GL_FALSE, value_ptr(LSpace)); // Set light space matrix
			drawMainScene(ShadowProg, Model, animTime); // Draw the entire scene with shadows
			ShadowProg->unbind();
		}

		//===================
		// Second Pass Cont.
		//===================

		if (Config::DEBUG_AABBS) {
			drawAABB(playerBB->min, playerBB->max, debugLineProg, Projection, View, { 1,0,0 });
			//drawAABB(sphereBB->min, sphereBB->max, debugLineProg, Projection, View, { 0,1,0 });
			for (int i = 0; i < bossActiveSpells.size(); i++) {
				if (!bossActiveSpells[i].active) {
					continue;
				}
				else {
					drawAABB(bossActiveSpells[i].aabbMin, bossActiveSpells[i].aabbMax, debugLineProg, Projection, View, { 0,1,0 });
				}
			}
			for (int i = 0; i < activeSpells.size(); i++) {
				if (activeSpells[i].active) {
					drawAABB(activeSpells[i].aabbMin, activeSpells[i].aabbMax, debugLineProg, Projection, View, { 0,1,0 });
				}
			}
			for (const auto* enemy : enemies) {
				if (enemy && enemy->isAlive()) {
					drawAABB(enemy->getAABBMin(), enemy->getAABBMax(), debugLineProg, Projection, View, {0,0,1});
				}
			}
		}

		if (Config::DRAW_PARTICLES) {
			particleProg->bind();
			// glPointSize(10.0f); // Remove this line, size is now per-particle in shader
			glUniformMatrix4fv(particleProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
			glUniformMatrix4fv(particleProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
			particleAlphaTex->bind(particleProg->getUniform("alphaTexture"));
			drawParticles(particleSystem, particleProg, Model); // draw particles if full scene render
			particleAlphaTex->unbind();
			particleProg->unbind();
		}

		if (Config::DRAW_HEALTHBAR) { // Draw the health bar
			//cout << "Drawing healthbar" << endl;
			drawHealthBar();
			drawEnemyHealthBars(View->topMatrix(), Projection->topMatrix());

			if (bossfightstarted && !bossfightended) {
				drawBossHealthBar(View->topMatrix(), Projection->topMatrix());
			}
		}

		if (Config::DRAW_PLAYER_DAMAGE && player->getDamageTimer() > 0.0f) {
			player->setDamageTimer(player->getDamageTimer() - frametime);

			float alpha = player->getDamageTimer() / Config::PLAYER_HIT_DURATION;
			// cout << "Red flash alpha: " << alpha << endl;
			// glEnable(GL_DEPTH_TEST);

			drawDamageIndicator(alpha);
		}
		else if (Config::DRAW_PLAYER_DAMAGE && !player->isAlive() && !debugCamera) {
			// If player is dead, show red flash
			movingForward = false;
			movingBackward = false;
			movingLeft = false;
			movingRight = false;
			drawDamageIndicator(1.0f);
		}

		// Needs to be before MiniMap rendering
		glEnable(GL_BLEND); // Enable blending for text rendering
		// RenderText(textProg, "Cats are ok.  Cur time: " + to_string(glfwGetTime()), 10.0f, 265.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.9f),
		// 		window_width, window_height);
		RenderText(textProg, "Cats are ok.  Cur time: " + to_string(glfwGetTime()), 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.9f),
				width, height);
		float formattedfps = floor(getFPS() * 100) / 100; // Format FPS to 2 decimal places
		RenderText(textProg, "FPS: " + to_string(formattedfps), width - 100.0f, height - 50.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.9f),
				width, height);
		glDisable(GL_BLEND); // Disable blending after text rendering

		if (Config::DRAW_MINIMAP) { // Draw the mini map
			ShadowProg->bind();
			//cout << "Drawing minimap" << endl;
			glClear(GL_DEPTH_BUFFER_BIT);
			glViewport(0, height - 350, 350, 350);
			SetOrthoMatrix(ShadowProg);
			SetTopView(ShadowProg); /*MINI MAP*/
			SetMaterial(ShadowProg, Material::brown);
			//drawScene(prog2, CULL);
			/* draws */
			// drawBorder(prog2, Model);
			// drawDoor(prog2, Model);
			// drawBooks(prog2, Model);
			// drawEnemies(prog2, Model);
			#if USE_INSTANCING
			drawLibInstancing(ShadowProg, false); // Draw the library shelves without culling
			#else
			drawCircularBorder(ShadowProg, false); // Draw the circular library shelves
			drawLibrary(ShadowProg, Model, false);
			drawBossRoom(ShadowProg, Model, false);
			#endif
			// drawLibInstancing(ShadowProg, false); // Draw the library shelves without culling
			drawBossEnemy(ShadowProg, Model);
			// drawOrbs(prog2, Model);
			drawMiniPlayer(ShadowProg, Model);
			drawBorderWalls(ShadowProg, Model);
			// SetMaterialMan(prog2,6 );
			drawLibGrnd(ShadowProg, Model);
			// drawBossRoom(ShadowProg, Model, false); //boss room not drawing
			drawEnemies(ShadowProg, Model);
			ShadowProg->unbind();
		}

		// glEnable(GL_BLEND); // Enable blending for text rendering
		// // RenderText(textProg, "Cats are ok.  Cur time: " + to_string(glfwGetTime()), 10.0f, 265.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.9f),
		// // 		window_width, window_height);
		// RenderText(textProg, "Cats are ok.  Cur time: " + to_string(glfwGetTime()), 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.9f),
		// 		width, height);

		// --- Cleanup ---
		Projection->popMatrix();
		View->popMatrix();

		// Unbind any VAO or Program that might be lingering (belt-and-suspenders)
		glBindVertexArray(0);
		glUseProgram(0);
	}

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);

		// Lighting / Shader settings
		if (key == GLFW_KEY_1 && action == GLFW_PRESS) Config::SATURATION -= 0.1f;
		if (key == GLFW_KEY_2 && action == GLFW_PRESS) Config::SATURATION += 0.1f;
		if (key == GLFW_KEY_3 && action == GLFW_PRESS) Config::EXPOSURE += 0.1f;
		if (key == GLFW_KEY_4 && action == GLFW_PRESS) Config::EXPOSURE -= 0.1f;
		if (key == GLFW_KEY_9 && action == GLFW_PRESS) Config::DEBUG_LIGHTING = !Config::DEBUG_LIGHTING;
		if (key == GLFW_KEY_0 && action == GLFW_PRESS) Config::DEBUG_GEOM = !Config::DEBUG_GEOM;
		if (key == GLFW_KEY_B && action == GLFW_PRESS) Config::DEBUG_AABBS = !Config::DEBUG_AABBS;

		if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS)
		{
			//Fullscreen Mode
			if (!windowMaximized) {
				glfwMaximizeWindow(window);
				windowMaximized = !windowMaximized;
			}
			else {
				glfwRestoreWindow(window);
				windowMaximized = !windowMaximized;
			}
		}

		if (player->isAlive() || debugCamera) {
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) != GLFW_RELEASE) {
				//Movement Variable
				movingForward = true;
				if (debug_pos) {
					cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
					cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
				}
				onStep(player->getPosition(), player->getRotY());
			}
			else if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
				//Movement Variable
				movingForward = false;
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) != GLFW_RELEASE) {

				//Movement Variable
				movingBackward = true;

				if (debug_pos) {
					cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
					cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
				}

				onStep(player->getPosition(), player->getRotY());
			}
			else if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
				//Movement Variable
				movingBackward = false;
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_A) != GLFW_RELEASE) {

				//Movement Variable
				movingLeft = true;

				if (debug_pos) {
					cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
					cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
				}

				onStep(player->getPosition(), player->getRotY());
			}
			else if (key == GLFW_KEY_A && action == GLFW_RELEASE) {

				//Movement Variable
				movingLeft = false;
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) != GLFW_RELEASE) {

				//Movement Variable
				movingRight = true;

				if (debug_pos) {
					cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
					cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
				}

				onStep(player->getPosition(), player->getRotY());
			}
			else if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
				//Movement Variable
				movingRight = false;
			}
			else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
				currentSpellSlotIndex = (currentSpellSlotIndex - 1 + 4) % 4;

				string spellName = "";

				if (currentSpellSlotIndex == 0) {
					spellName = "Fireball";
				}
				else if (currentSpellSlotIndex == 1) {
					spellName = "Ice Shard";
				}
				else if (currentSpellSlotIndex == 2) {
					spellName = "Lightning Bolt";
				}
				else if (currentSpellSlotIndex == 3) {
					spellName = "Heal";
				}

				cout << "Current Spell: " << spellName << endl;
				currentPlayerSpellType = spellSlots[currentSpellSlotIndex];
			}
			else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
				currentSpellSlotIndex = (currentSpellSlotIndex + 1) % 4;
				string spellName = "";
				if (currentSpellSlotIndex == 0) {
					spellName = "Fireball";
				}
				else if (currentSpellSlotIndex == 1) {
					spellName = "Ice Shard";
				}
				else if (currentSpellSlotIndex == 2) {
					spellName = "Lightning Bolt";
				}
				else if (currentSpellSlotIndex == 3) {
					spellName = "Heal";
				}
				cout << "Current Spell: " << spellName << endl;
				currentPlayerSpellType = spellSlots[currentSpellSlotIndex];
			}
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS) { // Interaction Key
			//F Time out to avoid pointer crash
			if (fTimeout <= 0) {
				interactWithBooks();
				fTimeout = 3.0f;
			}
		}
		if (key == GLFW_KEY_L && action == GLFW_PRESS) {
			cursor_visable = !cursor_visable;
			if (cursor_visable) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
		if (key == GLFW_KEY_U && action == GLFW_PRESS && keysCollectedCount == keysneededToCollect) {
			unlock = true;
			canFightboss = true;
		}

		if (key == GLFW_KEY_P && action == GLFW_PRESS) {
			unlock = true;
			canFightboss = true;
		}

		if (key == GLFW_KEY_K && action == GLFW_PRESS) {
			//Debug Camera
			debugCamera = !debugCamera;
		}
		if (debugCamera && key == GLFW_KEY_N && action == GLFW_PRESS) {
			//Debug Enemy Movement
			enemyActive = !enemyActive;
		}
		if (debugCamera && key == GLFW_KEY_M && action == GLFW_PRESS) {
			//Debug Player Movement Toggle
			playerActive = !playerActive;
		}

		// Shoot fireball with SPACEBAR
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			if (player->isAlive()) { // Only shoot if alive
				shootSpell();
			}
		}

		if (!player->isAlive() && key == GLFW_KEY_R && action == GLFW_PRESS) restartGen = true; // Changed restart to R
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
		theta = theta + deltaX * glm::radians(Config::CAMERA_SCROLL_SENSITIVITY_DEGREES);
		phi = phi - deltaY * glm::radians(Config::CAMERA_SCROLL_SENSITIVITY_DEGREES);

		if (phi > glm::radians(Config::CAMERA_PHI_MAX_DEGREES)) phi = glm::radians(Config::CAMERA_PHI_MAX_DEGREES);
		if (phi < glm::radians(Config::CAMERA_PHI_MIN_DEGREES)) phi = glm::radians(Config::CAMERA_PHI_MIN_DEGREES);

		updateCameraVectors();
	}

	void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
		if (!mouseIntialized) {
			lastX = xpos;
			lastY = ypos;
			mouseIntialized = true;
			return;
		}

		float deltaX = xpos - lastX;
		float deltaY = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		theta = theta + deltaX * Config::CAMERA_MOUSE_SENSITIVITY;
		phi = phi + deltaY * Config::CAMERA_MOUSE_SENSITIVITY;

		if (phi > glm::radians(Config::CAMERA_PHI_MAX_DEGREES)) phi = glm::radians(Config::CAMERA_PHI_MAX_DEGREES);
		if (phi < radians(-80.0f)) phi = radians(-80.0f);

		updateCameraVectors();
	}
};

void mouseMoveCallbackWrapper(GLFWwindow* window, double xpos, double ypos) {
	Application* app = (Application*)glfwGetWindowUserPointer(window);
	app->mouseMoveCallback(window, xpos, ypos);
}

int main(int argc, char* argv[]) {
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application* application = new Application();

	std::shared_ptr<Player> playerPtr = std::make_shared<Player>(
		vec3(0, 0, 0),
		Config::PLAYER_HP_MAX,
		Config::PLAYER_MOVE_SPEED,
		application->CatWizard,
		vec3(1.0f, 1.0f, 1.0f),
		vec3(0.0f, 0.0f, 0.0f)
	);
	application->player = playerPtr;

	// Your main will always include a similar set up to establish your window
	// and GL context, etc

	WindowManager* windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;


	// PlaySound(TEXT("C:/Users/trigu/OneDrive/Desktop/476-project/resources/Breaking_Ground.wav"), NULL, SND_FILENAME|SND_ASYNC|SND_LOOP);

	glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(windowManager->getHandle(), application);
	glfwSetCursorPosCallback(windowManager->getHandle(), mouseMoveCallbackWrapper);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialize miniaudio
	if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
		printf("Failed to initialize audio engine.\\n");
		return -1;
	}

	// Load and play sound
	if (ma_sound_init_from_file(&engine, "../resources/chess.mp3", 0, NULL, NULL, &sound) != MA_SUCCESS) {
		printf("Failed to load sound\n");
		ma_engine_uninit(&engine);
		return -1;
	}

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initMapGen();
	application->initGeom(resourceDir);
	application->initGround();
	application->initQuadTree();
	application->initSkyboxCube();
	application->initSkyboxTex(resourceDir);
	application->initCircularBorder();
	application->initAABBWireframe();

	#if USE_INSTANCING
	application->initInstancingMatrices();
	#endif
	glGenQueries(1, &application->occlusionQueryID);

	ma_sound_set_looping(&sound, MA_TRUE); // Set looping to true using the function
	ma_sound_start(&sound); // MUSIC STARTS HERE

	auto lastTime = chrono::high_resolution_clock::now();

	glfwSetInputMode(windowManager->getHandle(), GLFW_STICKY_KEYS, GLFW_TRUE);

	cout << "Controls: " << endl << "WASD: Move" << endl << "Mouse: Look around" << endl
		<< "'F': Interact with book" << endl << "'Q and E': Switch spell slot" << endl << "'~' Fullscreen" << endl << "'L': Toggle cursor mode" << endl
		<< "[DEBUG] Press K To Enter Debug Camera Mode." << endl << "+/- Change Brightness, 1/2 Change Saturation"
		<< endl << "While in Debug Camera mode, M toggles player movement and N toggles enemy movement" << endl;

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle())) {
		auto nextLastTIme = chrono::high_resolution_clock::now();

		float deltaTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - lastTime).count();

		deltaTime *= 0.000001f; // convert to seconds

		lastTime = nextLastTIme;

		float AnimcurrFrame = glfwGetTime();
		application->AnimDeltaTime = AnimcurrFrame - application->AnimLastFrame;
		application->AnimLastFrame = AnimcurrFrame;
		// Render scene.
		application->render(deltaTime, application->AnimDeltaTime);

		// Swap front and back buffers
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events
		glfwPollEvents();
	}

	// Quit program
	windowManager->shutdown();
	ma_engine_uninit(&engine); // Uninitialize miniaudio
	return 0;
}