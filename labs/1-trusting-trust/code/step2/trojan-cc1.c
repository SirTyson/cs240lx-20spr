// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "attck.c"

#define error(args...) do { fprintf(stderr, ##args); exit(1);  } while(0)

// a not very interesting compile
static void compile(char *program, char *outname) {
    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);
    /*
    // not a very robust finger-print, but what can you do.
    // in a real compiler we would do this match on the parse tree,
    // which strips out some non-essential features.
    static char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    // inject this after the assert.  not much of an attack.   
    //this is just a quick placeholder.
    static char compile_attack[] = "printf(\"%s:%d: could have run your attack here!!\\n\", __FUNCTION__, __LINE__);";

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";
    */
    /*  your code goes here */
    /*
    size_t new_program_size = strlen(program) + strlen(login_attack) 
      + strlen(compile_attack) + 1;
    char *new_program = malloc(new_program_size);
      
    char *login_pos = strstr(program, login_sig);
    if (login_pos)
    {
      char first_half[(size_t)(login_pos - program) + 1];
      strncpy(first_half, program, (size_t)(login_pos - program) + 1);
      first_half[(size_t)(login_pos - program)] = '\0';
      stpcpy(new_program, first_half);
      strcat(new_program, login_sig);
      strcat(new_program, login_attack);
      strcat(new_program, login_pos + strlen(login_sig));
      program = new_program;
    }

    char *compile_pos = strstr(program, compile_sig);
    if (compile_pos) 
    {
      char first_half[(size_t)(compile_pos - program) + 1];
      strncpy(first_half, program, (size_t)(compile_pos - program) + 1);
      first_half[(size_t)(compile_pos - program)] = '\0';
      stpcpy(new_program, first_half);
      strcat(new_program, compile_sig);
      strcat(new_program, compile_attack);
      strcat(new_program, compile_pos + strlen(compile_sig));
      program = new_program;
    }
    */
    attack(&program);
    //printf("%s", program);
    fprintf(fp, "%s", program);
    fclose(fp);

    // gross, call gcc.
    char buf[1024];
    sprintf(buf, "gcc ./temp-out.c -o %s", outname);
    if(system(buf) != 0)
        error("system failed\n");
}

#   define N  8 * 1024 * 1024
static char buf[N+1];

int main(int argc, char *argv[]) {
    if(argc != 4)
        error("expected 4 arguments have %d\n", argc);
    if(strcmp(argv[2], "-o") != 0)
        error("expected -o as second argument, have <%s>\n", argv[2]);

    // read in the entire file.
    int fd;
    if((fd = open(argv[1], O_RDONLY)) < 0)
        error("file <%s> does not exist\n", argv[1]);

    int n;
    if((n = read(fd, buf, N)) < 1)
        error("invalid read of file <%s>\n", argv[1]);
    if(n == N)
        error("input file too large\n");

    // "compile" it.
    compile(buf, argv[3]);
    return 0;
}
