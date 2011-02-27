/* replaceLabel.c
	Functions:	int replaceLabel(char *fname, char *fnameOut, char dTable[4096][LABELLEN], char dTable8[256][LABELLEN], char pTable[4096][LABELLEN]) 
			void substituteOps(char lOpcode[16], char lOperands[32], char dTable[4096][LABELLEN], char dTable8[256][LABELLEN], char pTable[4096][LABELLEN], int *pLabelCount, llNode *head) */
#include <stdio.h>
#include <string.h>
#include "qwiklst.h"

void substituteOps(char lOpcode[16], char lOperands[32], char dTable[4096][LABELLEN], char dTable8[256][LABELLEN], char pTable[4096][LABELLEN], int *pLabelCount, llNode **head);

int replaceLabel(char *fname, char *fnameOut, char dTable[4096][LABELLEN], char dTable8[256][LABELLEN], char pTable[4096][LABELLEN])
/* 	Inputs:	fname - name of input list file
			fnameOut - name of output list file to be written
			dTable - pointer to table of names corresponding to 12-bit addresses in data memory
			dTable8 - pointer to table of names corresponding to 8-bit addresses in program memory			
			pTable - pointer to table of names corresponding to addresses in program memory
	Returns: 	0 if successful
			1 if failure 
	Description: Formats the input list file fname to output list file fnameOut, based on variable names in tables */
{
	FILE* fid;
	FILE* fidout;
	char tline[1024];
	unsigned char foundByName = 0;
	char lOpcode[16], lOperands[32], lRest[1024];
	char spacesA[] = "          "; // spaces for between opcode and operands
	char spacesB[] = "               "; // spaces for between operands and source code, make this shorter for less space
	unsigned int lAddr, lValue;
	int pLabelCount=0;
	int i;
	int oldLength;
	char *lastcolon;
	int lineCount = 0;
	llNode *head = NULL;
	

	fid = fopen(fname,"r");
	if (fid==NULL)
	{
		printf("Error opening %s for reading\n", fname);
		return 1;
	}
	
	// get the file pointer in the right position
	while (1)
	{
		if (feof(fid)) { break; }
		fgets(tline,sizeof(tline),fid);	
		lineCount++;
		if (lineCount==223)
		break;
		/*
		// search for substring (NULL returned if noting found)
		if (strstr(tline,"-------  -------  -----------------------") != NULL)
		{
			foundByName = 1;			
			break;	
		}*/
	}
	if (lineCount != 223)
	{
		fclose(fid);
		return 1;
	}
	
	fidout = fopen("temp.lst","w");
	if (fid==NULL)
	{
		printf("Error opening %s for writing\n", fnameOut);
		return 1;
	}	
	while (1)
	{
		if (feof(fid)) { break; }
		fgets(tline,sizeof(tline),fid);		// also reads \n characters
		// match non-whitespace leading character
		if (tline[0] != ' ')
		{
			memset(lOpcode,0,16);		// blank variables between iterations
			memset(lOperands,0,32);
			memset(lRest,0,1024);
			sscanf(tline,"%6x %x %15s %15s%[^\n]", &lAddr, &lValue, lOpcode, lOperands, lRest);
			if (strlen(lOpcode) > 0)	// full opcode and source code line, parse
			{
				oldLength = strlen(lOperands);
				substituteOps(lOpcode, lOperands, dTable, dTable8, pTable, &pLabelCount, &head);
				
				if (strlen(lOpcode) > strlen(spacesA))
					fprintf(fidout,"%06X   %04X      %s %s", lAddr, lValue, lOpcode, lOperands);
				else
					fprintf(fidout,"%06X   %04X      %s%s%s", lAddr, lValue, lOpcode, &spacesA[strlen(lOpcode)], lOperands);
				
				// remove filename from the lRest string
				lastcolon = strrchr(lRest,':');
				if (lastcolon != NULL)					
					*(lastcolon-1) = '\0';					

				if (strlen(lOperands) < oldLength) // need extra whitespace characters
					fprintf(fidout, "%s%s%s\n", spacesB, &spacesB[strlen(spacesB)-(oldLength-strlen(lOperands))], lRest);
				else	// need less whitespace characters
					fprintf(fidout,"%s%s\n", &spacesB[strlen(lOperands)-oldLength], lRest);
			}
			else				// incomplete line with only 2 arguments (address and value)
			{
				fprintf(fidout,"%06X   %04X\n", lAddr, lValue);
			}
		}
		else
		{
			// remove the filename from the string
			lastcolon = strrchr(tline,':');
			if (lastcolon != NULL)
			{
				*(lastcolon-1) = '\n';
				*(lastcolon) = '\0';
			}
			fprintf(fidout,"%s %s", spacesB, tline);		// print out the line with extra whitespace
		}
	}
	fclose(fidout);
	fclose(fid);

	// Temporary file has been written.  Read it back and do the line number replacement
	if (labelLines("temp.lst",fnameOut,head))
	{
		return 1;			// problem labelling lines
	}
	printf("Modified list file written to %s\n", fnameOut);
	
	return 0;
}


void substituteOps(char lOpcode[16], char lOperands[32], char dTable[4096][17], char dTable8[256][17], char pTable[4096][17], int *pLabelCount, llNode **head)
/* 	Inputs:	lOpcode - string, pointer to name of opcode
			lOperands - string, pointer to operands of opcode	(modified by function)		
			dTable - pointer to table of names corresponding to 12-bit addresses in data memory
			dTable8 - pointer to table of names corresponding to 8-bit addresses in program memory			
			pTable - pointer to table of names corresponding to addresses in program memory
			pLabelCount - pointer to int, counter for temporary variable names (modified by function)
	Returns: 	none
	Description: Formats the lOperands string by replacing addresses with variable names and other formatting, such as supression of a parameter.  Modifies the lOperands string passed in based on lOpcode and input tables */
{
	// define all the different classes of instructions so the handling of operands can be based on the opcode
	char opByteFDA[] = " ADDWF ADDWFC ANDWF COMF DECF DECFSZ DECFSNZ INCF INCFSZ INCFSZ INFSNZ IORWF MOVF RLCF RLNCF RRCF RRNCF SUBFWB SUBWF SUBWFB SWAPF XORWF ";
	char opByteFA[] = " CLRF CPFSEQ CPFSGT CPFSLT MOVWF MULWF NEGF SETF TSTFSZ ";
	char opByteFF[] = " MOVFF ";
	char opBitFBA[] = " BCF BSF BTFSC BTFSS BTG ";
	char opControlN[] = " BC BN BNC BNN BNOV BNZ BOV BRA BZ GOTO RCALL ";
	char opControlNS[] = " CALL ";
	char opControlS[] = " RETURN RETFIE ";
	char opLitK[] = " ADDLW ANDLW IORLW MOVLB MOVLW MULLW RETLW SUBLW XORLW ";
	
	char Opcode[19];
	unsigned int opF, opD, opFd, opB, opN, opS, opK;

	// concatenate spaces on Opcode (necessary for search to distinguish MOVF from MOVFF, CALL from RCALL)
	sprintf(Opcode," %s ", lOpcode);

	// see what format the input instruction is
	if (strstr(opByteFDA,Opcode) != NULL)
	{
		/* convert f to name
			convert d to W if 0, F if 1
			eliminiate a */
		sscanf(lOperands,"%x,%x,%*x", &opF, &opD);
		if (opD==0)
		sprintf(lOperands, "%s,W", dTable8[opF&0xFF]);
		else
		sprintf(lOperands, "%s,F", dTable8[opF&0xFF]);
	}
	else if (strstr(opByteFA,Opcode) != NULL)
	{
		/* convert f to name
			eliminiate a */
		sscanf(lOperands,"%x,%*x", &opF);
		sprintf(lOperands,"%s", dTable8[opF&0xFF]);
	}
	else if (strstr(opByteFF,Opcode) != NULL)
	{
		/* convert fs to name
			convert fd to name */
		sscanf(lOperands,"%x,%x", &opF, &opFd);
		sprintf(lOperands,"%s,%s", dTable[opF&0xFFF], dTable[opFd&0xFFF]);
	}
	else if (strstr(opBitFBA,Opcode) != NULL)
	{
		/* convert f to name
			leave b unchanged, except display as decimal instead of hex
			eliminate a */
		sscanf(lOperands,"%x,%x,%*x", &opF, &opB);
		sprintf(lOperands,"%s,%d", dTable8[opF&0xFF],opB);	
	}
	else if (strstr(opControlN,Opcode) != NULL)
	{
		/* convert n to name */
		sscanf(lOperands,"%x", &opN);
		opN = opN & 0x0FFF; // limit to 12-bit address
		if (pTable[opN][0] != 0)	// if a label is already defined for this address, use it
		{
			if (pTable[opN][0] != '_')	// special labels will have _ appended to start, skip this character
			sprintf(lOperands,"%s", pTable[opN]);
			else
			sprintf(lOperands,"%s", &pTable[opN][1]);
		}
		else				// if not, make a label of the form Lxxx
		{
			sprintf(pTable[opN],"_L%03d",*pLabelCount);			
			llInsert(head,llCreate(opN,&pTable[opN][1]));			
			sprintf(lOperands,"%s", &pTable[opN][1]);
			(*pLabelCount)++;
		}
	}
	else if (strstr(opControlNS,Opcode) != NULL)
	{
		/* convert n to name
			leave s unchanged, except display as decimal instead of hex */
		sscanf(lOperands,"%x,%x", &opN,&opS);
		opN = opN & 0x0FFF; // limit to 12-bit address
		if (pTable[opN][0] != 0)	// if a label is already defined for this address, use it
		{
			if (pTable[opN][0] != '_')	// special labels will have _ appended to start, skip this character
			sprintf(lOperands,"%s,%d", pTable[opN],opS);
			else
			sprintf(lOperands,"%s,%d", &pTable[opN][1],opS);
		}
		else				// if not, make a label of the form Lxxx
		{
			sprintf(pTable[opN],"_L%03d",*pLabelCount);
			llInsert(head,llCreate(opN,&pTable[opN][1]));			
			sprintf(lOperands,"%s,%d", &pTable[opN][1],opS);
			(*pLabelCount)++;
		}
	}
	else if (strstr(opControlS,Opcode) != NULL)
	{
		/* eliminiate S if zero, put FAST if 1 */
		sscanf(lOperands,"%x",&opS);
		if (opS == 1)
		sprintf(lOperands,"FAST");
		else
		lOperands[0] = '\0';
	}
	else if (strstr(opLitK,Opcode) != NULL)
	{
		/* reformat hex as uppercase */
		sscanf(lOperands,"%x", &opK);
		sprintf(lOperands,"0x%02X", opK);
	}
}
