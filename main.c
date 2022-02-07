#include <windows.h>
#include <stdio.h>
#define bool int
#define true 1
#define false 0
#define MToKB(mb) (mb*1024)
#define MToB(mb) (mb*1024*1024)
#define STR_MAXLEN 50
#define FILE_MAXLEN 50

#define ALLOCSIZE MToB(10)
char Buffer[ALLOCSIZE];
char *allocp = Buffer;
struct ProgState{
  char *configPath; // .ppsg files path
  char *ignorefile; // .gitignore file
};

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

bool p_strcmpMulti(char *given, char *multi, int numl) {
  bool res = false; 
  while (numl-->0 && *multi != '\0') {
    res = res | p_strcmp(multi,given);
    if (res) return res;
    multi+=STR_MAXLEN;
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

/**
 * @brief Reads a file from a file path into a file buffer 
 * 
 * @param fpath filepath
 * @param fbuff file buff (with fixed row length)
 * @return int number of lines read
 */
int readFile(char *fpath, char *fbuff) {
  FILE *file;
  LARGE_INTEGER fsize;
  file = fopen(fpath, "r");
  long int size = p_filesz(file);
  char *res;
  int numl = 0;
  while(fgets(fbuff,size,file)) {
    p_replaceChar(fbuff,'\n','\0');
    fbuff+=STR_MAXLEN;
    numl++;
  };
  fclose(file);
  return numl;
}

/**
 * @brief Naive Stack Alloc impl
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
 * @brief Naive Stack Free impl
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
    // win32 for some reason, retunrs a plain name, without directory `\`
    // so we concat the directory flag to it (for simplicity)
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) strcat(ffd.cFileName,"/");
    // ..checking entries with ignore file
    if (p_strcmpMulti(ffd.cFileName,ignoreFile, numl)) continue;
    if (p_strcmp(ffd.cFileName,".pssg/")) {
      strcpy(state.configPath, fileDir);
      strcat(state.configPath,"/.pssg/");
      continue; // pssg folder 
    }
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
  p_stfree_ch(wfileDir);
  FindClose(hFind);
}

void SearchAndReplace(char *fileDir) {
  struct ProgState state;
  state.configPath = p_stalloc_ch(STR_MAXLEN); 
  char *fileReadBuff = p_stalloc_ch(STR_MAXLEN*FILE_MAXLEN);
  char *ignorefpath = p_stalloc_ch(STR_MAXLEN);
  strcpy(ignorefpath, fileDir);
  strcat(ignorefpath,"\\.gitignore");
  int numl = readFile(ignorefpath, fileReadBuff);
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