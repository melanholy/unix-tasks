#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char *read_type = "read";
const char *write_type = "write";

int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("Incorrect arguments!\r\n");
        return 1;
    }

    char *lck_type = argv[2];
    if (strcmp(lck_type, read_type) && strcmp(lck_type, write_type)) {
        printf("Incorrect lock type!\r\n");
        return 1;
    }

    char *filename = argv[1];
    int l = strlen(filename);
    char *lck_filename = malloc((l + 5) * sizeof(char));
    sprintf(lck_filename, "%s.lck", filename);

    struct stat file_info;
    while (!stat(lck_filename, &file_info)) { // .lck файл существует, ждем
        usleep(200);
    }

    pid_t pid = getpid();
    int lck_fileno = open(lck_filename, O_WRONLY | O_CREAT, 0666);
    char *content = malloc(64 * sizeof(char));
    int size = sprintf(content, "%d\t%s", pid, lck_type);
    write(lck_fileno, content, size);
    close(lck_fileno);
    free(content);

    int fileno = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    char buffer[1024];
    int r;
    while ((r = read(STDIN_FILENO, buffer, 1)) > 0) {
        write(fileno, buffer, r);
    }

    remove(lck_filename);
    free(lck_filename);

    return 0;
}
