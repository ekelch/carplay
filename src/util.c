//
// Created by Evan Kelch on 12/22/24.
//
#include <stdio.h>
#include <string.h>
#include "util.h"

#include <stdlib.h>

Ek_List* list_new (int capacity) {
    Ek_List* list = malloc(sizeof(Ek_List));
    list->capacity = capacity;
    list->arr = malloc(sizeof(char*) * capacity);
    return list;
}

void list_add(Ek_List* list, char* in) {
    if (list == NULL) {
        return;
    }
    if (list->size >= list->capacity) {
        char** newArr = malloc(sizeof(char*) * list->capacity * 2);
        list->capacity = list->capacity * 2;
        for (int i = 0; i < list->capacity; i++) {
            newArr[i] = list->arr[i];
        }
        newArr[list->size++] = in;
        free(list->arr);
        list->arr = newArr;
    } else {
        list->arr[list->size++] = in;
    }
}

void list_deleteIndex(Ek_List* list, const int index) {
    if (list == NULL) {
        return;
    }
    for (int i = index; i < list->size - 1; i++) {
        list->arr[i] = list->arr[i + 1];
    }
    list->arr[list->size--] = NULL;
}

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
    return hash((unsigned char *) key) % (unsigned long) map.capacity;
}

void* map_get(Ek_Map *map, char *key) {
    if (map == NULL || key == NULL) {
        return NULL;
    }
    const unsigned long index = getIndex(*map, key);
    Ek_LinkedList *node = map->mapArr[index];
    if (map->mapArr[index] == NULL) {
        return NULL;
    }
    while (node->next && strcmp(node->key, key) != 0) {
        node = node->next;
    }
    return node->value;
}

Ek_Map* map_new(const int cap) {
    Ek_Map* map = malloc(sizeof(Ek_Map));
    map->capacity = cap;
    map->size = 0;
    map->mapArr = malloc(sizeof(void*) * cap);
    return map;
}

void map_put(Ek_Map *map, char *key, void *value) {
    if (map == NULL || key == NULL || value == NULL) {
        return;
    }
    const unsigned long mapIndex = getIndex(*map, key);
    Ek_LinkedList *node = map->mapArr[mapIndex];
    if (node == NULL) {
        node = malloc(sizeof(Ek_LinkedList));
        strcpy(node->key, key);
        node->value = value;
        map->mapArr[mapIndex] = node;
    } else {
        Ek_LinkedList *next = malloc(sizeof(Ek_LinkedList));
        strcpy(next->key, key);
        next->value = value;
        node->next = next;
    }
    map->size++;
}

char** map_keys(const Ek_Map *map, int* keycount) {
    if (map == NULL) {
        return NULL;
    }
    char** keys = malloc(sizeof(char*) * map->size);
    int keypos = 0;
    for (int i = 0; i < map->capacity; i++) {
        Ek_LinkedList* node = map->mapArr[i];
        if (node != NULL) {
            keys[keypos++] = node->key;
            while ((node = node->next) != NULL) {
                keys[keypos++] = node->key;
            }
        }
    }
    *keycount = keypos;
    return keys;
}

void map_destroy(Ek_Map *map) {
    if (map == NULL) {
        return;
    }
    for (int i = 0; i < map->size; i++) {
        if (map->mapArr[i] != NULL) {
            destroyLinkedList(map->mapArr[i]);
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

