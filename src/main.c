#include <SDL.h>
#include <SDL_image.h>
#include "stdbool.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;

typedef struct SdlInstance {

} SdlInstance;

SDL_Window* gWindow = NULL;
SDL_Surface* gSurface = NULL;
SDL_Renderer* gRenderer = NULL;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Failed to init sdl!");
        return false;
    }
    gWindow = SDL_CreateWindow("carplay", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Failed to create sdl window!");
        return false;
    }
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == NULL) {
        printf("Failed to create sdl renderer!");
        return false;
    }
    if (!IMG_Init(IMG_INIT_PNG)) {
        printf("Failed to init sdl img!");
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    bool quit = false;
    SDL_Event e;

    if (!init()) {
        printf("Failed to init\n");
        return -1;
    }

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(gRenderer, 0,0,0,255);
        SDL_RenderClear(gRenderer);

        SDL_Rect r = {20, 20, 300, 200};
        SDL_SetRenderDrawColor(gRenderer, 0, 200, 200, 255);
        SDL_RenderFillRect(gRenderer, &r);

        SDL_RenderPresent(gRenderer);
    }

    return 0;
}