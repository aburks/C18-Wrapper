#include <stdio.h>

typedef struct
{
	unsigned int map	: 1;	// generate map file
	unsigned int nolst	: 1;	// do not generate LST file
	unsigned int obj	: 1;	// generate object file
	unsigned int noCod	: 1;	// do not generate COD file
	unsigned int cof	: 1;	// generate COFF file
	unsigned int extended : 1;			// extended flags
	unsigned int skip_find_extended : 1;
	unsigned int noQb		: 1;	// don't include QwikBug in program size
	unsigned int verbose	: 1;	// verbose mode
	unsigned int qwiklst	: 1;	// generate qwik.lst file
} flag_struct;

int main (int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World\n");
    return 0;
}
