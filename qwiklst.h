#define LABELLEN	17
// Define a linked list data structure for handling the line numbers
typedef struct {
        int lineNumber;
		char name[5];
        struct llNode *next;
} llNode;

int qwikLst(char *lstFile, char *mapFile, char *outFile);
int getSymbolTable(char *fname, char dTable[4096][LABELLEN], char pTable[4096][LABELLEN]);
int replaceLabel(char *fname, char *fnameOut, char dTable[4096][LABELLEN], char dTable8[256][LABELLEN], char pTable[4096][LABELLEN]);
llNode* llCreate(int lineNumber, char *name);
int labelLines(char *fname, char *fnameOut, llNode *head);

