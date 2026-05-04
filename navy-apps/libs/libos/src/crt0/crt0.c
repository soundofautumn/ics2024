#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  printf("call_main args: %p %p %p\n", (void *)args[0], (void *)args[1], (void *)args[2]);
  int argc = (int)args[0];
  char **argv = (char **)&args[1];
  char **envp = (char **)&args[argc + 2];
  environ = envp;
  exit(main(argc, argv, envp));
  assert(0);
}
