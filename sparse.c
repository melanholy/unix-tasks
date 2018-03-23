#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Incorrect arguments!\n");
        return 1;
    }

    int fileno = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileno == -1) {
        printf("Couldn't create file %s!\n", argv[1]);
        return;
    }

    char c;
    char buffer[1024]; // писать в файл не нолики быстрее будет кусками
    int buf_index = 0; // текущая позиция в buffer
    int zeroes = 0; // текущее количество подряд идущих ноликов
    int zero_seq = 0; // 1 значит сейчас идет последовательность ноликов, 0 иначе

    while (read(STDIN_FILENO, &c, 1) > 0) {
        if (c == '\0' && zero_seq) { // следующий нолик в идущей последовательности
            zeroes++;
        } else if (c == '\0') { // первый нолик
            if (buf_index > 0) {
                write(fileno, buffer, buf_index); // сбрасываем накопленный перед ноликами буфер
                buf_index = 0;
            }
            zero_seq = 1;
            zeroes = 1;
        } else {
            if (zero_seq) { // последовательность ноликов кончилась
                lseek(fileno, zeroes, SEEK_CUR);
                zero_seq = 0;
            }

            buffer[buf_index] = c;
            buf_index++;
            if (buf_index == 1024) { // буфер заполнился, нужно сбросить на диск
                write(fileno, buffer, 1024);
                buf_index = 0;
            }
        }
    }

    if (zero_seq) { // если файл окончился ноликами, нужно дописать все еще незаписанные
        lseek(fileno, zeroes, SEEK_CUR);
    } else if (buf_index > 0) { // если файл окончился не ноликами, а буфер не пуст, нужно сбросить остатки
        write(fileno, buffer, buf_index);
    }
    close(fileno);
}
