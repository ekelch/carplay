#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <sys/errno.h>
#include <unistd.h>

#include "dirent.h"
#include "sys/stat.h"
#include "stdbool.h"

const bool DEBUG_WINDOW = true;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;
const int DEBUG_WIDTH = 480;
const int DEBUG_HEIGHT = 600;
const int FRAME_RATE = 16;
const int TICKS_PER_FRAME = 1000 / FRAME_RATE;
const int LINE_SPACE = 16;
const int MAP_CAPACITY = 100;


const SDL_Color fontColor = {255, 172, 28, 255};
const SDL_Color selectedFontColor = {130, 233, 211, 255};

typedef struct {
    bool started;
    Uint64 startTicks;
} LTimer;

typedef struct {
    SDL_Texture* sdlTexture;
    SDL_Rect renderQuad;
} LTexture;

typedef struct {
    const char* description;
    int value;
    int index;
    int min;
    int max;
    LTexture* lTexture;
    SDL_Rect* renderQuad;
} LDebugOption;

typedef enum {
    DEBUG_BACKGROUND_COLOR_R,
    DEBUG_BACKGROUND_COLOR_G,
    DEBUG_BACKGROUND_COLOR_B,
    DEBUG_FONT_SCALE,
    DEBUG_PROPERTY_COUNT
} DebugOption;

typedef enum {
    MENU_WELCOME,
    MENU_NAVIGATE,
    MENU_ARTISTS,
    MENU_PLAYLISTS,
    MENU_STATIC_COUNT,
    MENU_ALL_SONGS
} MenuState;

char* menuTexts[] = {
    "Welcome\n\nPress any key to enter",
    "1. Artists\n2. Playlists\n",
    "0. Back\n1. Nikitata\n2. Bladee\n",
    "0. Back\n1. Playlist 1\n2. Playlist 2\n",
};

SDL_Window* gWindow = NULL;
SDL_Window* dWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Renderer* dRenderer = NULL;
TTF_Font* dFont = NULL;
MenuState menuState = MENU_WELCOME;
Mix_Music* gMusic = NULL;
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

bool loadTTFs() {
    dFont = TTF_OpenFont("./resources/fira.ttf", 16);
    if (dFont == NULL) {
        SDL_Log("Failed to open TTF font!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    return true;
}

typedef struct {
    char key;
    LTexture texture;
} TextureMapKVPair;

typedef struct {
    int size;
    TextureMapKVPair* pairs[MAP_CAPACITY];
} LTextureMap;

LTextureMap textureMap = { .pairs = 0 };

void add_to_texture_map(const char key, const LTexture value) {
    if (textureMap.size >= MAP_CAPACITY) {
        printf("map is full, cannot add key:%c", key);
        return;
    }
    TextureMapKVPair* entry = malloc(sizeof(TextureMapKVPair));
    entry->key = key;
    entry->texture = value;
    textureMap.pairs[textureMap.size] = entry;
    textureMap.size++;
}

LTexture get_from_texture_map(const char key) {
    for (int i = 0; i < textureMap.size; i++) {
        if (key == textureMap.pairs[i]->key) {
            return textureMap.pairs[i]->texture;
        }
    }
    printf("failed to find texture in map: %c", key);
    exit(1);
}

bool loadFontTextureMap() {
    const char* chars = " qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890!@#$%^&*()_+-=,./;'[]{}";
    for (int i = 0; i < strlen(chars); i++) {
        SDL_Surface* surface = TTF_RenderGlyph_Solid(dFont, chars[i], fontColor);
        if (surface == NULL) {
            printf("Failed to surface %d", chars[i]);
            return false;
        }
        SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(gRenderer, surface);
        if (sdlTexture == NULL) {
            printf("Failed to texture %d", chars[i]);
            return false;
        }
        SDL_Rect renderQuad = {0,0,surface->w, surface->h};
        LTexture lTexture = {sdlTexture, renderQuad};
        add_to_texture_map(chars[i], lTexture);
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
    if (lTexture->sdlTexture == NULL) {
        SDL_Log("TTF failed to render texture from surface!\nSDL_Error: %s", SDL_GetError());
        return NULL;
    }
    const SDL_Rect renderQuad = {0,0,surface->w, surface->h};
    lTexture->renderQuad = renderQuad;
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
    debugOption->renderQuad = getRenderQuad(0, LINE_SPACE * debugOption->index, fontSurface->w, fontSurface->h);
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
    return loadTTFs() && loadFontTextureMap();
}

void renderText(const int x, const int y, const char* text) {
    SDL_Rect renderQuad = {x,y,0,0};
    for (int i = 0; i < strlen(text); i++) {
        if (text[i] == '\n') {
            renderQuad.x = x;
            renderQuad.y += LINE_SPACE;
        } else {
            LTexture lTexture = get_from_texture_map(text[i]);
            renderQuad.w = lTexture.renderQuad.w;
            renderQuad.h = lTexture.renderQuad.h;
            SDL_RenderCopy(gRenderer, lTexture.sdlTexture, NULL, &renderQuad);
            renderQuad.x += lTexture.renderQuad.w;
        }
    }
}

void listSongs() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    char* dirPath = "/Users/evankelch/Library/Application Support/mp/resources";
    if (chdir(dirPath) == -1) {
        printf("Failed to chdir, errno: %d\n", errno);
        exit(1);
    }

    DIR* dirp = opendir(".");
    struct dirent* entry;
    struct stat filestat;

    if (dirp == NULL) {
        printf("Unable to read dir\n");
        exit(1);
    }
    while ((entry = readdir(dirp))) {
        int res = stat(entry->d_name, &filestat);
        if (res == -1) {
            printf("Unable to stat file: %s, errno: %d\n", entry->d_name, errno);
            exit(1);
        }
        if (S_ISREG(filestat.st_mode)) {
            printf("fileName: %s\n", entry->d_name);
        }
    }
    closedir(dirp);
    if (chdir(cwd) == -1) {
        printf("Failed to back to prev cwd, errno: %d\n", errno);
        exit(1);
    }
}

void playGSong() {
    if (Mix_PlayingMusic() == 1) {
        if (Mix_PausedMusic() == 1) {
            Mix_ResumeMusic();
        }
    } else {
        Mix_PlayMusic(gMusic, 0);
    }
}

void pauseGSong() {
    if (Mix_PlayingMusic() == 1) {
        if (Mix_PausedMusic() != 1) {
            Mix_PauseMusic();
        }
    }
}

bool playPauseCurrentSong() {
    if (Mix_PlayingMusic() == 1) {
        if (Mix_PausedMusic() == 1) {
            Mix_ResumeMusic();
        } else {
            Mix_PauseMusic();
        }
    } else {
        Mix_PlayMusic(gMusic, 0);
    }
    return Mix_PlayingMusic() == 1 && Mix_PausedMusic() == 1;
}

bool loadSong(int artist) {
    pauseGSong();
    Mix_FreeMusic(gMusic);
    char* songFile = artist == 1 ? "./resources/songs/nikitata.mp3" : "./resources/songs/thingsHappen.mp3";
    gMusic = Mix_LoadMUS(songFile);
    if (gMusic == NULL) {
        SDL_Log("Failed to play %s\nSDL_error: %s", songFile, SDL_GetError());
        return false;
    }
    return true;
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
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

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("SDL Mixer failed to init!\nSDL_Error: %s", SDL_GetError());
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

    listSongs();

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
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

void handleMenuNavigate(const SDL_Keysym ks) {
    const int s = ks.sym;
    if (s == SDLK_1 || s == SDLK_KP_1) {
        menuState = MENU_ARTISTS;
    } else if (s == SDLK_2 || s == SDLK_KP_2) {
        menuState = MENU_PLAYLISTS;
    }
}

void handleMenuArtists(const SDL_Keysym ks) {
    const int s = ks.sym;
    if (s == SDLK_0 || s == SDLK_KP_0) {
        menuState = MENU_NAVIGATE;
    } else if (s == SDLK_1 || s == SDLK_KP_1) {
        loadSong(1);
        playGSong();
    } else if (s == SDLK_2 || s == SDLK_KP_2) {
        loadSong(2);
        playGSong();
    }
}

void handleMenuPlaylists(const SDL_Keysym ks) {
    const int s = ks.sym;
    if (s == SDLK_0 || s == SDLK_KP_0) {
        menuState = MENU_NAVIGATE;
    }
}

void handlePlayPauseControls(SDL_Keysym ks) {
    int sym = ks.sym;

    if (sym == SDLK_ESCAPE) {
        playPauseCurrentSong();
    }
}

void handleMainWindowMenuNav(const SDL_Keysym ks) {
    handlePlayPauseControls(ks);
    if (ks.sym == SDLK_BACKSPACE) {
        menuState = MENU_WELCOME;
    }
    if (menuState == MENU_WELCOME) {
        menuState = MENU_NAVIGATE;
    } else if (menuState == MENU_NAVIGATE) {
        handleMenuNavigate(ks);
    } else if (menuState == MENU_ARTISTS) {
        handleMenuArtists(ks);
    } else if (menuState == MENU_PLAYLISTS) {
        handleMenuPlaylists(ks);
    }
}

void handleDebugWindowKeypress(SDL_Keysym ks) {
    const int sym = ks.sym;
    bool shifted = ks.mod == KMOD_LSHIFT ? true : false;

    if (sym == SDLK_UP || sym == SDLK_KP_8) {
        if (selectedOption - 1 >= 0) {
            selectedOption--;
            renderFontForDebugOption(&debugOptions[selectedOption]);
            renderFontForDebugOption(&debugOptions[selectedOption + 1]);
        }
    } else if (sym == SDLK_DOWN || sym == SDLK_KP_5) {
        if (selectedOption + 1 < DEBUG_PROPERTY_COUNT) {
            selectedOption++;
            renderFontForDebugOption(&debugOptions[selectedOption]);
            renderFontForDebugOption(&debugOptions[selectedOption - 1]);
        }
    } else if (sym == SDLK_LEFT || sym == SDLK_KP_4) {
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
    } else if (sym == SDLK_RIGHT || sym == SDLK_KP_6) {
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
    }

}

void renderMain() {
    if (menuState == MENU_ALL_SONGS) {

    } else {
        renderText(0,1,menuTexts[menuState]);
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
        renderMain();
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