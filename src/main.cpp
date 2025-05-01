/*
 * The start of our wizarding adventure
 */

#include <iostream>
#include <glad/glad.h>
#include <chrono>
#include <thread>
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
#include "Player.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp> // For glm::quat and glm::rotation
#include <glm/gtx/vector_angle.hpp> // Also sometimes needed for glm::rotation
#include <algorithm>              // For std::remove_if
#include <limits>                 // For std::numeric_limits (used in updateBoundingBox)

using namespace std;
using namespace glm;

#define NUM_LIGHTS 4
#define MAX_BONES 200

#define SHOW_HEALTHBAR 1 // 1 = show health bar, 0 = hide health bar

float randFloat(float l, float h) {
	float r = rand() / (float)RAND_MAX;
	return (1.0f - r) * l + r * h;
}

// Enum for book states
enum class BookState {
	ON_SHELF,
	FALLING,
	LANDED,
	OPENING,
	OPENED
};

enum class OrbState {
	SPAWNING,  // Initial state right after creation
	LEVITATING,// Moving upwards
	IDLE,      // Stationary, ready for collection
	COLLECTED // Visually attached to player (handled by drawing logic)
};

// --- SpellProjectile Struct ---
struct SpellProjectile {
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 scale = glm::vec3(0.05f, 0.05f, 0.6f);
	float speed = 20.0f;
	float lifetime = 2.0f;
	float spawnTime = 0.0f;
	bool active = true;
	AssimpModel* model = nullptr;

	glm::vec3 aabbMin;
	glm::vec3 aabbMax;
	glm::mat4 transform; // Still useful for drawing

	SpellProjectile(glm::vec3 startPos, glm::vec3 dir, float time, AssimpModel* mdl)
		: position(startPos), direction(normalize(dir)), spawnTime(time), model(mdl), transform(1.0f) // Initialize transform
	{
	}
};

class Book {
public:
	vec3 initialPosition; // Where the book starts
	vec3 position;        // Current position (updated by spline or stays initial)
	vec3 scale;           // Base scale for the book
	quat orientation;     // Initial orientation (using quaternion is often easier for complex rotations)
	// Alternatively, use glm::vec3 for Euler angles if you prefer

	BookState state = BookState::ON_SHELF;
	Spline* fallSpline = nullptr; // Pointer to the spline for falling animation
	float fallStartTime = 0.0f;   // Time the fall started

	float openAngle = 0.0f;       // Current angle for opening animation (radians)
	float maxOpenAngle = glm::radians(80.0f); // How far the book opens
	float openSpeed = glm::radians(120.0f); // Speed of opening in radians per second

	AssimpModel* bookModel; // Pointer to the cube model
	AssimpModel* orbModel;  // Pointer to the sphere model

	vec3 orbColor;
	float orbScale = 0.1f; // Scale of the spell orb
	bool orbSpawned = false;

	// Constructor
	Book(AssimpModel* bookMdl, AssimpModel* orbMdl, const glm::vec3& pos, const glm::vec3& scl, const glm::quat& orient, const glm::vec3& orbClr)
		: initialPosition(pos), position(pos), scale(scl), orientation(orient),
		bookModel(bookMdl), orbModel(orbMdl), orbColor(orbClr) {
	}

	// Destructor to clean up spline if needed
	~Book() {
		delete fallSpline;
	}

	// Method to start the fall
	void startFalling(float groundY, const glm::vec3& playerPos) {
		// Check state BEFORE accessing initialPosition etc.
		if (state != BookState::ON_SHELF) {
			return; // Already falling or in another state
		}

		state = BookState::FALLING;
		fallStartTime = (float)glfwGetTime(); // Cast to float

		// --- Calculate Landing Position (End Point) ---
		glm::vec3 endPosition;
		endPosition.y = groundY + (scale.y * 0.5f); // Land flat based on book scale

		// Calculate direction away from player towards the book's spawn point
		// Flatten the direction to the XZ plane to avoid influencing landing Y
		glm::vec3 dirToBook = playerPos - initialPosition;
		dirToBook.y = 0.0f; // Ignore vertical difference for landing direction

		// Handle case where player is exactly at the spawn point (or very close)
		float distSq = dot(dirToBook, dirToBook); // Use dot product for squared length
		if (distSq < 0.01f) { // If too close, pick a default direction (e.g., positive Z)
			dirToBook = glm::vec3(0.0f, 0.0f, 1.0f);
		}
		else {
			dirToBook = normalize(dirToBook); // Normalize the direction vector
		}

		// Define how far the book should land
		float landingDistance = 3.0f; // <-- ADJUST this value to throw further
		float randomSpread = 0.75f; // <-- Randomness around the target landing spot

		// Calculate landing X and Z based on direction and distance + randomness
		endPosition.x = initialPosition.x + dirToBook.x * landingDistance + randFloat(-randomSpread, randomSpread);
		endPosition.z = initialPosition.z + dirToBook.z * landingDistance + randFloat(-randomSpread, randomSpread);


		// --- Calculate Control Point for the Arc ---
		glm::vec3 controlPoint = (initialPosition + endPosition) * 0.5f; // Midpoint between start and end
		// Make the arc higher relative to the start position
		controlPoint.y = initialPosition.y + 3.0f; // <-- ADJUST arc height (relative to start)
		// Add some sideways deviation to the arc's peak
		controlPoint.x += randFloat(-1.5f, 1.5f); // More horizontal arc randomness

		// --- Create the Spline ---
		float fallDuration = 0.4f; // <-- ADJUST fall duration if needed
		delete fallSpline;
		fallSpline = new Spline(initialPosition, controlPoint, endPosition, fallDuration);

		// Debug output (optional)
		// cout << "Book Falling: Start=" << initialPosition.x << "," << initialPosition.y << "," << initialPosition.z
		//      << " End=" << endPosition.x << "," << endPosition.y << "," << endPosition.z
		//      << " Dir=" << dirToBook.x << "," << dirToBook.z << endl;
	}

	// Method to update the book's state and position
	void update(float deltaTime, float groundY) {
		switch (state) {
		case BookState::FALLING:
			if (fallSpline) {
				fallSpline->update(deltaTime);
				position = fallSpline->getPosition();
				// Optional: Add rotation during fall here

				if (fallSpline->isDone()) {
					state = BookState::LANDED; // Transition to landed state
					position.y = groundY + (scale.y * 0.5f); // Ensure it's exactly on the ground
					delete fallSpline;
					fallSpline = nullptr;
					// Set orientation flat on the ground if needed
					orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Reset orientation or calculate landing angle
				}
			}
			break;

		case BookState::LANDED:
			// Optional delay before opening?
			state = BookState::OPENING; // Immediately start opening after landing
			break;

		case BookState::OPENING:
			openAngle += openSpeed * deltaTime;
			if (openAngle >= maxOpenAngle) {
				openAngle = maxOpenAngle;
				state = BookState::OPENED;
			}
			break;

		case BookState::OPENED:
			// Book remains open, orb is visible
			break;

		case BookState::ON_SHELF:
		default:
			// Do nothing
			break;
		}
	}
};

class Collectible {
public:
	AssimpModel* model; // Will always be the sphere model
	glm::vec3 position; // CURRENT position (animated during levitation)
	float scale;
	glm::vec3 AABBmin;
	glm::vec3 AABBmax;
	bool collected;
	glm::vec3 color;

	// --- Levitation Members ---
	OrbState state = OrbState::SPAWNING; // Start in SPAWNING state
	glm::vec3 spawnPosition;             // Where the orb initially appears
	glm::vec3 idlePosition;              // Target position after levitating
	float levitationHeight = 0.6f;       // How far up it moves
	float levitationStartTime = 0.0f;
	float levitationDuration = 0.75f;     // Duration of the levitation animation (seconds)


	// Updated constructor for Orbs
	Collectible(AssimpModel* model, const glm::vec3& spawnPos, const float scale, const glm::vec3& clr)
		: model(model),
		position(spawnPos), // Initial position is spawn position
		scale(scale),
		collected(false),
		color(clr),
		state(OrbState::LEVITATING), // Immediately start levitating after spawn
		spawnPosition(spawnPos)
	{
		// Calculate target idle position
		idlePosition = spawnPosition + glm::vec3(0.0f, levitationHeight, 0.0f);

		// Record start time for animation
		levitationStartTime = glfwGetTime();

		// Initial AABB calculation (based on spawn position initially)
		updateAABB(); // Use a helper function for AABB updates
	}

	// Helper function to update AABB based on current position
	void updateAABB() {
		glm::vec3 localMin = model->getBoundingBoxMin();
		glm::vec3 localMax = model->getBoundingBoxMax();
		localMin *= scale;
		localMax *= scale;
		AABBmin = localMin + position; // Use current position
		AABBmax = localMax + position; // Use current position
	}

	// Function to update levitation animation
	void updateLevitation(float currentTime) {
		if (state == OrbState::LEVITATING) {
			float elapsedTime = currentTime - levitationStartTime;
			float t = glm::clamp(elapsedTime / levitationDuration, 0.0f, 1.0f);

			// Apply an easing function for smoother start/end (optional)
			// t = glm::sineEaseInOut(t); // Example using easing functions (requires #include <glm/gtx/easing.hpp>)
			t = t * t * (3.0f - 2.0f * t); // Manual smoothstep calculation

			// Interpolate position
			position = glm::mix(spawnPosition, idlePosition, t);

			// Update AABB as the orb moves
			updateAABB();

			// Check if animation finished
			if (t >= 1.0f) {
				state = OrbState::IDLE;
				position = idlePosition; // Ensure it's exactly at the target
				updateAABB();           // Final AABB update
				// std::cout << "Orb reached IDLE state." << std::endl; // Debug output
			}
		}
	}
};

class Application : public EventCallbacks {

public:
	std::shared_ptr<Player> player;
	WindowManager * windowManager = nullptr;

	bool windowMaximized = false;
	int window_width = 640;
	int window_height = 480;

	// Our shader programs
	std::shared_ptr<Program> texProg, hudProg, prog2, assimptexProg;

	// ground data - Reused for all flat ground planes
	GLuint GrndBuffObj = 0, GrndNorBuffObj = 0, GIndxBuffObj = 0; // Initialize to 0
	int g_GiboLen = 0;
	GLuint GroundVertexArrayID = 0; // Initialize to 0
	float groundSize = 20.0f; // Half-size of the main library ground square
	float groundY = 0.0f;     // Y level for all ground planes

	// Scene layout parameters
	vec3 libraryCenter = vec3(0.0f, groundY, 0.0f);
	vec3 bossAreaCenter = vec3(0.0f, groundY, 60.0f); // Further away
	vec3 doorPosition = vec3(0.0f, 1.5f, groundSize); // Center of door at library edge
	vec3 doorScale = vec3(1.5f, 3.0f, 0.2f); // Width, Height, Thickness
	float pathWidth = 4.0f; // Width of the path connecting areas

	// setup collectibles vector
	std::vector<Collectible> orbCollectibles;
	int orbsCollectedCount = 0;
	std::vector<Enemy*> enemies;

	// --- Spell Projectiles ---
	std::vector<SpellProjectile> activeSpells;
	glm::vec3 baseSphereLocalAABBMin; // Store base sphere AABB once
	glm::vec3 baseSphereLocalAABBMax;
	bool sphereAABBCalculated = false;

	// character bounding box
	glm::vec3 manAABBmin, manAABBmax;

	AssimpModel *book_shelf1;

	AssimpModel *healthBar;

	AssimpModel *cube, *sphere;
  
  AssimpModel *sky_sphere;
  
  AssimpModel *border;

	//  vector of books
	vector<Book> books;

	AssimpModel *stickfigure_running, *stickfigure_standing;
	Animation *stickfigure_anim, *stickfigure_idle;
	Animator *stickfigure_animator;

	float AnimDeltaTime = 0.0f;
	float AnimLastFrame = 0.0f;

	int change_mat = 0;

	// vec3 characterMovement = vec3(0, 0, 0);
	vec3 manScale = vec3(0.01, 0.01, 0.01);
	vec3 manMoveDir = vec3(sin(radians(0.0f)), 0, cos(radians(0.0f)));

	// initial position of light cycles
	vec3 start_lightcycle1_pos = vec3(-384, -11, 31);
	vec3 start_lightcycle2_pos = vec3(-365, -11, 9.1);


	float theta = 0.0f; // controls yaw
	// float theta = radians(90.0f); // controls yaw
	// float phi = 0.0f; // controls pitch
	float phi = radians(-30.0f); // controls pitch

	float radius = 5.0f;

	float wasd_sens = 0.5f;

	vec3 eye = vec3(-6, 1.03, 0);
	vec3 lookAt = vec3(0, 0, 0);
	vec3 up = vec3(0, 1, 0);

	vec3 right = normalize(cross(manMoveDir, up));

	bool mouseIntialized = false;
	double lastX, lastY;


	int debug = 0;
	int debug_pos = 0;

	bool cursor_visable = true;

	enum Man_State {
		WALKING,
		STANDING,
	};

	//Movement Variables (Maybe move?)
	bool movingForward = false;
	bool movingBackward = false;
	bool movingLeft = false;
	bool movingRight = false;

	float characterRotation = 0.0f;

	Man_State manState = STANDING;

	LibraryGen *library = new LibraryGen();
	Grid<LibraryGen::CellType> grid;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
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

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) != GLFW_RELEASE) {
			manState = WALKING;

			//Movement Variable
			movingForward = true;
			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}
		} else if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
			manState = STANDING;
			//Movement Variable
			movingForward = false;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) != GLFW_RELEASE) {
			manState = WALKING;

			//Movement Variable
			movingBackward = true;

			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}

		} else if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
			manState = STANDING;
			//Movement Variable
			movingBackward = false;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_A) != GLFW_RELEASE) {
			manState = WALKING;

			//Movement Variable
			movingLeft = true;

			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}

		} else if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
			manState = STANDING;
			//Movement Variable
			movingLeft = false;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) != GLFW_RELEASE) {
			manState = WALKING;

			//Movement Variable
			movingRight = true;

			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}
		} else if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
			manState = STANDING;
			//Movement Variable
			movingRight = false;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS) { // Interaction Key
			interactWithBooks();
    }
		if (key == GLFW_KEY_L && action == GLFW_PRESS){
			cursor_visable = !cursor_visable;
			if (cursor_visable) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	}

	void scrollCallback(GLFWwindow *window, double deltaX, double deltaY)
	{

			float sensitivity = 0.7f;

			theta = theta + deltaX * sensitivity;

			phi = phi - deltaY * sensitivity;

			if (phi > radians(-10.0f))
			{
				phi = radians(-10.0f);
			}
			// if (phi > radians(80.0f))
			// {
			// 	phi = radians(80.0f);
			// }
			if (phi < radians(-80.0f))
			{
				phi = radians(-80.0f);
			}

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

		float mouseSensitivity = 0.005f;

		theta = theta + deltaX * mouseSensitivity;
		phi = phi + deltaY * mouseSensitivity;
		if (phi > radians(-10.0f))
		{
			phi = radians(-10.0f);
		}
		if (phi < radians(-80.0f))
		{
			phi = radians(-80.0f);
		}

		updateCameraVectors();
	}

	void updateCameraVectors() {
		vec3 front;
		front.x = radius * cos(phi) * cos(theta);
		front.y = radius * sin(phi);
		front.z = radius * cos(phi) * cos((pi<float>()/2) - theta);

		eye = player->getPosition() - front;
		lookAt = player->getPosition();

		// manRot.y = theta + radians(-90.0f);
		// manRot.y = - manRot.y;
		// manRot.x = phi;

		player->setRotY(-(theta + radians(-90.0f)));
		player->setRotX(phi);

		// cout << "Theta: " << theta << " Phi: " << phi << endl;
		manMoveDir = vec3(sin(player->getRotY()), 0, cos(player->getRotY()));
		right = normalize(cross(manMoveDir, up));

		// lookAt = eye + front;


	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			shootSpell();
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}


	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color and enable z-buffer test
		glClearColor(.12f, .34f, .56f, 1.0f);
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("Texture0");
		texProg->addUniform("MatAmb");
		texProg->addUniform("MatSpec");
		texProg->addUniform("MatShine");
		texProg->addUniform("numLights");
		for (int i = 0; i < NUM_LIGHTS; i++) {
			texProg->addUniform("lightPos[" + to_string(i) + "]");
			texProg->addUniform("lightColor[" + to_string(i) + "]");
			texProg->addUniform("lightIntensity[" + to_string(i) + "]");
		}
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		// Initialize the GLSL program that we will use for rendering
		prog2 = make_shared<Program>();
		prog2->setVerbose(true);
		prog2->setShaderNames(resourceDirectory + "/simple_light_vert.glsl", resourceDirectory + "/simple_light_frag.glsl");
		prog2->init();
		prog2->addUniform("P");
		prog2->addUniform("V");
		prog2->addUniform("M");
		prog2->addUniform("MatAmb");
		prog2->addAttribute("vertPos");
		prog2->addAttribute("vertNor");
		prog2->addUniform("MatDif");
		prog2->addUniform("MatSpec");
		prog2->addUniform("MatShine");
		for (int i = 0; i < NUM_LIGHTS; i++) {
			prog2->addUniform("lightPos[" + to_string(i) + "]");
			prog2->addUniform("lightColor[" + to_string(i) + "]");
			prog2->addUniform("lightIntensity[" + to_string(i) + "]");
		}
		prog2->addUniform("numLights");
		prog2->addUniform("hasEmittance");
		prog2->addUniform("MatEmitt");
		prog2->addUniform("MatEmittIntensity");
		prog2->addUniform("discardCounter");
		prog2->addUniform("activateDiscard");
		prog2->addUniform("randFloat1");
		prog2->addUniform("randFloat2");
		prog2->addUniform("randFloat3");
		prog2->addUniform("randFloat4");

		// Initialize the GLSL program that we will use for assimp models
		assimptexProg = make_shared<Program>();
		assimptexProg->setVerbose(true);
		assimptexProg->setShaderNames(resourceDirectory + "/assimp_tex_vert.glsl", resourceDirectory + "/assimp_tex_frag.glsl");
		assimptexProg->init();
		assimptexProg->addUniform("P");
		assimptexProg->addUniform("V");
		assimptexProg->addUniform("M");
		assimptexProg->addUniform("texture_diffuse1");
		assimptexProg->addUniform("texture_specular1");
		assimptexProg->addUniform("texture_roughness1");
		assimptexProg->addUniform("texture_metalness1");
		assimptexProg->addUniform("texture_emission1");
		assimptexProg->addAttribute("vertPos");
		assimptexProg->addAttribute("vertNor");
		assimptexProg->addAttribute("vertTex");
		assimptexProg->addAttribute("boneIds");
		assimptexProg->addAttribute("weights");
		for (int i = 0; i < MAX_BONES; i++) {
			assimptexProg->addUniform("finalBonesMatrices[" + to_string(i) + "]");
		}
		assimptexProg->addUniform("MatAmb");
		assimptexProg->addUniform("MatDif");
		assimptexProg->addUniform("MatSpec");
		assimptexProg->addUniform("MatShine");
		for (int i = 0; i < NUM_LIGHTS; i++) {
			assimptexProg->addUniform("lightPos[" + to_string(i) + "]");
			assimptexProg->addUniform("lightColor[" + to_string(i) + "]");
			assimptexProg->addUniform("lightIntensity[" + to_string(i) + "]");
		}
		assimptexProg->addUniform("numLights");
		assimptexProg->addUniform("hasTexture");

		hudProg = make_shared<Program>();
		hudProg->setVerbose(true);
		hudProg->setShaderNames(resourceDirectory + "/hud_vert.glsl", resourceDirectory + "/hud_frag.glsl");
		hudProg->init();
		hudProg->addUniform("projection");
		hudProg->addUniform("model");
		hudProg->addUniform("healthPercent");
		hudProg->addUniform("BarStartX");
		hudProg->addUniform("BarWidth");

		updateCameraVectors();

		library->generate(glm::ivec2(30, 30)); // grid size
		grid = library->getGrid();

	}

	void initGeom(const std::string& resourceDirectory)
	{
 		string errStr;

		// load the walking character model
		stickfigure_running = new AssimpModel(resourceDirectory + "/Vanguard/Vanguard.fbx");
		stickfigure_anim = new Animation(resourceDirectory + "/Vanguard/Vanguard.fbx", stickfigure_running, 0);
		stickfigure_idle = new Animation(resourceDirectory + "/Vanguard/Vanguard.fbx", stickfigure_running, 1);

		// --- Calculate Player Collision Box NOW that model is loaded ---
		calculatePlayerLocalAABB();

		stickfigure_animator = new Animator(stickfigure_anim);

		// load the cube (books)
		cube = new AssimpModel(resourceDirectory + "/cube.obj");

		book_shelf1 = new AssimpModel(resourceDirectory + "/book_shelf/source/bookshelf_cluster.obj");

		book_shelf1->assignTexture("texture_diffuse1", resourceDirectory + "/book_shelf/textures/bookstack_textures_2.jpg");
		book_shelf1->assignTexture("texture_specular1", resourceDirectory + "/book_shelf/textures/bookstack_specular.jpg");
    
    sky_sphere = new AssimpModel(resourceDirectory + "/sky_sphere/skybox_sphere.obj");
		sky_sphere->assignTexture("texture_diffuse1", resourceDirectory + "/sky_sphere/sky_sphere.fbm/infinite_lib2.png");
    
    border = new AssimpModel(resourceDirectory + "/border.obj");

		// load the sphere (spell)
		sphere = new AssimpModel(resourceDirectory + "/SmoothSphere.obj");

		// health bar
		healthBar = new AssimpModel(resourceDirectory + "/Quad/hud_quad.obj");
		healthBar->assignTexture("texture_diffuse1", resourceDirectory + "/healthbar.bmp");

		baseSphereLocalAABBMin = sphere->getBoundingBoxMin();
		baseSphereLocalAABBMax = sphere->getBoundingBoxMax();
		sphereAABBCalculated = true;
		cout << "[DEBUG] Stored Base Sphere Local AABB." << endl;

		// --- Initialize Enemy(s) ---
		cout << "Initializing enemies..." << endl;
		// Use the scale factor used in drawEnemies
		// Body scale was (0.5f, bodyBaseScaleY * 1.6f, 0.5f) where bodyBaseScaleY = 0.8f => (0.5, 1.28, 0.5)
		glm::vec3 enemyCollisionScale = glm::vec3(0.5f, 1.28f, 0.5f); // Define the scale
		vec3 bossSpawnPos = bossAreaCenter + vec3(0.0f, 0.8f, 0.0f);

		// Check if sphere model is loaded before creating enemies that use it
		if (sphere) {
			enemies.push_back(new Enemy(bossSpawnPos, 200.0f, 0.0f, sphere, enemyCollisionScale, vec3(0.0f))); // <<-- Pass sphere and scale
			cout << " Enemy placed at boss area: (" << bossSpawnPos.x << ", " << bossSpawnPos.y << ", " << bossSpawnPos.z << ")" << endl;
			enemies.push_back(new Enemy(libraryCenter + vec3(-5.0f, 0.8f, 8.0f), 50.0f, 0.0f, sphere, enemyCollisionScale, vec3(0.0f))); // <<-- Pass sphere and scale
		}
		else {
			cerr << "ERROR: Sphere model not loaded, cannot create enemies." << endl;
		}
	}

	void SetMaterialMan(shared_ptr<Program> curS, int i) {
		switch (i) {
			case 0:
			// gold
				glUniform3f(curS->getUniform("MatAmb"), 0.24725f, 0.1995f, 0.0745f);
				glUniform3f(curS->getUniform("MatDif"), 0.75164f, 0.60648f, 0.22648f);
				glUniform3f(curS->getUniform("MatSpec"), 0.628281f, 0.555802f, 0.366065f);
				glUniform1f(curS->getUniform("MatShine"), 51.2f);
			break;
			case 1:
			// silver
				glUniform3f(curS->getUniform("MatAmb"), 0.19225f, 0.19225f, 0.19225f);
				glUniform3f(curS->getUniform("MatDif"), 0.50754f, 0.50754f, 0.50754f);
				glUniform3f(curS->getUniform("MatSpec"), 0.508273f, 0.508273f, 0.508273f);
				glUniform1f(curS->getUniform("MatShine"), 51.2f);
			break;
			case 2:
			// bronze
				glUniform3f(curS->getUniform("MatAmb"), 0.2125f, 0.1275f, 0.054f);
				glUniform3f(curS->getUniform("MatDif"), 0.714f, 0.4284f, 0.18144f);
				glUniform3f(curS->getUniform("MatSpec"), 0.393548f, 0.271906f, 0.166721f);
				glUniform1f(curS->getUniform("MatShine"), 25.6f);
			break;
			case 3:
			// black
				glUniform3f(curS->getUniform("MatAmb"), 0.01f, 0.01f, 0.01f);
				glUniform3f(curS->getUniform("MatDif"), 0.07f, 0.07f, 0.07f);
				glUniform3f(curS->getUniform("MatSpec"), 0.1f, 0.1f, 0.1f);
				glUniform1f(curS->getUniform("MatShine"), 10.0f);
			break;
			case 4:
			// dark white
				glUniform3f(curS->getUniform("MatAmb"), 0.05f, 0.05f, 0.05f);
				glUniform3f(curS->getUniform("MatDif"), 0.5f, 0.5f, 0.5f);
				glUniform3f(curS->getUniform("MatSpec"), 0.7f, 0.7f, 0.7f);
				glUniform1f(curS->getUniform("MatShine"), 10.0f);
			break;
		}
	}

	/* helper for sending top of the matrix strack to GPU */
	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    }

	/* helper function to set model trasnforms */
  	void setModel(shared_ptr<Program> curS, vec3 trans, float rotY, float rotX, float sc) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
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
		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(libraryCenter); // Center the ground plane
		// No scaling needed if initGround used groundSize correctly relative to its vertices
		setModel(shader, Model);
		SetMaterialMan(shader, 1); // Silver material
		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
		Model->popMatrix();

		// 2. Draw Boss Area Ground
		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(bossAreaCenter); // Position the boss ground plane
		setModel(shader, Model);
		SetMaterialMan(shader, 2); // Bronze material
		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
		Model->popMatrix();

		// 3. Draw Path (Scaled ground geometry)
		Model->pushMatrix();
		Model->loadIdentity();
		// Calculate path dimensions and position
		float pathLength = bossAreaCenter.z - libraryCenter.z - 2 * groundSize;
		if (pathLength < 0) pathLength = 0; // Avoid negative length if areas overlap
		float pathCenterZ = libraryCenter.z + groundSize + pathLength * 0.5f;
		// Calculate scaling factors based on the original ground quad size (groundSize * 2)
		float scaleX = pathWidth / (groundSize * 2.0f);
		float scaleZ = pathLength / (groundSize * 2.0f);

		Model->translate(vec3(libraryCenter.x, groundY, pathCenterZ)); // Center the path segment
		Model->scale(vec3(scaleX, 1.0f, scaleZ)); // Scale ground quad to path dimensions
		setModel(shader, Model);
		SetMaterialMan(shader, 4); // Dark white material
		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);
		Model->popMatrix();

		// Unbind VAO after drawing all ground parts
		glBindVertexArray(0);

		shader->unbind(); // Unbind the simple shader
	}

	void drawPlayer(shared_ptr<Program> curS, shared_ptr<MatrixStack> Model, float animTime) {
		if (!curS || !Model || !stickfigure_running || !stickfigure_animator || !stickfigure_anim || !stickfigure_idle) {
			cerr << "Error: Null pointer in drawPlayer." << endl;
			return;
		}
		curS->bind();

		// Animation update
		stickfigure_animator->UpdateAnimation(1.5f * animTime);
		if (manState == WALKING) {
			stickfigure_animator->SetCurrentAnimation(stickfigure_anim);
		}
		else {
			stickfigure_animator->SetCurrentAnimation(stickfigure_idle);
		}

		// Update bone matrices
		vector<glm::mat4> transforms = stickfigure_animator->GetFinalBoneMatrices();
		int numBones = std::min((int)transforms.size(), MAX_BONES);
		for (int i = 0; i < numBones; ++i) {
			string uniformName = "finalBonesMatrices[" + std::to_string(i) + "]";
			glUniformMatrix4fv(curS->getUniform(uniformName), 1, GL_FALSE, value_ptr(transforms[i]));
		}

		// Model matrix setup
		Model->pushMatrix();
		Model->loadIdentity();
		// Model->translate(characterMovement); // Use final player position
		Model->translate(player->getPosition());
		// *** USE CAMERA ROTATION FOR MODEL ***
		// Model->rotate(manRot.y, vec3(0, 1, 0)); // <<-- FIXED ROTATION
		Model->rotate(player->getRotY(), vec3(0, 1, 0)); // <<-- FIXED ROTATION
		Model->scale(manScale);

		// Update VISUAL bounding box (can be different from collision box if needed)
		// Using the same AABB calculation logic as before for consistency
		glm::mat4 manTransform = Model->topMatrix();
		updateBoundingBox(stickfigure_running->getBoundingBoxMin(),
			stickfigure_running->getBoundingBoxMax(),
			manTransform,
			manAABBmin, // This is the visual/interaction AABB
			manAABBmax);

		// Set uniforms and draw
		glUniform1i(curS->getUniform("hasTexture"), 1);
		setModel(curS, Model);
		stickfigure_running->Draw(curS);

		Model->popMatrix();
		curS->unbind();
	}

	void drawBooks(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		shader->bind();

		for (const auto& book : books) {
			// Common values for book halves
			float halfThickness = book.scale.z * 0.5f;
			glm::vec3 halfScaleVec = glm::vec3(book.scale.x, book.scale.y, halfThickness);

			// Set Material (e.g., brown for cover) - Apply once if same for both halves
			glUniform3f(shader->getUniform("MatAmb"), 0.15f, 0.08f, 0.03f);
			glUniform3f(shader->getUniform("MatDif"), 0.6f, 0.3f, 0.1f);
			glUniform3f(shader->getUniform("MatSpec"), 0.1f, 0.1f, 0.1f);
			glUniform1f(shader->getUniform("MatShine"), 4.0f);
			glUniform1i(shader->getUniform("hasEmittance"), 0);

			// --- Draw Left Cover/Pages ---
			Model->pushMatrix(); // SAVE current stack state
			{
				// 1. Apply base world transformation
				Model->translate(book.position);
				// Apply orientation - Use multMatrix if orientation is a quat
				Model->multMatrix(glm::mat4_cast(book.orientation));
				// If using Euler angles (vec3 rot), apply multiple Model->rotate(...) here

				// 2. Apply opening rotation (relative to book's local Y)
				if (book.state == BookState::OPENING || book.state == BookState::OPENED) {
					Model->rotate(-book.openAngle * 0.5f, glm::vec3(0, 1, 0));
				}

				// 3. Apply offset for this half (relative to spine)
				Model->translate(glm::vec3(0, 0, -halfThickness * 0.5f));

				// 4. Apply scale for this half
				Model->scale(halfScaleVec);

				// 5. Set the uniform with the final matrix from the stack top
				setModel(shader, Model); // Assumes setModel uses Model->topMatrix()

				// 6. Draw
				book.bookModel->Draw(shader);
			}
			Model->popMatrix(); // RESTORE saved stack state

			// --- Draw Right Cover/Back ---
			Model->pushMatrix(); // SAVE current stack state
			{
				// 1. Apply base world transformation
				Model->translate(book.position);
				Model->multMatrix(glm::mat4_cast(book.orientation));

				// 2. Apply opening rotation
				if (book.state == BookState::OPENING || book.state == BookState::OPENED) {
					Model->rotate(book.openAngle * 0.5f, glm::vec3(0, 1, 0));
				}

				// 3. Apply offset for this half
				Model->translate(glm::vec3(0, 0, halfThickness * 0.5f));

				// 4. Apply scale for this half
				Model->scale(halfScaleVec);

				// 5. Set the uniform
				setModel(shader, Model);

				// 6. Draw
				book.bookModel->Draw(shader);
			}
			Model->popMatrix(); // RESTORE saved stack state
		}

		shader->unbind();
	}

	void drawSkybox(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		shader->bind(); // Use prog2 for simple colored shapes

		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(vec3(bossAreaCenter.x, bossAreaCenter.y, bossAreaCenter.z - 20)); // Center the sky sphere at the player position
		Model->scale(vec3(5.0f)); // Scale up the sky sphere to cover the scene

		setModel(shader, Model);
		sky_sphere->Draw(shader);
    
    Model->popMatrix();
    shader->unbind();
  }

	void drawBorder(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model){
		shader->bind();

		glUniform3f(shader->getUniform("MatAmb"), 0.15f, 0.08f, 0.03f);
		glUniform3f(shader->getUniform("MatDif"), 0.6f, 0.3f, 0.1f);
		glUniform3f(shader->getUniform("MatSpec"), 0.1f, 0.1f, 0.1f);
		glUniform1f(shader->getUniform("MatShine"), 4.0f);
		glUniform1i(shader->getUniform("hasEmittance"), 0);

		Model->pushMatrix();
			Model->translate(bossAreaCenter);
			Model->scale(0.5f);
			setModel(shader, Model);
			border->Draw(shader);
		Model->popMatrix();
		shader->unbind();
	}


void drawOrbs(shared_ptr<Program> simpleShader, shared_ptr<MatrixStack> Model) {
		// --- Collision Check Logic ---
		for (auto& orb : orbCollectibles) {
			// Perform collision check ONLY if not collected AND in the IDLE state
			if (!orb.collected && orb.state == OrbState::IDLE && // <<<--- ADD STATE CHECK
				checkAABBCollision(manAABBmin, manAABBmax, orb.AABBmin, orb.AABBmax)) {
				orb.collected = true;
				// orb.state = OrbState::COLLECTED; // Optionally set state
				orbsCollectedCount++;
				std::cout << "Collected a Spell Orb! (" << orbsCollectedCount << ")\n";
			}
		}

		// --- Drawing Logic ---
		simpleShader->bind();

		int collectedOrbDrawIndex = 0;

		for (auto& orb : orbCollectibles) {

			glm::vec3 currentDrawPosition;
			float currentDrawScale = orb.scale; // Use base scale

			if (orb.collected) {
				// Calculate position behind the player (same logic as before)
				float backOffset = 0.4f;
				float upOffsetBase = 0.6f;
				float stackOffset = orb.scale * 2.5f;
				float sideOffset = 0.15f;
				glm::vec3 playerForward = normalize(manMoveDir);
				glm::vec3 playerUp = glm::vec3(0.0f, 1.0f, 0.0f);
				glm::vec3 playerRight = normalize(cross(playerForward, playerUp));
				float currentUpOffset = upOffsetBase + (collectedOrbDrawIndex * stackOffset);
				float currentSideOffset = (collectedOrbDrawIndex % 2 == 0 ? -sideOffset : sideOffset);
				currentDrawPosition = charMove() - playerForward * backOffset
					+ playerUp * currentUpOffset
					+ playerRight * currentSideOffset;
				collectedOrbDrawIndex++;
				// currentDrawScale = orb.scale * 0.8f; // Optional: shrink collected orbs
			}
			else {
				// Use the orb's current position (potentially animated by updateOrbs)
				currentDrawPosition = orb.position;
			}

			// --- Set up transformations ---
			Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(currentDrawPosition);
			Model->scale(currentDrawScale); // Use current scale

			// --- Set Material & Draw ---
			// (Material setting code remains the same)
			glUniform3f(simpleShader->getUniform("MatAmb"), orb.color.r * 0.2f, orb.color.g * 0.2f, orb.color.b * 0.2f);
			glUniform3f(simpleShader->getUniform("MatDif"), orb.color.r * 0.8f, orb.color.g * 0.8f, orb.color.b * 0.8f);
			glUniform3f(simpleShader->getUniform("MatSpec"), 0.8f, 0.8f, 0.8f);
			glUniform1f(simpleShader->getUniform("MatShine"), 32.0f);
			glUniform1i(simpleShader->getUniform("hasEmittance"), 0);

			setModel(simpleShader, Model);
			orb.model->Draw(simpleShader);

			Model->popMatrix();
		} // End drawing loop

		simpleShader->unbind();
	}


	void drawEnemies(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!sphere) return; // Need the sphere model

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


		for (const auto* enemy : enemies) {
			if (!enemy || !enemy->isAlive()) continue; // Skip null or dead enemies

			glm::vec3 enemyPos = enemy->getPosition();

			// --- Draw Main Body (Pill Shape) ---
			Model->pushMatrix();
			{
				Model->translate(enemyPos);
				// Scale for pill shape ( taller in Y, squished in X/Z )
				Model->scale(glm::vec3(0.5f, bodyBaseScaleY * 1.6f, 0.5f)); // Adjust scale factors as needed

				// Set body material
				glUniform3f(shader->getUniform("MatAmb"), bodyColor.r * 0.3f, bodyColor.g * 0.3f, bodyColor.b * 0.3f);
				glUniform3f(shader->getUniform("MatDif"), bodyColor.r, bodyColor.g, bodyColor.b);
				glUniform3f(shader->getUniform("MatSpec"), 0.3f, 0.3f, 0.3f);
				glUniform1f(shader->getUniform("MatShine"), 8.0f);

				setModel(shader, Model);
				sphere->Draw(shader); // Draw the scaled sphere as the body
			}
			Model->popMatrix();


			// --- Draw Eyes (Relative to Enemy Center) ---

			// Set Eye Materials Once
			// White Material Setup (done inside loop per part for clarity now)
			// Black Material Setup (done inside loop per part for clarity now)

			// Left Eye
			Model->pushMatrix();
			{
				// Go to enemy center, then offset to eye position
				Model->translate(enemyPos);
				Model->translate(eyeOffsetBase + glm::vec3(-eyeSeparation, 0, 0));

				// White Part
				Model->pushMatrix();
				{
					Model->scale(glm::vec3(whiteScale));
					// Set white material
					glUniform3f(shader->getUniform("MatAmb"), eyeWhiteColor.r * 0.3f, eyeWhiteColor.g * 0.3f, eyeWhiteColor.b * 0.3f);
					glUniform3f(shader->getUniform("MatDif"), eyeWhiteColor.r, eyeWhiteColor.g, eyeWhiteColor.b);
					glUniform3f(shader->getUniform("MatSpec"), 0.1f, 0.1f, 0.1f);
					glUniform1f(shader->getUniform("MatShine"), 4.0f);
					setModel(shader, Model);
					sphere->Draw(shader);
				}
				Model->popMatrix(); // Pop white scale

				// Pupil Part
				Model->pushMatrix();
				{
					// Move slightly forward from white surface and scale down
					Model->translate(glm::vec3(0, 0, whiteScale * 0.5f + pupilOffsetForward)); // Offset relative to white scale
					Model->scale(glm::vec3(pupilScale));
					// Set black material
					glUniform3f(shader->getUniform("MatAmb"), eyePupilColor.r * 0.3f, eyePupilColor.g * 0.3f, eyePupilColor.b * 0.3f);
					glUniform3f(shader->getUniform("MatDif"), eyePupilColor.r, eyePupilColor.g, eyePupilColor.b);
					glUniform3f(shader->getUniform("MatSpec"), 0.5f, 0.5f, 0.5f); // Some specular highlight
					glUniform1f(shader->getUniform("MatShine"), 32.0f);
					setModel(shader, Model);
					sphere->Draw(shader);
				}
				Model->popMatrix(); // Pop pupil transform
			}
			Model->popMatrix(); // Pop left eye transform


			// Right Eye (Similar to Left)
			Model->pushMatrix();
			{
				Model->translate(enemyPos);
				Model->translate(eyeOffsetBase + glm::vec3(+eyeSeparation, 0, 0)); // Offset to the right

				// White Part
				Model->pushMatrix();
				{
					Model->scale(glm::vec3(whiteScale));
					// Set white material
					glUniform3f(shader->getUniform("MatAmb"), eyeWhiteColor.r * 0.3f, eyeWhiteColor.g * 0.3f, eyeWhiteColor.b * 0.3f);
					glUniform3f(shader->getUniform("MatDif"), eyeWhiteColor.r, eyeWhiteColor.g, eyeWhiteColor.b);
					glUniform3f(shader->getUniform("MatSpec"), 0.1f, 0.1f, 0.1f);
					glUniform1f(shader->getUniform("MatShine"), 4.0f);
					setModel(shader, Model);
					sphere->Draw(shader);
				}
				Model->popMatrix();

				// Pupil Part
				Model->pushMatrix();
				{
					Model->translate(glm::vec3(0, 0, whiteScale * 0.5f + pupilOffsetForward));
					Model->scale(glm::vec3(pupilScale));
					// Set black material
					glUniform3f(shader->getUniform("MatAmb"), eyePupilColor.r * 0.3f, eyePupilColor.g * 0.3f, eyePupilColor.b * 0.3f);
					glUniform3f(shader->getUniform("MatDif"), eyePupilColor.r, eyePupilColor.g, eyePupilColor.b);
					glUniform3f(shader->getUniform("MatSpec"), 0.5f, 0.5f, 0.5f);
					glUniform1f(shader->getUniform("MatShine"), 32.0f);
					setModel(shader, Model);
					sphere->Draw(shader);
				}
				Model->popMatrix();
			}
			Model->popMatrix(); // Pop right eye transform

		} // End loop through enemies

		shader->unbind();
	}

	void drawLibrary(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model || !book_shelf1 || grid.getSize().x == 0 || grid.getSize().y == 0) return; // Safety checks

		shader->bind();
		glUniform1i(shader->getUniform("hasTexture"), 1); // Bookshelves should use texture

		float gridWorldWidth = groundSize * 2.0f; // The world space the grid should occupy (library floor width)
		float gridWorldDepth = groundSize * 2.0f; // The world space the grid should occupy (library floor depth)
		float cellWidth = gridWorldWidth / (float)grid.getSize().x;
		float cellDepth = gridWorldDepth / (float)grid.getSize().y;
		float shelfScaleFactor = 1.8f; // Adjust scale of the bookshelf model itself

		for (int z = 0; z < grid.getSize().y; ++z) {
			for (int x = 0; x < grid.getSize().x; ++x) {
				glm::ivec2 gridPos(x, z);
				if (grid[gridPos] == LibraryGen::SHELF) {
					// Calculate world position based on grid cell, centering the grid on libraryCenter
					float worldX = libraryCenter.x - gridWorldWidth * 0.5f + (x + 0.5f) * cellWidth;
					float worldZ = libraryCenter.z - gridWorldDepth * 0.5f + (z + 0.5f) * cellDepth;

					Model->pushMatrix();
					Model->loadIdentity();
					Model->translate(vec3(worldX, libraryCenter.y, worldZ)); // Position shelf at cell center on ground
					// Scale based on cell size and factor, adjust Y scale for desired height
					Model->scale(vec3(shelfScaleFactor * cellWidth * 1.5f, // Adjust scale factor
						shelfScaleFactor * 1.8f, // Taller shelves
						shelfScaleFactor * cellDepth * 1.5f));
					// Optional: Add random rotation?
					// Model->rotate(randFloat(0.f, 3.14f), vec3(0, 1, 0));
					setModel(shader, Model);
					book_shelf1->Draw(shader);
					Model->popMatrix();
				}
			}
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

		SetMaterialMan(shader, 5); // Use Wood material
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

	void updateBooks(float deltaTime) { // deltaTime might not be needed if using glfwGetTime()
		for (auto& book : books) {
			book.update(deltaTime, 0.0f);

			if (book.state == BookState::OPENED && !book.orbSpawned) {
				glm::mat4 baseRotation = glm::mat4_cast(book.orientation);
				// Spawn slightly above the book center to avoid immediate ground collision?
				glm::vec3 orbOffset = glm::vec3(0.0f, book.scale.y * 0.1f + 0.05f, 0.0f); // Small initial Y offset
				glm::vec3 orbSpawnPos = book.position + glm::vec3(baseRotation * glm::vec4(orbOffset, 0.0f));

				// Constructor handles setting state to LEVITATING and calculating idlePosition
				orbCollectibles.emplace_back(sphere, orbSpawnPos, book.orbScale, book.orbColor);

				book.orbSpawned = true;
				cout << "Orb Spawned! State: LEVITATING" << endl;
			}
		}
	}

	void interactWithBooks() {
		float interactionRadius = 5.0f;
		float interactionRadiusSq = interactionRadius * interactionRadius;

		float gridWorldWidth = groundSize * 2.0f;
		float gridWorldDepth = groundSize * 2.0f;
		float cellWidth = gridWorldWidth / (float)grid.getSize().x;
		float cellDepth = gridWorldDepth / (float)grid.getSize().y;

		bool interacted = false;

		for (int z = 0; z < grid.getSize().y && !interacted; ++z) {
			for (int x = 0; x < grid.getSize().x && !interacted; ++x) {
				glm::ivec2 gridPos(x, z);
				if (grid[gridPos] == LibraryGen::SHELF) {
					float shelfWorldX = libraryCenter.x - gridWorldWidth * 0.5f + (x + 0.5f) * cellWidth;
					float shelfWorldZ = libraryCenter.z - gridWorldDepth * 0.5f + (z + 0.5f) * cellDepth;
					glm::vec3 shelfCenterPos = glm::vec3(shelfWorldX, groundY + 1.0f, shelfWorldZ);

					// glm::vec3 diff = shelfCenterPos - characterMovement;
					glm::vec3 diff = shelfCenterPos - player->getPosition(); 
					diff.y = 0.0f; // Ignore Y difference for interaction distance
					float distSq = dot(diff, diff); // Use dot product for squared distance

					if (distSq <= interactionRadiusSq) {

						// --- ADJUST Spawn Height ---
						float minSpawnHeight = 1.8f; // Minimum height above groundY
						float maxSpawnHeight = 2.8f; // Maximum height above groundY
						float spawnHeight = groundY + randFloat(minSpawnHeight, maxSpawnHeight); // <-- ADJUSTED height range

						glm::vec3 spawnPos = glm::vec3(shelfWorldX, spawnHeight, shelfWorldZ);

						glm::vec3 bookScale = glm::vec3(0.7f, 0.9f, 0.2f);
						glm::quat bookOrientation = glm::angleAxis(glm::radians(randFloat(-10.f, 10.f)), glm::vec3(0, 1, 0));
						glm::vec3 orbColor = glm::vec3(randFloat(0.2f, 1.0f), randFloat(0.2f, 1.0f), randFloat(0.2f, 1.0f));

						books.emplace_back(cube, sphere, spawnPos, bookScale, bookOrientation, orbColor);

						Book& newBook = books.back();

						// --- PASS Player Position to startFalling ---
						// newBook.startFalling(groundY, characterMovement); // <<-- MODIFIED call
						newBook.startFalling(groundY, player->getPosition());

						interacted = true;
						break;
					}
				}
			}
		}

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
		// TODO: Add enemy movement, AI, attack logic later
		for (auto* enemy : enemies) {
			if (!enemy->isAlive()) enemy->setPosition(enemy->getPosition() - vec3(0.0f, 3.0f, 0.0f));
			// Example: Simple bobbing motion
			// float bobSpeed = 2.0f;
			// float bobHeight = 0.05f;
			// glm::vec3 currentPos = enemy->getPosition();
			// enemy->setPosition(glm::vec3(currentPos.x, 0.8f + sin(glfwGetTime() * bobSpeed) * bobHeight, currentPos.z));
			 // IMPORTANT: Update enemy AABB if it moves
			 // enemy->updateAABB(); // Need to add AABB members and update method to Enemy/Entity class
		}
	}

	// --- Player Collision ---
	// Store player's local AABB (scaled) for easier access
	glm::vec3 playerLocalAABBMin;
	glm::vec3 playerLocalAABBMax;
	bool playerAABBCalculated = false; // Flag to calculate once

	// Helper to calculate player's local AABB
	void calculatePlayerLocalAABB() {
		if (!stickfigure_running || playerAABBCalculated) return;

		// Get base AABB from the *standing* or *running* model (choose one representative)
		// Using stickfigure_running as it's loaded first
		glm::vec3 baseMin = stickfigure_running->getBoundingBoxMin();
		glm::vec3 baseMax = stickfigure_running->getBoundingBoxMax();

		// Apply the player's base scale
		playerLocalAABBMin = baseMin * manScale.x; // Assuming uniform scale for collision box
		playerLocalAABBMax = baseMax * manScale.x;

		// Optional: Add padding or adjust Y if needed
		// Example: Make collision box slightly taller or ensure base is at y=0 locally
		// playerLocalAABBMin.y = 0.0f; // If player origin is at feet

		playerAABBCalculated = true;
		// cout << "[DEBUG] Calculated Player Local AABB Min: (" << playerLocalAABBMin.x << "," << playerLocalAABBMin.y << "," << playerLocalAABBMin.z << ")" << endl;
		// cout << "[DEBUG] Calculated Player Local AABB Max: (" << playerLocalAABBMax.x << "," << playerLocalAABBMax.y << "," << playerLocalAABBMax.z << ")" << endl;
	}


	// --- Collision Checking Helper ---
	bool checkCollisionAt(const glm::vec3& checkPos, const glm::quat& playerOrientation) {
		if (!playerAABBCalculated || !book_shelf1 || grid.getSize().x == 0) return false; // Need data

		// 1. Calculate Player's World AABB at checkPos
		glm::mat4 playerTransform = glm::translate(glm::mat4(1.0f), checkPos) * glm::mat4_cast(playerOrientation);
		// Note: We use the PRE-SCALED local AABB calculated earlier
		glm::vec3 playerWorldMin, playerWorldMax;
		updateBoundingBox(playerLocalAABBMin, playerLocalAABBMax, playerTransform, playerWorldMin, playerWorldMax);

		// 2. Iterate through grid for shelves
		float gridWorldWidth = groundSize * 2.0f;
		float gridWorldDepth = groundSize * 2.0f;
		float cellWidth = gridWorldWidth / (float)grid.getSize().x;
		float cellDepth = gridWorldDepth / (float)grid.getSize().y;
		// Use the same scale factor as drawLibrary
		float shelfScaleFactor = 1.8f;
		glm::vec3 shelfVisScale = vec3(shelfScaleFactor * cellWidth * 1.5f,
			shelfScaleFactor * 1.8f,
			shelfScaleFactor * cellDepth * 1.5f);
		// --- Get shelf model's local AABB ONCE ---
		glm::vec3 shelfLocalMin = book_shelf1->getBoundingBoxMin();
		glm::vec3 shelfLocalMax = book_shelf1->getBoundingBoxMax();
		// --- Apply visual scale to shelf local AABB for collision ---
		// Important: Scale the AABB min/max points correctly
		glm::vec3 collisionShelfLocalMin = shelfLocalMin * shelfVisScale;
		glm::vec3 collisionShelfLocalMax = shelfLocalMax * shelfVisScale;
		// Handle potential inversion if scale is negative (unlikely here)
		for (int i = 0; i < 3; ++i) {
			if (collisionShelfLocalMin[i] > collisionShelfLocalMax[i]) std::swap(collisionShelfLocalMin[i], collisionShelfLocalMax[i]);
		}

		for (int z = 0; z < grid.getSize().y; ++z) {
			for (int x = 0; x < grid.getSize().x; ++x) {
				glm::ivec2 gridPos(x, z);
				if (grid[gridPos] == LibraryGen::SHELF) {
					// 3. Calculate this shelf's World AABB
					float worldX = libraryCenter.x - gridWorldWidth * 0.5f + (x + 0.5f) * cellWidth;
					float worldZ = libraryCenter.z - gridWorldDepth * 0.5f + (z + 0.5f) * cellDepth;
					glm::vec3 shelfPos = vec3(worldX, libraryCenter.y, worldZ); // Base position on ground

					// Shelf transform (Position only, assuming no rotation for collision)
					// The scale is applied to the local AABB above
					glm::mat4 shelfTransform = glm::translate(glm::mat4(1.0f), shelfPos);

					glm::vec3 shelfWorldMin, shelfWorldMax;
					updateBoundingBox(collisionShelfLocalMin, collisionShelfLocalMax, shelfTransform, shelfWorldMin, shelfWorldMax);

					// 4. Check for Overlap
					if (checkAABBCollision(playerWorldMin, playerWorldMax, shelfWorldMin, shelfWorldMax)) {
						// cout << "[DEBUG] Collision DETECTED with shelf at grid (" << x << "," << z << ")" << endl;
						return true; // Collision found
					}
				}
			}
		}

		return false; // No collision found
	}

	// --- Modified charMove ---
	vec3 charMove() {
		// Calculate player's local AABB once if not done yet
		if (!playerAABBCalculated) {
			calculatePlayerLocalAABB();
		}

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
		if (orbsCollectedCount <= 0 || !sphere || !sphereAABBCalculated) {
			cout << "[DEBUG] Cannot shoot: No orbs or sphere model not ready." << endl;
			return; // Need orbs and the sphere model/AABB
		}

		// Consume an orb
		orbsCollectedCount--;
		// Remove visual orb logic... (find first collected orb and erase)
		for (auto it = orbCollectibles.begin(); it != orbCollectibles.end(); ++it) {
			if (it->collected) {
				orbCollectibles.erase(it);
				break;
			}
		}

		// *** Use Character's Forward Direction ***
		// 'manMoveDir' is updated in updateCameraVectors based on manRot.y (which matches theta)
		vec3 shootDir = manMoveDir; // Already normalized and horizontal

		// *** Use Character's Right Vector ***
		// Calculate the horizontal right vector based on manMoveDir
		vec3 playerRight = normalize(cross(manMoveDir, vec3(0.0f, 1.0f, 0.0f)));


		// Spawn Position Calculation (relative to character's position and orientation)
		float forwardOffset = 0.5f; // How far in front of player center
		float upOffset = 0.8f;      // Height relative to player base (groundY)
		float rightOffset = 0.2f;   // Offset to the side (e.g., right hand)

		// vec3 spawnPos = characterMovement
		// 	+ vec3(0.0f, upOffset, 0.0f) // Vertical offset from base
		// 	+ shootDir * forwardOffset   // Forward offset along character's facing direction
		// 	+ playerRight * rightOffset; // Sideways offset along character's right

		vec3 spawnPos = player->getPosition()
		+ vec3(0.0f, upOffset, 0.0f) // Vertical offset from base
		+ shootDir * forwardOffset   // Forward offset along character's facing direction
		+ playerRight * rightOffset; // Sideways offset along character's right

		// Create and add projectile
		activeSpells.emplace_back(spawnPos, shootDir, (float)glfwGetTime(), sphere);
		cout << "[DEBUG] Spell Fired! Start:(" << spawnPos.x << "," << spawnPos.y << "," << spawnPos.z
			<< ") Dir: (" << shootDir.x << "," << shootDir.y << "," << shootDir.z // y should be 0
			<< "). Active spells: " << activeSpells.size() << endl;
	}

	// --- updateProjectiles ---
	void updateProjectiles(float deltaTime) {
		if (!sphereAABBCalculated) return;

		float damageAmount = 25.0f;

		// Iterate using index for potential removal
		for (int i = 0; i < activeSpells.size(); ++i) {
			if (!activeSpells[i].active) continue;

			SpellProjectile& proj = activeSpells[i]; // Use reference

			// Check lifetime
			if (glfwGetTime() - proj.spawnTime > proj.lifetime) {
				proj.active = false;
				// cout << "[DEBUG] Spell lifetime expired." << endl;
				continue;
			}

			// Update position
			proj.position += proj.direction * proj.speed * deltaTime;

			// Calculate transform HERE
			glm::quat rotation = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), proj.direction);
			proj.transform = glm::translate(glm::mat4(1.0f), proj.position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), proj.scale);

			// Update AABB using Application's function
			this->updateBoundingBox(baseSphereLocalAABBMin, baseSphereLocalAABBMax, proj.transform, proj.aabbMin, proj.aabbMax);

			// Check collision with enemies
			for (auto* enemy : enemies) {
				if (!enemy || !enemy->isAlive()) continue;

				// Ensure enemy AABB is up-to-date (call if they move)
				// enemy->updateAABB();

				if (checkAABBCollision(proj.aabbMin, proj.aabbMax, enemy->getAABBMin(), enemy->getAABBMax())) {
					cout << "[DEBUG] Spell HIT enemy!" << endl;
					
					if (!enemy->isHit()) {
						enemy->setHit(true); // Mark enemy as hit
					}

					enemy->takeDamage(damageAmount);
					proj.active = false; // Deactivate projectile
					break; // Hit one enemy
				}
			}
		} // End loop

		// Remove inactive projectiles
		activeSpells.erase(
			std::remove_if(activeSpells.begin(), activeSpells.end(),
				[](const SpellProjectile& p) { return !p.active; }),
			activeSpells.end()
		);
	}

	void drawProjectiles(shared_ptr<Program> shader, shared_ptr<MatrixStack> Model) {
		if (!shader || !Model || !sphere) return; // Need shader, stack, model

		shader->bind();
		// Set material for projectiles (e.g., bright yellow/white, maybe emissive if shader supports)
		SetMaterialMan(shader, 0); // Gold material for now
		glUniform3f(shader->getUniform("MatAmb"), 0.8f, 0.8f, 0.1f);
		glUniform3f(shader->getUniform("MatDif"), 1.0f, 1.0f, 0.5f);
		glUniform3f(shader->getUniform("MatSpec"), 1.0f, 1.0f, 1.0f);
		glUniform1f(shader->getUniform("MatShine"), 64.0f);
		// Optional: Emissive properties if shader supports them
		// if(shader->hasUniform("hasEmittance")) glUniform1i(shader->getUniform("hasEmittance"), 1);
		// if(shader->hasUniform("MatEmitt")) glUniform3f(shader->getUniform("MatEmitt"), 1.0f, 1.0f, 0.8f);

		for (const auto& proj : activeSpells) {
			if (!proj.active) continue;

			Model->pushMatrix();
			Model->loadIdentity(); // Start from identity for projectile

			// Use the pre-calculated transform from updateAABB
			Model->multMatrix(proj.transform);

			/* // --- Manual Transform Calculation (Alternative to using proj.transform) ---
			Model->translate(proj.position);
			// Calculate rotation to align local Z with direction
			glm::quat rotation = glm::rotation(glm::vec3(0.0f, 0.0f, 1.0f), proj.direction);
			Model->multMatrix(glm::mat4_cast(rotation));
			Model->scale(proj.scale);
			*/

			setModel(shader, Model);
			proj.model->Draw(shader); // Draw the sphere model

			Model->popMatrix();
		}

		shader->unbind();
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
		glUniform1f(hudProg->getUniform("healthPercent"), player->getHitpoints() / PLAYER_HP_MAX); // Pass health value
		glUniform1f(hudProg->getUniform("BarStartX"), healthBarStartX); // Pass max health value
		glUniform1f(hudProg->getUniform("BarWidth"), heatlhBarWidth); // Pass max health value
	
		healthBar->Draw(hudProg);
		hudProg->unbind();
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
	
	

	void render(float frametime, float animTime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float aspect = width/(float)height;

		// Create the matrix stacks
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// --- Update Game Logic ---
		charMove();
		updateCameraVectors();
		updateBooks(frametime);
		updateOrbs((float)glfwGetTime());
		updateEnemies(frametime);
		updateProjectiles(frametime);

		// --- Setup Camera ---
		Projection->pushMatrix();
		Projection->perspective(radians(45.0f), aspect, 0.1f, 1000.0f); // Adjusted near/far
		View->pushMatrix();
		View->loadIdentity();
		View->lookAt(eye, lookAt, up); // Use updated eye/lookAt

		// --- Setup Lights ---
		// Example: One bright light in the library, one dimmer in boss area
		// vec3 lightPositions[NUM_LIGHTS] = {
		// 	libraryCenter + vec3(0, 15, 0),      // Library light overhead
		// 	bossAreaCenter + vec3(0, 10, 0),     // Boss area light overhead
		// 	characterMovement + vec3(0, 1, 0.5), // Small light near player (optional)
		// 	vec3(0, 0, 0)                        // Unused or ambient fill
		// };

		vec3 lightPositions[NUM_LIGHTS] = {
			libraryCenter + vec3(0, 15, 0),      // Library light overhead
			bossAreaCenter + vec3(0, 10, 0),     // Boss area light overhead
			player->getPosition() + vec3(0, 1, 0.5), // Small light near player (optional)
			vec3(0, 0, 0)                        // Unused or ambient fill
		};

		vec3 lightColors[NUM_LIGHTS] = {
			vec3(1.0f, 1.0f, 0.9f), // Slightly warm white
			vec3(0.8f, 0.6f, 1.0f), // Dim purple/blue
			vec3(0.3f, 0.3f, 0.3f),
			vec3(0.1f, 0.1f, 0.1f)
		};
		float lightIntensities[NUM_LIGHTS] = {
			1.5f, // Bright library
			0.8f, // Dimmer boss area
			0.5f, // Player light
			0.0f
		};
		int numActiveLights = 3; // How many lights we're actually using

		// --- Update Shader Uniforms (Lights, P, V) ---
		// Update prog2 (Simple Lighting)
		if (prog2) {
			prog2->bind();
			glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
			glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
			glUniform1i(prog2->getUniform("numLights"), numActiveLights);
			for (int i = 0; i < numActiveLights; ++i) {
				string prefix = "lightPos[" + to_string(i) + "]";
				glUniform3fv(prog2->getUniform(prefix), 1, value_ptr(lightPositions[i]));
				prefix = "lightColor[" + to_string(i) + "]";
				glUniform3fv(prog2->getUniform(prefix), 1, value_ptr(lightColors[i]));
				prefix = "lightIntensity[" + to_string(i) + "]";
				glUniform1f(prog2->getUniform(prefix), lightIntensities[i]);
			}
			prog2->unbind();
		}

		// Update assimptexProg (Textured/Animated Lighting)
		if (assimptexProg) {
			assimptexProg->bind();
			glUniformMatrix4fv(assimptexProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
			glUniformMatrix4fv(assimptexProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
			glUniform1i(assimptexProg->getUniform("numLights"), numActiveLights);
			for (int i = 0; i < numActiveLights; ++i) {
				string prefix = "lightPos[" + to_string(i) + "]";
				glUniform3fv(assimptexProg->getUniform(prefix), 1, value_ptr(lightPositions[i]));
				prefix = "lightColor[" + to_string(i) + "]";
				glUniform3fv(assimptexProg->getUniform(prefix), 1, value_ptr(lightColors[i]));
				prefix = "lightIntensity[" + to_string(i) + "]";
				glUniform1f(assimptexProg->getUniform(prefix), lightIntensities[i]);
			}
			assimptexProg->unbind();
		}

		// --- Draw Scene Elements ---
		// ORDER MATTERS for transparency, but with opaque objects and depth testing, it's less critical.
		// Drawing grounds first is logical.

		// 1. Draw Ground, Path
		drawGroundSections(prog2, Model);

		drawBorder(prog2, Model);

		// 2. Draw the Static Library Shelves
		drawLibrary(assimptexProg, Model);

		// 3. Draw the Door
		drawDoor(prog2, Model);

		// 4. Draw Falling/Interactable Books
		drawBooks(prog2, Model);

		// 5. Draw Enemies
		drawEnemies(prog2, Model);

		// 6. Draw Collectible Orbs
		drawOrbs(prog2, Model);

		drawProjectiles(prog2, Model);

		// 7. Draw Player (often drawn last or near last)
		drawPlayer(assimptexProg, Model, animTime);

		drawSkybox(assimptexProg, Model); // Draw the skybox last

		#if SHOW_HEALTHBAR
		drawHealthBar();
		drawEnemyHealthBars(View->topMatrix(), Projection->topMatrix());
		#endif

		// --- Cleanup ---
		Projection->popMatrix();
		View->popMatrix();

		// Unbind any VAO or Program that might be lingering (belt-and-suspenders)
		glBindVertexArray(0);
		glUseProgram(0);
	}
};

void mouseMoveCallbackWrapper(GLFWwindow* window, double xpos, double ypos) {
	Application* app = (Application*)glfwGetWindowUserPointer(window);
	app->mouseMoveCallback(window, xpos, ypos);
}

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	std::shared_ptr<Player> playerPtr = std::make_shared<Player>(
		vec3(0, 0, 0),
		PLAYER_HP_MAX,
		PLAYER_MOVE_SPEED,
		application->sphere,
		vec3(1.0f, 1.0f, 1.0f),
		vec3(0.0f, 0.0f, 0.0f)
	);
	application->player = playerPtr;

	// Your main will always include a similar set up to establish your window
	// and GL context, etc

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(windowManager->getHandle(), application);
	glfwSetCursorPosCallback(windowManager->getHandle(), mouseMoveCallbackWrapper);

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);
	application->initGround();

	auto lastTime = chrono::high_resolution_clock::now();

	glfwSetInputMode(windowManager->getHandle(), GLFW_STICKY_KEYS, GLFW_TRUE);

	cout << "Controls: " << endl << "WASD: Move" << endl << "Mouse: Look around" << endl
		<< "'F': Interact with book" << "F11 Fullscreen" << endl << "'L': Toggle cursor mode" << endl;

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle())) {
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
	return 0;
}
