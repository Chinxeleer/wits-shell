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
      exit(EXIT_SUCCESS); // We recieved an EOF
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return input;
}

char **path_list() {
  char *path_env = getenv("PATH");
  if (path_env == NULL) {
    // Fallback in case PATH is not set
    path_env = "/bin";
  }

  char *path_copy = strdup(path_env); // Make a copy of the PATH
  char **path_dict = malloc(MAX_TOKENS * sizeof(char *));
  char *token;
  int i = 0;

  token = strtok(path_copy, ":"); // Tokenize the copy
  while (token != NULL && i < MAX_TOKENS - 1) {
    path_dict[i] = malloc(strlen(token) + 1);
    strcpy(path_dict[i], token);
    i++;
    token = strtok(NULL, ":");
  }
  path_dict[i] = NULL; // Null-terminate the array

  free(path_copy); // Free the copy after tokenizing
  return path_dict;
}

int exevute(char **args) {
  pid_t pid;
  int status;
  char **path = path_list();
  pid = fork();

  if (pid == 0) {
    // Child process
    for (int i = 0; path[i] != NULL; i++) {
      char *cmd =
          malloc(strlen(path[i]) + strlen(args[0]) + 2); // +2 for '/' and
      sprintf(cmd, "%s/%s", path[i], args[0]);

      if (access(cmd, X_OK) == 0) {
        execv(cmd, args);
      }

      free(cmd);
    }
    exit(EXIT_SUCCESS);
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
    fprintf(stderr, "expected argument to \"cd\"\n");
  } else {
    if (chdir(path[1]) != 0) {
      perror("no such file or directory\n");
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

char *builtin_dict[] = {"cd", "exit", "help"};
int (*builtin_func[])(char **) = {
    &cd_function,
    &exit_function,
    &help_function,
};

int run_builtins(char **args) {

  for (int i = 0; i < 3; i++) {
    if (strcmp(args[0], builtin_dict[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  return exevute(args);
}

int batch_mode(int argc, char *argv[]) {
  if (argc == 2) {
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
      perror("File does not exist\n");
      exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    int status = 1; // Set status to 1 initially to continue the loop

    while (getline(&line, &len, file) != -1 && status) {
      char **args = split_input(line);

      if (args[0] == NULL) {
        free(args);
        continue;
      }

      status = run_builtins(args);

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

void interactive_mode() {
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
      // Trim leading/trailing whitespace if necessary
      while (*command == ' ')
        command++; // Skip leading spaces
      char *end = command + strlen(command) - 1;
      while (end > command && *end == ' ')
        end--;           // Skip trailing spaces
      *(end + 1) = '\0'; // Null-terminate the trimmed string

      if (strlen(command) > 0) {
        args = split_input(command); // Split the individual command
        status = run_builtins(args); // Execute the command

        free(args);
      }
      command = strtok(NULL, "\n"); // Move to the next command
    }

    free(input);
  }
}

/* =============================== main ===================================*/

int main(int argc, char *argv[]) {

  if (argc == 2) {
    batch_mode(argc, argv);
    return 0;
  } else if (argc > 2) {
    printf("Throw an error because we only require one input\n");
    return 0;
  }
  interactive_mode();

  return 0;
}

// #include <signal.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/wait.h>
// #include <time.h>
// #include <unistd.h>
//
// #define DELIM " \t\n\r\a"
// #define MAX_TOKENS 64
//
// char *get_input(FILE *stream) {
//   char *input = NULL;
//   size_t len = 0;
//
//   if (getline(&input, &len, stream) == -1) {
//     if (feof(stream)) {
//       exit(EXIT_SUCCESS); // We received an EOF
//     } else {
//       perror("readline");
//       exit(EXIT_FAILURE);
//     }
//   }
//
//   return input;
// }
//
// char **path_list() {
//   char *path_env = getenv("PATH");
//   if (path_env == NULL) {
//     path_env = "/bin"; // Fallback in case PATH is not set
//   }
//
//   char *path_copy = strdup(path_env); // Make a copy of the PATH
//   char **path_dict = malloc(MAX_TOKENS * sizeof(char *));
//   char *token;
//   int i = 0;
//
//   token = strtok(path_copy, ":"); // Tokenize the copy
//   while (token != NULL && i < MAX_TOKENS - 1) {
//     path_dict[i] = malloc(strlen(token) + 1);
//     strcpy(path_dict[i], token);
//     i++;
//     token = strtok(NULL, ":");
//   }
//   path_dict[i] = NULL; // Null-terminate the array
//
//   free(path_copy); // Free the copy after tokenizing
//   return path_dict;
// }
//
// int exevute(char **args) {
//   pid_t pid;
//   int status;
//   char **path = path_list();
//   pid = fork();
//
//   if (pid == 0) {
//     // Child process
//     for (int i = 0; path[i] != NULL; i++) {
//       char *cmd =
//           malloc(strlen(path[i]) + strlen(args[0]) + 2); // +2 for '/' and
//           '\0'
//       sprintf(cmd, "%s/%s", path[i], args[0]);
//
//       if (access(cmd, X_OK) == 0) {
//         execv(cmd, args);
//       }
//
//       free(cmd);
//     }
//     exit(EXIT_SUCCESS);
//   } else {
//     // Parent process
//     do {
//       waitpid(pid, &status, WUNTRACED);
//     } while (!WIFEXITED(status) && !WIFSIGNALED(status));
//   }
//   return 1;
// }
//
// char **split_input(char *line) {
//   char **tokens = malloc(MAX_TOKENS * sizeof(char *));
//   char *token;
//   int position = 0;
//
//   if (!tokens) {
//     fprintf(stderr, "allocation error\n");
//     exit(EXIT_FAILURE);
//   }
//
//   // Tokenize the input based on DELIM
//   while ((token = strsep(&line, DELIM)) != NULL) {
//     if (*token == '\0') {
//       continue; // Skip empty tokens
//     }
//     tokens[position] = token;
//     position++;
//
//     if (position >= MAX_TOKENS - 1) {
//       break; // Stop if we've reached the maximum number of tokens
//     }
//   }
//   tokens[position] = NULL; // Null-terminate the array
//   return tokens;
// }
//
// int cd_function(char **path) {
//   if (path[1] == NULL) {
//     fprintf(stderr, "expected argument to \"cd\"\n");
//   } else {
//     if (chdir(path[1]) != 0) {
//       perror("no such file or directory\n");
//     }
//   }
//   return 1;
// }
//
// int exit_function() {
//   exit(0);
//   return 0;
// }
//
// int help_function() {
//   printf("Witsshell is a simple shell written in C\n");
//   printf("The following commands are built in:\n");
//   printf("cd\n");
//   printf("exit\n");
//   printf("help\n");
//   return 1;
// }
//
// char *builtin_dict[] = {"cd", "exit", "help"};
// int (*builtin_func[])(char **) = {
//     &cd_function,
//     &exit_function,
//     &help_function,
// };
//
// int run_builtins(char **args) {
//   for (int i = 0; i < 3; i++) {
//     if (strcmp(args[0], builtin_dict[i]) == 0) {
//       return (*builtin_func[i])(args);
//     }
//   }
//   return exevute(args);
// }
//
// int batch_mode(int argc, char *argv[]) {
//   if (argc == 2) {
//     FILE *file = fopen(argv[1], "r");
//     if (file == NULL) {
//       perror("File does not exist");
//       exit(EXIT_FAILURE);
//     }
//
//     char *line = NULL;
//     size_t len = 0;
//     int status = 1; // Set status to 1 initially to continue the loop
//
//     while (status && getline(&line, &len, file) != -1) {
//       char **args = split_input(line);
//
//       if (args[0] == NULL) {
//         free(args);
//         continue;
//       }
//
//       // Debug: Show which command is being executed
//       printf("Executing command: %s\n", args[0]);
//
//       status = run_builtins(args); // Execute the command
//
//       free(args);
//
//       // Break loop if exit command was issued
//       if (status == 0) {
//         break;
//       }
//     }
//
//     free(line);
//     fclose(file);
//   } else if (argc > 2) {
//     printf("Error: Only one input file is allowed\n");
//     return 0;
//   }
//   return 1;
// }
//
// void interactive_mode() {
//   char *input;
//   char **args;
//   int status = 1;
//
//   while (status) {
//     printf("witsshell> ");
//     fflush(stdout);
//     input = get_input(stdin);
//
//     // Split input by '\n' to handle multiple commands in a single input
//     char *command = strtok(input, "\n");
//     while (command != NULL) {
//       while (*command == ' ')
//         command++; // Skip leading spaces
//       char *end = command + strlen(command) - 1;
//       while (end > command && *end == ' ')
//         end--;           // Skip trailing spaces
//       *(end + 1) = '\0'; // Null-terminate the trimmed string
//
//       if (strlen(command) > 0) {
//         args = split_input(command); // Split the individual command
//         status = run_builtins(args); // Execute the command
//         free(args);
//       }
//       command = strtok(NULL, "\n"); // Move to the next command
//     }
//
//     free(input);
//   }
// }
//
// int main(int argc, char *argv[]) {
//   if (argc == 2) {
//     // Batch mode: Execute commands from a file
//     batch_mode(argc, argv);
//   } else {
//     // Interactive mode: Launch interactive shell
//     interactive_mode();
//   }
//
//   return 0;
// }
