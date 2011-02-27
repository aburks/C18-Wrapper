/* qwiklst.c
	QwikLst v0.15
	November 16, 2007
	by Cody Planteen
	
	The QwikLst utility takes in a list file and map file from a compiled PIC program.  It then writes a new
	modified list file, with labels and variable names substituted in for addresses.
	
	This tool should be written in a scripting language (eg Perl) but was written in C to eliminate the
	need for external dependent software to be installed in the lab environment. */

#include <stdio.h>
#include <string.h>
#include "qwiklst.h"

void fixTables(char dTable[4096][LABELLEN], char dTable8[256][LABELLEN], char pTable[4096][LABELLEN]);

/*
int main(int argc, char* argv[])
{	
	// display the information message
	printf("QwikLst v0.15\n");
	printf("November 16, 2007\n");
	printf("by Cody Planteen\n\n");
	
	// check the number of input arguments
	if (argc != 4)
	{
		printf("Usage: qwiklst Program.lst Program_map.txt Program_formatted.lst\n");
		return 1;
	}
	
	// call qwikLst, the heart of the program
	qwikLst(argv[1], argv[2], argv[3]);

	return 0;
}
*/

int qwikLst(char *lstFile, char *mapFile, char *outFile)
{
/* 	Inputs:	lstFile - filename of input list file
			mapFile - filename of input map file
			outFile - filename of output modified list file
	Description: Creates outFile based on lstFile and labels in mapFile. */
	
	// dTable is the full 12-bit address space and holds the name of a variable at that location
	char dTable[4096][LABELLEN];
	// dTable8 is the 8-bit address space
	char dTable8[256][LABELLEN];
	// pTable holds the labels in program memory, PIC18F4321 and 18F2321 have 4Kwords of program memory maximum
	char pTable[4096][LABELLEN];

	// zero the arrays
	memset(dTable,0,4096*LABELLEN);
	memset(dTable8,0,256*LABELLEN);
	memset(pTable,0,4096*LABELLEN);
	
	// get the symbol table from the map file
	if(getSymbolTable(mapFile,dTable,pTable))	return 1;
	// form the 8-bit table and populate table the 8- and 12-bit tables with array indicies and values for larger data types
	fixTables(dTable, dTable8, pTable);
	// replace the labels and write to a new file
	replaceLabel(lstFile,outFile,dTable,dTable8,pTable);
}

void fixTables(char dTable[4096][LABELLEN], char dTable8[256][LABELLEN], char pTable[4096][LABELLEN])
{
/* 	Inputs:	dTable - pointer to table of names corresponding to addresses in data memory (modified by function)
			dTable8 - pointer to table of names corresponding to 8-bit addresses in data memory (modified by function)
			pTable - pointer to table of names corresponding to addresses in program memory (modified by function)		
	Description: Forms dTable8 based on dTable. For both dTables, fills in empty variable positions with LASTVARIABLENAME+1, +2, etc */
	int lastVarIdx = 0;
	int i; 
	int offset;
	char *result;
	
	// strip "bits" out of SFR names
	for (i=0xFFF;i>=0xF80;i--)
	{
		result = strstr(dTable[i],"bits");
		if (result != NULL)
			*result = '\0';
	}
	
	// form the 8-bit table, going in reverse order over 12-bit table so the user variables have highest priority
	for (i=4095;i>=0;i--)
		if (dTable[i][0] != 0) strcpy(dTable8[i & 0xFF], dTable[i]);

	// fill empty variable positions with LASTVARIABLENAME+1, +2, etc for the 12-bit address table
	for (i=0;i<4096;i++)
	{
		if (dTable[i][0] != 0)
			lastVarIdx=i;
		else
		{
			offset = i-lastVarIdx;
			if (offset > 99) offset = 99;		// must keep offset to less than 2 characters
			sprintf(dTable[i], "%s+%d", dTable[lastVarIdx],offset);
		}
	}
	// fill empty variable positions with LASTVARIABLENAME+1, +2, etc for the 8-bit address table
	lastVarIdx = 0;	
	for (i=0;i<256;i++)
	{
		if (dTable8[i][0] != 0)
			lastVarIdx=i;
		else
		{
			offset = i-lastVarIdx;
			if (offset > 99) offset = 99;		// must keep offset to less than 2 characters
			sprintf(dTable8[i], "%s+%d", dTable8[lastVarIdx],offset);
		}
		
	}
}
