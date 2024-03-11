//
//  hw3.c
//  hw3
//
//  Created by Aditya Behal on 7/26/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

extern int next_thread_id;
extern int max_squares;
extern int total_tours;

int m;
int n;
int r;
int c;

bool disable_parallelism = false;

typedef struct {
    int rowNum;
    int colNum;
} coordinates;

typedef struct {
    int threadID;
    int depth;
    int numValidMoves;
    coordinates* currentPosition;
    coordinates* validMoves;
    char** board;
} boardInformation;

pthread_mutex_t mutex_next_thread_id = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_max_squares = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_total_tours = PTHREAD_MUTEX_INITIALIZER;

void printBoardInformation(boardInformation* solver) {
    printf("Thread ID: %d\n", solver->threadID);
    printf("Depth (number of squares covered): %d\n", solver->depth);
    printf("Number of valid moves: %d\n", solver->numValidMoves);
    printf("Current position: (%d, %d)\n", solver->currentPosition->rowNum, solver->currentPosition->colNum);
    for (int i = 0; i < solver->numValidMoves; i++) {
        printf("Valid move #%d: (%d, %d)\n", i + 1, solver->validMoves[i].rowNum, solver->validMoves[i].colNum);
    }
    printf("Board:\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%c", solver->board[i][j]);
        }
        printf("\n");
    }
}

void generateValidMoves(boardInformation* solver) {
    solver->numValidMoves = 0;
    int validX;
    int validY;
    int offsetByTwo = 2;
    int offsetByOne = 1;
    // there are 8 possible moves - we'll check them all and keep all the legal ones
    for (int i = 0; i < 8; i++) {
        if (i == 0) {
            // first possible move
            validX = solver->currentPosition->rowNum - offsetByTwo;
            validY = solver->currentPosition->colNum - offsetByOne;
            // printf("First possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 1) {
            // second possible move
            validX = solver->currentPosition->rowNum - offsetByOne;
            validY = solver->currentPosition->colNum - offsetByTwo;
            // printf("Second possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 2) {
            // third possible move
            validX = solver->currentPosition->rowNum + offsetByOne;
            validY = solver->currentPosition->colNum - offsetByTwo;
            // printf("Third possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 3) {
            // fourth possible move
            validX = solver->currentPosition->rowNum + offsetByTwo;
            validY = solver->currentPosition->colNum - offsetByOne;
            // printf("Fourth possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 4) {
            // fifth possible move
            validX = solver->currentPosition->rowNum + offsetByTwo;
            validY = solver->currentPosition->colNum + offsetByOne;
            // printf("Fifth possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 5) {
            // sixth possible move
            validX = solver->currentPosition->rowNum + offsetByOne;
            validY = solver->currentPosition->colNum + offsetByTwo;
            // printf("Sixth possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 6) {
            // seventh possible move
            validX = solver->currentPosition->rowNum - offsetByOne;
            validY = solver->currentPosition->colNum + offsetByTwo;
            // printf("Seventh possible move: validX: %d, validY: %d\n", validX, validY);
        } else {
            // eighth possible move
            validX = solver->currentPosition->rowNum - offsetByTwo;
            validY = solver->currentPosition->colNum + offsetByOne;
            // printf("Eighth possible move: validX: %d, validY: %d\n", validX, validY);
        }
        // make sure move is valid and destination square of move hasn't been visited before
        if (validX >= 0 && validX < m && validY >= 0 && validY < n && solver->board[validX][validY] == '.') {
            (solver->validMoves[solver->numValidMoves]).rowNum = validX;
            (solver->validMoves[solver->numValidMoves]).colNum = validY;
            solver->numValidMoves++;
        }
    }
}

void* runSolver(void* args) {
    boardInformation* solver = (boardInformation*)args;
    
    int* retVal;
    int rc;
    int* rcExit;
    
    // printf("Character at board[currentR][currentC]: %c", solver->board[solver->currentPosition->rowNum][solver->currentPosition->colNum]);
    
    /*
    // update number of moves
    if (solver->board[solver->currentPosition->rowNum][solver->currentPosition->colNum] != 'S') {
        solver->depth = atoi(&solver->board[solver->currentPosition->rowNum][solver->currentPosition->colNum]);
    }
    */
    
    // checking if knight's tour is complete so thread can exit
    if (solver->depth == m * n) {
        pthread_mutex_lock(&mutex_max_squares);
        max_squares = solver->depth;
        pthread_mutex_unlock(&mutex_max_squares);
        pthread_mutex_lock(&mutex_next_thread_id);
        pthread_mutex_lock(&mutex_total_tours);
        total_tours++;
        printf("T%d: Sonny found a full knight's tour; incremented total_tours\n", solver->threadID);
        pthread_mutex_unlock(&mutex_total_tours);
        pthread_mutex_unlock(&mutex_next_thread_id);
        if (solver->threadID == 0) {
            return (void*)NULL;
        } else if (solver->threadID > 0) {
            pthread_exit(rcExit);
        }
    }
    
    generateValidMoves(solver);
    
    if (solver->numValidMoves == 0) {
        // we've reached a dead end
        pthread_mutex_lock(&mutex_next_thread_id);
        pthread_mutex_lock(&mutex_max_squares);
        
        if (solver->threadID == 0) {
            printf("MAIN: Dead end at move %d", solver->depth);
        } else {
            printf("T%d: Dead end at move %d", solver->threadID, solver->depth);
        }
        
        if (solver->depth > max_squares) {
            max_squares = solver->depth;
            printf("; updated max_squares\n");
        } else {
            printf("\n");
        }
        
        pthread_mutex_unlock(&mutex_max_squares);
        pthread_mutex_unlock(&mutex_next_thread_id);
        
        if (solver->threadID == 0) {
            return (void*)NULL;
        } else {
            pthread_exit(rcExit);
        }
    } else if (solver->numValidMoves == 1) {
        solver->currentPosition->rowNum = solver->validMoves[0].rowNum;
        solver->currentPosition->colNum = solver->validMoves[0].colNum;
        solver->depth = solver->depth + 1;
        solver->board[solver->currentPosition->rowNum][solver->currentPosition->colNum] = solver->depth - 1 + '0';
        printBoardInformation(solver);
        runSolver((void*)solver);
    } else {
        if (solver->threadID == 0) {
            printf("MAIN: %d possible moves after move #%d; creating %d child threads...\n", solver->numValidMoves, solver->depth, solver->numValidMoves);
        } else {
            printf("T%d: %d possible moves after move #%d; creating %d child threads...\n", solver->threadID, solver->numValidMoves, solver->depth, solver->numValidMoves);
        }

        pthread_t threads[solver->numValidMoves];
        int threadNums[solver->numValidMoves];
        
        boardInformation* solvers = (boardInformation*)calloc(solver->numValidMoves, sizeof(boardInformation));

        for (int i = 0; i < solver->numValidMoves; i++) {
            (solvers[i]).threadID = solver->threadID;
            (solvers[i]).depth = solver->depth + 1;
            (solvers[i]).numValidMoves = solver->numValidMoves;
            (solvers[i]).validMoves = (coordinates*)calloc(8, sizeof(coordinates));
            for (int j = 0; j < solver->numValidMoves; j++) {
                (solvers[i]).validMoves[j].rowNum = solver->validMoves[j].rowNum;
                (solvers[i]).validMoves[j].colNum = solver->validMoves[j].colNum;
            }
            (solvers[i]).currentPosition = (coordinates*)calloc(1, sizeof(coordinates));
            (solvers[i]).currentPosition->rowNum = solver->validMoves[i].rowNum;
            (solvers[i]).currentPosition->colNum = solver->validMoves[i].colNum;
            (solvers[i]).board = (char**)calloc(m, sizeof(char*));
            for (int j = 0; j < m; j++) {
                (solvers[i]).board[j] = (char*)calloc(n + 1, sizeof(char));
            }
            for (int j = 0; j < m; j++) {
                for (int k = 0; k < n; k++) {
                    (solvers[i]).board[j][k] = solver->board[j][k];
                }
            }
            (solvers[i]).board[(solvers[i]).currentPosition->rowNum][(solvers[i]).currentPosition->colNum] = (solvers[i]).depth - 1 + '0';
            pthread_mutex_lock(&mutex_next_thread_id);
            (solvers[i]).threadID = next_thread_id;
            threadNums[i] = next_thread_id;
            next_thread_id++;
            pthread_mutex_unlock(&mutex_next_thread_id);
            printBoardInformation(&solvers[i]);
            rc = pthread_create(&threads[i], NULL, (void *)runSolver, &solvers[i]);
            if (rc != 0) {
                perror("ERROR");
                pthread_exit(rcExit);
            }
            // DELETE THIS SEGMENT
            if (disable_parallelism) {
                rc = pthread_join(threads[i], (void**)&(retVal));
                if (rc != 0) {
                    perror("ERROR");
                    pthread_exit(rcExit);
                } else {
                    if (solver->threadID == 0) {
                        printf("MAIN: T%d joined\n", threadNums[i]);
                    } else {
                        printf("T%d: T%d joined\n", solver->threadID, threadNums[i]);
                    }
                }
                // freeing dynamically allocated memory
                for (int j = 0; j < m; j++) {
                    free(solvers[i].board[j]);
                }
                
                free(solvers[i].board);
                
                free(solvers[i].currentPosition);
                
                free(solvers[i].validMoves);
            }
#ifdef NO_PARALLEL
            rc = pthread_join(threads[i], (void**)&(retVal));
            if (rc != 0) {
                perror("ERROR");
                pthread_exit(rcExit);
            } else {
                if (solver->threadID == 0) {
                    printf("MAIN: T%d joined\n", threadNums[i]);
                } else {
                    printf("T%d: T%d joined\n", solver->threadID, threadNums[i]);
                }
            }
            // freeing dynamically allocated memory
            for (int i = 0; i < m; i++) {
                free(solvers[i].board[i]);
            }
            
            free(solvers[i].board);
            
            free(solvers[i].currentPosition);
            
            free(solvers[i].validMoves);
            
            free(solvers + i);
#endif
        }
        if (!disable_parallelism) {
            for (int i = 0; i < solver->numValidMoves; i++) {
                rc = pthread_join(threads[i], (void**)&(retVal));
                if (rc != 0) {
                    perror("ERROR");
                    pthread_exit(rcExit);
                } else {
                    if (solver->threadID == 0) {
                        printf("MAIN: T%d joined\n", threadNums[i]);
                    } else {
                        printf("T%d: T%d joined\n", solver->threadID, threadNums[i]);
                    }
                }
                // freeing dynamically allocated memory
                for (int i = 0; i < m; i++) {
                    free(solvers[i].board[i]);
                }
                
                free(solvers[i].board);
                
                free(solvers[i].currentPosition);
                
                free(solvers[i].validMoves);
                
                free(solvers + i);
            }
        }
    }
    return (void*)NULL;
}

int simulate(int argc, char* argv[]) {
    // no buffering for stdout
    setvbuf(stdout, NULL, _IONBF, 0);

    // parsing command line arguments and checking their validity
    // CHANGE BACK TO 5
    if (argc != 6) {
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: a.out <m> <n> <r> <c>\n");
        return EXIT_FAILURE;
    }
    
    m = atoi(argv[1]);
    n = atoi(argv[2]);
    r = atoi(argv[3]);
    c = atoi(argv[4]);
    
    if (m <= 2 || n <= 2 || r < 0 || r >= m || c < 0 || c >= n) {
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: a.out <m> <n> <r> <c>\n");
        return EXIT_FAILURE;
    }
    
    // DELETE THIS SEGMENT TILL #IFDEF NO_PARALLEL
    int x = atoi(argv[5]);
    if (x == 0) {
        disable_parallelism = true;
    }
#ifdef NO_PARALLEL
    disable_parallelism = true;
#endif
    
    printf("MAIN: Solving Sonny's knight's tour problem for a %dx%d board\n", m, n);
    printf("MAIN: Sonny starts at row %d and column %d (move #1)\n", r, c);
    
    // dynamically allocating memory
    boardInformation* solver = (boardInformation*)calloc(1, sizeof(boardInformation));
    
    solver->depth = 1;
    
    coordinates* currentPosition = (coordinates*)calloc(1, sizeof(coordinates));
    currentPosition->rowNum = r;
    currentPosition->colNum = c;
    solver->currentPosition = currentPosition;
    
    coordinates* validMoves = (coordinates*)calloc(8, sizeof(coordinates));
    solver->validMoves = validMoves;
    
    solver->board = (char**)calloc(m, sizeof(char*));
    
    for (int i = 0; i < m; i++) {
        solver->board[i] = (char*)calloc(n + 1, sizeof(char));
    }
    
    // initializing board
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            solver->board[i][j] = '.';
        }
    }
    
    solver->board[r][c] = 'S';
    
    runSolver((void*)solver);
    
    if (total_tours == 0) {
        if (max_squares == 1) {
            printf("MAIN: Search complete; best solution(s) visited %d square out of %d\n", max_squares, m * n);
        } else if (max_squares > 1) {
            printf("MAIN: Search complete; best solution(s) visited %d squares out of %d\n", max_squares, m * n);
        }
    } else if (total_tours > 0) {
        printf("MAIN: Search complete; found %d possible paths to achieving a full knight's tour\n", total_tours);
    }

    // freeing dynamically allocated memory
    for (int i = 0; i < m; i++) {
        free(solver->board[i]);
    }
    
    free(solver->board);
    
    free(solver->currentPosition);
    
    free(solver->validMoves);
    
    free(solver);
    
    return EXIT_SUCCESS;
}
