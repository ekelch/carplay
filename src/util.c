//
// Created by Evan Kelch on 12/22/24.
//
#include <stdio.h>
#include <string.h>
#include "util.h"

#include <stdlib.h>

void destroyLinkedList(Ek_LinkedList *head) {
    while (head != NULL) {
        Ek_LinkedList *temp = head;
        head = head->next;
        free(temp);
    }
}

unsigned long hash(unsigned char *k) {
    unsigned long hash = 5381;
    int c;
    while ((c = *k++)) {
        hash = (hash << 5) + hash + c;
    }
    return hash;
}

unsigned long getIndex(const Ek_Map map, char *key) {
    return hash((unsigned char *) key) % (unsigned long) map.size;
}

void *map_get(Ek_Map *map, char *key) {
    const unsigned long index = getIndex(*map, key);
    Ek_LinkedList *node = map->arr[index];
    if (map->arr[index] == NULL) {
        return NULL;
    }
    while (node->next && strcmp(node->key, key) != 0) {
        node = node->next;
    }
    return node->value;
}

void map_put(Ek_Map *map, char *key, void *value) {
    const unsigned long mapIndex = getIndex(*map, key);
    Ek_LinkedList *node = map->arr[mapIndex];
    if (node == NULL) {
        node = malloc(sizeof(Ek_LinkedList));
        strcpy(node->key, key);
        node->value = value;
        map->arr[mapIndex] = node;
    } else {
        Ek_LinkedList *next = malloc(sizeof(Ek_LinkedList));
        strcpy(next->key, key);
        next->value = value;
        node->next = next;
    }
}

void map_destroy(Ek_Map *map) {
    for (int i = 0; i < map->size; i++) {
        if (map->arr[i] != NULL) {
            destroyLinkedList(map->arr[i]);
        }
    }
}

void startTimer(LTimer* t) {
    t->started = true;
    t->startTicks = SDL_GetTicks64();
}
void stopTimer(LTimer* t) {
    t->started = false;
    t->startTicks = 0;
}