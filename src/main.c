#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <sys/errno.h>
#include <unistd.h>

#include "dirent.h"
#include "sys/stat.h"
#include "stdbool.h"
#include "util.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 480;
const int FRAME_RATE = 16;
const int TICKS_PER_FRAME = 1000 / FRAME_RATE;
const int MAX_SONGS = 100;
const int ITEMS_PER_PAGE = 9;
const int MAX_FILE_NAME = 240;
const int VOLUME_STEP = 4;
const char* resourceDir = "/Users/evankelch/Library/Application Support/mp/resources";
const char* fontsDir = "/Users/evankelch/Library/Application Support/mp/fonts";
const char* configPath = "/Users/evankelch/Library/Application Support/mp/config/config.txt";
char* songsArr[MAX_SONGS];
int songCount = 0;
char* fontFiles[9];

const SDL_Color fontColor = {255, 172, 28, 255};
const SDL_Color selectedFontColor = {130, 233, 211, 255};

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
} LDebugOption;

typedef enum {
    DEBUG_BACKGROUND_COLOR_R,
    DEBUG_BACKGROUND_COLOR_G,
    DEBUG_BACKGROUND_COLOR_B,
    DEBUG_FONT,
    DEBUG_FONT_SIZE,
    DEBUG_LINE_SPACE,
    DEBUG_PROPERTY_COUNT
} DebugOption;

typedef enum {
    MENU_WELCOME,
    MENU_NAVIGATE,
    MENU_ARTISTS,
    MENU_PLAYLISTS,
    MENU_ALL_SONGS,
    MENU_PROP_COUNT
} MenuState;

typedef struct {
    MenuState m_stack[4];
    int m_i;
    int pageIndex;
    DebugOption selectedDebug;
    bool optionsOpen;
    int volume;
} State;

char* menuTexts[] = {
    "Welcome\n\nPress any key to enter",
    "1. Artists\n2. Playlists\n3. All songs",
    "0. Back\n1. Nikitata\n2. Bladee\n",
    "0. Back\n1. Playlist 1\n2. Playlist 2\n",
};

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
TTF_Font* dFont = NULL;
State state = {{0,0,0,0}, 0, 0, 0,false,40};
Mix_Music* gMusic = NULL;
int linePos = 0;
LDebugOption debugOptions[DEBUG_PROPERTY_COUNT];
LTexture textureMap[255];
Ek_Map* artistMap;

//DEBUG OPTIONS SETUP
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
    updDebug(DEBUG_FONT, "font", 0, 0, 1);
    updDebug(DEBUG_FONT_SIZE, "font size", 24, 6, 64);
    updDebug(DEBUG_LINE_SPACE, "line space", 24, 6, 64);
}

//CONFIG
void writeToConfig() {
    char configBuf[1024];
    for (int i = 0; i < DEBUG_PROPERTY_COUNT; i++) {
        char lineBuf[32];
        sprintf(lineBuf, "%s=%d\n", debugOptions[i].description, debugOptions[i].value);
        strcat(configBuf, lineBuf);
    }
    FILE* cfgF = fopen(configPath, "w");
    if (cfgF == NULL) {
        printf("Failed to open config for write");
        exit(1);
    }
    fprintf(cfgF, "%s", configBuf);
    fclose(cfgF);
}

void setConfigFromFile(char* cfg) {
    char* key = strtok(cfg, "=");
    char* value = strtok(NULL, "");
    for (int i = 0; i < DEBUG_PROPERTY_COUNT; i++) {
        if (strcmp(debugOptions[i].description, key) == 0) {
            debugOptions[i].value = (int) strtol(value, NULL, 10);
            break;
        }
    }
}

void readConfigFile() {
    FILE* cfgF = fopen(configPath, "r");
    if (cfgF == NULL) {
        puts("failed to read config");
        exit(1);
    }
    char cfg[32];
    while (fgets(cfg, 256, cfgF)) {
        setConfigFromFile(cfg);
    }
    fclose(cfgF);
}
//END CONFIG
//FONTS
void scanFontDir() {
    DIR* dirp = opendir(fontsDir);
    struct dirent* entry;
    int i = 0;
    while ((entry = readdir(dirp))) {
        if (entry->d_name[0] != '.') {
            fontFiles[i++] = entry->d_name;
        }
    }
    debugOptions[DEBUG_FONT].max = i - 1;
}

bool loadFont() {
    TTF_CloseFont(dFont);
    char fontfullpath[200];
    sprintf(fontfullpath, "%s/%s", fontsDir, fontFiles[debugOptions[DEBUG_FONT].value]);
    dFont = TTF_OpenFont(fontfullpath, debugOptions[DEBUG_FONT_SIZE].value);
    if (dFont == NULL) {
        SDL_Log("Failed to open TTF font %s!\n", fontfullpath);
        return false;
    }
    return true;
}

bool loadFontTextureMap() {
    const char* chars = " qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890!@#$%^&*()_+-=,./:;'[]{}";
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
        textureMap[chars[i]] = lTexture;
    }
    return true;
}
//END FONTS
//STATE
MenuState getMenuState() {
    return state.m_stack[state.m_i];
}

void pushMenuState(const MenuState nState) {
    if (nState == getMenuState()) {
        return;
    }
    const int max = 3;
    if (state.m_i < max) {
        state.m_stack[++state.m_i] = nState;
    } else {
        for (int i = 0; i < max - 1; i++) {
            state.m_stack[i] = state.m_stack[i + 1];
        }
        state.m_stack[max] = nState;
    }
}

MenuState popMenuState() {
    state.m_stack[state.m_i] = 0;
    if (state.m_i > 0) {
        state.m_i--;
    }
    return state.m_stack[state.m_i];
}
//END STATE
//RENDERING
void renderTextWithColor(const int x, const int y, const char* text, const SDL_Color color) {
    SDL_Rect renderQuad = {x,y,0,0};
    for (int i = 0; i < strlen(text); i++) {
        if (text[i] == '\n') {
            renderQuad.x = x;
            renderQuad.y += debugOptions[DEBUG_FONT_SIZE].value;
        } else {
            const LTexture lTexture = textureMap[text[i]];
            if (color.a != 0) {
                SDL_SetTextureColorMod(lTexture.sdlTexture, color.r, color.g, color.b);
            } else {
                SDL_SetTextureColorMod(lTexture.sdlTexture, fontColor.r, fontColor.g, fontColor.b);
            }
            renderQuad.w = lTexture.renderQuad.w;
            renderQuad.h = lTexture.renderQuad.h;
            SDL_RenderCopy(gRenderer, lTexture.sdlTexture, NULL, &renderQuad);
            renderQuad.x += lTexture.renderQuad.w;
        }
    }
}
void renderText(const int x, const int y, const char* text) {
    SDL_Color c = {.a = 0};
    renderTextWithColor(x, y, text, c);
}

void renderSongsPage() {
    char lineText[MAX_FILE_NAME] = "";
    sprintf(lineText, "0. Back   Page: %d/%d   Previous Page: (/)   Next Page: (*)\n\n", state.pageIndex, songCount / ITEMS_PER_PAGE);
    renderText(0,0,lineText);
    for (int i = 0; i < ITEMS_PER_PAGE; i++) {
        sprintf(lineText, "%d. %s\n", i + 1, songsArr[i + state.pageIndex * ITEMS_PER_PAGE]);
        renderText(0,debugOptions[DEBUG_LINE_SPACE].value * (i + 2), lineText);
    }
}

void renderArtistsPage() {
    char lineText[MAX_FILE_NAME] = "";
    sprintf(lineText, "0. Back   Page: %d/%d   Previous Page: (/)   Next Page: (*)\n\n", state.pageIndex, songCount / ITEMS_PER_PAGE);
    renderText(0,0,lineText);
    int keycount = 0;
    char** keys = map_keys(artistMap, &keycount);
    for (int i = 0; i < ITEMS_PER_PAGE; i++) {
        if (i + state.pageIndex * ITEMS_PER_PAGE >= keycount) {
            break;
        }
        sprintf(lineText, "%d. %s\n", i + 1, keys[i + state.pageIndex * ITEMS_PER_PAGE]);
        renderText(0,debugOptions[DEBUG_LINE_SPACE].value * (i + 2), lineText);
    }
    free(keys);
}

void renderMain() {
    if (getMenuState() == MENU_ALL_SONGS) {
        renderSongsPage();
    } else if (getMenuState() == MENU_ARTISTS) {
        renderArtistsPage();
    } else {
        renderText(0,0,menuTexts[getMenuState()]);
    }
}
void renderOptions() {
    const int o = 80;
    const SDL_Rect bgRect = {SCREEN_WIDTH / 2 + o, 0, SCREEN_WIDTH / 2 - o, SCREEN_HEIGHT};
    SDL_SetRenderDrawColor(gRenderer, debugOptions[0].value, debugOptions[1].value, debugOptions[2].value, 255);
    SDL_RenderFillRect(gRenderer, &bgRect);
    for (int i = 0; i < DEBUG_PROPERTY_COUNT; i++) {
        char dbBuf[64];
        sprintf(dbBuf, "%10s: %03d  [%d,%d]", debugOptions[i].description, debugOptions[i].value, debugOptions[i].min, debugOptions[i].max);
        renderTextWithColor(SCREEN_WIDTH / 2 + o, i * debugOptions[DEBUG_LINE_SPACE].value, dbBuf, state.selectedDebug == i ? selectedFontColor : fontColor);
    }
}
void renderVolumeBar() {
    const int boxes = 128/4;
    const int gap = 1;
    const int w = SCREEN_WIDTH / boxes - 2 * gap;
    const int filled = state.volume * boxes / MIX_MAX_VOLUME;
    SDL_Rect rQuad = {gap, SCREEN_HEIGHT - 12, w - gap * 2, 10};

    SDL_SetRenderDrawColor(gRenderer, 60,20,0,0);
    for (int i = 0; i < filled; i++) {
        SDL_RenderFillRect(gRenderer, &rQuad);
        rQuad.x += w + 2 * gap;
    }
    SDL_SetRenderDrawColor(gRenderer, 60,20,0,0);
    for (int i = 0; i < boxes - filled; i++) {
        SDL_RenderDrawRect(gRenderer, &rQuad);
        rQuad.x += w + 2 * gap;
    }
    char buf[5];
    sprintf(buf, "%d", state.volume);
    renderText(SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40, buf);
}
//END RENDERING
//SONG LOAD / CONTROLS
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

bool loadAndPlaySongByIndex(const int index) {
    Mix_VolumeMusic(state.volume);

    if (songsArr[state.pageIndex * ITEMS_PER_PAGE + index] == NULL) {
        return false;
    }
    char* fileName = songsArr[ITEMS_PER_PAGE * state.pageIndex + index];
    pauseGSong();
    Mix_FreeMusic(gMusic);
    char path[300] = "";
    strcat(path, resourceDir);
    strcat(path, "/");
    strcat(path, fileName);
    gMusic = Mix_LoadMUS(path);
    if (gMusic == NULL) {
        SDL_Log("Failed to play %s\nSDL_error: %s", path, SDL_GetError());
        return false;
    }
    playGSong();
    return true;
}
//END SONG LOAD / CONTROLS

// INIT / LOAD MEDIA
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
    if (gWindow == NULL) {
        SDL_Log("Failed to create SDL Window!\nSDL_Error: %s", SDL_GetError());
        return false;
    }
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == NULL) {
        SDL_Log("Failed to create renderer!\nSDL_Error: %s", SDL_GetError());
        return false;
    }

    return true;
}

// should probably try to understand this lol
// https://benjaminwuethrich.dev/2015-03-07-sorting-strings-in-c-with-qsort.html
// basically pointer to start of string (pointer)
int cmpstr(const void* a, const void* b) {
    return strcmp(*(char* const*) a, *(char* const*) b);
}

void sortSongsArr() {
    qsort(songsArr, songCount, sizeof(char*), cmpstr);
}

bool detectSongs() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    if (chdir(resourceDir) == -1) {
        printf("Failed to chdir, errno: %d\n", errno);
        return false;
    }

    DIR* dirp = opendir(".");
    struct dirent* entry;
    struct stat filestat;

    if (dirp == NULL) {
        printf("Unable to read dir\n");
        return false;
    }
    while ((entry = readdir(dirp))) {
        int res = stat(entry->d_name, &filestat);
        if (res == -1) {
            printf("Unable to stat file: %s, errno: %d\n", entry->d_name, errno);
            return false;
        }
        if (S_ISREG(filestat.st_mode) && entry->d_name[0] != '.') {
            char* sn = malloc(sizeof(char) * 128);
            strcpy(sn, entry->d_name);
            songsArr[songCount++] = sn;
        }
    }
    closedir(dirp);
    if (chdir(cwd) == -1) {
        printf("Failed to back to prev cwd, errno: %d\n", errno);
        return false;
    }
    return true;
}

void mapArtists() {
    artistMap = map_new(30);
    char* artistName = strtok(songsArr[0], "-");
    Ek_List* list = list_new(6);
    for (int i = 1; i < songCount; i++) {
        char* next = strtok(songsArr[i], "-");
        if (strcmp(artistName, next) == 0) {
            list_add(list, strtok(NULL, "\n"));
        } else {
            map_put(artistMap, artistName, list);
            artistName = next;
            list = list_new(5);
        }
    }
}

bool loadMedia() {
    populateDebugOptions();
    readConfigFile();
    scanFontDir();
    loadFont();
    loadFontTextureMap();
    detectSongs();
    sortSongsArr();
    mapArtists();
    return true;
}
//END INIT / LOAD MEDIA
// CLEANUP
void cleanup() {
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}
// END CLEANUP
// MENU NAVIGATION
int keysymToInt(SDL_Keysym ks) {
    if (ks.sym == SDLK_0 || ks.sym == SDLK_KP_0) {
        return 0;
    }
    if (ks.sym >= SDLK_1 && ks.sym <= SDLK_9) {
        return ks.sym - SDLK_0;
    }
    if (ks.sym >= SDLK_KP_1 && ks.sym <= SDLK_KP_9) {
        return ks.sym - SDLK_KP_ENTER;
    }
    return -1;
}

void adjustSelectedDebugValue(const int delta) {
    LDebugOption* db = &debugOptions[state.selectedDebug];
    const int res = db->value + delta;
    if (res > db->max) {
        db->value = db->max;
    } else if (res < db->min) {
        db->value = db->min;
    } else {
        db->value = res;
    }
    if (state.selectedDebug == DEBUG_FONT || state.selectedDebug == DEBUG_FONT_SIZE) {
        loadFont();
        loadFontTextureMap();
    }
}

void adjustVolume(const int delta) {
    int res = state.volume + delta;
    if (res > MIX_MAX_VOLUME) {
        state.volume = MIX_MAX_VOLUME;
    } else if (res < 0) {
        state.volume = 0;
    } else {
        state.volume = res;
    }
    Mix_VolumeMusic(state.volume);
}

void handleSettingsKeypress(SDL_Keysym ks) {
    const int sym = ks.sym;
    bool shifted = ks.mod == KMOD_LSHIFT ? true : false;
    int keyNum = keysymToInt(ks);

    if (sym == SDLK_UP || keyNum == 8) {
        if ((int) state.selectedDebug - 1 >= 0) {
            state.selectedDebug--;
        }
    } else if (sym == SDLK_DOWN || keyNum == 5) {
        if (state.selectedDebug + 1 < DEBUG_PROPERTY_COUNT) {
            state.selectedDebug++;
        }
    } else if (sym == SDLK_LEFT || keyNum == 4) {
        shifted ? adjustSelectedDebugValue(-5) : adjustSelectedDebugValue(-1);
    } else if (sym == SDLK_RIGHT || keyNum == 6) {
        shifted ? adjustSelectedDebugValue(5) : adjustSelectedDebugValue(1);
    }
}

void handleKeypress(const SDL_Keysym ks) {
    const SDL_Keycode k = ks.sym;
    const int keyIndex = keysymToInt(ks);
    const MenuState menu_state = getMenuState();

    if (k == SDLK_PERIOD || k == SDLK_KP_PERIOD) {
        if (state.optionsOpen) {
            state.optionsOpen = false;
            writeToConfig();
        } else {
            state.optionsOpen = true;
        }
        return;
    }
    if (state.optionsOpen) {
        handleSettingsKeypress(ks);
        return;
    }

    if (keyIndex > 0) { // pos number pressed
        if (menu_state == MENU_WELCOME) {
            pushMenuState(MENU_NAVIGATE);
        } else if (menu_state == MENU_NAVIGATE) {
            if (keyIndex < MENU_PROP_COUNT) {
                pushMenuState(keyIndex + 1);
            }
        } else if (menu_state == MENU_ALL_SONGS) {
            loadAndPlaySongByIndex(keyIndex - 1);
        }
    }
    if (keyIndex == 0) {
        popMenuState();
    }
    if (k == SDLK_ESCAPE) {
        playPauseCurrentSong();
    }
    if (k == SDLK_BACKSPACE) {
        pushMenuState(MENU_WELCOME);
    }
    if (k == SDLK_KP_MULTIPLY && songCount / ITEMS_PER_PAGE > state.pageIndex) {
        state.pageIndex++;
    }
    if (k == SDLK_KP_DIVIDE && state.pageIndex > 0) {
        state.pageIndex--;
    }
    if (k == SDLK_KP_MINUS) {
        adjustVolume(-4);
    }
    if (k == SDLK_KP_PLUS) {
        adjustVolume(4);
    }
}
//END MENU NAVIGATION

//MAIN LOOP
int main(int argc, char *argv[]) {
    bool quit = false;
    LTimer syncTimer;
    SDL_Event e;
    int32_t countedFrames = 0;

    if (!(init() && loadMedia())) {
        return 0;
    }

    while (!quit) {
        startTimer(&syncTimer);
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                handleKeypress(e.key.keysym);
            }

        }

        SDL_SetRenderDrawColor(gRenderer, debugOptions[0].value, debugOptions[1].value, debugOptions[2].value, 255);
        SDL_RenderClear(gRenderer);
        renderMain();
        renderVolumeBar();
        if (state.optionsOpen) {
            renderOptions();
        }
        SDL_RenderPresent(gRenderer);

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