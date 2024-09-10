#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

char **path_list() {
  char *paths[] = {
      "/bin/",
      "/usr/bin/",
      "/usr/local/bin/",
  };

  // Allocate memory for the array of strings
  char **path_dict = malloc(sizeof(paths));

  // Copy each string to the allocated memory
  for (int i = 0; i < 3; i++) {
    path_dict[i] =
        malloc(strlen(paths[i]) + 1); // Allocate memory for each string
    strcpy(path_dict[i], paths[i]);   // Copy the string
  }

  return path_dict;
}

int exevute(char **args) {
  pid_t pid;
  int status;
  char **path = path_list();
  pid = fork();
  if (pid == 0) {
    // Child process
    for (int i = 0; i < 3; i++) {
      char *cmd = malloc(strlen(path[i]) + strlen(args[0]) + 1);
      strcpy(cmd, path[i]);
      strcat(cmd, args[0]);
      if (execvp(cmd, args) == -1) {
        perror("error executing");
      }
      free(cmd);
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

int cd_function(char **path) {

  if (path[1] == NULL) {
    fprintf(stderr, "expected argument to \"cd\"\n");
  } else {
    if (chdir(path[1]) != 0) {
      perror("no such file or directory");
    }
  }

  return 1;
}

int exit_function() {
  exit(0);
  return 0;
}

int help_function() {
  printf("Witsshell is a simple shell written in C\n");
  printf("The following commands are built in:\n");
  printf("cd\n");
  printf("exit\n");
  printf("help\n");
  return 1;
}

int (*builtin_func[])(char **) = {
    &cd_function,
    &exit_function,
    &help_function,
};

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

char *builtin_dict[] = {"cd", "exit", "help"};

int run_builtins(char **args) {

  for (int i = 0; i < 3; i++) {
    if (strcmp(args[0], builtin_dict[i]) == 0) {

      printf("running built in function args[0] %s\n", args[0]);

      return (*builtin_func[i])(args);
    }

    printf("running exevute function args[0] %s\n", args[0]);
  }
  return exevute(args);
}

void interactive_mode() {
  char *input;
  char **args;
  int status = 1;

  while (status) {
    printf("witsshell> ");
    input = get_input();
    args = split_input(input); // correct args
    status = run_builtins(args);

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
