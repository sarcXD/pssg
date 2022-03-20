#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib/p_lib.h"
#define bool int
#define true 1
#define false 0
#define MToKB(mb) (mb*1024)
#define MToB(mb) (mb*1024*1024)

#define STR_MAXLEN 256
#define FILE_MAXLEN 256
#define MAX_PSSG_FILES 50
#define BLOG_FILE_LEN 2048
#define GIT_IGN_MAXLEN 32

#define ALLOCSIZE MToB(64)

char *Buffer;
char *allocp;
struct ProgState{
  char *configPath; // .ppsg files path
  LookupTable *table; // pssg files store in a basic lookup table
};

// Utility Functions
/**
 * @brief Basic Stack Alloc impl
 * 
 * @param ptr pointer to allocate space to
 * @param reqSz size we need to allocate
 */
void *p_stalloc_ch(int reqSz) {
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
void p_stfree_ch(void *ptr) {
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
  int fdLen = strlen(wfileDir) -1;
  // windows requires dir path have a trailing slash
  // we add that automatically, as users can conventionally add both
  bool dirPadding = wfileDir[fdLen] == '/' || wfileDir[fdLen] == '\\' ? false : true;
  dirPadding ? strcat(wfileDir, (char *)("/*")):strcat(wfileDir, (char *)("*"));
  
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
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        printf("%s   <DIR>\n", ffd.cFileName);
        char *tempDir = p_stalloc_ch(STR_MAXLEN);
        strcpy(tempDir, fileDir);
        if (dirPadding) strcat(tempDir,"/");
        strcat(tempDir, ffd.cFileName);
        SearchInPath(tempDir, ignoreFile, numl, state);
        p_stfree_ch(tempDir);
        continue;
    }
    filesize.LowPart = ffd.nFileSizeLow;
    filesize.HighPart = ffd.nFileSizeHigh;
    if (!p_substrcmp(ffd.cFileName,".html")) { // file is not html
      continue;
    }
    if (p_substrcmp(fileDir,".pssg")) {
      // load into state LookupTable

    }
    printf("|  %s   %lld bytes\n", ffd.cFileName, filesize.QuadPart);
  }
  while (FindNextFile(hFind, &ffd) != 0);
  dwError = GetLastError();
  p_stfree_ch(wfileDir);
  FindClose(hFind);
}


void SearchAndReplace(char *fileDir) {
  // @Mem Memory init
  Buffer = (char *) malloc (ALLOCSIZE);
  allocp = Buffer;
  // @MemEnd
  struct ProgState state;
  state.configPath = p_stalloc_ch(STR_MAXLEN);
  char *fileReadBuff = p_stalloc_ch(STR_MAXLEN*GIT_IGN_MAXLEN);
  // Memory allocation size explanation 
  //          key     +    max(str)*max(lines)
  const int kvSz = STR_MAXLEN + STR_MAXLEN*FILE_MAXLEN;
  //             KV_Pair Size*max(files)      + count
  KV_Pair *buckets = p_stalloc_ch(kvSz*MAX_PSSG_FILES);
  LookupTable table = {buckets, 0};
  // const int tableSz = (kvSz*MAX_PSSG_FILES) + 32;
  state.table = &table; // 3.13~ mb

  LookupPush(state.table, kvSz, "testKey", "This is a test value");
  KV_Pair *testRes = LookupGet(state.table, "testKey");
  LookupPop(state.table, kvSz);
  // @debug
  free(Buffer);
  return;
  // @debugEnd

  char *ignorefpath = p_stalloc_ch(STR_MAXLEN);
  strcpy(ignorefpath, fileDir);
  strcat(ignorefpath,"/.gitignore");
  printf("%s \n",ignorefpath);
  int numl = readFile(ignorefpath, fileReadBuff, "r", STR_MAXLEN);
  p_stfree_ch(ignorefpath);
  SearchInPath(fileDir, fileReadBuff, numl, state);
  p_stfree_ch(fileReadBuff);
  p_stfree_ch(state.configPath);
  free(Buffer);
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
          printf("use: pssg.exe -[arguments]\n");
          printf("arguments\t\tDescription\n");
          printf("-e {$project folder}\tExecute Custom html replacement operation. This will find any and all custom elements\n\
                \tin your static site that are marked as follows:\n\
                \t`.obj\n\
                \t<Navbar />`\n\
                \tThe Program then searches in a .pssg\\objects folder in your root directory.\n\
                \tIf it finds a Navbar.html file, it will replace all such entries in your static site html\n\
                \tWith this custom html");
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