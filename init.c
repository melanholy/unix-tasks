#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/wait.h>
#include <libgen.h>
#include <signal.h>
#include "list.h"

#define CONF_DEFAULT       "conf.ini"
#define WAIT_OPTION        "wait"
#define WAIT_OPTION_I      0
#define RESPAWN_OPTION     "respawn"
#define RESPAWN_OPTION_I   1
#define STDIN              0
#define STDOUT             1
#define STDERR             2
#define ERR_EXIT_CODE      42
#define FORK_FAIL          -1

int reload_flag = 0;

struct Child {
    pid_t pid;
    char *process_name;
    char **argv;
    int option;
    int finished;
};

void hup_handler(int sig) {
    syslog(LOG_INFO, "Got HUP signal, reloading.");
    reload_flag = 1;
}

char **parse_argv(char *str, int *argc) {
    struct List *list = list_create(sizeof(char *));
    int inside_quotes = 0;
    int start = 0;
    int i;
    for (i = 0; 1; i++) {
        if (str[i] == '"') {
            inside_quotes = 1 - inside_quotes;
            if (inside_quotes) {
                start = i + 1;
                continue;
            }
        } else if (str[i] == ' ' && i == start) {
            start++;
            continue;
        } else if (str[i] == '\0') {
            break;
        } else if (str[i] != ' ' || inside_quotes) {
            continue;
        }

        char *start_ptr = &str[start];
        list_add(list, &start_ptr);
        str[i] = '\0';
        start = i + 1;
    }

    if (i != start && !inside_quotes) {
        char *start_ptr = &str[start];
        list_add(list, &start_ptr);
    }
    
    char **argv = malloc(list->count * sizeof(char *));
    for (int i = 0; i < list->count; i++)
        argv[i] = *(char **)list_item(list, i);
    *argc = list->count;

    list_free(list);

    return argv;
}

int create_pid_file(char *process_name, pid_t pid) {
    char *filename = basename(process_name);
    char *pid_file = malloc((strlen(filename) + 64) * sizeof(char));
    sprintf(pid_file, "/tmp/%s.pid", filename);
    char pid_str[64];
    int len = sprintf(pid_str, "%d", pid);

    int fileno = open(pid_file, O_WRONLY | O_CREAT, 0666);
    free(pid_file);
    if (fileno == -1)
        return 0;

    write(fileno, pid_str, len);
    close(fileno);

    return 1;
}

int remove_pid_file(char *process_name) {
    char *filename = basename(process_name);
    char *pid_file = malloc((strlen(filename) + 64) * sizeof(char));
    sprintf(pid_file, "/tmp/%s.pid", filename);

    int res = remove(pid_file);
    free(pid_file);

    return res;
}

int daemonize() {
    pid_t pid = fork();
    if (pid < 0)
        return 0;
    if (pid) {
        //printf("Daemon %d created. Detaching from console.\n", pid);
        printf("%d", pid);
        exit(0);
    }

    setsid();
    chdir("/");
    close(STDIN);
    close(STDOUT);
    close(STDERR);

    return 1;
}

pid_t run_proc(char **pargv) {
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Failed to create fork for %s.\n", pargv[0]);
        return FORK_FAIL;
    }
    if (pid) {
        syslog(LOG_INFO, "Created fork with PID=%d for %s.\n", pid, pargv[0]);
        if (!create_pid_file(pargv[0], pid)) {
            syslog(LOG_WARNING, "Failed to create .pid file for %s.", pargv[0]);
        }

        return pid;
    } else {
        // форк создан, и он либо превратится в нужный процесс, либо особым кодом просигналит, что не смог
        execvp(pargv[0], pargv);
        exit(ERR_EXIT_CODE);
    }
}

void parse_children(char *commands, int size, struct List *children) {
    char *next_command = strtok(commands, "\n");
    while (next_command != NULL && next_command != commands + size) {
        char *current_command = next_command;
        next_command = strtok(NULL, "\n");

        int pargc;
        char** pargv = parse_argv(current_command, &pargc);

        int option_i;
        char *option = pargv[pargc - 1];
        if (!strcmp(option, WAIT_OPTION)) {
            option_i = WAIT_OPTION_I;
        } else if (!strcmp(option, RESPAWN_OPTION)) {
            option_i = RESPAWN_OPTION_I;
        } else {
            syslog(LOG_WARNING, "Unknown run option %s for %s.\n", option, pargv[0]);
            continue;
        }

        pargv[pargc - 1] = NULL;
        struct Child child;
        child.process_name = pargv[0];
        child.finished = 0;
        child.argv = pargv;
        child.option = option_i;
        list_add(children, &child);
    }
}

int main(int argc, char **argv) {
    const char *conf_file = argc > 1 ? argv[1] : CONF_DEFAULT;

    if (!daemonize()) {
        printf("Failed to create daemon.\n");
        return 1;
    }
    syslog(LOG_INFO, "Successfully detached from console.");

    int fileno;
entry_point: // Да простит меня Дейкстра
    fileno = open(conf_file, O_RDONLY);
    if (fileno == -1) {
        syslog(LOG_ERR, "Couldn't open file %s. You should use absolute paths.", conf_file);
        return 1;
    }
    
    int size = lseek(fileno, 0, SEEK_END);
    char* commands = malloc(size * sizeof(char));
    lseek(fileno, 0, SEEK_SET);
    if (read(fileno, commands, size) < 0) {
        syslog(LOG_ERR, "Couldn't read file %s.", conf_file);
        close(fileno);
        return 1;
    }
    close(fileno);

    signal(SIGHUP, hup_handler);
    struct List *children = list_create(sizeof(struct Child));
    // argv дочерних процессов указывает на куски commands, поэтому он не освобождается сразу
    parse_children(commands, size, children);

    int children_left = children->count;
    for (int i = 0; i < children->count; i++) {
        struct Child *child = list_item(children, i);
        pid_t child_pid = run_proc(child->argv);
        if (child_pid > 0) {
            child->pid = child_pid;
        } else {
            syslog(LOG_ERR, "Failed to start process %s.", child->process_name);
            child->finished = 1;
            children_left--;
        }
    }
    
    int *run_attempts = malloc(children->count * sizeof(int)); // количество неуспешных попыток запуска
    while (children_left) {
        int status;
        pid_t finished_pid = waitpid(-1, &status, 0);

        for (int i = 0; i < children->count; i++) {
            struct Child *child = list_item(children, i);
            if (child->finished || child->pid != finished_pid)
                continue;
            
            int need_remove = 0; // флаг, означающий нужно ли его пометить дочерний процесс как завершенный
            if (!WIFEXITED(status)) { // процесс завершился, но насильно. считаем, что в таком случае не респавним
                syslog(LOG_WARNING, "Process %s didn't close by itself.", child->process_name);
                need_remove = 1;
            } else if (WEXITSTATUS(status) == ERR_EXIT_CODE) { // не удалось запустить целевой процесс
                run_attempts[i]++;
                if (run_attempts[i] > 50) { // больше 50 попыток запуска значит что файл не исполняемый или поврежден
                    syslog(LOG_ERR, "Process %s didn't start after 50 attempts.", child->process_name);
                    need_remove = 1;
                } else {
                    pid_t newpid = run_proc(child->argv);
                    if (newpid > 0) {
                        child->pid = newpid;
                    } else {
                        syslog(LOG_ERR, "Failed to start process %s.", child->process_name);
                        need_remove = 1;
                    }
                }
            } else if (child->option == WAIT_OPTION_I) { // успешно завершился процесс, который не надо респавнить
                syslog(LOG_INFO, "Process %s successfully finished.", child->process_name);
                need_remove = 1;
            } else if (child->option == RESPAWN_OPTION_I) { // успешно завершился процесс, который надо респавнить
                syslog(LOG_INFO, "Restarting process %s.", child->process_name);
                pid_t newpid = run_proc(child->argv);
                if (newpid > 0) {
                    child->pid = newpid;
                    run_attempts[i] = 0;
                } else {
                    syslog(LOG_ERR, "Failed to respawn process %s.", child->process_name);
                    need_remove = 1;
                }
            }

            if (need_remove) {
                children_left--;
                child->finished = 1;
                remove_pid_file(child->process_name);
            }
        }

        if (reload_flag) { // случился HUP, высвобождаем все и уходим в начало
            reload_flag = 0;
            free(commands);
            free(run_attempts);
            for (int i = 0; i < children->count; i++) {
                struct Child *child = list_item(children, i);
                remove_pid_file(child->process_name);
                kill(child->pid, SIGKILL);
                free(child->argv);
            }
            list_free(children);
            goto entry_point;
        }
    }

    syslog(LOG_INFO, "All processes exited.");
}
