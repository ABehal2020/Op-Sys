//
//  main.c
//  hw2
//
//  Created by Aditya Behal on 6/20/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

void childProcessCache(int* pipefd, int* childProcessPIDs);
int childProcessParse(int* pipefd, char* filename, int* childProcessPIDs);
int readFile(char* buffer, FILE* fp, int* pipefd, int* numWords);

// child process executing hw2-cache.out
void childProcessCache(int* pipefd, int* childProcessPIDs) {
    // close write end of the child pipe
    close(*(pipefd + 1));
    /*
     CHILD CODE GOES HERE
     */
    // how to call execl and cast to char * appropriately: https://submitty.cs.rpi.edu/courses/u22/csci4210/forum/threads/61
    char* pipefdRead = calloc(128, sizeof(char));
    sprintf(pipefdRead, "%d", *(pipefd + 0));
    execl("hw2-cache.out", "./hw2.cache.out", pipefdRead, NULL);
    // close read end of the child pipe
    close(*(pipefd + 0));
    free(pipefdRead);
    free(pipefd);
    free(childProcessPIDs);
}

// child process parsing through input text file
int childProcessParse(int* pipefd, char* filename, int* childProcessPIDs) {
    // close read end of the child pipe
    close(*(pipefd + 0));
    /*
     CHILD CODE GOES HERE
     */
    // dynamically allocating memory for buffer
    char* buffer = calloc(128, sizeof(char));
    
    // opening file for reading
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("ERROR");
        return 2;
    }
    
    int* numWords = calloc(1, sizeof(int));
    *numWords = 0;
    
    int returnValue = readFile(buffer, fp, pipefd, numWords);
    if (returnValue == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    printf("CHILD: Successfully wrote %d words on the pipe\n", *numWords);
    if (*numWords == 0) {
        return 3;
    }
    
    // close write end of the child pipe
    close(*(pipefd + 1));
    
    free(pipefd);
    free(buffer);
    free(childProcessPIDs);
    free(numWords);
    
    return EXIT_SUCCESS;
}

// reading in the file
int readFile(char* buffer, FILE* fp, int* pipefd, int* numWords) {
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
                if (endIndex - startIndex >= 2) {
                    strncpy(word, buffer + startIndex, endIndex - startIndex);
                    // *(word + endIndex - startIndex) = '\0';
                    // write data to the pipe
                    int bytes_written = write(*(pipefd + 1), word, strlen(word));
                    int bytes_written2 = write(*(pipefd + 1), ".", 1);
                    if (bytes_written == -1 || bytes_written2 == -1) {
                        perror("ERROR");
                        return EXIT_FAILURE;
                    }
                    // printf("Word: %s\n", word);
                    memset(word, '\0', 128);
                    (*numWords)++;
                }
                startIndex = -1;
                endIndex = -1;
            }
        }
    }
    
    // printf("Word count: %d\n", *numWords);
    
    free(word);
    
    return EXIT_SUCCESS;
}

void block_on_waitpid(pid_t p, pid_t* childProcessPIDs) {
    int status;
    waitpid(p, &status, 0);
    if (WIFSIGNALED(status)) {
        if (p == *(childProcessPIDs + 0)) {
            printf("PARENT: Child running hw2-cache.out terminated abnormally\n");
        } else {
            printf("PARENT: Child process terminated abnormally\n");
        }
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (p == *(childProcessPIDs + 0)) {
            printf("PARENT: Child running hw2-cache.out terminated with exit status %d\n", exit_status);
        } else {
            printf("PARENT: Child process terminated with exit status %d\n", exit_status);
        }
    }
}


int main(int argc, char** argv) {
    // no buffering for stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    
    int* pipefd = calloc(2, sizeof(int));
    int rc = pipe(pipefd);
    
    if (rc == -1) {
        perror("ERROR");
        return EXIT_FAILURE;
    }
    
    printf("PARENT: Created pipe successfully\n");
    
    pid_t* childProcessPIDs = calloc(argc, sizeof(pid_t));
    
    for (int i = 0; i < argc; i++) {
        if (i == 0) {
            printf("PARENT: Calling fork() to create child process to execute hw2-cache.out...\n");
            pid_t p = fork();
            
            if (p == -1) {
                perror("ERROR");
                return EXIT_FAILURE;
            }
            
            if (p == 0) {
                // child process calling cache executable
                childProcessCache(pipefd, childProcessPIDs);
                return EXIT_SUCCESS;
            } else {
                *(childProcessPIDs + i) = p;
            }
        } else {
            printf("PARENT: Calling fork() to create child process for \"%s\" file...\n", *(argv + i));
            pid_t p = fork();
            
            if (p == -1) {
                perror("ERROR");
                return EXIT_FAILURE;
            }
            
            if (p == 0) {
                char* filename = *(argv + i);
                return childProcessParse(pipefd, filename, childProcessPIDs);
            }
        }
    }
    
    // close read end of the parent pipe
    close(*(pipefd + 0));
    // close write end of the parent pipe
    close(*(pipefd + 1));
    
    for (int i = 1; i < argc; i++) {
        block_on_waitpid(*(childProcessPIDs + i), childProcessPIDs);
    }
    
    block_on_waitpid(*(childProcessPIDs + 0), childProcessPIDs);
    
    // close read end of the parent pipe
    close(*(pipefd + 0));
    // close write end of the parent pipe
    close(*(pipefd + 1));
    
    free(childProcessPIDs);
    free(pipefd);
    
    return EXIT_SUCCESS;
}


