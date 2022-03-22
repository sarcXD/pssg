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
#define BUCKET_SZ (STR_MAXLEN + STR_MAXLEN*FILE_MAXLEN)
#define ALLOCSIZE MToB(64)

char *Buffer;
char *allocp;
struct ProgState{
  char *configPath; // .ppsg files path
  char *rootDir; // root directory
  char *buildDir; // build dir
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

void concatFNameToDir(char *tempDir, char *fileDir, char *fName, bool pad) {
  strcpy(tempDir, fileDir);
  if (pad) strcat(tempDir, "/");
  strcat(tempDir, fName);
}

void SearchInPath(char *fileDir, char *ignoreFile, int numl, struct ProgState state, char *buildDir) {
  WIN32_FIND_DATA ffd;
  HANDLE hFind;
  LARGE_INTEGER filesize;
  DWORD dwError=0;
  
  // 1. update the fileDir with /*
  char *tempDir=p_stalloc_ch(STR_MAXLEN);
  strcpy(tempDir,fileDir);
  int fdLen = strlen(tempDir) -1;
  // windows requires dir path have a trailing slash
  // we add that automatically, as users can conventionally add both
  bool dirPadding = tempDir[fdLen] == '/' || tempDir[fdLen] == '\\' ? false : true;
  dirPadding ? strcat(tempDir, (char *)("/*")):strcat(tempDir, (char *)("*"));
  
  // 2. Read file
  hFind = FindFirstFile(tempDir, &ffd);
  p_stfree_ch(tempDir);
  if (hFind == INVALID_HANDLE_VALUE) 
  {
    printf ("FindFirstFile failed (%d)\n", GetLastError());
    return;
  }
  bool root = p_strcmp(fileDir, state.rootDir);
  // CreateBuildFolder() or overwrite
  if (root) {
    char *tempDir = p_stalloc_ch(STR_MAXLEN);
    concatFNameToDir(tempDir, fileDir, "build", dirPadding);
    CreateDirectoryA(tempDir, NULL);
    strcpy(buildDir, tempDir);
    p_stfree_ch(tempDir);
  }
  do
  {
    // .,.. are os specific and will cause inf loops
    if (p_strcmp(ffd.cFileName,".") || p_strcmp(ffd.cFileName, "..")) continue;
    // ..checking entries with ignore file
    if (p_substrcmpMulti(ignoreFile, ffd.cFileName, numl, STR_MAXLEN)) continue;
    // checking if build dir (ignore if)
    if (p_strcmp(ffd.cFileName, "build")) continue;
    // checking for pssg dir
    if (p_substrcmp(fileDir, ".pssg")) {
        // load into state LookupTable
        // these pointers are entries into the lookup table
        // they will not be cleared
        char* key = p_stalloc_ch(STR_MAXLEN);
        char* value = p_stalloc_ch(STR_MAXLEN * FILE_MAXLEN);

        // prepare the new appended file path
        char* tempDir = p_stalloc_ch(STR_MAXLEN);
        concatFNameToDir(tempDir, fileDir, ffd.cFileName, dirPadding);

        // read and add to lookupTable
        p_readFile(tempDir, value, STR_MAXLEN);
        p_substrFiltered(ffd.cFileName, ".html", key);
        p_LookupPush(state.table, BUCKET_SZ, key, value);

        p_stfree_ch(tempDir);
        continue;
    }
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        printf("%s   <DIR>\n", ffd.cFileName);
        char *tempDir = p_stalloc_ch(STR_MAXLEN);
        concatFNameToDir(tempDir, fileDir, ffd.cFileName, dirPadding);
        
        // createFolderInBuildDir()
        char *subBuildDir = p_stalloc_ch(STR_MAXLEN);
        if (!p_strcmp(ffd.cFileName, ".pssg"))
        {
          concatFNameToDir(subBuildDir, buildDir, ffd.cFileName, dirPadding);
          CreateDirectoryA(subBuildDir, NULL);
        }
        SearchInPath(tempDir, ignoreFile, numl, state, subBuildDir);

        p_stfree_ch(subBuildDir);
        p_stfree_ch(tempDir);
        continue;
    }
    filesize.LowPart = ffd.nFileSizeLow;
    filesize.HighPart = ffd.nFileSizeHigh;
    printf("|  %s   %lld bytes\n", ffd.cFileName, filesize.QuadPart);
    // prepare the new appended file path
    char *tempDir = p_stalloc_ch(STR_MAXLEN);
    concatFNameToDir(tempDir, fileDir, ffd.cFileName, dirPadding);
    char *fileBuff = p_stalloc_ch(STR_MAXLEN*FILE_MAXLEN);
    int flen = p_readFile(tempDir, fileBuff, STR_MAXLEN);
    if (p_substrcmp(ffd.cFileName,".html")) 
    { // file is html
      bool found = p_substrcmpMulti(fileBuff, ".obj", flen, STR_MAXLEN);
      if (found) 
      {
        // replaceComponentCallInFile()
        printf("found it");
      }  
    }
    // writeFileInBuildDir()
    char *subBuildDir = p_stalloc_ch(STR_MAXLEN);
    concatFNameToDir(subBuildDir, buildDir, ffd.cFileName, dirPadding);
    p_writeFile(subBuildDir, fileBuff, flen, STR_MAXLEN);
    p_stfree_ch(subBuildDir);
    p_stfree_ch(fileBuff);
    p_stfree_ch(tempDir);
    
  }
  while (FindNextFile(hFind, &ffd) != 0);
  dwError = GetLastError();
  FindClose(hFind);
}


void SearchAndReplace(char *fileDir) {
  // @Mem Memory init
  Buffer = (char *) malloc (ALLOCSIZE);
  allocp = Buffer;
  // @MemEnd
  struct ProgState state;
  state.configPath = p_stalloc_ch(STR_MAXLEN);
  state.rootDir = p_stalloc_ch(STR_MAXLEN);
  strcpy(state.rootDir, fileDir);
  char *fileReadBuff = p_stalloc_ch(STR_MAXLEN*GIT_IGN_MAXLEN);
  // Memory allocation size explanation 
  //          key     +    max(str)*max(lines)
  //             KV_Pair Size*max(files)      + count
  KV_Pair *buckets = p_stalloc_ch(BUCKET_SZ*MAX_PSSG_FILES);
  LookupTable table = {buckets, 0};
  // const int tableSz = (kvSz*MAX_PSSG_FILES) + 32;
  state.table = &table; // 3.13~ mb

  char *ignorefpath = p_stalloc_ch(STR_MAXLEN);
  strcpy(ignorefpath, fileDir);
  strcat(ignorefpath,"/.gitignore");
  printf("%s \n",ignorefpath);
  int numl = p_readFile(ignorefpath, fileReadBuff, STR_MAXLEN);
  p_stfree_ch(ignorefpath);
  
  char *buildDir = p_stalloc_ch(STR_MAXLEN);
  SearchInPath(fileDir, fileReadBuff, numl, state, buildDir);
  p_stfree_ch(buildDir);
  p_stfree_ch(fileReadBuff);
  p_stfree_ch(state.rootDir);
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