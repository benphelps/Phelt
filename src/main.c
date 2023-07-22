#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
#include <readline/history.h>
#include <readline/readline.h>

static void repl(void)
{
    utf8_int8_t* line;
    while ((line = readline("> ")) != NULL) {
        interpret("repl", line);
        if (*line)
            add_history(line);
        free(line);
    }
}

static void runFile(const char* path)
{
    utf8_int8_t*    source = readFile(path);
    InterpretResult result = interpret(path, source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argc, const char* argv[])
{
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: phelt [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
