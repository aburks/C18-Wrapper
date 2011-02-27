/* getSymbolTable.c
	Functions:	int getSymbolTable(char *fname, char dTable[4096][LABELLEN], char pTable[4096][LABELLEN]) */
#include <stdio.h>
#include <string.h>
#include "qwiklst.h"

#define MAX_LABEL_SIZE	12
int getSymbolTable(char *fname, char dTable[4096][LABELLEN], char pTable[4096][LABELLEN])
/* 	Inputs:	fname - name of map file to read
			dTable - pointer to table of names corresponding to addresses in data memory (modified by function)
			pTable - pointer to table of names corresponding to addresses in program memory (modified by function)
	Returns: 	0 if successful
			1 if failure
	Description: Populates dTable and pTable with strings corresponding to names at the addresses (indicies) */
{
	FILE* fid;
	unsigned char foundByName = 0;
	char tline[1024];
	char *strptr;
	char lName[27], lLocation[11], lType[7];
	unsigned int lAddr;
	int tempVarCount=0;

	fid = fopen(fname,"r");
	if (fid==NULL)
	{
		printf("Error opening %s for reading\n", fname);
		return 1;
	}
	// move file pointer to the lines after "Symbols - Sorted by Address"
	while (1)
	{
		if (feof(fid)) { break; }
		fgets(tline,sizeof(tline),fid);
		if (strstr(tline,"Symbols - Sorted by Address") != NULL)
		{
			foundByName = 1;
			// get 2 more lines
			fgets(tline,sizeof(tline),fid);
			fgets(tline,sizeof(tline),fid);
			break;	
		}
	}

	if (!foundByName)
	{
		printf("Error: Didn't find \"Symbols - Sorted by Name\" in the file %s\n", fname);
		return 1;
	}

	while(1)
	{
		if (feof(fid)) { break; }
		fgets(tline,sizeof(tline),fid);
		if (strlen(tline) > 2)
		{
			sscanf(tline,"%26s %x %10s %6s", lName, &lAddr, lLocation, lType);	
			lAddr = lAddr & 0x0FFF;		// limit to 12-bit address
			if (strcmp(lLocation,"data") == 0)		// handle variables in the data memory
			{
				if (dTable[lAddr][0] == 0x00)		// make sure nothing is in this address yet
				{
					if (lName[0] != '_')			// make sure the variable name doesn't start with _
					{
						if (strcmp(lType,"extern") == 0) // only take extern variables (ignore static)
						{
							// add a table entry
							strncpy(dTable[lAddr], lName, MAX_LABEL_SIZE);
						}
					}
					else	// if variable starts with an underscore, give it a name in the form TEMPxxx
					{
						// add a table entry for the TEMP variable
						sprintf(dTable[lAddr],"TEMP%03d",tempVarCount);
						tempVarCount++;
					}
				}
			}
			else if (strcmp(lLocation,"program") == 0)	// handle variables in the program memory
			{
				// ignore everything strarting with a _
				if (lName[0] != '_')
				{
					strncpy(pTable[lAddr], lName, MAX_LABEL_SIZE);
				}
			}
		}
	}

	fclose(fid);
	return 0;
}
