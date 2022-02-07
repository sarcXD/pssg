#include <windows.h>
#include <stdio.h>
#define bool int
#define true 1
#define false 0
#define MToKB(mb) (mb*1024)
#define MToB(mb) (mb*1024*1024)
#define ALLOCSIZE MToB(10)
#define STR_MAXLEN 50
#define FILE_MAXLEN 50

static char readBuffer[ALLOCSIZE];
// Utility Functions
/**
 * @brief Self Implemented string compare operation
 * 
 * @param given variable char * 
 * @param toMatch constant char *
 * @return true 
 * @return false 
 */
bool p_strcmp(char *given, char *toMatch) {
  int i = 0;
  while ((given[i] != '\0' && toMatch[i] != '\0')) 
  {
    if (given[i] != toMatch[i]) return false;
    i++;
  }
  return given[i] != toMatch[i] ? false : true; 
}

bool p_strcmpMulti(char *given, char *Mstr, int numl) {
  bool res = false; 
  while (numl-->0 && *Mstr != '\0') {
    res = res | p_strcmp(Mstr,given);
    if (res) return res;
    Mstr+=STR_MAXLEN;
  }
  return res;
}

long int p_filesz(FILE *file) {
  fseek(file, 0, SEEK_END); // seek to end of file
  long int size = ftell(file); // get current file pointer
  fseek(file, 0, SEEK_SET);
  return size;
}

void p_replaceChar(char *str, char find, char repl) {
  while (*str != '\0') {
    if (*str == find) {
      *str = repl;
    }
    str++;
  }
}

//**********UTIL END********************

void SearchInPath(char *fileDir, char *ignoreFile, int numl) {
  WIN32_FIND_DATA ffd;
  HANDLE hFind;
  LARGE_INTEGER filesize;
  DWORD dwError=0;
  // 1. update the fileDir with \\*
  strcat(fileDir, (char *)("/*"));
  // 2. Read file
  hFind = FindFirstFile(fileDir, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) 
  {
    printf ("FindFirstFile failed (%d)\n", GetLastError());
    return;
  }
  do
  {
    // .,.. are os specific and will cause inf loops
    if (p_strcmp(ffd.cFileName,".") || p_strcmp(ffd.cFileName, "..")) continue;
    // win32 for some reason, retunrs a plain name, without directory `\`
    // so we concat the directory flag to it (for simplicity)
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) strcat(ffd.cFileName,"/");
    // ..checking entries with ignore file
    if (p_strcmpMulti(ffd.cFileName,ignoreFile, numl)) continue;
    if (p_strcmp(ffd.cFileName,".pssd")) continue; // pssd folder
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        printf("  %s   <DIR>\n", ffd.cFileName);
    }
    else {
        filesize.LowPart = ffd.nFileSizeLow;
        filesize.HighPart = ffd.nFileSizeHigh;
        printf("  %s   %lld bytes\n", ffd.cFileName, filesize.QuadPart);
    }
  }
  while (FindNextFile(hFind, &ffd) != 0);
  dwError = GetLastError();
  FindClose(hFind);
}

int LoadIgnoreFile(char *ignoreFile, char *fileReadBuff) {
  // Windows specific stuff
  FILE *file;
  LARGE_INTEGER fsize;
  file = fopen(ignoreFile, "r");
  long int size = p_filesz(file);
  char *res;
  int numl = 0;
  // char *reader = fileReadBuff;
  while(fgets(fileReadBuff,size,file)) {
    p_replaceChar(fileReadBuff,'\n','\0');
    fileReadBuff+=STR_MAXLEN;
    numl++;
  };
  fclose(file);
  return numl;
}

void SearchAndReplace(char *fileDir) {
  char *fileReadBuff = readBuffer;
  char ignorefpath[STR_MAXLEN];
  strcpy(ignorefpath, fileDir);
  strcat(ignorefpath,"\\.gitignore");
  int numl = LoadIgnoreFile(ignorefpath, fileReadBuff);
  SearchInPath(fileDir, fileReadBuff, numl);
  // ReplaceInPath
}

int main( int argc, char *argv[]) {
  while(--argc > 0) {
    if ((*++argv)[0] == '-') {
      char c = *++argv[0];
      switch (c) {
        case 'x': {
          SearchAndReplace((++argv)[0]);
          return 1;
        }
        case 'h': {
          printf("PSSG: The Bloat free, static site generator (helper)\n");
          printf("use: main.exe -[arguments]\n");
          printf("arguments             Description\n");
          printf("-x {$project folder}  Execute Custom html replacement operation. This will find any and all custom elements\n\
                      in your static site that are marked as follows:\n\
                      `.obj\n\
                      <Navbar />`\n\
                      The Program then searches in a .pssg\\objects folder in your root directory.\n\
                      If it finds a Navbar.html file, it will replace all such entries in your static site html\n\
                      With this custom html");
          return 1;
        }
        default: {
          break;
        }
      }
    }
  }
  printf("Incorrect Execution, please use `main.exe -h` for help");
  return 0;
}