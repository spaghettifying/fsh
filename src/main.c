#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>

/* define */
#define FSH_RL_BUFSIZE 1024
#define FSH_TOK_BUFSIZE 64
#define FSH_TOK_DELIM " \t\r\n\a"

/* Function Declarations for builtin shell commands */
int fsh_cd(char **args);
int fsh_ls(char **args);
int fsh_jotter(char **args);

int fsh_help(char **args);
int fsh_exit(char **args);

/* List of builtin commands */
char *builtin_str[] = {
        "cd",
        "ls",
        "jotter",
        "help",
        "exit"
};

int (*builtin_func[]) (char **) = {
        &fsh_cd,
        &fsh_ls,
        &fsh_jotter,
        &fsh_help,
        &fsh_exit
};

int fsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

void die(char *error) {
    fprintf(stderr, "%s", strcat(strcat("fsh: ", error), "\n"));
    exit(EXIT_FAILURE);
}

char* fsh_read_line(){
    int bufsize = FSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c; //eof is an integer, not char

    if (!buffer) {
        die("allocation error");
    }

    while (1) {
        // read a character
        c = getchar();

        // if we hit eof, replace with null character and return
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // if exceeded buffer, reallocate
        if (position >= bufsize) {
            bufsize += FSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                die("allocation error");
            }
        }
    }
}

char **fsh_split_line(char *line) {
    int bufsize = FSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        die("allocation error");
    }

    token = strtok(line, FSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >=bufsize) {
            bufsize += FSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                die("allocation error");
            }
        }

        token = strtok(NULL, FSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/* Builtin function implementations */
int fsh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "fsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("fsh");
        }
    }
    return 1;
}

int fsh_ls(char **args) {
    int op_a = 0, op_1 = 0;
    if (args[1] == NULL) {
        fprintf(stderr, "fsh: expected argument to \"ls\"\n");
    } else {
        if (args[1][0] == '-') {
            // checking if flag has been passed
            // flags: a, l
            char *p = (char*) (args[1] + 1);
            while (*p) {
                if (*p == 'a') op_a = 1;
                else if (*p == 'l') op_1 = 1;
                else {
                    perror("fsh: option not available");
                    exit(EXIT_FAILURE);
                }
                p++;
            }

            struct dirent *d;
            DIR *dh = opendir(".");
            if (!dh) {
                if (errno == ENOENT) {
                    // if dir not found
                    perror("fsh: directory does not exist");
                } else {
                    // if dir is not reachable throw error
                    perror("fsh: unable to read directory");
                }
                exit(EXIT_FAILURE);
            }
            // while the next entry is not readable we will print directory files
            while ((d = readdir(dh)) != NULL) {
                // if hidden files are found we continue
                if (!op_a && d->d_name[0] == '.')
                    continue;
                printf("%s  ", d->d_name);
                if (op_1) printf("\n");
            }
            if (!op_1)
            printf("\n");
        }
    }
    return 1;
}

int fsh_jotter(char **args) { // works but not with folders
    printf("%s\n", args[1]);
//    execl("/home/adam/CLionProjects/Jotter/jotter", args[1]);

    return 1;
}

int fsh_help(char **args) {
    printf("FSH\n");
    printf("Type program names and args\n");
    printf("The following are built in:\n");

    for (int i = 0; i < fsh_num_builtins() ; i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int fsh_exit(char **args) {
    return 0;
}

int fsh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process
        if (execvp(args[0], args) == -1) {
            perror("fsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // error forking
        perror("fsh");
    } else {
        // parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int fsh_execute(char **args) {
    if (args[0] == NULL) {
        // empty command was entered
        return 1;
    }

    for (int i = 0; i < fsh_num_builtins(); ++i) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return fsh_launch(args);
}

void fsh_loop(){
    char *line;
    char **args;
    int status;

    do {
        printf(">");
        line = fsh_read_line();
        args = fsh_split_line(line);
        status = fsh_execute(args);

        free(line);
        free(args);
    } while (status);

}

int main() {
    // config files

    // program loop
    fsh_loop();

    // cleanup

    return EXIT_SUCCESS;
}
