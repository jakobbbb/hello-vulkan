#include <iostream>
#include "glm/common.hpp"
#include "glm/gtx/norm.hpp"
#include <SDL2/SDL.h>

int main() {
    // glm
    glm::vec3 hello{1, 1, 0};
    float dist = glm::distance(glm::vec3{}, hello);
    std::cout << "Hello, " << dist << "!\n";

    // sdl2
    SDL_Window* win = NULL;
    SDL_Window* surf = NULL;
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Error initializing SDL2\n";
        return 1;
    }

    return 0;
}
