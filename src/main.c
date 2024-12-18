#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "stdbool.h"

const bool DEBUG_WINDOW = true;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;
const int DEBUG_WIDTH = 480;
const int DEBUG_HEIGHT = 600;
const int FRAME_RATE = 16;
const int TICKS_PER_FRAME = 1000 / FRAME_RATE;
const int FONT_RESOLUTION = 8;
const int FONT_SCALE = 2;
const int FONT_SIZE = FONT_RESOLUTION * FONT_SCALE;
const int LINE_SPACE = 8;
const int LINE_BUFFER_SIZE = 255;

const SDL_Color fontColor = {255, 172, 28, 255};
const SDL_Color bgColor = {126,47,8, 255};

typedef struct LTimer {
    bool started;
    Uint64 startTicks;
} LTimer;

typedef struct LTexture {
    SDL_Texture* texture;
    int w;
    int h;
} LTexture;

SDL_Window* gWindow = NULL;
SDL_Window* dWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Renderer* dRenderer = NULL;
LTexture gFontSprite;
int linePos = 0;
char lineBuffer[LINE_BUFFER_SIZE];

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
    SDL_Rect renderQuad = {x, y, FONT_SIZE, FONT_SIZE};
    SDL_Rect clip = {0,0,FONT_RESOLUTION, FONT_RESOLUTION};
    for (int i = 0; i < strlen(text); i++) {
        char c = text[i];
        int offset = 0;
        if (c >= 'A' && c <= 'Z') {
            offset = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            offset = c - 'a';
        } else if (c >= '0' && c <= '9') {
            offset = c - '0' + 26;
        } else if (c == ' ') {
            offset = 63;
        } else if (c == '\n') {
            renderQuad.x = 0;
            renderQuad.y += FONT_SIZE + LINE_SPACE;
            continue;
        } else {
            continue;
        }

        clip.x = FONT_RESOLUTION * (offset % FONT_RESOLUTION);
        clip.y = FONT_RESOLUTION * (offset / FONT_RESOLUTION);
        SDL_RenderCopy(gRenderer, gFontSprite.texture, &clip, &renderQuad);
        renderQuad.x += FONT_SIZE;
    }
}

void renderTexture(const int x, const int y, LTexture* texture) {
    const SDL_Rect renderQuad = {x, y, texture->w, texture->h};
    SDL_RenderCopy(gRenderer, texture->texture, NULL, &renderQuad);
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Failed to init SDL!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    if (!IMG_Init(IMG_INIT_PNG)) {
        SDL_Log("SDL IMG could not init!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    if (TTF_Init() == -1) {
        SDL_Log("SDL TTF could not init!\nSDL_Error: %s", SDL_GetError());
        return false;
    }

    gWindow = SDL_CreateWindow("carplay", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    dWindow = SDL_CreateWindow("carplay debug", SCREEN_WIDTH + 10, 0, DEBUG_WIDTH, DEBUG_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL || dWindow == NULL) {
        SDL_Log("Failed to create SDL Window!\nSDL_Error: %s", SDL_GetError());
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    dRenderer = SDL_CreateRenderer(dWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == NULL || dRenderer == NULL) {
        SDL_Log("Failed to create renderer!\nSDL_Error: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool loadMedia() {
    return loadFontSprite();
}

void destroyDebugWindow() {
    SDL_DestroyRenderer(dRenderer);
    SDL_DestroyWindow(dWindow);
}

void cleanup() {
    destroyDebugWindow();
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    IMG_Quit();
    SDL_Quit();
}

void handleKeyPress(SDL_Keysym ks) {
    if (ks.sym >= SDLK_a && ks.sym <= SDLK_z || ks.sym >= SDLK_0 && ks.sym <= SDLK_9 || ks.sym == SDLK_SPACE) {
        lineBuffer[linePos++] = ks.sym;
    } else if (ks.sym == SDLK_BACKSPACE) {
        lineBuffer[--linePos] = '\0';
    } else if (ks.sym == SDLK_RETURN) {
        lineBuffer[linePos++] = '\n';
    }
}

int main(int argc, char *argv[]) {
    bool quit = false;
    SDL_Event e;

    if (!(init() && loadMedia())) {
        return 0;
    }

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
                if (SDL_GetWindowID(dWindow) == e.window.windowID) {
                    destroyDebugWindow();
                } else {
                    quit = true;
                }
            }
            if (e.type == SDL_KEYDOWN) {
                handleKeyPress(e.key.keysym);
            }
        }

        SDL_SetRenderDrawColor(gRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(gRenderer);

        renderText(0,1,lineBuffer);

        SDL_RenderPresent(gRenderer);
    }
    cleanup();
    return 0;
}