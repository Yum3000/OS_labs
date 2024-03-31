//last update 28-03-24 
// archive programm
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

char *archiveName = "data.yar";

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

int writeHeader(char *path, FILE *archive, int *entriesCount) {
  char *currentArchivePath = concatPath(".", archiveName);

  if (!strcmp(path, currentArchivePath)) {
    return 0;
  }

  struct stat *fileAtr = malloc(sizeof(struct stat));
  if (stat(path, fileAtr) == -1 ) return 0;

  bool isDir = S_ISDIR(fileAtr->st_mode);

  struct headerEntry element;
  element.isDir = isDir;  
  element.fileNameSize = strlen(path);

// записываем заголовок о файле
  if (!isDir) {
    element.size = fileAtr->st_size;

    *entriesCount += 1;

    printf("file %ld - %ld %s\n", fileAtr->st_size, strlen(path), path);
    fwrite(&element, sizeof(struct headerEntry), 1, archive);
    fputs(path, archive);

    return 0;
  }

// записываем заголовок о папке, если это не . верхнего уровня
  if (strcmp(path, ".")) {

    element.size = 0;

    *entriesCount += 1;

    printf("dir  %ld - %s\n", fileAtr->st_size, path);
    fwrite(&element, sizeof(struct headerEntry), 1, archive);
    fputs(path, archive);
  }

  DIR *dp = opendir(path);
  if (dp == NULL) {
    perror("opendir");
    return -1;
  }

  char *dot = concatPath(path, ".");
  char *dotdot = concatPath(path, "..");

  struct dirent *entry;

  while ((entry = readdir(dp))) {
    char *adr = concatPath(path, entry->d_name);
 
    if (!strcmp(adr, dot) || !strcmp(adr, dotdot)) {
      continue;
    }

    writeHeader(adr, archive, entriesCount);
  }

  free(fileAtr);
  closedir(dp);
  return 0;
}

int main(int argc, char **argv) {

  int entriesCount = 0;

  FILE *archive = fopen(archiveName, "w+");
  long int startPos = ftell(archive);

  if (argc == 1) {
    writeHeader(".", archive, &entriesCount);
  }

  if (argc == 2) {
    printf("\nListing %s...\n", argv[1]);
    writeHeader(argv[1], archive, &entriesCount);
  }

  long int posAfterHeader = ftell(archive);

  printf("ENTRIES COUNT: %d\n", entriesCount);

  long int currentDataWritePosition = posAfterHeader;
  long int currentHeaderReadPosition = startPos;
  
  for (int i = 0; i < entriesCount; i++) {
    fseek(archive, currentHeaderReadPosition, SEEK_SET);

    struct headerEntry newEntry;
    fread(&newEntry, sizeof(struct headerEntry), 1, archive);
    
    printf("%ld %d %d\n", newEntry.size, newEntry.isDir, newEntry.fileNameSize);

    newEntry.dataPosition = currentDataWritePosition;

    fseek(archive, currentHeaderReadPosition, SEEK_SET);
    fwrite(&newEntry, sizeof(struct headerEntry), 1, archive);

    char *fileName = malloc(newEntry.fileNameSize + 1);
      
    fread(fileName, newEntry.fileNameSize, 1, archive);
    fileName[newEntry.fileNameSize] = '\0';

    printf("name: %s size: %ld type: %s position: %ld\n", fileName, newEntry.size, newEntry.isDir ? "d" : "f", newEntry.dataPosition);
    currentHeaderReadPosition = ftell(archive);

    fseek(archive, currentDataWritePosition, SEEK_SET);

    if (!newEntry.isDir) {
      FILE *file = fopen(fileName, "r");
      puts("trying write file...");
      char ch = fgetc(file);
      puts(&ch);
      for (int i = 0; i < newEntry.size; i++ ) {
        fputc(ch, archive);
        ch = fgetc(file);
      }
      fclose(file);
    }

    currentDataWritePosition = ftell(archive);

    free(fileName);
  }

  fclose(archive);

  return 0;
}
