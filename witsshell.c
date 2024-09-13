#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define DELIM " \t\n\r\a"
#define MAX_TOKENS 64

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

char **path_list(char **patharg) {
  char **path_dict = malloc(MAX_TOKENS * sizeof(char *));
  int i = 0;

  if (patharg != NULL && patharg[0] != NULL) {
    // Case 1: Use the provided patharg
    for (int j = 0; patharg[j] != NULL && i < MAX_TOKENS - 1; j++) {
      char *token = strtok(patharg[j], ":"); // Tokenize each patharg element
      while (token != NULL && i < MAX_TOKENS - 1) {
        path_dict[i] = strdup(token); // Copy the token to path_dict
        i++;
        token = strtok(NULL, ":");
      }
    }
  } else {
    path_dict[0] = NULL;
    // // Case 2: Fallback to the PATH environment variable
    // char *path_env = getenv("PATH");
    // if (path_env == NULL) {
    //   // Fallback in case PATH is not set
    //   path_env = "";
    // }
    //
    // char *path_copy = strdup(path_env);   // Make a copy of the PATH
    // char *token = strtok(path_copy, ":"); // Tokenize the PATH
    //
    // while (token != NULL && i < MAX_TOKENS - 1) {
    //   path_dict[i] = strdup(token); // Copy the token to path_dict
    //   i++;
    //   token = strtok(NULL, ":");
    // }
    // free(path_copy); // Free the copy after tokenizing
  }

  path_dict[i] = NULL; // Null-terminate the array
  return path_dict;
}

int exevute(char **args, char **patharg, char *output_file) {
  pid_t pid;
  int status;
  char **path = path_list(patharg);
  pid = fork();

  if (pid == 0) {

    if (output_file != NULL) {
      // Open the output file, overwrite if it exists, create if it doesn't
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
      }

      // Redirect both stdout and stderr to the file
      dup2(fd, STDOUT_FILENO); // Redirect stdout
      dup2(fd, STDERR_FILENO); // Redirect stderr
      close(fd);               // Close file descriptor after redirection
    }

    // Child process
    for (int i = 0; path[i] != NULL; i++) {
      char *cmd = malloc(strlen(path[i]) + strlen(args[0]) + 2); // +2 for '/'
      sprintf(cmd, "%s/%s", path[i], args[0]);

      if (access(cmd, X_OK) == 0) {
        execv(cmd, args);
      }

      free(cmd);
    }
    // If we failed to exec a command, print error message and exit
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
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
    fprintf(stderr, "allocation error\n");
    exit(EXIT_FAILURE);
  }

  // Use strsep to tokenize the input based on space (" ")
  while ((token = strsep(&line, DELIM)) != NULL) {
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
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
  } else {
    if (chdir(path[1]) != 0) {
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
    }
  }

  return 1;
}

int exit_function(char **path) {
  if (path[1] == NULL) {
    exit(0);
  } else {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
  }

  return 0;
}

char **path_function(char **args) {
  int count = 0;
  for (int i = 1; args[i] != NULL; i++) {
    count++;
  }

  char **path = malloc((count + 1) * sizeof(char *));
  if (path == NULL) {
    perror("malloc failed");
    return NULL;
  }

  for (int i = 0; i < count; i++) {
    path[i] = strdup(args[i + 1]);
    if (path[i] == NULL) {
      perror("strdup failed");
      for (int j = 0; j < i; j++) {
        free(path[j]);
      }
      free(path);
      return NULL;
    }
  }
  path[count] = NULL;

  return path;
}

int run_builtins(char **args, char ***path) {
  char *output_file = NULL;
  int i = 0;

  while (args[i] != NULL) {
    if (strcmp(args[i], ">") == 0) {
      if (args[i + 1] == NULL || args[i + 2] != NULL) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));

        // Error: no file or too many arguments after '>'
        return 1;
      }
      output_file = args[i + 1]; // File to redirect output
      args[i] = NULL;            // Terminate the command arguments at '>'
      break;
    }
    i++;
  }

  if (strcmp(args[0], "cd") == 0) {
    return cd_function(args);
  } else if (strcmp(args[0], "exit") == 0) {
    return exit_function(args);
  } else if (strcmp(args[0], "path") == 0) {
    if (*path != NULL) {
      for (int i = 0; (*path)[i] != NULL; i++) {
        free((*path)[i]);
      }
      free(*path);
    }
    *path = path_function(args);
    return 1;
  }

  return exevute(args, *path, output_file);
}

int batch_mode(int argc, char *argv[], char ***official_path) {
  if (argc == 2) {
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
      perror("File does not exist\n");
      exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    int status = 1;

    while (getline(&line, &len, file) != -1 && status) {
      char **args = split_input(line);

      if (args[0] == NULL) {
        free(args);
        continue;
      }

      status = run_builtins(args, official_path);
      free(args);
    }

    free(line);
    fclose(file);
  } else if (argc > 2) {
    printf("Error: Only one input file is allowed\n");
    return 0;
  }
  return 0;
}

void interactive_mode(char ***official_path) {
  char *input;
  char **args;
  int status = 1;

  while (status) {
    printf("witsshell> ");
    fflush(stdout);
    input = get_input();

    // Split input by '\n' to handle multiple commands in a single input
    char *command = strtok(input, "\n");
    while (command != NULL) {
      while (*command == ' ')
        command++;
      char *end = command + strlen(command) - 1;
      while (end > command && *end == ' ')
        end--;
      *(end + 1) = '\0';

      if (strlen(command) > 0) {
        args = split_input(command);
        status = run_builtins(args, official_path);
        free(args);
      }
      command = strtok(NULL, "\n");
    }
    free(input);
  }
}

int main(int argc, char *argv[]) {
  char **official_path =
      malloc(2 * sizeof(char *));    // Allocate space for two elements
  official_path[0] = strdup("/bin"); // Set the first element to "/bin"
  official_path[1] = NULL;

  if (argc == 2) {
    batch_mode(argc, argv, &official_path);
  } else if (argc > 2) {
    printf("Error: Only one input file is allowed\n");
    return 0;
  } else {
    interactive_mode(&official_path);
  }
  for (int i = 0; official_path[i] != NULL; i++) {
    free(official_path[i]);
  }
  free(official_path);

  return 0;
}
