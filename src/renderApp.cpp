#include "renderApp.h"

void renderApp::initWindow()
{
    SDL_Surface* surface = NULL;

    //If you cannot intialize SDL2, exit with error code    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Could not initialize SDL! SDL_Error: " << SDL_GetError() << "\n";
        throw EXIT_FAILURE;
    }

    window = SDL_CreateWindow("Vulkan Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN);

    //If you cannot create the window, exit with error code    
    if (window == NULL)
    {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        throw EXIT_FAILURE;
    }

    SDL_SetWindowResizable(window, SDL_FALSE);
}

void renderApp::initVulkan()
{

}

void renderApp::loop()
{
    SDL_Event event;
    bool running = true;

    //Main loop
    while (running)
    {
        while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                running = false;
    }
}

void renderApp::clean()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void renderApp::run()
{
    initWindow();
    initVulkan();
    loop();
    clean();
}
