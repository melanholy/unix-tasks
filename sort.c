#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

struct List {
    int count;
    int capacity;
    int elem_size;
    void *arr;
};

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

int parse_numbers(char *content, int size, struct List *list) {
    char* current = content;
    while (current < content + size) {
        char* num_end = current;
        long long num = strtoll(current, &num_end, 10);
        if (errno == ERANGE) {
            while (current < content + size && *current >= '0' && *current <= '9')
                current++;
            current++;
        } else if (num_end == current) {
            while (current < content + size && *current < '0' && *current > '9')
                current++;
            current++;
        } else {
            current = num_end;
            if (!list_add(list, &num)) {
                printf("Failed to allocate memory. Exit.\n");
                return 0;
            }
        }
    }

    return 1;
}

int ll_compare(const void* a, const void* b) {
    return *(long long*)a >= *(long long*)b ? 1 : -1;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Incorrect arguments!\n");
        return 1;
    }

    struct List* list = list_create(sizeof(long long));
    if (!list) {
        printf("Failed to allocate memory. Exit.\n");
        return 1;
    }

    for (int i = 1; i < argc - 1; i++) {
        int fileno = open(argv[i], O_RDONLY);
        if (fileno == -1) {
            printf("Couldn't open file %s.\n", argv[1]);
            continue;
        }

        int size = lseek(fileno, 0, SEEK_END);
        if (size == -1L) {
            printf("Couldn't read file %s.\n", argv[1]);
            close(fileno);
            continue;
        }
        char* content = malloc(size * sizeof(char));
        if (!content) {
            printf("Failed to allocate memory for file %s. Exit.\n", argv[i]);
            close(fileno);
            return 1;
        }

        if (lseek(fileno, 0, SEEK_SET) == -1L || read(fileno, content, size) < 0) {
            printf("Couldn't read file %s.\n", argv[1]);
            close(fileno);
            continue;
        }
        close(fileno);

        if (!parse_numbers(content, size, list))
            return 1;

        free(content);
    }

    qsort(list->arr, list->count, list->elem_size, ll_compare);

    int fout = open(argv[argc - 1], O_WRONLY | O_CREAT, 0666);
    if (fout == -1) {
        printf("Couldn't open file %s.\n", argv[argc - 1]);
        return 1;
    }

    char number[64];
    for (int i = 0; i < list->count * list->elem_size; i += list->elem_size) {
        int number_len = sprintf(number, "%lld ", *(long long*)(list->arr + i));
        write(fout, number, number_len);
    }
    write(fout, "\n", 1);
    close(fout);

    list_free(list);
}
