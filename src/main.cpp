/*
 * Example two meshes and two shaders (could also be used for Program 2)
 * includes modifications to shape and initGeom in preparation to load
 * multi shape objects
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <glad/glad.h>
#include <chrono>
#include <thread>

#include "GLSL.h"
#include "Program.h"
// #include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "Spline.h"
#include "stb_image.h"
#include "AssimpModel.h"
#include "Animator.h"
#include "LightTrail.h"

// #define TINYOBJLOADER_IMPLEMENTATION
// #include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

#define NUM_LIGHTS 4
#define MAX_BONES 200

class Application : public EventCallbacks {

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog;

	// Our shader program
	std::shared_ptr<Program> solidColorProg;

	std::shared_ptr<Program> prog2;

	std::shared_ptr<Program> texProg;

	std::shared_ptr<Program> skyProg;

	std::shared_ptr<Program> assimptexProg;

	GLuint skyMapTexture;

	// ground data
	GLuint GrndBuffObj, GrndNorBuffObj, GIndxBuffObj;
	int g_GiboLen;
	GLuint GroundVertexArrayID;
	float groundSize = 20.0f;

	shared_ptr<Texture> sidewalkTexture;

<<<<<<< Updated upstream
	shared_ptr<Texture> skyTexture;

	shared_ptr<Texture> monitor1Texture;
	shared_ptr<Texture> monitor2Texture;
	shared_ptr<Texture> monitor3Texture;

	// shared_ptr<Shape> sky; // big sphere

	// shared_ptr<Shape> cube_sky;

	// vector<shared_ptr<Shape>> mans;

	AssimpModel *cube;

	AssimpModel *vampire;

	AssimpModel *wolf;

	AssimpModel *stickfigure_running;

	AssimpModel *stickfigure_standing;

	Animation *stickfigure_anim;

	Animation *stickfigure_idle;
=======
	AssimpModel *cube, *barrel, *wizard_hat;
>>>>>>> Stashed changes

	Animator *stickfigure_animator;

	float AnimDeltaTime = 0.0f;
	float AnimLastFrame = 0.0f;

	// vector<string> light_cycles_names;

	//example data that might be useful when trying to compute bounds on multi-shape
	vec3 gMin;

	float lightTrans = -2;

	int change_mat = 0;

	//animation data
	float sTransx = -2.5;
	float sTransy = -1.3;
	float vTransz = 0;
	float vThetax = 0.0f;
	float vThetay = 0.0f;
	float vTransx = 2;
	float vTransy = 0;

	// vec3 manTrans = vec3(-2.5, -1.3, 0);
	vec3 manTrans = vec3(0, 0, 0);
	vec3 manScale = vec3(0.01, 0.01, 0.01);
	vec3 manRot = vec3(radians(0.0f), radians(0.0f), radians(0.0f));

	vec3 manMoveDir = vec3(sin(manRot.y), 0, cos(manRot.y));

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
	// vec3 lookAt = vec3(-1.58614, -0.9738, 0.0436656);
	vec3 lookAt = manTrans;
	vec3 up = vec3(0, 1, 0);

	bool mouseIntialized = false;
	double lastX, lastY;


	int debug = 0;
	int debug_pos = 0;

	enum Man_State {
		WALKING,
		STANDING,
	};

	Man_State manState = STANDING;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		//update global camera rotate
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) != GLFW_RELEASE) {
			// eye = eye + wasd_sens * normalize(lookAt - eye); // w vector
			// lookAt = lookAt + wasd_sens * normalize(lookAt - eye); // w vector
			// manTrans.x += 0.2f;
			// eye.x += 0.2f;
			manTrans += manMoveDir * 0.2f;
			eye += manMoveDir * 0.2f;
			lookAt = manTrans;


			manState = WALKING;

			// cout << "pressing W" << endl;


			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}
		} else if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
			manState = STANDING;

			// cout << "releasing W" << endl;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) != GLFW_RELEASE) {
			// eye = eye - wasd_sens * normalize(lookAt - eye); // w vector
			// lookAt = lookAt - wasd_sens * normalize(lookAt - eye); // w vector
			// manTrans.x -= 0.2f;
			// eye.x -= 0.2f;
			manTrans -= manMoveDir * 0.2f;
			eye -= manMoveDir * 0.2f;
			lookAt = manTrans;


			manState = WALKING;

			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}

		} else if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
			manState = STANDING;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_A) != GLFW_RELEASE) {
			// eye = eye - wasd_sens * normalize(cross(lookAt - eye, up)); // u vector
			// lookAt = lookAt - wasd_sens * normalize(cross(lookAt - eye, up)); // u vector

			// manTrans.z -= 0.2f;
			// eye.z -= 0.2f;
			vec3 right = normalize(cross(manMoveDir, up));
			manTrans -= right * 0.2f;
			eye -= right * 0.2f;
			lookAt = manTrans;


			manState = WALKING;

			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}

		} else if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
			manState = STANDING;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) != GLFW_RELEASE) {
			// eye = eye + wasd_sens * normalize(cross(lookAt - eye, up)); // u vector
			// lookAt = lookAt + wasd_sens * normalize(cross(lookAt - eye, up)); // u vector

			// manTrans.z += 0.2f;
			// eye.z += 0.2f;
			vec3 right = normalize(cross(manMoveDir, up));
			manTrans += right * 0.2f;
			eye += right * 0.2f;
			lookAt = manTrans;


			manState = WALKING;

			if (debug_pos) {
				cout << "eye: " << eye.x << " " << eye.y << " " << eye.z << endl;
				cout << "lookAt: " << lookAt.x << " " << lookAt.y << " " << lookAt.z << endl;
			}
		} else if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
			manState = STANDING;
		}
		if (glfwGetKey(window, GLFW_KEY_Q)){
			lightTrans += 1.0;
		}
		if (glfwGetKey(window, GLFW_KEY_E)){
			lightTrans -= 1.0;
		}

		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
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

		eye = manTrans - front;
		lookAt = manTrans;

		manRot.y = theta + radians(-90.0f);
		manRot.y = - manRot.y;
		manRot.x = phi;

		// cout << "Theta: " << theta << " Phi: " << phi << endl;

		manMoveDir = vec3(sin(manRot.y), 0, cos(manRot.y));

		// lookAt = eye + front;


	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX << " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}


	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

<<<<<<< Updated upstream
		// Set background color.
=======
		// Set background color and enable z-buffer test
>>>>>>> Stashed changes
		glClearColor(.12f, .34f, .56f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		// Initialize the GLSL program.
		solidColorProg = make_shared<Program>();
		solidColorProg->setVerbose(true);
		solidColorProg->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/solid_frag.glsl");
		solidColorProg->init();
		solidColorProg->addUniform("P");
		solidColorProg->addUniform("V");
		solidColorProg->addUniform("M");
		solidColorProg->addUniform("solidColor");
		solidColorProg->addAttribute("vertPos");
		solidColorProg->addAttribute("vertNor");

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

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("Texture0");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");
		texProg->addUniform("MatAmb");
		texProg->addUniform("MatSpec");
		texProg->addUniform("MatShine");
		for (int i = 0; i < NUM_LIGHTS; i++) {
			texProg->addUniform("lightPos[" + to_string(i) + "]");
			texProg->addUniform("lightColor[" + to_string(i) + "]");
			texProg->addUniform("lightIntensity[" + to_string(i) + "]");
		}

		texProg->addUniform("numLights");

		skyProg = make_shared<Program>();
		skyProg->setVerbose(true);
		skyProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
		skyProg->init();
		skyProg->addUniform("P");
		skyProg->addUniform("V");
		skyProg->addUniform("M");
		skyProg->addUniform("skybox");
		skyProg->addAttribute("vertPos");
		skyProg->addAttribute("vertNor");
		skyProg->addUniform("MatShine");
		skyProg->addUniform("lightPos[0]");
		// skyProg->addUniform("lightPos[1]");
		skyProg->addUniform("lightColor[0]");
		// skyProg->addUniform("lightColor[1]");
		skyProg->addUniform("lightIntensity[0]");
		// skyProg->addUniform("lightIntensity[1]");
		// for (int i = 0; i < 1; i++) {
		// 	skyProg->addUniform("lightPos[" + to_string(i) + "]");
		// 	skyProg->addUniform("lightColor[" + to_string(i) + "]");
		// 	skyProg->addUniform("lightIntensity[" + to_string(i) + "]");
		// }
		skyProg->addUniform("numLights");


		assimptexProg = make_shared<Program>();
			assimptexProg->setVerbose(true);
			assimptexProg->setShaderNames(resourceDirectory + "/assimp_tex_vert.glsl", resourceDirectory + "/assimp_tex_frag.glsl");
			assimptexProg->init();
			assimptexProg->addUniform("P");
			assimptexProg->addUniform("V");
			assimptexProg->addUniform("M");

			assimptexProg->addUniform("texture_diffuse1");
			assimptexProg->addUniform("texture_specular1");
			// assimptexProg->addUniform("texture_normal1");
			// assimptexProg->addUniform("texture_height1");
			assimptexProg->addUniform("texture_roughness1");
			assimptexProg->addUniform("texture_metalness1");
			assimptexProg->addUniform("texture_emission1");
			assimptexProg->addAttribute("vertPos");
			assimptexProg->addAttribute("vertNor");
			assimptexProg->addAttribute("vertTex");
			// assimptexProg->addAttribute("vertTan");
			// assimptexProg->addAttribute("vertBitan");
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


		//read in a load the texture
		// texture0 = make_shared<Texture>();
		// texture0->setFilename(resourceDirectory + "/asphalt-texture-close-up.jpg");
		// texture0->init();
		// texture0->setUnit(0);
		// texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		updateCameraVectors();



	}

	void initGeom(const std::string& resourceDirectory)
	{

		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information.We'll ignore them for this assignment.
 		string errStr;


		// vector<tinyobj::shape_t> TOshapes4;
		// vector<tinyobj::material_t> objMaterials4;

		// bool rc = tinyobj::LoadObj(TOshapes4, objMaterials4, errStr, (resourceDirectory + "/rest_of_gaming_room.obj").c_str());


		//read out information stored in the shape about its size - something like this...
		//then do something with that information.....
		// gMin.x = mesh->min.x;
		// gMin.y = mesh->min.y;

		// stickfigure_running = new AssimpModel(resourceDirectory + "/stickfigure_anim.fbx");
		// stickfigure_anim = new Animation(resourceDirectory + "/stickfigure_anim.fbx", stickfigure_running, 0);
		// stickfigure_animator = new Animator(stickfigure_anim);

		// vampire = new AssimpModel(resourceDirectory + "/vampire/dancing_vampire.dae");
		// stickfigure_running = new AssimpModel(resourceDirectory + "/wolf2.fbx");
		stickfigure_running = new AssimpModel(resourceDirectory + "/Vanguard/Vanguard.fbx");
		// stickfigure_running =  new AssimpModel(resourceDirectory + "/vampire/dancing_vampire.dae");
		// stickfigure_running =  new AssimpModel(resourceDirectory + "/Walking/Walking.dae");

		// cout << "gMax: x: " << stickfigure_running->boundingBoxMax.x << " y: " << stickfigure_running->boundingBoxMax.y << " z: " << stickfigure_running->boundingBoxMax.z << endl;
		// cout << "gMin: x: " << stickfigure_running->boundingBoxMin.x << " y: " << stickfigure_running->boundingBoxMin.y << " z: " << stickfigure_running->boundingBoxMin.z << endl;
		// stickfigure_anim = new Animation(resourceDirectory + "/wolf2.fbx", stickfigure_running, 0);
		// stickfigure_idle = new Animation(resourceDirectory + "/wolf.fbx", stickfigure_running, 1);
		// stickfigure_anim = new Animation(resourceDirectory + "/vampire/dancing_vampire.dae", stickfigure_running, 0);
		// stickfigure_anim = new Animation(resourceDirectory + "/Walking/Walking.dae", stickfigure_running, 0);
		stickfigure_anim = new Animation(resourceDirectory + "/Vanguard/Vanguard.fbx", stickfigure_running, 0);
		stickfigure_idle = new Animation(resourceDirectory + "/Vanguard/Vanguard.fbx", stickfigure_running, 1);
		stickfigure_animator = new Animator(stickfigure_anim);

		cube = new AssimpModel(resourceDirectory + "/cube.obj");


	}

<<<<<<< Updated upstream
	unsigned int createSky(string dir, vector<string> faces) {
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false);
		for(GLuint i = 0; i < faces.size(); i++) {
		unsigned char *data =
		stbi_load((dir+faces[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
		0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		} else {
		cout << "failed to load: " << (dir+faces[i]).c_str() << endl;
		}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		// cout << " creating cube map any errors : " << glGetError() << endl;
		return textureID;
	}
=======
		wizard_hat = new AssimpModel(resourceDirectory + "/Wizardhat/hat_LP.obj");

		wizard_hat->assignTexture("texture_diffuse1", resourceDirectory + "/Wizardhat/textures/diffuse.png");
		wizard_hat->assignTexture("texture_roughness1", resourceDirectory + "/Wizardhat/textures/roughness.png");
		wizard_hat->assignTexture("texture_normal1", resourceDirectory + "/Wizardhat/textures/normal.png");



		// add 2 instances of the barrel to the collectibles vector Collectible(<model>, <position>)
		collectibles.push_back(Collectible(barrel, vec3(3.0f, 0.0f, 1.0f)));
		collectibles.push_back(Collectible(barrel, vec3(-2.0f, 0.0f, 2.0f)));
		collectibles.push_back(Collectible(wizard_hat, vec3(-10.0f, -4.0f, -2.0f)));
		collectibles.push_back(Collectible(wizard_hat, vec3(10.0f, -4.0f, -2.0f)));
>>>>>>> Stashed changes

	void SetMaterialMonitor(shared_ptr<Program> curS) {
		glUniform3f(curS->getUniform("MatAmb"), 0.7f, 0.7f, 0.7f); // silver ambient
		glUniform3f(curS->getUniform("MatDif"), 0.85f, 0.85f, 0.85f); // dark white diffuse
		glUniform3f(curS->getUniform("MatSpec"), 1.0f, 1.0f, 1.0f); // white specular
		glUniform1f(curS->getUniform("MatShine"), 60.0f); // medium shine
	}

	void SetMaterialGamingChair(shared_ptr<Program> curS) {
		glUniform3f(curS->getUniform("MatAmb"), 0.0f, 0.0f, 0.0f); // black ambient
		glUniform3f(curS->getUniform("MatDif"), 1.0f, 0.0f, 0.0f); // black diffuse
		glUniform3f(curS->getUniform("MatSpec"), 0.5f, 0.5f, 0.5f); // grey specular
		glUniform1f(curS->getUniform("MatShine"), 60.0f); // low shine
	}

	void SetMaterialGamingRoom(shared_ptr<Program> curS) {
		glUniform3f(curS->getUniform("MatAmb"), 0.0f, 0.0f, 0.0f); // black ambient
		glUniform3f(curS->getUniform("MatDif"), 0.05f, 0.05f, 0.05f); // black diffuse
		glUniform3f(curS->getUniform("MatSpec"), 1.0f, 1.0f, 1.0f); // grey specular
		glUniform1f(curS->getUniform("MatShine"), 60.0f); // low shine
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

	void initBboxpos() {
		// lightcycle1_bbox_max += start_lightcycle1_pos;
		// lightcycle1_bbox_min += start_lightcycle1_pos;
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

	//directly pass quad for the ground to the GPU
	void initGround() {
		float g_groundSize = groundSize;
		float g_groundY = 0.0f;
		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};
		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};
		unsigned short idx[] = { 0, 1, 2, 0, 2, 3 };

		// Generate the ground VAO
		glGenVertexArrays(1, &GroundVertexArrayID);
		glBindVertexArray(GroundVertexArrayID);

		g_GiboLen = 6;
		glGenBuffers(1, &GrndBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

		glGenBuffers(1, &GrndNorBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

		glGenBuffers(1, &GIndxBuffObj);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
	}

      //code to draw the ground plane
	  void drawGround(shared_ptr<Program> curS, std::shared_ptr<MatrixStack> Model) {
		curS->bind();
		glBindVertexArray(GroundVertexArrayID);

		// Set material for ground
		SetMaterialMan(curS, 1);

		// Use the matrix stack for the model matrix
		Model->pushMatrix();
		Model->loadIdentity();
		Model->translate(vec3(0, 0, 0));
		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		// Draw
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		Model->popMatrix();
		curS->unbind();
	}


	float randFloat(float l, float h)
	{
		float r = rand() / (float) RAND_MAX;
		return (1.0f - r) * l + r * h;
	}


	bool isCollision(vec3 p1, vec3 p2) {
		return (p2.x - p1.x <= 0.001) && (p2.z - p1.z <= 0.001);
	}


	void render(float frametime, float animTime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		// Projection->perspective(45.0f, aspect, 0.01f, 200.0f);
		Projection->perspective(45.0f, aspect, 0.01f, 400.0f);

		// View is global translation along negative z for now
		View->pushMatrix();
			View->loadIdentity();
			// View->translate(vec3(0, 0, -5));
			View->lookAt(eye, lookAt, vec3(0, 1, 0));


			// }
		// Draw a solid colored sphere
		solidColorProg->bind();
		//send the projetion and view for solid shader
		glUniformMatrix4fv(solidColorProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(solidColorProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
		//send in the color to use

		glUniform3f(solidColorProg->getUniform("solidColor"), 1.0, 1.0, 1.0); // white light




		// different color for different objects

		solidColorProg->unbind();


		prog2->bind();
		glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog2->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));

		glUniform3f(prog2->getUniform("lightColor[0]"), 1.0, 1.0, 1.0); // white light
		glUniform1f(prog2->getUniform("lightIntensity[0]"), 1.0); // light intensity
		glUniform3f(prog2->getUniform("lightPos[0]"), 0, 2, 0); // light position at the computer screen

		glUniform1i(prog2->getUniform("numLights"), 1); // light position at the computer screen

		// glUniform1i(prog2->getUniform("hasEmittance"), 1);
		// glUniform3f(prog2->getUniform("MatEmitt"), 1.0, 1.0, 1.0); // white light
		drawGround(prog2, Model);




		prog2->unbind();

		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));


		glUniform3f(texProg->getUniform("lightColor[0]"), 1.0, 1.0, 1.0); // white light
		glUniform1f(texProg->getUniform("lightIntensity[0]"), 1.0); // light intensity
		glUniform3f(texProg->getUniform("lightPos[0]"), 0, 2, 0); // light position at the computer screen

		glUniform1i(texProg->getUniform("numLights"), 1); // light position at the computer screen


		texProg->unbind();

		skyProg->bind();
		glUniformMatrix4fv(skyProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(skyProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));

		skyProg->unbind();

		assimptexProg->bind();
			glUniformMatrix4fv(assimptexProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
			glUniformMatrix4fv(assimptexProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));


			glUniform3f(assimptexProg->getUniform("lightColor[0]"), 1.0, 1.0, 1.0); // white light
			glUniform1f(assimptexProg->getUniform("lightIntensity[0]"), 0.0); // light intensity
			glUniform3f(assimptexProg->getUniform("lightPos[0]"), 0, 10, 0); // light position at the computer screen

			glUniform1i(assimptexProg->getUniform("numLights"), 1); // light position at the computer screen

			stickfigure_animator->UpdateAnimation(1.5 * animTime);
			if (manState == WALKING) {
				// stickfigure_animator->UpdateAnimation(1.5 * animTime);
				stickfigure_animator->SetCurrentAnimation(stickfigure_anim);
			} else if (manState == STANDING) {
				// stickfigure_animator->UpdateAnimation(0);
				stickfigure_animator->SetCurrentAnimation(stickfigure_idle);
			}

			vector<glm::mat4> transforms = stickfigure_animator->GetFinalBoneMatrices();
			for (int i = 0; i < transforms.size(); ++i) {
				glUniformMatrix4fv(assimptexProg->getUniform("finalBonesMatrices[" + std::to_string(i) + "]"), 1, GL_FALSE, value_ptr(transforms[i]));
			}


			Model->pushMatrix();
				Model->loadIdentity();
				Model->translate(manTrans);
				Model->scale(0.01f);
				// Model->scale(1.0f);
				// Model->rotate(radians(90.0f), vec3(0, 1, 0));
				Model->rotate(manRot.y, vec3(0, 1, 0));
				// Model->rotate(manRot.x, vec3(1, 0, 0));
				Model->rotate(manRot.z, vec3(0, 0, 1));


				// glm::mat4 transform = Model->topMatrix();

				// glm::vec3 worldMin, worldMax;
				// updateBoundingBox(stickfigure_running->boundingBoxMin, stickfigure_running->boundingBoxMax, transform, worldMin, worldMax);
				// stickfigure_running->boundingBoxMin = worldMin;
				// stickfigure_running->boundingBoxMax = worldMax;


				glUniform1i(assimptexProg->getUniform("hasTexture"), 1);
				SetMaterialMan(assimptexProg, 0);
				setModel(assimptexProg, Model);
				stickfigure_running->Draw(assimptexProg);


			Model->popMatrix();

			// Model->pushMatrix();
			// 	Model->loadIdentity();
			// 	Model->translate(vec3(0, 0, 0));
			// 	// Model->scale(0.25f);
			// 	Model->scale(0.01f);
			// 	glUniform1i(assimptexProg->getUniform("hasTexture"), 0);
			// 	setModel(assimptexProg, Model);
			// 	stickfigure_running->Draw(assimptexProg);
			// Model->popMatrix();

			// Model->pushMatrix();
			// 	Model->loadIdentity();
			// 	Model->translate(vec3(0, 0, 0));
			// 	Model->scale(0.1f);
			// 	glUniform1i(assimptexProg->getUniform("hasTexture"), 1);
			// 	setModel(assimptexProg, Model);
			// 	vampire->Draw(assimptexProg);
			// Model->popMatrix();

			// Model->pushMatrix();
			// 	Model->loadIdentity();
			// 	Model->translate(vec3(0, 0, 0));
			// 	Model->scale(1.0f);
			// 	glUniform1i(assimptexProg->getUniform("hasTexture"), 0);
			// 	SetMaterialMan(assimptexProg, 1);
			// 	setModel(assimptexProg, Model);
			// 	wolf->Draw(assimptexProg);
			// Model->popMatrix();

		// Model->pushMatrix();
		// 	Model->loadIdentity();
		// 	Model->translate(stickfigure_running->boundingBoxMax);
		// 	Model->scale(0.1f);
		// 	glUniform1i(assimptexProg->getUniform("hasTexture"), 0);
		// 	SetMaterialMan(assimptexProg, 1);
		// 	setModel(assimptexProg, Model);
		// 	cube->Draw(assimptexProg);
		// Model->popMatrix();

		// Model->pushMatrix();
		// 	Model->loadIdentity();
		// 	Model->translate(stickfigure_running->boundingBoxMin);
		// 	Model->scale(0.1f);
		// 	glUniform1i(assimptexProg->getUniform("hasTexture"), 0);
		// 	SetMaterialMan(assimptexProg, 1);
		// 	setModel(assimptexProg, Model);
		// 	cube->Draw(assimptexProg);
		// Model->popMatrix();


		assimptexProg->unbind();

		// Pop matrix stacks.
		Projection->popMatrix();
		View->popMatrix();

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

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	// glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetWindowUserPointer(windowManager->getHandle(), application);
	glfwSetCursorPosCallback(windowManager->getHandle(), mouseMoveCallbackWrapper);

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);
	application->initGround();
	application->initBboxpos();

	auto lastTime = chrono::high_resolution_clock::now();

	glfwSetInputMode(windowManager->getHandle(), GLFW_STICKY_KEYS, GLFW_TRUE);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		auto nextLastTIme = chrono::high_resolution_clock::now();

		float deltaTime = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - lastTime).count();

		deltaTime *= 0.000001f; // convert to seconds

		lastTime = nextLastTIme;

		float AnimcurrFrame = glfwGetTime();
		application->AnimDeltaTime = AnimcurrFrame - application->AnimLastFrame;
		application->AnimLastFrame = AnimcurrFrame;
		// Render scene.
		application->render(deltaTime, application->AnimDeltaTime);

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
