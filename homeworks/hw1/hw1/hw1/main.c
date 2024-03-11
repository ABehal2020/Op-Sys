//
//  main.c
//  hw1
//
//  Created by Aditya Behal on 6/6/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// hashing the word by summing up the characters' ASCII values and then taking the mod over the cacheSize
int hash(char* word, int cacheSize) {
    int index = 0;
    for (unsigned int i = 0; i < strlen(word); i++) {
        index += (int)(*(word + i));
    }
    index %= cacheSize;
    return index;
}

// reading in the file
void readFile(char* buffer, FILE* fp, int cacheSize, char** hashTable) {
    char** ptr = hashTable;
    char* word = calloc(128, sizeof(char));
    
    // read words into buffer
    while (fgets(buffer, 128, fp)) {
        int startIndex = -1;
        int endIndex = -1;
        for (unsigned int i = 0; i < strlen(buffer); i++) {
            // processing real words (they have at least 2 alphanumeric characters and may only have alphanumeric characters)
            if (isalnum(*(buffer + i))) {
                if (startIndex == -1) {
                    startIndex = i;
                } else {
                    endIndex = i + 1;
                }
            } else {
                // putting real words in hash table
                if (endIndex - startIndex >= 2) {
                    strncpy(word, buffer + startIndex, endIndex - startIndex);
                    *(word + endIndex - startIndex) = '\0';
                    int index = hash(word, cacheSize);
                    if (*(hashTable + index) == NULL) {
                        *(hashTable + index) = calloc(strlen(word) + 1, sizeof(char));
                        strcpy(*(hashTable + index), word);
                        printf("Word \"%s\" ==> %d (calloc)\n", *(hashTable + index), index);
                    } else {
                        if (strlen(word) == strlen(*(hashTable + index))) {
                            strcpy(*(hashTable + index), word);
                            printf("Word \"%s\" ==> %d (nop)\n", *(hashTable + index), index);
                        } else {
                            *(hashTable + index) = realloc(*(hashTable + index), strlen(word) + 1);
                            strcpy(*(hashTable + index), word);
                            printf("Word \"%s\" ==> %d (realloc)\n", *(hashTable + index), index);
                        }
                    }
                    memset(word, '\0', 128);
                }
                startIndex = -1;
                endIndex = -1;
            }
        }
    }
    
    // outputting cache values
    for (int i = 0; i < cacheSize; i++) {
        if (*(ptr + i) != NULL) {
            printf("Cache index %d ==> \"%s\"\n", i, *(ptr + i));
        }
    }
    
    free(word);
}

int main(int argc, char** argv) {
    // no buffering for stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    
    // error checking for incorrect command line arguments
    if (argc != 3) {
        fprintf(stderr, "ERROR: Invalid arguments\n");
        fprintf(stderr, "USAGE: a.out <x> <filename>\n");
        return EXIT_FAILURE;
    }

    int cacheSize = atoi(*(argv + 1));
    
    if (!isdigit(*(*(argv + 1)))) {
        fprintf(stderr, "ERROR: Cache size must be a valid integer");
        return EXIT_FAILURE;
    }
    
    // dynamically allocating memory for buffer and hashTable
    char* filename = *(argv + 2);
    char* buffer = calloc(128, sizeof(char));
    char** hashTable = calloc(cacheSize, sizeof(char*));
    char** ptr = hashTable;
    
    // opening file for reading
    FILE* fp = fopen(filename, "r");
    
    if (!fp) {
        perror("ERROR");
        return EXIT_FAILURE;
    }
    
    readFile(buffer, fp, cacheSize, hashTable);
    
    // closing file and deallocating dynamically allocated memory
    fclose(fp);
    
    free(buffer);
    
    for (int i = 0; i < cacheSize; i++) {
        if (*(ptr + i) != NULL) {
            free(*(ptr + i));
        }
    }
    
    free(ptr);
    
    return EXIT_SUCCESS;
}
