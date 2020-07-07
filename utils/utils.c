#include <stdio.h>
#include "utils.h"

void print_hex(void *hex, int size)
{
	int i = 0;
	char *tmp = hex;
	static const char buffer[] = "0123456789ABCDEF";
	for(i=0; i < size; i++) {
		printf("%c%c ", buffer[(tmp[i] >> 4) & 0xF],
			buffer[tmp[i] & 0xF]);
		if(i % 16 == 15 && i < (size - 1)) {
			printf("\n");
		}
	}
}
