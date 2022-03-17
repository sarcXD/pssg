#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib\p_lib.h"
#define bool int
#define true 1
#define false 0
#define MToKB(mb) (mb*1024)
#define MToB(mb) (mb*1024*1024)
#define STR_MAXLEN 64
#define FILE_MAXLEN 64
#define ALLOCSIZE MToB(64)
char *Buffer;
char *allocp;
struct ProgState{
  char *configPath; // .ppsg files path
  char *ignorefile; // .gitignore file
};

struct configState {
  /*
    how this is organized: 
    char name -> File data, 
    char name -> File data,
    ...
  */
  char *obj;
};

// Utility Functions
/**
 * @brief Basic Stack Alloc impl
 * 
 * @param ptr pointer to allocate space to
 * @param reqSz size we need to allocate
 */
char *p_stalloc_ch(int reqSz) {
  if (Buffer + ALLOCSIZE - allocp >= reqSz) {
    allocp+=reqSz;
    return allocp-reqSz;
  }
  return 0;
};

/**
 * @brief Basic Stack Free impl
 * 
 * @param ptr the pointer we want to free
 * @param sz the size of the pointer to free
 */
void p_stfree_ch(char *ptr) {
  if (ptr >= Buffer && ptr <= Buffer + ALLOCSIZE) {
    allocp = ptr;
  }
};

//**********UTIL END********************

void SearchInPath(char *fileDir, char *ignoreFile, int numl, struct ProgState state) {
  WIN32_FIND_DATA ffd;
  HANDLE hFind;
  LARGE_INTEGER filesize;
  DWORD dwError=0;
  // 1. update the fileDir with /*
  char *wfileDir=p_stalloc_ch(STR_MAXLEN);
  strcpy(wfileDir,fileDir);
  strcat(wfileDir, (char *)("/*"));
  // 2. Read file
  hFind = FindFirstFile(wfileDir, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) 
  {
    printf ("FindFirstFile failed (%d)\n", GetLastError());
    return;
  }
  do
  {
    // .,.. are os specific and will cause inf loops
    if (p_strcmp(ffd.cFileName,".") || p_strcmp(ffd.cFileName, "..")) continue;
    // ..checking entries with ignore file
    if (p_strcmpMulti(ffd.cFileName,ignoreFile, numl, STR_MAXLEN)) continue;
    // if the dir is the pssg config
    if (p_strcmp(ffd.cFileName,".pssg")) {
      strcpy(state.configPath, fileDir);
      strcat(state.configPath,"/.pssg");
      continue;
    }
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        printf("  %s   <DIR>\n", ffd.cFileName);
        char *tempDir = p_stalloc_ch(STR_MAXLEN);
        strcpy(tempDir, fileDir);
        strcat(tempDir,"/");
        strcat(tempDir,ffd.cFileName);
        SearchInPath(tempDir, ignoreFile, numl, state);
        p_stfree_ch(tempDir);
    }
    else {
        filesize.LowPart = ffd.nFileSizeLow;
        filesize.HighPart = ffd.nFileSizeHigh;
        p_substrcmp(ffd.cFileName,".html") ? /*readFile*/ printf("true") : printf("false");
        printf("  %s   %lld bytes\n", ffd.cFileName, filesize.QuadPart);
    }
  }
  while (FindNextFile(hFind, &ffd) != 0);
  dwError = GetLastError();
  p_stfree_ch(wfileDir);
  FindClose(hFind);
}

void SearchAndReplace(char *fileDir) {
  // Memory init
  Buffer = (char *) malloc (ALLOCSIZE);
  allocp = Buffer;
  struct ProgState state;
  state.configPath = p_stalloc_ch(STR_MAXLEN); 
  char *fileReadBuff = p_stalloc_ch(STR_MAXLEN*FILE_MAXLEN);
  char *ignorefpath = p_stalloc_ch(STR_MAXLEN);
  strcpy(ignorefpath, fileDir);
  strcat(ignorefpath,"\\.gitignore");
  printf(ignorefpath);
  int numl = readFile(ignorefpath, fileReadBuff, "r", STR_MAXLEN);
  p_stfree_ch(ignorefpath);
  SearchInPath(fileDir, fileReadBuff, numl, state);
  p_stfree_ch(fileReadBuff);
  p_stfree_ch(state.configPath);
}

int main( int argc, char *argv[]) {
  while(--argc > 0) {
    if ((*++argv)[0] == '-') {
      char c = *++argv[0];
      switch (c) {
        case 'e': {
          SearchAndReplace((++argv)[0]);
          return 1;
        }
        case 'h': {
          printf("PSSG: The Bloat free, static site generator (helper)\n");
          printf("use: main.exe -[arguments]\n");
          printf("arguments             Description\n");
          printf("-e {$project folder}  Execute Custom html replacement operation. This will find any and all custom elements\n\
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