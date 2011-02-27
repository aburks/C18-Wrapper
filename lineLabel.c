#include <stdio.h>
#include <string.h>
#include "qwiklst.h"

void llInsert(llNode **head, llNode *new)
/* 	Inputs:	head - address of the pointer to the head node (the address of *head) (modified by function)
			new - new node to be inserted			
	Description: Inserts new to the proper position in the linked list starting with head */
{
	llNode **p;
	if (*head == NULL)	// if head is null, then new becomes head
		*head = new;
	else if (new->lineNumber <= (*head)->lineNumber)
	{
		// need to insert new at the beginning, since its lineNumber is less than head
		new->next = (void*)*head;				
		//((*new).next) = (llNode*)&(**head);
		*head = new;                
	}
	else
	{
		// new comes after head, find proper position by iterating through the list
		for (p=&(*head); *p != NULL; p = (llNode**)&(*p)->next)         
		if (new->lineNumber <= (*p)->lineNumber)	break;                
		new->next = (void*)*p;
		*p = new;
	}
}

void llDisplay(llNode *head)
{
/* 	Inputs:	head - pointer to the head node
	Description: Displays the contents of the linked list on the screen */
	llNode **p;
	p = &head;		
	for (p=&head; *p != NULL; p = (llNode**)&(*p)->next)        
	printf("%d %s\n",(*p)->lineNumber, (*p)->name);        
}

llNode* llCreate(int lineNumber, char *name)
{
/* 	Inputs:	lineNumber - int, line number of label
			name - character array, name of the label
	Outputs:	a *llNode for a the new node with lineNumber and name
	Description: Creates a linked list node */
	llNode *new;
	new = (llNode*)malloc(sizeof(llNode));
	new->lineNumber = lineNumber;
	strncpy(new->name,name,5);
	new->next = NULL;
	return new; 
}

int labelLines(char *fname, char *fnameOut, llNode *head)
{
/* 	Inputs:	fname - name of temporary file which needs line numbers (note: file is deleted!)
			fnameOut - name of final file with line numbers
			head - linked list of line numbers and labels
	Description: Adds line numbers to temporary file and saves as a new file called fnameOut */
	llNode *cur = NULL;
	unsigned int lAddr, lValue;
	char lRest[1024];
	FILE* fid;
	FILE* fidout;
	char tline[1024];
	
	fid = fopen(fname,"r");
	if (fid==NULL)
	{
		printf("Error opening %s for reading\n", fname);
		return 1;
	}
	fidout = fopen(fnameOut,"w");
	if (fidout==NULL)
	{
		printf("Error opening %s for writing\n", fnameOut);
		return 1;
	}
	cur = head;
	
	while (1)
	{
		if (feof(fid)) { break; }		
		fgets(tline,sizeof(tline),fid);	
		if (cur == NULL)			// nothing left in linked list, just output lines directly
		{
			if (strstr(tline,"RCS Header $Id:") != NULL) { break; }		// stop at the end of the file
			fprintf(fidout,"%s",tline);
		}
		else						// linked list not empty, still need to look for line numbers
		{
			if (tline[0] != ' ')			// match non-whitespace leading character
			{
				lAddr = 0;
				lValue = 0;				
				memset(lRest,0,1024);		// blank variables between iterations								
				sscanf(tline,"%6x %x %[^\n]", &lAddr, &lValue, lRest);
				if (lAddr==cur->lineNumber)
				{		
					fprintf(fidout,"%04X   %04X %s %s\n", lAddr, lValue, cur->name, lRest); 					
					cur = (llNode*)cur->next;					
				}
				else
					fprintf(fidout,"%04X   %04X      %s\n", lAddr, lValue, lRest);
			}
			else		// output line directly
				fprintf(fidout,"%s",tline);
		}
	}
	fclose(fid);
	fclose(fidout);
	// delete the temporary file
	remove(fname);
	return 0;
}
