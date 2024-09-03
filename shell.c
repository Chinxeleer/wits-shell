#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void inputValidation() {
  char *input = NULL;
  size_t len = 0;

  printf("witsshell> ");

  fflush(stdout);
  ssize_t nread = getline(&input, &len, stdin);

  if (nread == -1) {
    perror("getline");
    exit(EXIT_FAILURE);
  }

  if (strcmp(input, "exit\n") == 0) {
    free(input);
  }

  printf("input ----> %s", input);
}

void batchFileInput() {}
void splitInputs() {}

int main(int argc, char *argv[]) {

  int pid = fork();

  if (pid == 0) {

    if (argc == 2) {

      printf("the batch file is : %s \n", argv[1]);
      return 0;

    } else if (argc > 2) {

      printf("Throw an error because we only require one input");
      return 0;
    }

    /* strep --> to parse the input line into constituent pieces */

    while (1) {
      inputValidation();
    }
  }

  else {
    wait(NULL);
    printf("exiting the programme...");
  }
  return 0;
}
