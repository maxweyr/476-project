#pragma once

#include "../seecs.h"
#include "../Components.h"
#include "../WindowManager.h"
#include "../Config.h"
#include "Random.h"
#include <GLFW/glfw3.h>

class WindowManager;

class InputSystem {
public:
    static void Update(seecs::ECS& ecs, WindowManager* windowManager);
    static void HandleKeyEvent(seecs::ECS& ecs, int key, int action);
    static void ResetParticleSystem(seecs::ECS& ecs);
};