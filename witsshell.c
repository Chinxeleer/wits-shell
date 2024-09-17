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
  }

  path_dict[i] = NULL; // Null-terminate the array
  return path_dict;
}

int exevute(char **args, char **path, char *output_file) {
  pid_t pid;
  int status;
  char **cmd_path = path_list(path);

  pid = fork();

  if (pid == 0) {
    // Child process
    if (output_file != NULL) {
      // Open the output file, overwrite if it exists, create if it doesn't
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
      }

      // Redirect both stdout and stderr to the file
      if (dup2(fd, STDOUT_FILENO) < 0) { // Redirect stdout to file
        perror("dup2 stdout");
        exit(EXIT_FAILURE);
      }
      if (dup2(fd, STDERR_FILENO) < 0) { // Redirect stderr to file
        perror("dup2 stderr");
        exit(EXIT_FAILURE);
      }
      close(fd); // Close the file descriptor after redirection
    }

    // Try to execute the command using each path from `cmd_path`
    for (int i = 0; cmd_path[i] != NULL; i++) {
      char *cmd = malloc(strlen(cmd_path[i]) + strlen(args[0]) +
                         2); // +2 for '/' and null terminator
      sprintf(cmd, "%s/%s", cmd_path[i], args[0]);
      if (access(cmd, X_OK) == 0) {
        execv(cmd, args);
        perror("execv failed");
        free(cmd);
        exit(EXIT_FAILURE);
      }

      free(cmd);
    }

    // If we reached here, the command was not found
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
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));

    exit(EXIT_FAILURE);
  }

  while ((token = strsep(&line, DELIM)) != NULL) {
    if (*token == '\0') {
      continue; // Skip empty tokens
    }

    // Check if the token contains '>'
    char *redirect_ptr = strchr(token, '>');
    if (redirect_ptr != NULL) {
      // Handle case where '>' is inside the token
      if (redirect_ptr == token) {
        // '>' cannot be the first character in the command
        if (position == 0) {
          char error_message[30] = "An error has occurred\n";
          write(STDERR_FILENO, error_message, strlen(error_message));

          free(tokens);
          return NULL; // Signal an error
        }
      }

      // Split the token into two parts: before and after the '>'
      if (redirect_ptr != token) {
        // First, add the part before '>'
        tokens[position++] = strndup(token, redirect_ptr - token);
      }
      // Then, add the '>' symbol itself
      tokens[position++] = strdup(">");

      // If there's anything after '>', add that as a separate token
      if (*(redirect_ptr + 1) != '\0') {
        tokens[position++] = strdup(redirect_ptr + 1);
      }
    } else {
      // No '>' in the token, just add it as is
      tokens[position++] = token;
    }

    if (position >= MAX_TOKENS - 1) {
      break; // Stop if we've reached the maximum number of tokens
    }
  }

  // Additional validation for incorrect usage of `>` redirection
  for (int i = 0; i < position; i++) {
    if (strcmp(tokens[i], ">") == 0) {
      // Check if `>` is the first token or the last token
      if (i == 0 || tokens[i + 1] == NULL) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        free(tokens);
        return NULL; // Signal an error
      }
      // Check if multiple `>` are used without a command in between
      if (i > 0 && strcmp(tokens[i - 1], ">") == 0) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        free(tokens);
        return NULL; // Signal an error
      }
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
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    int status = 1;

    while (getline(&line, &len, file) != -1 && status) {
      char **args = split_input(line);

      if (args != NULL && args[0] != NULL) {
        status = run_builtins(args, official_path);
      }
      free(args);

      // status = run_builtins(args, official_path);
      // free(args);
    }

    free(line);
    fclose(file);
  } else if (argc > 2) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
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

        if (args != NULL) {
          status = run_builtins(args, official_path);
          free(args);
        }
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
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));

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
