# you should not need to modify this.
CC =gcc
CFLAGS = -O0 -g -Wall -Wno-unused-variable

all: trojan-cc2

trojan-cc1: trojan-cc2.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f  cc-attacked login-attacked out login identity-cc trojan-cc1 *.o a.out  *~ cc-attacked.c login-attacked.c out.c temp-out.c temp-out.c
