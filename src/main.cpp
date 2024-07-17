#include <iostream>
#include <SDL.h>

#undef main

const uint32_t SCREEN_WIDTH = 1080;
const uint32_t SCREEN_HEIGHT = 720;

int main()
{
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;

    //If you cannot intialize SDL2, exit with error code    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Could not initialize SDL! SDL_Error: " << SDL_GetError() << "\n";
        return -1;
    }

    window = SDL_CreateWindow("SDL Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN);

    //If you cannot create the window, exit with error code    
    if (window == NULL)
    {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_Event event;
    bool running = true;

    //Main loop
    while (running)
    {
        while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                running= false;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
