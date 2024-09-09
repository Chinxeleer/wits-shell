#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_TOKENS 64

int built_ins(char **vec) {
  if (strcmp(vec[0], "exit\n") == 0) {
    free(vec);
    exit(0);
  } else if (strcmp(vec[0], "cd\n")) {
    // change directory chdir
  }

  return 1;
}

char *get_input() {
  char *input = NULL;
  size_t len = 0;

  if (getline(&input, &len, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS); // We recieved an EOF
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return input;
}

int exevute(char **args, char **path) {
  pid_t pid;
  int status;

  pid = fork();

  if (pid == 0) {
    // Child process

    for (int i = 0; i < 2; i++) {
      perror("error executing");
      if (execvp(path[i], args) == -1) {
        perror("error executing");
      }
    }
    exit(EXIT_FAILURE);
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

char **split_input(char *line) {
  char **tokens = malloc(MAX_TOKENS * sizeof(char *));
  char *token;
  int position = 0;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  // Use strsep to tokenize the input based on space (" ")
  while ((token = strsep(&line, " ")) != NULL) {
    if (*token == '\0') { // Skip empty tokens
      continue;
    }
    tokens[position] = token;
    position++;

    if (position >= MAX_TOKENS - 1) {
      break; // Stop if we've reached the maximum number of tokens
    }
  }
  tokens[position] = NULL; // Null-terminate the array
  return tokens;
}

int batch_mode(int argc, char *argv[]) {
  if (argc == 2) {

    printf("the batch file is : %s \n", argv[1]);
    return 0;

  } else if (argc > 2) {

    printf("Throw an error because we only require one input\n");
    return 0;
  }
  return 0;
}

void interactive_mode() {
  char *input;
  char **args;
  int status = 1;

  char *path_list[] = {
      "/bin/",
      "/usr/bin/",
      "/usr/local/bin/",
  };

  while (status) {
    printf("witsshell> ");
    input = get_input();
    args = split_input(input);
    status = exevute(args, path_list);

    for (int i = 0; args[i] != NULL; i++) {
      printf("args[%d] = %s\n", i, args[i]);
    }

    free(args);
    free(input);
  }
}

/* =============================== main ===================================*/

int main(int argc, char *argv[]) {

  batch_mode(argc, argv);
  interactive_mode();

  return 0;
}
