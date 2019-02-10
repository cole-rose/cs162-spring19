#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;
int PATH_MAX = 1024;
/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);

int cmd_help(struct tokens *tokens);

int cmd_cd(struct tokens *tokens);

int cmd_pwd(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
    cmd_fun_t *fun;
    char *cmd;
    char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
        {cmd_help, "?",    "show this help menu"},
        {cmd_exit, "exit", "exit the command shell"},
        {cmd_cd,   "cd",   "go to a directory"},
        {cmd_pwd,  "pwd",  "show the current working directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
    for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
        printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
    return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
    exit(0);
}

/* Go to a directory */
int cmd_cd(unused struct tokens *tokens) {
    chdir(tokens_get_token(tokens, 1));
    return 1;
}

/* Show the current working directory */
int cmd_pwd(unused struct tokens *tokens) {
    char cwd[4096]; // 1024
    chdir("/path/to/change/directory/to");
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
    return 1;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
    for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
        if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
            return i;
    return -1;
}

char *find_command(char *path, char *cmd) {
    size_t length_path = strlen(path);
    if (length_path == 0) {
        return NULL;
    }
    int length = 0;
    while (length < length_path) {
        if (path[length] == ':') {
            break;
        }
        length++;
    }
    char *path_new = malloc(length + strlen(cmd) + 2);
    strncpy(path_new, path, length);
//    fprintf(stdout, "path_new: %s\n", path_new);
    strcat(path_new, "/"); // path with "/"
    strcat(path_new, cmd); // path with the command
    //path resolution
    if (access(path_new, F_OK) == 0) {
        return path_new;
    }
    path = path + length + 1;
    return find_command(path, cmd);
}

char *get_path(char *cmd) {
    char *path = getenv("PATH");
    char *command = malloc(4096); // 1024

//    fprintf(stdout, "Inside get_path: path: %s\n", path);

    if (cmd[0] == '/' || cmd[0] == '.') {
        char *c = realpath(cmd, command);
        //path resolution
        if (access(c, F_OK) == 0) {
            return c;
        }
        return NULL;
    }
    return find_command(path, cmd);
}

void file_redirection(char *file, char *operation) {
    if (operation[0] == '>') {
        freopen(file[0], "w", stdout);
    } else if (operation[0] == '<') {
        freopen(file[0], "r", stdin);
    }
}

void restore_signal() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

void ignore_signal(){
    signal(SIGTTOU, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}

/* Intialization procedures for this shell */
void init_shell() {
    /* Our shell is connected to standard input. */
    shell_terminal = STDIN_FILENO;

    /* Check if we are running interactively */
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
         * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
         * foreground, we'll receive a SIGCONT. */
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        ignore_signal();

        /* Saves the shell's process id */
        shell_pgid = getpid();

        /* Take control of the terminal */
        tcsetpgrp(shell_terminal, shell_pgid);

        /* Save the current termios to a variable, so it can be restored later. */
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

int main(unused int argc, unused char *argv[]) {
    init_shell();

    static char line[4096];
    int line_num = 0;
    enum flag {
        In = 0, Out = 1
    } operator;

    /* Please only print shell prompts when standard input is not a tty */
    if (shell_is_interactive)
        fprintf(stdout, "%d: ", line_num);

    while (fgets(line, 4096, stdin)) {
        /* Split our line into words. */
        struct tokens *tokens = tokenize(line);

        // print out each words of the line
//        size_t index;
        size_t tokens_length = tokens_get_length(tokens);
//        for (index = 0; index < tokens_length; index++) {
//            fprintf(stdout, "%s\t", tokens_get_token(tokens, index));
//        }
//        fprintf(stdout, "\n");
        char *first_arg = tokens_get_token(tokens, 0);
        char *file;
        /* Find which built-in function to run. */
        int fundex = lookup(tokens_get_token(tokens, 0));

        if (fundex >= 0) {
            cmd_table[fundex].fun(tokens);


        } else {
            /* REPLACE this to run commands as programs. */


//            fprintf(stdout, "cmd: %s\t", first_arg);
            char *path = get_path(first_arg);
//            fprintf(stdout, "cmd_path: %s\t\n", cmd_path);


            // if cmd_path is a legal path
            if (path != NULL) {
//                fprintf(stdout, "Path is a legal path\n");
                // do something here
                pid_t pid = fork();

                int status;

                if (pid == 0) {
                    restore_signal();
                    char *args[tokens_length + 1];
                    int index = 0;
                    setpgid(getpid(), getpid());

                    for (int i = 0; i < tokens_length; i++) {
                        char *token = tokens_get_token(tokens, i);

                        if (token[0] == '>') {
                            operator = Out;
                            file = tokens_get_token(tokens, ++i);
                            freopen(file, "w", stdout);
                        } else if (token[0] == '<') {
                            operator = In;
                            file = tokens_get_token(tokens, ++i);
                            freopen(file, "r", stdin);
                        } else {
                            args[index++] = tokens_get_token(tokens, i);
                        }
                    }

                    args[index] = NULL; // set null pointer at the end of char
                    execv(path, args);
                    if (operator == 1) {
                        fclose(stdout);
                    }
                    if (operator == 0) {
                        fclose(stdin);
                    }

                    exit(0);
                } else {
                    tcsetpgrp(STDIN_FILENO, pid);
                    // wait till the child process finishes
                    wait(&status);
                    tcsetpgrp(STDIN_FILENO, getpid());
                }


            } else {
//                fprintf(stdout, "I hate this\n");
                fprintf(stdout, "CS 162 is amazing\n");
            }
        }
        if (shell_is_interactive)
            /* Please only print shell prompts when standard input is not a tty */
            fprintf(stdout, "%d: ", ++line_num);

        /* Clean up memory */
        tokens_destroy(tokens);
    }

    return 0;
}
