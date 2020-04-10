// convert the contents of stdin to their ASCII values (e.g., 
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>
#include <unistd.h>

int 
main (void) 
{
  printf ("char prog[] = {\n");
  char cur;
  for (int i = 0; read (STDIN_FILENO, &cur, 1) > 0; i++)
    printf("\t%d,%c", cur, (i+1)%8==0 ? '\n' : ' ');    
  printf ("0 };\n");

	return 0;
}
