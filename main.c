#include <windows.h>
#include <stdio.h>
# define bool int
# define true 1
# define false 0

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
  while (given[i] != '\0' && toMatch[i] != '\0') 
  {
    if (given[i] != toMatch[i]) return false;
    i++;
  }
  return given[i] != toMatch[i] ? false : true; 
}

/**
 * @brief Compares the string with each line of a given file
 * 
 * @param given variable string
 * @param filePath path to file to open
 * @return true 
 * @return false 
 */
bool p_strcmpfile(char *given, char *filePath);


void SearchInPath(char *fileDir, char *ignoreFile[]) {
  WIN32_FIND_DATA ffd;
  HANDLE hFind;
  LARGE_INTEGER filesize;
  DWORD dwError=0;
  hFind = FindFirstFile(fileDir, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) 
  {
    printf ("FindFirstFile failed (%d)\n", GetLastError());
    return;
  }
  do
  {
    if ((p_strcmp(ffd.cFileName,".") || p_strcmp(ffd.cFileName, ".."))
    /* && p_strcmpfile */) continue;
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

void LoadIgnoreFile(char *fileDir) {

}

void SearchAndReplace(char *fileDir) {
  char *ignoreFile; 
  LoadIgnoreFile(fileDir, ignoreFile); 
  SearchInPath(fileDir, ignoreFile);
  // ReplaceInPath
}

int main( int argc, char *argv[]) {
  while(--argc > 0 && (*++argv)[0] == '-') {
    char c = *++argv[0];
    switch (c) {
      case 'x': {
        SearchAndReplace((++argv)[0]);
        break;
      }
      case 'h': {
        printf("PSSG: The Bloat free, static site generator (helper)\n");
        printf("use: main.exe [arguments]\n");
        printf("arguments             Description\n");
        printf("x {$project folder}   Execute Custom html replacement operation. This will find any and all custom elements\n\
                    in your static site that are marked as follows:\n\
                    `.obj\n\
                    Navbar`\n\
                    The Program then searches in a .pssg\\objects folder in your root directory.\n\
                    If it finds a Navbar.html file, it will replace all such entries in your static site html\n\
                    With this custom html");
        break;
      }
      default: {
        printf("Unknown option, please use `main.exe -h` for help");
        break;
      }
    }
  }
  return 1;
}