// CPUlator_stdio_example.c
#include <stdio.h>
#include <stdbool.h> // Include the stdbool.h header file for boolean type and constants, not needed with C23
int main () {
	int i = 123;
	char* text1 ="abcd";
	bool logic = false;
	if (! logic) {
		printf("Testing stdio on CPUlator: i = %d, text1 = %s\n", i, text1);
	}
}
