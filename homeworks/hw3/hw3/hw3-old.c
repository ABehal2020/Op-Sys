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

const int m;
const int n;
const int r;
const int c;

bool disable_parallelism = false;

typedef struct {
    int x;
    int y;
} coordinates;

pthread_mutex_t mutex_next_thread_id = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_max_squares = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_total_tours = PTHREAD_MUTEX_INITIALIZER;

void generate_valid_moves(char** board, coordinates* valid_moves, unsigned int* num_valid_moves, coordinates* current_position, unsigned int* depth) {
    *num_valid_moves = 0;
    int* validX = (int*)calloc(1, sizeof(int));
    int* validY = (int*)calloc(1, sizeof(int));
    int offsetByTwo = 2;
    int offsetByOne = 1;
    // there are 8 possible moves - we'll check them all and keep all the legal ones
    for (unsigned int i = 0; i < 8; i++) {
        if (i == 0) {
            // first possible move
            *validX = current_position->x - offsetByTwo;
            *validY = current_position->y - offsetByOne;
            // printf("First possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 1) {
            // second possible move
            *validX = current_position->x - offsetByOne;
            *validY = current_position->y - offsetByTwo;
            // printf("Second possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 2) {
            // third possible move
            *validX = current_position->x + offsetByOne;
            *validY = current_position->y - offsetByTwo;
            // printf("Third possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 3) {
            // fourth possible move
            *validX = current_position->x + offsetByTwo;
            *validY = current_position->y - offsetByOne;
            // printf("Fourth possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 4) {
            // fifth possible move
            *validX = current_position->x + offsetByTwo;
            *validY = current_position->y + offsetByOne;
            // printf("Fifth possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 5) {
            // sixth possible move
            *validX = current_position->x + offsetByOne;
            *validY = current_position->y + offsetByTwo;
            // printf("Sixth possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 6) {
            // seventh possible move
            *validX = current_position->x - offsetByOne;
            *validY = current_position->y + offsetByTwo;
            // printf("Seventh possible move: validX: %d, validY: %d\n", validX, validY);
        } else if (i == 7) {
            // eighth possible move
            *validX = current_position->x - offsetByTwo;
            *validY = current_position->y + offsetByOne;
            // printf("Eighth possible move: validX: %d, validY: %d\n", validX, validY);
        }
        // make sure move is valid and destination square of move hasn't been visited before
        if (*validX >= 0 && *validX < m && *validY >= 0 && *validY < n && board[*validX][*validY] == '.') {
            (valid_moves[*num_valid_moves]).x = *validX;
            (valid_moves[*num_valid_moves]).y = *validY;
            num_valid_moves++;
        }
    }
    free(validX);
    free(validY);
}

int simulate(int argc, char* argv[]) {
    // no buffering for stdout
    setvbuf(stdout, NULL, _IONBF, 0);

    // parsing command line arguments and checking their validity
    if (argc != 5) {
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
    
    printf("MAIN: Solving Sonny's knight's tour problem for a %dx%d board\n", m, n);
    printf("MAIN: Sonny starts at row %d and column %d (move #1)\n", r, c);
    
    // dynamically allocating memory
    char** board = (char **)calloc(m, sizeof(char*));
    coordinates* valid_moves = (coordinates*)calloc(8, sizeof(coordinates));
    unsigned int num_valid_moves = 0;
    coordinates* current_position = (coordinates*)calloc(1, sizeof(coordinates));
    unsigned int depth = 0;
    
    for (unsigned int i = 0; i < m; i++) {
        board[i] = (char *)calloc(n + 1, sizeof(char));
    }
    
    // initializing board
    for (unsigned int i = 0; i < m; i++) {
        for (unsigned int j = 0; j < n; j++) {
            board[i][j] = '.';
        }
    }
    
    board[r][c] = 'S';
    
    pthread_t* threadId = NULL;
    
    do {
        generate_valid_moves(board, valid_moves, &num_valid_moves, current_position, &depth);
        if (num_valid_moves == 0) {
            if (depth > max_squares) {
                pthread_mutex_lock(&mutex);
                max_squares = depth;
                pthread_mutex_unlock(&mutex);
            }
            if (depth == m * n) {
                pthread_mutex_lock(&mutex);
                total_tours++;
                pthread_mutex_unlock(&mutex);
            }
        } else if (num_valid_moves == 1) {
            generate_valid_moves();
        } else if (num_valid_moves > 1) {
            printf("%d possible moves after move #%d; creating %d child threads\n", num_valid_moves, depth, num_valid_moves);
            // create new threads
            for (unsigned int i = 0; i < num_valid_moves; i++) {
                pthread_mutex_lock(&mutex);
                next_thread_id++;
                pthread_mutex_unlock(&mutex);
                threadId = (pthread_t*)calloc(next_thread_id, sizeof(pthread_t));
                for (unsigned int i = 0; i < next_thread_id; i++) {
                    threadId[i] = (pthread_t)(i + 1);
                }
                int* rc = (int*)calloc(1, sizeof(int));
                *rc = pthread_create(&next_thread_id, NULL, generate_valid_moves, NULL);
                if (*rc != 0) {
                    unsigned int* ret = (unsigned int*)calloc(1, sizeof(unsigned int));
                    *ret = next_thread_id;
                    perror("ERROR");
                    pthread_exit(ret);
                    free(ret);
                }
#ifdef NO_PARALLEL
                unsigned int* ret2 = (unsigned int*)calloc(1, sizeof(unsigned int));
                *rc = pthread_join(threadId[i], (void**)&ret2);
                if (*rc != 0) {
                    perror("ERROR");
                    pthread_exit(ret2);
                }
                free(ret2);
#endif
                free(rc);
                free(threadId);
            }
        }
    } while (num_valid_moves != 0);
    
#ifndef NO_PARALLEL
    int* rc = (int*)calloc(1, sizeof(int));
    unsigned int* ret2 = (unsigned int*)calloc(1, sizeof(unsigned int));
    for (unsigned int i = 0; i <= next_thread_id; i++) {
        *rc = pthread_join(threadId[i], (void**)&ret2);
        if (*rc != 0) {
            perror("ERROR");
            pthread_exit(ret2);
        }
    }
    free(ret2);
    free(rc);
#endif
    
    // freeing dynamically allocated memory
    free(valid_moves);
    free(current_position);
    
    for (unsigned int i = 0; i < m; i++) {
        free(board[m]);
    }
    
    free(board);
    
    return EXIT_SUCCESS;
}

