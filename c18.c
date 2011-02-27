/* C18 -- wrapper for the mcc18 compiler.
 * Written for use in ECE 4175   Summer 2007  by Alex Singh (TA)
 */
//#define DEBUG
#define VERSION_NUM  "1.4"


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <Windows.h>	// For CopyFile and FileExists
#include "qwiklst.h"

typedef struct
{
	unsigned map :1;				//intermediate file flags
	unsigned nolst :1;
	unsigned obj :1;
	unsigned noCod :1;
	unsigned cof :1;
	unsigned extended :1;			// extended flags
	unsigned skip_find_extended :1;
	unsigned noQb :1;
	unsigned verbose:1;
	unsigned qwiklst:1;
} flag_struct;

/* The program seems short/simple enough to not need a separate header file yet. */
void PrintHelp(void);
int FileExists(const char *cpFileName);
unsigned int FindFamily(char *FileNamepExt);
char FindXINST(char *FileNamepExt);
char *BaseFileName(const char *cpFileName); 
void *MyMalloc(size_t size);
char *MyStrdup(const char *cpStr);
char *myAllocStr(int argc, char *argv[],rsize_t* dstSize);
char *myProcessOptions(int argc, char *argv[], flag_struct *f, int *iFileName, unsigned int*family);
void ProcessOutput(void);


int main(int argc, char *argv[]) {
	FILE *fp;
	rsize_t dstSize;			//processor family
	flag_struct f;
	char cpBuf[256]; // to hold temp strings
	char line[120];
	int err, iFileName;
	unsigned int i,j;
	unsigned int family = 0;
	const char *cpCommand = "C:\\MCC18\\bin\\mcc18 -I C:\\MCC18\\h -Opa- -Oa+";
	const char *lkrCommand = "C:\\MCC18\\bin\\mplink /l c:\\MCC18\\lib /q c:\\MCC18\\lkr\\18f";
	char *cmdLine;
	char *Flag_string;
	char *FileName,*FileNamepExt,*cpPeriod;

	if (argc <= 1) {
		PrintHelp();
		exit(-1);
	}

	//create and fill in Flag_strings
	Flag_string = myProcessOptions(argc,argv,&f,&iFileName,&family);

	if(!iFileName){ //if couldn't find a FileName:
		PrintHelp();
		free(Flag_string);
		exit(-1);
	}

	//assign t3 and t3.c to FileName and FileNamepExt respectively
	FileName     = argv[iFileName];
	FileNamepExt = argv[iFileName];
	if(!(cpPeriod = strrchr(argv[iFileName], '.'))){
		FileNamepExt = MyMalloc(sizeof(char) * (strlen(argv[iFileName]) +3));
		sprintf_s(FileNamepExt,strlen(argv[iFileName])+3,"%s.c",argv[iFileName]);
		cpPeriod = strrchr(FileNamepExt, '.');
	} else {
		FileName = MyStrdup(argv[iFileName]);
		*(FileName + (cpPeriod-argv[iFileName])) = 0; //endstring where period was
	}
	//at this point, FileNamepExt is a "filename.c" and cpPeriod points to '.'

	//if processor family not specified in command line, find it in .c file
	if(!family){
		if(FileExists(FileNamepExt)){
			if(family = FindFamily(FileNamepExt)){
				//printf("Compiling for the 18f%d\n",family); //move to end later
			} else { 
				printf("Error: Could not find processor from file '%s'\n",FileNamepExt);
				printf("       Please specify using option [-p=<processor>] \n");
				printf("       or using '#include <p18f____.h>' in file '%s'\n",FileNamepExt);
				free(Flag_string);
				if(FileNamepExt != argv[iFileName]) { free(FileNamepExt); }
				if(FileName != argv[iFileName]) { free(FileName); }
				exit(-2);
			}
		} else {
			printf("Error: Could not open file '%s'. File not found.",FileNamepExt);
			free(Flag_string);
			if(FileNamepExt != argv[iFileName]) { free(FileNamepExt); }
			if(FileName != argv[iFileName]) { free(FileName); }
			exit(-2);
		}
	}
	//if --no-extended not specified, check if it uses extended instructions:
	if(!f.skip_find_extended){
		if(FileExists(FileNamepExt)){
			if(FindXINST(FileNamepExt)){
				//printf("Using extended mode code\n");
				f.extended = 1;
			}
		} else {
			printf("Error: Could not open file '%s'. File not found.",FileNamepExt);
			free(Flag_string);
			if(FileNamepExt != argv[iFileName]) { free(FileNamepExt); }
			if(FileName != argv[iFileName]) { free(FileName); }
			exit(-2);
		}
	}
	//copy over command, filename, then flags
    cmdLine = myAllocStr(argc, argv,&dstSize);
	sprintf_s(cmdLine,dstSize,"%s -p=18f%d ",cpCommand,family);
	strcat_s(cmdLine,dstSize, argv[iFileName]); //filename
	if(f.extended){ strcat_s(cmdLine,dstSize," --extended"); }
	strcat_s(cmdLine,dstSize,Flag_string);
	if(!f.verbose){strcat_s(cmdLine,dstSize," > out.txt");}
    
	#ifdef DEBUG
	printf("\n%s\n",cmdLine);
	// C:\MCC18\bin\mcc18 -I C:\MCC18\h -Opa- -Oa+ -p=18f4321 <filename> > out.txt
    #endif

	if(err = system(cmdLine)){
		ProcessOutput();
		free(Flag_string);
		free(cmdLine);
		if(FileNamepExt != argv[iFileName]) { free(FileNamepExt); }
		if(FileName != argv[iFileName]) { free(FileName); }
		return err;
	}
	ProcessOutput();
	free(cmdLine);
	//look for .o file: assume -? was a flag if it doesn't exist (don't link)
	*(cpPeriod+1) = 'o';
	if(!FileExists(FileNamepExt)){
		free(Flag_string);
		if(FileNamepExt != argv[iFileName]) { free(FileNamepExt); }
		if(FileName != argv[iFileName]) { free(FileName); }
		return 0;
	}

	//invoke linker:
	dstSize = 90; //total size of linker command (no filenames)
	dstSize += 3*strlen(FileName);
	cmdLine = (char *) MyMalloc(sizeof(char) * (dstSize + 4));
	cmdLine[0] = 0;
	sprintf_s(cmdLine,dstSize,"%s%d.lkr   /m %s_map.txt /o %s.cof %s\n",lkrCommand,family,FileName,FileName,FileNamepExt);
	if(f.extended){ 
		cpPeriod = strchr(cmdLine, '.');
		*cpPeriod++ = '_'; *cpPeriod++ = 'e';
		*cpPeriod++ = '.'; *cpPeriod++ = 'l';
		*cpPeriod++ = 'k'; *cpPeriod++ = 'r';
	}
	#ifdef DEBUG
	printf("\n%s\n",cmdLine);
	//C:\MCC18\bin\mplink /l c:\MCC18\lib /q c:\MCC18\lkr\18f4321.lkr /m <filename>_map.txt /o <filename>.cof <filename>.o
    #endif
	if(err = system(cmdLine)){
		cmdLine[36] = ' '; cmdLine[37] = ' '; //remove the "/q"
		system(cmdLine);
		free(Flag_string);
		free(cmdLine);
		if(FileNamepExt != argv[iFileName]) { free(FileNamepExt); }
		if(FileName != argv[iFileName]) { free(FileName); }
		return err;
	}

	//verify map file was created:
	if(FileNamepExt != argv[iFileName]) { free(FileNamepExt); }
	FileNamepExt = cmdLine;
	sprintf_s(FileNamepExt, dstSize,"%s_map.txt",FileName);

	i=0; j=0;
	if ((err = fopen_s(&fp, FileNamepExt, "rt")) == 0) {
	   while(fgets(line, sizeof(line), fp) != NULL)
	   {
		   if(2 == sscanf_s (line, "%d out of %d", &i,&j)){
			 break;
		   }
	   }
	   fclose(fp);
	   // Before deleting map file, call on qwikLst
	   //qwiklst Program.lst Program_map.txt qwik.lst
	   if(f.qwiklst){ 
		   sprintf_s(line,sizeof(line),"%s.lst",FileName);
		   sprintf_s(cpBuf,sizeof(cpBuf),"%s_map.txt",FileName);
		   qwikLst(line, cpBuf, "qwik.lst");
	   }

	   if(!f.map) {sprintf_s(cpBuf,sizeof(cpBuf),"del %s_map.txt",FileName); system(cpBuf);}
	} else { 
		printf("Warning: Map file '%s' not found. Cannot determine byte count\n",FileNamepExt);
	}

	if(family == 4321) { printf("Compiled and linked successfully for the PIC18LF%d. ", family); }
	else { printf("Compiled and linked successfully for the 18F%d. ", family); }

	if(f.extended) { printf("(extended)"); } printf("\n");

	if(i == 0) {
		free(Flag_string);
		free(cmdLine);
		if(FileName != argv[iFileName]) { free(FileName); }
		return 1;
	}

	if(!f.noQb){ j = 7000; }
	printf("%d out of %d program memory bytes. Program memory utilization is %d%c.\n",
		i, j, i * 100 /j,'%');

	//clean up intermediate files:
	if(f.noCod) {sprintf_s(cpBuf,sizeof(cpBuf),"del %s.cod",FileName); system(cpBuf); }
	if(!f.cof) {sprintf_s(cpBuf,sizeof(cpBuf),"del %s.cof",FileName); system(cpBuf); }
	if(f.nolst) {sprintf_s(cpBuf,sizeof(cpBuf),"del %s.lst",FileName); system(cpBuf); }
	if(!f.obj) {sprintf_s(cpBuf,sizeof(cpBuf),"del %s.o",FileName); system(cpBuf); }

	free(Flag_string);
	free(cmdLine);
	if(FileName != argv[iFileName]) { free(FileName); }
	return 0;
}

/* Checks to see if the specified file exists and can be opened for reading.
 * Returns zero if not, and non-zero if it can.
 */
int FileExists(const char *cpFileName) {
	FILE *fp;
	errno_t err;

	if ((err = fopen_s(&fp, cpFileName, "rb")) == 0) {
		fclose(fp);
		return 1;
	}
	return 0;
}

/* Returns a new string containing the base name of the given file name.
 * MAY RETURN THE ORIGINAL POINTER -- This should probably be changed for consistency.
 */
char *BaseFileName(const char *cpFileName) {
	char *cpBuf;
	char *cpPeriod;

	assert(cpFileName != NULL);

	cpBuf = MyStrdup(cpFileName);

	cpPeriod = strrchr(cpBuf, '.');
	if (cpPeriod == NULL) {
		free(cpBuf);
		return (char *) cpFileName;		/* Get rid of warning. */
	}
	*cpPeriod = 0;	/* Easy way to cut off the file extension. */

	return cpBuf;
}

/* Wrapper around malloc that checks that the allocation succeeded. */
void *MyMalloc(size_t size) {
	void *vp = malloc(size);

	if (vp == NULL) {
		fprintf(stderr, "Malloc: Out of memory!\n");
		exit(-10);
	}

	return vp;
}

/* Wrapper around strdup that checks that the allocation succeeded. */
char *MyStrdup(const char *cpStr) {
	char *cpOut = _strdup(cpStr);

	if (cpOut == NULL) {
		fprintf(stderr, "Strdup: Out of memory!\n");
		exit(-11);
	}

	return cpOut;
}

//Returns a char * with enough space to fit in all options
char * myAllocStr(int argc, char *argv[], rsize_t* dstSize){
	char* result;
	int i;
	*dstSize = 76;  //total size of command line (-extended and -p=.. and > out.txt)(no file or options)
	for (i = 1; i < argc; i++) {
		//add this size + space + endchar
		*dstSize += strlen(argv[i]) + 2;
	}
	/* And allocate buffer space. */
	result = (char *) MyMalloc(sizeof(char) * (*dstSize + 4));
	result[0] = 0; //add endstring to signify empty string
	return result;
}

void ProcessOutput(void){
	char line[120];
	char substr[120];
	FILE *fp;
	int i;
	char skipped;
	if (fopen_s(&fp, "out.txt", "rt") == 0) {
		i = 0; skipped = 0;
	   while(fgets(line, 120, fp) != NULL)
	   { 
		   if(i < 2){
			   printf(line);
		   } else if(!skipped){
			   //printf(line); //sscanf_s returns -1 as well?
			   if(sscanf_s(line,"%s",substr,sizeof(substr)) == 1){
				   //printf("%s\n",substr);
				   //CHANGE LATER (DEPENDENT ON C: DRIVE)
				   if( substr[0] == 'C' || substr[0] == '-' ) { skipped = 1; printf("\n%s",line);}
			   }
		   } else { printf(line); }
		   i++;
	   }
	   printf("\n");
	   fclose(fp);
	   system("del out.txt");
	} //else { printf("Couldn't open file 'out.txt'\n"); }
}

unsigned int FindFamily(char * FileNamepExt){
	char line[120];
	char substr[120];
	FILE *fp;
	unsigned int family;
	char* nextStr;
	if (fopen_s(&fp, FileNamepExt, "rt") == 0) {
	   while(fgets(line, 120, fp) != NULL)
	   {
		   if(!sscanf_s(line, "%s",substr,sizeof(substr))){continue;}
		   if(substr[0] != '#'){ continue; }
		   nextStr = strchr(line,'#')+1; //character after pound
		   if(!sscanf_s(nextStr,"%s",substr,sizeof(substr))){ continue; }
		   if(_strnicmp(substr,"include",7) != 0){ continue; }
		   nextStr = strchr(nextStr,'e')+1;
		   if(!sscanf_s(nextStr,"%s",substr,sizeof(substr))){ continue; }
		   if(_strnicmp(substr,"<p18f",5) != 0){ continue; }//check first part
		   nextStr = strchr(substr,'.');
		   if(_strnicmp(nextStr,".h>",3) != 0){ continue; }//check last part
		   if(!sscanf_s(substr+5,"%d",&family,sizeof(family))){ continue; }
		   fclose(fp);
		   return family;
	   }
	   fclose(fp);
	} else { printf("Couldn't open file '%s'\n",FileNamepExt); }
	return 0;
}

/***
 * FindXINST()
 *  Looks for '#pragma config XINST=ON'
 ***/
char FindXINST(char *FileNamepExt){
	char line[120];
	char substr[120];
	FILE *fp;
	char* nextStr;
	if (fopen_s(&fp, FileNamepExt, "rt") == 0) {
	   while(fgets(line, 120, fp) != NULL)
	   {
		   if(!sscanf_s(line, "%s",substr,sizeof(substr))){continue;}
		   if(substr[0] != '#'){ continue; }
		   nextStr = strchr(line,'#')+1; //character after pound
		   if(!sscanf_s(nextStr,"%s",substr,sizeof(substr))){ continue; }
		   if(strncmp(substr,"pragma",6) != 0){ continue; }
		   nextStr = strchr(nextStr,'m')+2;
		   if(!sscanf_s(nextStr,"%s",substr,sizeof(substr))){ continue; }
		   if(strncmp(substr,"config",6) != 0){ continue; }
		   nextStr = strchr(nextStr,'g')+1;
		   if(!sscanf_s(nextStr,"%s",substr,sizeof(substr))){ continue; }
		   if(_strnicmp(substr,"XINST",5) != 0){ continue; }
		   nextStr = strchr(nextStr,'T')+1;
		   if(!sscanf_s(nextStr,"%s",substr,sizeof(substr))){ continue; }
		   if(substr[0] != '='){ continue; }
		   nextStr = strchr(nextStr,'=')+1;
		   if(!sscanf_s(nextStr,"%s",substr,sizeof(substr))){ continue; }
		   if(_strnicmp(substr,"ON",2) != 0){ continue; } //match
		   fclose(fp);
		   return 1;
	   }
	   fclose(fp);
	} else { printf("Couldn't open file '%s'\n",FileNamepExt); }
	return 0;
}
/*
* return a pointer to a string containing all flags to send to mcc18
* need to deallocate pointer. Also fills in the flag structure accordingly
*/
char * myProcessOptions(int argc, char *argv[],flag_struct* f, int* iFileName, unsigned int* family){
	int i;
	rsize_t dstSize;
	char * Flag_string;
	//initialize flags:
	f->cof=0;	f->noCod=0;	f->map=0;	f->nolst=0; f->noQb=0;
	f->obj=0;	f->extended=0;	f->skip_find_extended=0; f->verbose=0; f->qwiklst=0;
	//create flags string:
	Flag_string = myAllocStr(argc, argv, &dstSize);
	//fill in flag string:
	*iFileName = 0;  // No filename found yet
	for (i = 1; i < argc; i++) {
		//filename is the only option that doesn't start with '-'
		if( argv[i][0] != '-'){
			if(*iFileName == 0) {
				*iFileName = i;
			} else {
				//already have a filename, given another
				printf("Warning: Already using '%s' as filename.",argv[*iFileName]);
				printf(" Ignoring parameter '%s'\n",argv[i]);
			}
		} else if(_stricmp(argv[i],"-cof") == 0){
			f->cof = 1;
		} else if(_stricmp(argv[i],"-noCod") == 0){
			f->noCod = 1;
		} else if(_stricmp(argv[i],"-map") == 0){
			f->map = 1;
		} else if(_stricmp(argv[i],"-nolst") == 0){
			f->nolst = 1;
		} else if(_stricmp(argv[i],"-object") == 0){
			f->obj = 1;
		} else if(_stricmp(argv[i],"-noFilter") == 0){
			f->verbose = 1;
		} else if(_stricmp(argv[i],"-qwiklst") == 0){
			f->qwiklst = 1;
		} else if(_stricmp(argv[i],"--extended") == 0){
			f->skip_find_extended = 1;
			f->extended = 1;
		} else if(_stricmp(argv[i],"--no-extended") == 0){
			f->skip_find_extended = 1;
			f->extended = 0;  //for the case of contention
		} else if(_strnicmp(argv[i],"-p=18f",6) == 0){
			sscanf_s(argv[i],"-p=18f%d",family);
		} else if (_stricmp(argv[i],"-noQb") == 0){
			f->noQb = 1;
		} else { // if not one of the target flags, pass it to mcc18
			strcat_s(Flag_string,dstSize," ");
			strcat_s(Flag_string,dstSize, argv[i]);
		}
	}
	return Flag_string;
}


void PrintHelp(void){
	printf("C18 (version %s)\nWritten by Alex Singh, Summer 2007\n", VERSION_NUM);
	printf("Usage: c18 [options] file [options]\n");
	printf("  -qwiklst\t\t  Generate a qwik.lst file\n");
	printf("  -map\t\t\t  Generate a map file\n");
	printf("  -nolst\t\t  Do not generate MASM Listing file\n");
	printf("  -object\t\t  Generate object file\n");
	printf("  -noCod\t\t  Do not generate C/C++ Code Listing\n");
	printf("  -cof\t\t\t  Generate COFF file\n");
	printf("  -noQb\t\t\t  Don't include QwikBug when calculating\n");
	printf("  \t\t\t  program memory utilization\n");
	printf("  -noFilter\t\t  Show all of MCC18's output\n");
	printf("  -h\t\t\t  Display MCC18's help screen for additional\n");
	printf("  \t\t\t  options (please include a file name)\n");
}