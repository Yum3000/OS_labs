//last update 21-03-24 
// unarchive programm
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>

char *archiveName = "data.yar";

/* Normalize the given path */

char *path_normalize(const char *path) {
  if (!path) return NULL;

  char *copy = strdup(path);
  if (NULL == copy) return NULL;
  char *ptr = copy;

  for (int i = 0; copy[i]; i++) {
    *ptr++ = path[i];
    if ('/' == path[i]) {
      i++;
      while ('/' == path[i]) i++;
      i--;
    }
  }

  *ptr = '\0';

  return copy;
}

char *strdup(const char *str) {
  if (NULL == (char *) str) {
    return NULL;
  }

  int len = strlen(str) + 1;
  char *buf = malloc(len);

  if (buf) {
    memset(buf, 0, len);
    memcpy(buf, str, len - 1);
  }
  return buf;
}


struct headerEntry {
  off_t size;
  bool isDir;
  int fileNameSize;
  long int dataPosition;
};


char *concatPath(const char *s1, const char *s2) {
  // +1 for the null-terminator
  size_t firstLen = strlen(s1);
  size_t finalLen = firstLen + strlen(s2) + 1;
  bool needSlash = s1[firstLen-1] != '/';

  finalLen += needSlash ? 1 : 0;

  char *result = malloc(finalLen);

  strcpy(result, s1);

  if (needSlash) {
    strcat(result, "/");
  }

  strcat(result, s2);

  return result;
}

int readHeader(char *path, FILE *archive, long int descriptor) {


  //struct dirent *entry;
  //DIR *dp;
  //struct stat *fileAtr = malloc(sizeof(struct stat));

  //dp = opendir(path);
  //if (dp == NULL) {
  //  perror("opendir");
  //  return -1;
  //}

  //char *dot = concatPath(path, ".");
  //char *dotdot = concatPath(path, "..");
  //char *banned = concatPath(path, archiveName);

  long int currentHeaderReadPosition = descriptor;

  struct headerEntry firstEntry;
  fread(&firstEntry, sizeof(struct headerEntry), 1, archive);

  long int dataWritePosition = firstEntry.dataPosition;

  char *firstFileName = malloc(firstEntry.fileNameSize + 1);
      
  fread(firstFileName, firstEntry.fileNameSize, 1, archive);
  firstFileName[firstEntry.fileNameSize] = '\0';

  mode_t mode = 0777;
  char absolutePath[PATH_MAX];
  
  if (strcmp(path, ".") != 0) {
    path = concatPath(path, firstFileName);
    printf("%s\n", path);
    char *normPath = path_normalize(path);
    //char *rpath = malloc(50);
    //realpath("./mew/./test_directory/", rpath);
    printf("%s\n", normPath);
    //printf("%s\n", path);
    //printf("%s\n", rpath);
    //printf("%s\n", realpath(path, absolutePath));
    if (realpath(path, absolutePath) != NULL) {
      path = realpath(path, absolutePath);
    } else {
        puts("Paths error!");
        return -1;
      }
  }

  if (firstEntry.isDir) {
    int er = mkdir(path, mode);
    if (er == -1) {
      perror("error description:");
      return -1;
    }
    printf("%s %d\n", firstFileName, er); 
  } else {
    FILE *firstFile = fopen(path, "wb");
    //fseek(archive, dataWritePosition, SEEK_SET);
    //fwrite(archive, firstEntry.size, 1, firstFile);
    //dataWritePosition = ftell(archive);
    char *text = "HELLO WORLD 1";
    fwrite(text, 15, 1, firstFile);
    fclose(firstFile);
  }

  printf("name: %s size: %ld type: %s position: %ld\n", firstFileName, firstEntry.size, firstEntry.isDir ? "d" : "f", firstEntry.dataPosition);
  currentHeaderReadPosition = ftell(archive);
  printf("%ld\n", currentHeaderReadPosition);

  while (currentHeaderReadPosition < dataWritePosition) {
    fseek(archive, currentHeaderReadPosition, SEEK_SET);

    struct headerEntry newEntry;

    fread(&newEntry, sizeof(struct headerEntry), 1, archive);

    char *fileName = malloc(newEntry.fileNameSize + 1);
      
    fread(fileName, newEntry.fileNameSize, 1, archive);
    fileName[newEntry.fileNameSize] = '\0';
    
    currentHeaderReadPosition = ftell(archive);
    
    char *currentPath = concatPath(path, fileName);
    printf("%s\n", currentPath);

    if (newEntry.isDir) {
      currentPath = realpath(currentPath, absolutePath);
      mkdir(currentPath, mode);
      perror("DIR description:");
    } else {
    
      FILE *entryFile = fopen(currentPath, "wb");
      perror("FILE description:");
      fseek(archive, newEntry.dataPosition, SEEK_SET);
      
      char ch;
      for(int i = newEntry.size; i>0; i--) {
        ch = fgetc(archive);
        fputc(ch, entryFile);
      }

      fclose(entryFile);
    }

    printf("name: %s size: %ld type: %s position: %ld\n", fileName, newEntry.size, newEntry.isDir ? "d" : "f", newEntry.dataPosition);

    free(fileName);
  }


  free(firstFileName);
  return 0;
}

int main(int argc, char **argv) {
  int entriesCount = 0;
  if (strcmp(argv[1],archiveName)!=0) {
    printf("2 parametr %s\n", argv[1]);
    puts("Archive name error!");
    return -1;
  }
  char *archiveName = argv[1];

  FILE *unarchive = fopen(archiveName, "r");
  long int startPos = ftell(unarchive);

  if (argc == 2) {
    readHeader(".", unarchive, startPos);
  }

  if (argc == 3) {
    printf("\nUnpacking to %s ...\n", argv[2]);
    int readingError = readHeader(argv[2], unarchive, startPos);
    if (readingError == -1) {
      return -1;
    }
  }

  fclose(unarchive);

  return 0;
}
