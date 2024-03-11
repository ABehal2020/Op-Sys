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

// reading in the file
int readFile(char* buffer, FILE* fp, int* pipefd, int numWords) {
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
                    *(word + endIndex - startIndex) = '\0';
                    // write data to the pipe
                    int bytes_written = write(*(pipefd + 1), word, strlen(word));
                    int bytes_written2 = write(*(pipefd + 1), ".", 1);
                    if (bytes_written == -1 || bytes_written2 == -1) {
                        perror("ERROR");
                        return EXIT_FAILURE;
                    }
                    memset(word, '\0', 128);
                    numWords++;
                }
                startIndex = -1;
                endIndex = -1;
            }
        }
    }
    
    free(word);
    
    return EXIT_SUCCESS;
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
    
    // restructure with block waitpid function like prof did in monday's lecture
    // also close out both parent read and write file descriptors after all child processes have been invoked
    // also have one for loop calling fork() and second for loop calling waitpid() to achieve true parallelism
    // see the following for more information: https://submitty.cs.rpi.edu/courses/u22/csci4210/forum/threads/59
    for (int i = 0; i < argc; i++) {
        if (i == 0) {
            printf("PARENT: Calling fork() to create child process to execute hw2-cache.out...\n");
            pid_t p = fork();
            
            if (p == -1) {
                perror("ERROR");
                return EXIT_FAILURE;
            }
            
            if (p == 0) {
                // child process executing hw2-cache.out
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
                return EXIT_SUCCESS;
            } else {
                // parent process
                /*
                 PARENT CODE GOES HERE
                 */
                // read data from pipe
                char* buffer = calloc(128, sizeof(char));
                int bytes_read = -2;
                int bytes_written = -2;
                do {
                    bytes_read = read(*(pipefd + 0), buffer, strlen(buffer));
                    bytes_written = write(*(pipefd + 1), buffer, strlen(buffer));
                } while (bytes_read > 0 && bytes_written > 0);
                if (bytes_read == -1 || bytes_written == -1) {
                    perror("ERROR");
                    return EXIT_FAILURE;
                }
                int status;
                waitpid(p, &status, 0);
                if (WIFSIGNALED(status)) {
                    printf("PARENT: Child running hw2-cache.out terminated abnormally\n");
                } else if (WIFEXITED(status)) {
                    int exit_status = WEXITSTATUS(status);
                    printf("PARENT: Child process terminated with exit status %d\n", exit_status);
                }
            }
        } else {
            printf("PARENT: Calling fork() to create child process for \"%s\" file...\n", *(argv + i));
            pid_t p = fork();
            
            if (p == -1) {
                perror("ERROR");
                return EXIT_FAILURE;
            }
            
            if (p == 0) {
                // child process parsing through input text file
                // close read end of the child pipe
                close(*(pipefd + 0));
                /*
                 CHILD CODE GOES HERE
                 */
                // dynamically allocating memory for buffer and hashTable
                char* filename = *(argv + i);
                char* buffer = calloc(128, sizeof(char));
                
                // opening file for reading
                FILE* fp = fopen(filename, "r");
                if (fp == NULL) {
                    perror("ERROR");
                    return 2;
                }
                int numWords = 0;
                int returnValue = readFile(buffer, fp, pipefd, numWords);
                if (returnValue == EXIT_FAILURE) {
                    return EXIT_FAILURE;
                }
                if (numWords == 0) {
                    return 3;
                }
                printf("CHILD: Successfully wrote %d words on the pipe\n", numWords);
                
                // close write end of the child pipe
                close(*(pipefd + 1));
                
                free(pipefd);
                free(buffer);
            } else {
                // parent process
                /*
                 PARENT CODE GOES HERE
                 */
                int status;
                waitpid(p, &status, 0);
                if (WIFSIGNALED(status)) {
                    printf("PARENT: Child process terminated abnormally\n");
                } else if (WIFEXITED(status)) {
                    int exit_status = WEXITSTATUS(status);
                    printf("PARENT: Child process terminated with exit status %d\n", exit_status);
                }
            }
        }
    }
    
    // close read end of the parent pipe
    close(*(pipefd + 0));
    // close write end of the parent pipe
    close(*(pipefd + 1));
    free(pipefd);
    
    return EXIT_SUCCESS;
}


