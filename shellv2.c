#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_TOKENS 64

// Function to get input from the user
char *get_input() {
  char *input = NULL;
  size_t len = 0;

  if (getline(&input, &len, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS); // We received an EOF
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return input;
}

// Function to retrieve the PATH environment variable and split it into
// directories
char **path_list() {
  char *path_env = getenv("PATH");
  if (path_env == NULL) {
    // Fallback in case PATH is not set
    path_env = "/bin";
  }

  char **path_dict = malloc(MAX_TOKENS * sizeof(char *));
  char *token;
  int i = 0;

  token = strtok(path_env, ":");
  while (token != NULL && i < MAX_TOKENS - 1) {
    path_dict[i] = malloc(strlen(token) + 1);
    strcpy(path_dict[i], token);
    i++;
    token = strtok(NULL, ":");
  }
  path_dict[i] = NULL; // Null-terminate the array

  return path_dict;
}

// Function to execute a command by searching the directories in the PATH
int execute(char **args) {
  pid_t pid;
  int status;
  char **path = path_list();
  pid = fork();

  if (pid == 0) {
    // Child process
    for (int i = 0; path[i] != NULL; i++) {
      char *cmd = malloc(strlen(path[i]) + strlen(args[0]) + 2);
      sprintf(cmd, "%s/%s", path[i], args[0]);

      if (access(cmd, X_OK) == 0) {
        execv(cmd, args);
      }
      free(cmd);
    }
    perror("command not found");
    exit(EXIT_FAILURE);
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

// Function to split the input line into tokens
char **split_input(char *line) {
  char **tokens = malloc(MAX_TOKENS * sizeof(char *));
  char *token;
  int position = 0;

  if (!tokens) {
    fprintf(stderr, "allocation error\n");
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

// Built-in functions
int cd_function(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
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
  printf("The following commands are built-in:\n");
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

char *builtin_dict[] = {"cd", "exit", "help"};

// Function to run built-in commands or external executables
int run_builtins(char **args) {
  for (int i = 0; i < 3; i++) {
    if (strcmp(args[0], builtin_dict[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  return execute(args);
}

// Batch mode handler
int batch_mode(int argc, char *argv[]) {
  if (argc == 2) {
    printf("Batch file is: %s\n", argv[1]);
    return 0;
  } else if (argc > 2) {
    printf("Error: only one batch file is required\n");
    return 0;
  }
  return 0;
}

// Interactive mode for the shell
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

// Main function
int main(int argc, char *argv[]) {
  batch_mode(argc, argv);
  interactive_mode();

  return 0;
}
