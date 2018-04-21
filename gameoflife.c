#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>

const int listen_port = 8888;
const char cell = '#';
const char empty = ' ';
const char error_marker = 222;

char *field;

// utf-8
char error_message[] = {
    226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226,
    150, 132, 226, 150, 128, 226, 150, 128, 226, 150, 128, 226, 150, 132, 226, 150,
    145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145,
    226, 150, 145, 226, 150, 145, 10, 226, 150, 132, 226, 150, 136, 226, 150, 136,
    226, 150, 136, 226, 150, 128, 226, 150, 145, 226, 151, 144, 226, 150, 145, 226,
    150, 145, 226, 150, 145, 226, 150, 140, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 10, 226, 150,
    145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 140, 226, 150, 145,
    226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 144, 226,
    150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 226, 150, 145, 10, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 226, 150, 144, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145,
    226, 150, 145, 226, 150, 144, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226,
    150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 10, 226, 150, 145, 226,
    150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 140, 226, 150, 145, 226, 150,
    145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 144, 226, 150, 132,
    226, 150, 132, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226,
    150, 145, 10, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226,
    150, 140, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    132, 226, 150, 128, 226, 150, 146, 226, 150, 146, 226, 150, 128, 226, 150, 128,
    226, 150, 128, 226, 150, 128, 226, 150, 132, 10, 226, 150, 145, 226, 150, 145,
    226, 150, 145, 226, 150, 144, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226,
    150, 145, 226, 150, 144, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150,
    146, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 128,
    226, 150, 128, 226, 150, 132, 10, 226, 150, 145, 226, 150, 145, 226, 150, 145,
    226, 150, 144, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226,
    150, 144, 226, 150, 132, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150,
    146, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 146,
    226, 150, 146, 226, 150, 128, 226, 150, 132, 10, 226, 150, 145, 226, 150, 145,
    226, 150, 145, 226, 150, 145, 226, 150, 128, 226, 150, 132, 226, 150, 145, 226,
    150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 128, 226, 150, 132, 226, 150,
    146, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 146,
    226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 146, 226, 150, 128, 226,
    150, 132, 10, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226,
    150, 145, 226, 150, 145, 226, 150, 128, 226, 150, 132, 226, 150, 132, 226, 150,
    132, 226, 150, 132, 226, 150, 132, 226, 150, 136, 226, 150, 132, 226, 150, 132,
    226, 150, 132, 226, 150, 132, 226, 150, 132, 226, 150, 132, 226, 150, 132, 226,
    150, 132, 226, 150, 132, 226, 150, 132, 226, 150, 132, 226, 150, 128, 226, 150,
    132, 10, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145,
    226, 150, 145, 226, 150, 140, 226, 150, 140, 226, 150, 145, 226, 150, 140, 226,
    150, 140, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 10, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145,
    226, 150, 145, 226, 150, 140, 226, 150, 140, 226, 150, 145, 226, 150, 140, 226,
    150, 140, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 10, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150,
    145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 132,
    226, 150, 132, 226, 150, 140, 226, 150, 140, 226, 150, 132, 226, 150, 140, 226,
    150, 140, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145, 226, 150, 145 };

void simulate(int size, int width) {
    const int offsets_count = 8;

    int *to_die = malloc(size * sizeof(int)); // те кто на очередной итерации умирают
    int *to_repr = malloc(size * sizeof(int)); // те кто на очередной итерации рождаются
    int offsets[] = {1, -width + 1, width + 1, -1, -width - 1, width - 1, -width, width};
    while (1) {
        clock_t start = clock();

        int to_repr_count = 0;
        int to_die_count = 0;

        for (int i = 0; i < size; i++) {
            int neighbours = 0;
            for (int j = 0; j < offsets_count; j++) {
                if (i % width == width - 1 && j < 3 || i % width == 0 && j > 2 && j < 6) // такие сдвиги не соседние
                    continue;

                int position = i + offsets[j];
                if (position >= 0 && position < size && field[position] == cell)
                    neighbours++;
            }

            if (field[i] == cell && (neighbours < 2 || neighbours > 3)) {
                to_die[to_die_count] = i;
                to_die_count++;
            } else if (field[i] == empty && neighbours == 3) {
                to_repr[to_repr_count] = i;
                to_repr_count++;
            }
        }

        for (int i = 0; i < to_die_count; i++)
            field[to_die[i]] = empty;
        for (int i = 0; i < to_repr_count; i++)
            field[to_repr[i]] = cell;

        clock_t end = clock();
        float passed = (float)(end - start) / CLOCKS_PER_SEC;
        if (passed > 1) {
            field[0] = error_marker; // сигнал, что посчитать не успели
            break;
        }

        usleep((1 - passed) * 1000000);
    }

    free(to_die);
    free(to_repr);
}

void serve(int size, int width) {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        printf("Couldn't create socket!\n");
        return;
    }

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listen_port);
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("Couldn't bind to port %i!\n", listen_port);
        return;
    }
    listen(server, 5);

    struct sockaddr_in client_addr;
    int c;
    while (1) {
        int client = accept(server, (struct sockaddr *)&client_addr, (socklen_t *)&c);
        if (client < 0) {
            printf("Accept failed!\n");
            continue;
        }

        char* message;
        int message_len;
        if (field[0] == error_marker) { // если в секунду не уложились то 0 символ поля это error_marker
            message = error_message;
            message_len = sizeof(error_message);
        } else {
            message = field;
            message_len = size;
        }

        int left = message_len;
        while (left)
            left -= write(client, message, left);
        close(client);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Incorrect arguments!\n");
        return 1;
    }

    int fileno = open(argv[1], O_RDONLY);
    if (fileno == -1) {
        printf("Couldn't open file %s!\n", argv[1]);
        return 1;
    }

    int size = lseek(fileno, 0, SEEK_END);
    field = mmap( // общее с форком поле
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0);

    lseek(fileno, 0, SEEK_SET);
    read(fileno, field, size);
    close(fileno);

    int width = strchr(field, '\n') - field + 1;

    if (!fork())
        simulate(size, width);
    else
        serve(size, width);
}
