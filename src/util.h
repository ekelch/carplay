//
// Created by Evan Kelch on 12/22/24.
//

#ifndef UTIL_H
#define UTIL_H

#define MAP_MAX 200
#define MAP_KEY_MAX 100
#define MAP_VALUE_MAX 256
#include "stdbool.h"
#include <SDL.h>

typedef struct Ek_LinkedList {
    char key[MAP_KEY_MAX];
    void* value;
    struct Ek_LinkedList* next;
} Ek_LinkedList;

typedef struct {
    int size;
    Ek_LinkedList* arr[MAP_MAX];
} Ek_Map;

typedef struct {
    bool started;
    u_int64_t startTicks;
} LTimer;

void destroyLinkedList(Ek_LinkedList* head);
unsigned long hash(unsigned char* k);
unsigned long getIndex(Ek_Map map, char* key);
void* map_get(Ek_Map* map, char* key);
void map_put(Ek_Map* map, char* key, void* value);
void map_destroy(Ek_Map* map);

void startTimer(LTimer* t);
void stopTimer(LTimer* t);
#endif //UTIL_H
