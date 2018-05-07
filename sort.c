#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "list.h" // самописный расширяющийся масссив, который может хранить любые типы

int parse_numbers(char *content, int size, struct List *list) {
    char* current = content;
    while (current < content + size) {
        char* num_end = current;
        long long num = strtoll(current, &num_end, 10); // пытаемся преобразовать строку в число
        if (errno == ERANGE) { // если число слишком большое, его пропускаем
            while (current < content + size && *current >= '0' && *current <= '9')
                current++;
            current++;
        } else if (num_end == current) { // текущая позиция не является началом числа
            while (current < content + size && *current < '0' && *current > '9')
                current++;
            current++;
        } else { // прочитали число
            current = num_end;
            if (!list_add(list, &num)) {
                printf("Failed to allocate memory. Exit.\n");
                return 0;
            }
        }
    }

    return 1;
}

int ll_compare(const void *a, const void *b) {
    if (*(long long *)a == *(long long *)b)
        return 0;

    return *(long long *)a > *(long long *)b ? 1 : -1;
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
    for (int i = 0; i < list->count; i++) {
        int number_len = sprintf(number, "%lld ", *(long long*)list_item(list, i));
        write(fout, number, number_len);
    }
    write(fout, "\n", 1);
    close(fout);

    list_free(list);
}
