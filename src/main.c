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
const int LINE_SPACE = 8;
const int DEBUG_LINE_SPACE = 16;
const int LINE_BUFFER_SIZE = 255;

const SDL_Color fontColor = {255, 172, 28, 255};
const SDL_Color selectedFontColor = {130, 233, 211, 255};

typedef struct LTimer {
    bool started;
    Uint64 startTicks;
} LTimer;

typedef struct LTexture {
    SDL_Texture* sdlTexture;
    int w;
    int h;
} LTexture;

typedef struct LDebugOption {
    const char* description;
    int value;
    int index;
    int min;
    int max;
    LTexture* lTexture;
    SDL_Rect* renderQuad;
} LDebugOption;

typedef enum DebugOption {
    DEBUG_BACKGROUND_COLOR_R,
    DEBUG_BACKGROUND_COLOR_G,
    DEBUG_BACKGROUND_COLOR_B,
    DEBUG_FONT_SCALE,
    DEBUG_PROPERTY_COUNT
} DebugOption;

typedef enum MenuState {
    MENU_WELCOME,
    MENU_NAVIGATE,
    MENU_ARTISTS,
    MENU_PLAYLISTS,
    MENU_SHUFFLE,
    MENU_PROPERTY_COUNT
} MenuState;
char* menuTexts[] = {
    "Welcome to miata\n\nPress any key to enter",
    "1. Artists\n2. Playlists\n3. Shuffle All",
    "0. Back\n1. Artist 1\n2. Artist 2\n",
    "0. Back\n1. Playlist 1\n2. Playlist 2\n",
    "0. Back\n1. Reshuffle 1\n",
};

SDL_Window* gWindow = NULL;
SDL_Window* dWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Renderer* dRenderer = NULL;
TTF_Font* dFont = NULL;
LTexture gFontSprite;
MenuState menuState = MENU_WELCOME;
int linePos = 0;
int selectedOption = 0;
LDebugOption debugOptions[DEBUG_PROPERTY_COUNT];

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

    gFontSprite.sdlTexture = SDL_CreateTextureFromSurface(gRenderer, surface);
    SDL_SetTextureColorMod(gFontSprite.sdlTexture, fontColor.r, fontColor.g, fontColor.b);
    if (gFontSprite.sdlTexture == NULL) {
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
    sprintf(r, " %-12s: %-d", debugOption.description, debugOption.value);
    return r;
}

SDL_Rect* getRenderQuad(const int x, const int y, const int w, const int h) {
    SDL_Rect* rq = malloc(sizeof(SDL_Rect));
    rq->x = x;
    rq->y = y;
    rq->w = w;
    rq->h = h;
    return rq;
}

LTexture* createTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
    LTexture* lTexture = malloc(sizeof(LTexture));
    lTexture->sdlTexture = malloc(sizeof(SDL_Texture*));
    lTexture->sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
    lTexture->w = surface->w;
    lTexture->h = surface->h;
    if (lTexture->sdlTexture == NULL) {
        SDL_Log("TTF failed to render texture from surface!\nSDL_Error: %s", SDL_GetError());
        return NULL;
    }
    return lTexture;
}

bool renderFontForDebugOption(LDebugOption* debugOption) {
    free(debugOption->lTexture);
    free(debugOption->renderQuad);
    SDL_Surface* fontSurface = TTF_RenderText_Solid(dFont, formatDebugFont(*debugOption), fontColor);
    if (fontSurface == NULL) {
        SDL_Log("TTF failed to render text to surface!\nSDL_Error: %s", SDL_GetError());
        return false;
    }

    debugOption->lTexture = createTextureFromSurface(dRenderer, fontSurface);
    debugOption->renderQuad = getRenderQuad(0, DEBUG_LINE_SPACE * debugOption->index, fontSurface->w, fontSurface->h);
    const SDL_Color color = debugOption->index == selectedOption ? selectedFontColor : fontColor;
    SDL_SetTextureColorMod(debugOption->lTexture->sdlTexture, color.r, color.g, color.b);
    SDL_FreeSurface(fontSurface);
    return true;
}

LDebugOption* mallocDebugOption(const char* description, const int index, const int value, const int min, const int max) {
    LDebugOption* db = malloc(sizeof(LDebugOption));
    db->description = description;
    db->index = index;
    db->value = value;
    db->min = min;
    db->max = max;
    return db;
}

void updDebug(const int index, const char* description, const int value, const int min, const int max) {
    debugOptions[index].index = index;
    debugOptions[index].description = description;
    debugOptions[index].value = value;
    debugOptions[index].min = min;
    debugOptions[index].max = max;
}

void populateDebugOptions() {
    updDebug(DEBUG_BACKGROUND_COLOR_R, "r", 60, 0, 255);
    updDebug(DEBUG_BACKGROUND_COLOR_G, "g", 25, 0, 255);
    updDebug(DEBUG_BACKGROUND_COLOR_B, "b", 0, 0, 255);
    updDebug(DEBUG_FONT_SCALE, "font scale", 3, 1, 9);
}

bool loadMedia() {
    populateDebugOptions();
    return loadFontSprite() && loadTTFs();
}

void renderText(const int x, const int y, const char* text) {
    const int fontSize = debugOptions[DEBUG_FONT_SCALE].value * FONT_RESOLUTION;
    SDL_Rect renderQuad = {x, y, fontSize, fontSize};
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
            renderQuad.y += fontSize + LINE_SPACE;
            continue;
        } else {
            continue;
        }

        clip.x = FONT_RESOLUTION * (offset % FONT_RESOLUTION);
        clip.y = FONT_RESOLUTION * (offset / FONT_RESOLUTION);
        SDL_RenderCopy(gRenderer, gFontSprite.sdlTexture, &clip, &renderQuad);
        renderQuad.x += fontSize;
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

// void handleMainMenuTypingInput(const SDL_Keysym ks) {
//     if (ks.sym >= SDLK_a && ks.sym <= SDLK_z || ks.sym >= SDLK_0 && ks.sym <= SDLK_9 || ks.sym == SDLK_SPACE) {
//         displayText[linePos++] = ks.sym;
//     } else if (ks.sym == SDLK_BACKSPACE) {
//         displayText[--linePos] = '\0';
//     } else if (ks.sym == SDLK_RETURN) {
//         displayText[linePos++] = '\n';
//     }
// }

void handleMenuNavigate(const SDL_Keysym ks) {
    switch (ks.sym) {
        case SDLK_1:
            menuState = MENU_ARTISTS;
            break;
        case SDLK_2:
            menuState = MENU_PLAYLISTS;
            break;
        case SDLK_3:
            menuState = MENU_SHUFFLE;
            break;
        default:
            break;
    }
}

void handleMenuArtists(const SDL_Keysym ks) {
    switch (ks.sym) {
        case SDLK_0:
            menuState = MENU_NAVIGATE;
            break;
        default:
            break;
    }
}

void handleMenuPlaylists(const SDL_Keysym ks) {
    switch (ks.sym) {
        case SDLK_0:
            menuState = MENU_NAVIGATE;
        break;
        default:
            break;
    }
}

void handleMenuShuffle(const SDL_Keysym ks) {
    switch (ks.sym) {
        case SDLK_0:
            menuState = MENU_NAVIGATE;
        break;
        default:
            break;
    }
}

void handleMainWindowMenuNav(const SDL_Keysym ks) {
    if (ks.sym == SDLK_BACKSPACE) {
        menuState = MENU_WELCOME;
    }
    switch (menuState) {
        case MENU_WELCOME:
            menuState = MENU_NAVIGATE;
            break;
        case MENU_NAVIGATE:
            handleMenuNavigate(ks);
            break;
        case MENU_ARTISTS:
            handleMenuArtists(ks);
            break;
        case MENU_PLAYLISTS:
            handleMenuPlaylists(ks);
            break;
        case MENU_SHUFFLE:
            handleMenuShuffle(ks);
            break;
        default:
            break;
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
            if (selectedOption + 1 < DEBUG_PROPERTY_COUNT) {
                selectedOption++;
                renderFontForDebugOption(&debugOptions[selectedOption]);
                renderFontForDebugOption(&debugOptions[selectedOption - 1]);
            }
            break;
        case SDLK_LEFT:
            if (shifted) {
                if (debugOptions[selectedOption].value - 5 > debugOptions[selectedOption].min) {
                    debugOptions[selectedOption].value -= 5;
                } else {
                    debugOptions[selectedOption].value = debugOptions[selectedOption].min;
                }
            } else if (debugOptions[selectedOption].value - 1 >= debugOptions[selectedOption].min) {
                debugOptions[selectedOption].value--;
            }
            renderFontForDebugOption(&debugOptions[selectedOption]);
            break;
        case SDLK_RIGHT:
            if (shifted) {
                if (debugOptions[selectedOption].value + 5 < debugOptions[selectedOption].max) {
                    debugOptions[selectedOption].value += 5;
                } else {
                    debugOptions[selectedOption].value = debugOptions[selectedOption].max;
                }
            } else if (debugOptions[selectedOption].value + 1 <= debugOptions[selectedOption].max) {
                debugOptions[selectedOption].value++;
            }
            renderFontForDebugOption(&debugOptions[selectedOption]);
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

    for (int i = 0; i < DEBUG_PROPERTY_COUNT; i++) {
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
                    handleMainWindowMenuNav(e.key.keysym);
                } else if (e.window.windowID == SDL_GetWindowID(dWindow)) {
                    handleDebugWindowKeypress(e.key.keysym);
                }
            }

        }

        //main window
        SDL_SetRenderDrawColor(gRenderer, debugOptions[0].value, debugOptions[1].value, debugOptions[2].value, 255);
        SDL_RenderClear(gRenderer);
        renderText(0,1,menuTexts[menuState]);
        SDL_RenderPresent(gRenderer);

        //debug window
        if (dRenderer != NULL) {
            SDL_SetRenderDrawColor(dRenderer, 33,33,33,255);
            SDL_RenderClear(dRenderer);
            for (int i = 0; i < DEBUG_PROPERTY_COUNT; i++) {
                SDL_RenderCopy(dRenderer, debugOptions[i].lTexture->sdlTexture, NULL, debugOptions[i].renderQuad);
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