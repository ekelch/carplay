#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "stdbool.h"

const bool DEBUG_WINDOW = true;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;
const int DEBUG_WIDTH = 480;
const int DEBUG_HEIGHT = 600;
const int FRAME_RATE = 12;
const int TICKS_PER_FRAME = 1000 / FRAME_RATE;
const int FONT_RESOLUTION = 8;
const int FONT_SCALE = 2;
const int FONT_SIZE = FONT_RESOLUTION * FONT_SCALE;
const int LINE_SPACE = 8;
const int DEBUG_LINE_SPACE = 16;
const int LINE_BUFFER_SIZE = 255;
const int DEBUG_OPTIONS_COUNT = 3;

const SDL_Color fontColor = {255, 172, 28, 255};
const SDL_Color selectedFontColor = {130, 233, 211, 255};
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

typedef struct LDebugOption {
    const char* description;
    int value;
    int index;
    LTexture* sdlTexture;
    SDL_Rect* renderQuad;
} LDebugOption;

SDL_Window* gWindow = NULL;
SDL_Window* dWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Renderer* dRenderer = NULL;
TTF_Font* dFont = NULL;
LTexture gFontSprite;
int linePos = 0;
char lineBuffer[LINE_BUFFER_SIZE];
int selectedOption = 0;
LDebugOption debugOptions[DEBUG_OPTIONS_COUNT] = {
    {"value 1", 23, 0},
    {"value 2", 9, 1},
    {"value 3", 4, 2}
};

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
    SDL_FreeSurface(surface);
    return true;
}

bool loadTTFs() {
    dFont = TTF_OpenFont("./resources/fira.ttf", 16);
    if (dFont == NULL) {
        SDL_Log("Failed to open TTF font!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    return true;
}

char* formatDebugFont(const LDebugOption debugOption) {
    char* r = malloc(sizeof(char) * 63);
    sprintf(r, "%s:%4d", debugOption.description, debugOption.value);
    return r;
}

bool renderFontForDebugOption(LDebugOption* debugOption) {
    SDL_Surface* fontSurface = TTF_RenderText_Solid(dFont, formatDebugFont(*debugOption), fontColor);
    if (fontSurface == NULL) {
        SDL_Log("TTF failed to render text to surface!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    free(debugOption->sdlTexture);
    free(debugOption->renderQuad);
    debugOption->sdlTexture = malloc(sizeof(SDL_Texture*));
    debugOption->sdlTexture->texture = SDL_CreateTextureFromSurface(dRenderer, fontSurface);
    if (debugOption->sdlTexture == NULL) {
        SDL_Log("TTF failed to render texture from surface!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    debugOption->sdlTexture->w = fontSurface->w;
    debugOption->sdlTexture->h = fontSurface->h;
    SDL_Rect* renderQuad = malloc(sizeof(SDL_Rect*));
    renderQuad->x = 0;
    renderQuad->y = DEBUG_LINE_SPACE * debugOption->index;
    renderQuad->w = fontSurface->w;
    renderQuad->h = fontSurface->h;
    debugOption->renderQuad = renderQuad;
    if (debugOption->index == selectedOption) {
        SDL_SetTextureColorMod(debugOption->sdlTexture->texture, selectedFontColor.r, selectedFontColor.g, selectedFontColor.b);
    } else {
        SDL_SetTextureColorMod(debugOption->sdlTexture->texture, fontColor.r, fontColor.g, fontColor.b);
    }

    SDL_FreeSurface(fontSurface);
    return true;
}

bool loadMedia() {
    return loadFontSprite() && loadTTFs();
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

void handleMainWindowKeyPress(SDL_Keysym ks) {
    if (ks.sym >= SDLK_a && ks.sym <= SDLK_z || ks.sym >= SDLK_0 && ks.sym <= SDLK_9 || ks.sym == SDLK_SPACE) {
        lineBuffer[linePos++] = ks.sym;
    } else if (ks.sym == SDLK_BACKSPACE) {
        lineBuffer[--linePos] = '\0';
    } else if (ks.sym == SDLK_RETURN) {
        lineBuffer[linePos++] = '\n';
    }
}

void handleDebugWindowKeypress(SDL_Keysym ks) {
    bool shifted = ks.mod == KMOD_LSHIFT ? true : false;
    switch (ks.sym) {
        case SDLK_UP:
            if (selectedOption - 1 >= 0) {
                selectedOption--;
                renderFontForDebugOption(&debugOptions[selectedOption]);
                renderFontForDebugOption(&debugOptions[selectedOption + 1]);
            }
            break;
        case SDLK_DOWN:
            if (selectedOption + 1 < DEBUG_OPTIONS_COUNT) {
                selectedOption++;
                renderFontForDebugOption(&debugOptions[selectedOption]);
                renderFontForDebugOption(&debugOptions[selectedOption - 1]);
            }
            break;
        case SDLK_LEFT:
            if (debugOptions->value - 1 > 0) {
                if (shifted) {
                    debugOptions[selectedOption].value -= 5;
                } else {
                    debugOptions[selectedOption].value -= 1;
                }
                renderFontForDebugOption(&debugOptions[selectedOption]);
            }
            break;
        case SDLK_RIGHT:
            if (debugOptions->value + 1 < 100) {
                if (shifted) {
                    debugOptions[selectedOption].value += 5;
                } else {
                    debugOptions[selectedOption].value += 1;
                }
                renderFontForDebugOption(&debugOptions[selectedOption]);
            }
            break;
        default:
            break;
    }

}

int main(int argc, char *argv[]) {
    bool quit = false;
    LTimer syncTimer;
    SDL_Event e;
    int32_t countedFrames = 0;

    if (!(init() && loadMedia())) {
        return 0;
    }

    for (int i = 0; i < DEBUG_OPTIONS_COUNT; i++) {
        renderFontForDebugOption(&debugOptions[i]);
    }

    while (!quit) {
        startTimer(&syncTimer);
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
                if (SDL_GetWindowID(dWindow) == e.window.windowID) {
                    destroyDebugWindow();
                } else {
                    quit = true;
                }
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.window.windowID == SDL_GetWindowID(gWindow)) {
                    handleMainWindowKeyPress(e.key.keysym);
                } else if (e.window.windowID == SDL_GetWindowID(dWindow)) {
                    handleDebugWindowKeypress(e.key.keysym);
                }
            }

        }

        //main window
        SDL_SetRenderDrawColor(gRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(gRenderer);
        renderText(0,1,lineBuffer);
        SDL_RenderPresent(gRenderer);

        //debug window
        if (dRenderer != NULL) {
            SDL_SetRenderDrawColor(dRenderer, 33,33,33,255);
            SDL_RenderClear(dRenderer);
            for (int i = 0; i < DEBUG_OPTIONS_COUNT; i++) {
                SDL_RenderCopy(dRenderer, debugOptions[i].sdlTexture->texture, NULL, debugOptions[i].renderQuad);
            }
            SDL_RenderPresent(dRenderer);
        }
        //wait
        Uint64 frameTicks = SDL_GetTicks64() - syncTimer.startTicks;
        if (frameTicks < TICKS_PER_FRAME) {
            SDL_Delay(TICKS_PER_FRAME - frameTicks);
        }
        countedFrames++;
    }
    cleanup();
    return 0;
}