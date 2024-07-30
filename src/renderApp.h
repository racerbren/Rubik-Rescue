#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

class renderApp 
{
private:
    void init();
    void loop();
    void clean();
public:
    void run();
};