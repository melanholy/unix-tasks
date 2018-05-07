#include <stdlib.h>
#include <string.h>
#include "list.h"

struct List* list_create(int elem_size) {
    struct List* list = malloc(sizeof(struct List));
    if (!list)
        return NULL;

    list->count = 0;
    list->capacity = 4;
    list->elem_size = elem_size;
    list->arr = malloc(list->capacity * elem_size);
    if (!list->arr)
        return NULL;

    return list;
}

int list_add(struct List* list, void* elem) {
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->arr = realloc(list->arr, list->capacity * list->elem_size);
        if (!list->arr)
            return 0;
    }

    memcpy(list->arr + list->count * list->elem_size, elem, list->elem_size);
    list->count++;

    return 1;
}

void list_free(struct List* list) {
    free(list->arr);
    free(list);
}

void *list_item(struct List *list, int index) {
    if (index >= list->count)
        return NULL;

    return list->arr + index * list->elem_size;
}
