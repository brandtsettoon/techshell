// Name: Cayden Mckelvey & Brandt Settoon
// Date: 02/19/2026
// Description: A Simple Shell

// imports
#include <stdio.h> // printf, fprintf, fgets, stderr, stdin, fflush
#include <stdlib.h> // exit, getenv
#include <string.h> // strcmp, strtok, strcspn
#include <unistd.h> // fork, execvp, getcwd, chdir, dup2
#include <errno.h> // errno, EINVAL
#include <sys/wait.h> // waitpid
#include <fcntl.h> // open, O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC

// 02/10/2026 created print_cwd
// 02/12/2026 started interpret_input and added tokenization method
// 02/16/2026 added fork method to interpret_input
// 02/18/2026 added exit, cd, and redirection methods to interpret_input
// 02/19/2026 fixed cd by changing tokenization method

// prototyping
void print_cwd(void);
void interpret_input(void);

// constants
#define PATH_MAX 4096 // maximum buffer size for cwd and input

// main
int main(void) {
    while (1) {
        print_cwd();
        interpret_input();
    }
    return 0;
}

// functions
void print_cwd(void) {
    char cwd[PATH_MAX]; // character array to store cwd

    // if getcwd fails (returns NULL on error), print error message to error output stream (stderr)
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno)); // strerror turns errno to human readable
        printf("$ "); // fallback prompt
        fflush(stdout); // forces prompt to appear immediately
        return;
    }

    // if no failure, print cwd like normal
    printf("%s$ ", cwd);
    fflush(stdout);
}

void interpret_input(void) {
    char input[PATH_MAX]; // character array to store input

    // read entire input
    if (fgets(input, sizeof(input), stdin) == NULL) {
        // if NULL, exit cleanly
        printf("\n");
        exit(0);
    }

    // remove trailing newline if available
    input[strcspn(input, "\n")] = '\0';

    // if enter was just pressed, do nothing
    if (input[0] == '\0') return;

    // tokenize into argv[] (an array of pointers)
    char *argv[256];
    int count = 0;

    char *p = input;

    while (*p != '\0' && count < 255) {
        
        // skip leading spaces
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        // break on enter
        if (*p == '\0') {
            break;
        }

        // if argument starts with quote
        if (*p == '"') {
            p++; // skip opening quote
            argv[count++] = p;

            while (*p && *p != '"') {
                p++;
            }

            if (*p == '"') {
                *p = '\0'; // terminate string
                p++;
            }
        }
        // regular tokenization
        else {
            argv[count++] = p;

            while (*p && *p != ' ' && *p != '\t')
                p++;

            if (*p) {
                *p = '\0';
                p++;
            }
        }
    }

    argv[count] = NULL; // sets final term to NULL

    // I/O redirection
    char *input_file = NULL;
    char *output_file = NULL;

    // check for redirection symbols
    for (int i = 0; i < count; i++) {

        // input redirection
        if (strcmp(argv[i], "<") == 0) {

            // throw error
            if (i + 1 >= count) {
                fprintf(stderr, "Error %d (%s)\n", EINVAL, strerror(EINVAL));
                return;
            }

            input_file = argv[i + 1];

            // remove "< file" from argv
            for (int j = i; j + 2 <= count; j++) {
                argv[j] = argv[j + 2];
            }

            count -= 2;
            i--; // recheck index
        }

        // output redirection (very similar)
        else if (strcmp(argv[i], ">") == 0) {

            // throw error
            if (i + 1 >= count) {
                fprintf(stderr, "Error %d (%s)\n", EINVAL, strerror(EINVAL));
                return;
            }

            output_file = argv[i + 1];

            // remove "> file" from argv
            for (int j = i; j + 2 <= count; j++) {
                argv[j] = argv[j + 2];
            }

            count -= 2;
            i--; // recheck index
        }
    }
    
    argv[count] = NULL; // sets final term to NULL


    // exit method (very simple)
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }

    // change directory method
    if (strcmp(argv[0], "cd") == 0) {

        // if no argument, go to home directory
        if (count == 1) {
            char *home = getenv("HOME");
            // throw error
            if (home == NULL || chdir(home) != 0) {
                fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
            }
        }
        else {
            // cd with argument
            if (chdir(argv[1]) != 0) {
                fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
            }
        }
        return;
    }

    // fork method
    pid_t pid = fork();

    if (pid < 0) {
        // fork failed
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
        return;
    }

    if (pid == 0) {
        // child process

        // handle input redirection
        if (input_file != NULL) {

            // open file
            int fd = open(input_file, O_RDONLY);

            // throw error
            if (fd < 0) {
                fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
                exit(1);
            }

            // redirects stdin to file
            dup2(fd, STDIN_FILENO);

            // close file
            close(fd);
        }

        // handle output redirection (very similar)
        if (output_file != NULL) {

            // open file
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            // throw error
            if (fd < 0) {
                fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
                exit(1);
            }

            // redirects stdout to file
            dup2(fd, STDOUT_FILENO);

            // close file
            close(fd);
        }

        // execute command
        execvp(argv[0], argv);

        // if execvp returns, it failed
        fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
        exit(1);
    }

    else {
        // parent process

        // suspend execution until child process changes state
        int status;
        waitpid(pid, &status, 0);
    }

}
