#include <SDL.h>
#include <SDL_image.h>
#include "stdbool.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;
const int FRAME_RATE = 16;
const int TICKS_PER_FRAME = 1000 / FRAME_RATE;
const int FONT_RESOLUTION = 8;
const int FONT_SCALE = 2;
const int LINE_BUFFER_SIZE = 255;

typedef struct LTimer {
    bool started;
    Uint64 startTicks;
} LTimer;

typedef struct LTexture {
    SDL_Texture* texture;
    int w;
    int h;
} LTexture;

SDL_Color fontColor = {255, 172, 28, 255};
SDL_Window* gWindow = NULL;
SDL_Surface* gSurface = NULL;
SDL_Renderer* gRenderer = NULL;
LTexture gFontSprite;
LTimer gTickTimer;

void startTimer(LTimer* t) {
    t->started = true;
    t->startTicks = SDL_GetTicks64();
}
void stopTimer(LTimer* t) {
    t->started = false;
    t->startTicks = 0;
}

bool loadFontSprite() {
    SDL_Surface* surface = IMG_Load("./resources/evanFont2.png");
    if (surface == NULL) {
        SDL_Log("Failed to create text surface from png!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 255 , 255));

    gFontSprite.texture = SDL_CreateTextureFromSurface(gRenderer, surface);
    if (gFontSprite.texture == NULL) {
        SDL_Log("Failed to render text texture from surface!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    gFontSprite.w = surface->w;
    gFontSprite.h = surface->h;
    SDL_SetTextureColorMod(gFontSprite.texture, fontColor.r, fontColor.g, fontColor.b);
    SDL_FreeSurface(surface);
    return true;
}

void renderText(const int x, const int y, const char* text) {
    SDL_Rect renderQuad = {x, y, FONT_RESOLUTION * FONT_SCALE, FONT_RESOLUTION * FONT_SCALE};
    SDL_Rect clip = {0,0,FONT_RESOLUTION, FONT_RESOLUTION};
    for (int i = 0; i < strlen(text); i++) {
        char c = text[i];
        int offset = 0;
        if (c >= 'A' && c <= 'Z') {
            offset = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            offset = c - 'a';
        } else if (c >= '1' && c <= '9') {
            offset = c - '1' + 26;
        } else {
            offset = 63;
        }
        clip.x = FONT_RESOLUTION * (offset % FONT_RESOLUTION);
        clip.y = FONT_RESOLUTION * (offset / FONT_RESOLUTION);
        SDL_RenderCopy(gRenderer, gFontSprite.texture, &clip, &renderQuad);
        renderQuad.x = renderQuad.x + FONT_RESOLUTION * FONT_SCALE;
    }
}

void renderTexture(const int x, const int y, LTexture* texture) {
    const SDL_Rect renderQuad = {x, y, texture->w, texture->h};
    // SDL_SetTextureColorMod(texture->texture, color_coral.r, color_coral.g, color_coral.b);
    SDL_RenderCopy(gRenderer, texture->texture, NULL, &renderQuad);
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Failed to init SDL!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    gWindow = SDL_CreateWindow("carplay", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        SDL_Log("Failed to create SDL Window!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == NULL) {
        SDL_Log("Failed to create renderer!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    if (!IMG_Init(IMG_INIT_PNG)) {
        SDL_Log("SDL IMG could not init!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool loadMedia() {
    return loadFontSprite();
}

void cleanup() {
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    bool quit = false;
    SDL_Event e;
    char lineBuffer[LINE_BUFFER_SIZE];
    int linePos = 0;


    if (!(init() && loadMedia())) {
        return 0;
    }

    startTimer(&gTickTimer);

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym >= 'a' && e.key.keysym.sym <= 'z' || e.key.keysym.sym >= '0' && e.key.keysym.sym <= '9' || e.key.keysym.sym == ' ') {
                    lineBuffer[linePos++] = e.key.keysym.sym;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    lineBuffer[--linePos] = EOF;
                }
            }
        }

        SDL_SetRenderDrawColor(gRenderer, 139,64,0,255);
        SDL_RenderClear(gRenderer);
        renderText(0, 0, lineBuffer);

        SDL_RenderPresent(gRenderer);
    }
    cleanup();
    return 0;
}