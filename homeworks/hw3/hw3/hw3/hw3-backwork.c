#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int next_thread_id;
extern int max_squares;
extern int total_tours;
static int m, n;
static int **board;
static char paralell_check = 0;

/* global mutex variables */
static pthread_mutex_t mutex_max_squares = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_total_tours = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_next_Thread_id = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int KnightRow;
  int KnightCol;
  int moveCount;
  int **pboard;
  int id;
} data;

// static void printBoard(int **b) {
//   for (int i = 0; i < m; i++) {
//     for (int j = 0; j < n; j++) printf(" %d ", b[i][j]);
//     printf("\n");
//   }
// }

// checks if the posistion is valid
static bool isPosValid(int **cboard, int currentRow, int currentCol) {
  return currentRow >= 0 && currentRow < m && currentCol >= 0 &&
         currentCol < n && cboard[currentRow][currentCol] <= 0;
}

// from HW2
static int possiblePositions(int **currentBoard, int possibleMoves[][2],
                             int currentRow, int currentCol) {
  static const int Xmove[] = {-2, -1, 1, 2, 2, 1, -1, -2};
  static const int Ymove[] = {-1, -2, -2, -1, 1, 2, 2, 1};

  int numPossibleMoves = 0;

  for (int i = 0; i < 8; i++){
    if (isPosValid(currentBoard, currentRow + Xmove[i],currentCol + Ymove[i]) == true){
      possibleMoves[numPossibleMoves][0] = currentRow + Xmove[i],
      possibleMoves[numPossibleMoves][1] = currentCol + Ymove[i],
      numPossibleMoves++;
    }
  }
  return numPossibleMoves;
}

// form HW2 helper function for need to add thread or not
static void *solverHelper(void *argIn) {
  data arg = *(data *)argIn;
  int KnightRow = arg.KnightRow;
  int KnightCol = arg.KnightCol;
  size_t moveCount = arg.moveCount;
  size_t maxChildMoves = 0;

  // printf("\n");
  arg.pboard[KnightRow][KnightCol] = moveCount;

  // before doing all the calculations check if its done
  char tour = 1;
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      if (arg.pboard[i][j] <= 0) {
        tour = 0;
        break;
      }
    }
  }

  // printBoard(arg.pboard);
  // printf("\n");
  if (tour == 1) {
    pthread_mutex_lock(&mutex_max_squares);
    max_squares = moveCount;
    pthread_mutex_unlock(&mutex_max_squares);
    pthread_mutex_lock(&mutex_next_Thread_id);
    printf("T%d: Sonny found a full knight's tour; incremented total_tours\n",
           next_thread_id);
    pthread_mutex_unlock(&mutex_next_Thread_id);
    pthread_mutex_lock(&mutex_total_tours);
    total_tours++;
    pthread_mutex_unlock(&mutex_total_tours);

   unsigned int *ret = (unsigned int*)calloc(1,sizeof(unsigned int));
    pthread_exit(ret);
    return(void*)moveCount;
  }
  int possibleLocation[8][2];
  memset(possibleLocation, -1, sizeof possibleLocation);
  // printBoard(arg.pboard);
  int numberofMoves =
      possiblePositions(arg.pboard, possibleLocation, KnightRow, KnightCol);
  // printf("%d,%d %d\n" ,KnightRow,KnightCol,numberofMoves);
// for(int i=0; i<8;i++){
//         printf("POS %d, %d\n",possibleLocation[i][0],possibleLocation[i][1]);
// }
  // maybe try array of threads to keep track
  if (numberofMoves == 1) {
    // printBoard(arg.pboard);
    // printf("\n");
    data *d1 = calloc(1,sizeof(data));
    d1->KnightCol = possibleLocation[0][1];
    d1->KnightRow = possibleLocation[0][0];
    d1->moveCount = moveCount + 1;
    d1->pboard = arg.pboard;
    d1->pboard[d1->KnightRow][d1->KnightCol] = moveCount+1;
    d1->id = arg.id;
    maxChildMoves = (size_t)solverHelper((void *)d1);

  } else if (numberofMoves == 0) {
    // dead end case
    // lock just incase it changes find a way to go with out this????
    pthread_mutex_lock(&mutex_next_Thread_id);
    {
      if ((int)moveCount == 1) {
        printf("MAIN: Dead end at move #%d", (int)moveCount);
      } else {
        printf("T%d: Dead end at move #%d", arg.id, (int)moveCount);
      }

      if (moveCount > max_squares) {
        max_squares = moveCount;
        printf("; updated max_squares\n");
      } else {
        printf("\n");
      }
    }
    pthread_mutex_unlock(&mutex_next_Thread_id);
    // printf("%d\n",moveCount);
    maxChildMoves = moveCount;
    if(arg.id==0){
      return (void*)moveCount;
    }else{
      pthread_exit((void *)maxChildMoves);
      return (void*)moveCount;
    }
    

  } else if (numberofMoves > 1) {
    if (next_thread_id == 1) {
      printf(
          "MAIN: %d possible moves after move #%d; creating %d child "
          "threads...\n",
          numberofMoves, (int)moveCount, numberofMoves);
    } else {
      printf(
          "T%d: %d possible moves after move #%d; creating %d child "
          "threads...\n",
          arg.id, numberofMoves, (int)moveCount, numberofMoves);
    }
    pthread_t threads[numberofMoves];
    int threadNames[numberofMoves];
    void *returnValue;

    // store array of structs to free
    data *storage[numberofMoves];

    for (int i = 0; i < numberofMoves; i++) {
      // printf("Possible Locations %d,%d\n", possibleLocation[i][0],
      //  possibleLocation[i][1]);
      if (!(possibleLocation[i][0] == -1 && possibleLocation[i][1] == -1)) {
        // moveCount++;
        // printf("test\n");
        data *pass = calloc(1, sizeof(data));
        // printf("tes2\n");
        pass->KnightRow = possibleLocation[i][0];  // row
        pass->KnightCol = possibleLocation[i][1];  // col
        // printf("Row %d, Col %d\n",pass->KnightRow,pass->KnightCol);
    
        pass->moveCount = moveCount+1;
        // printf("Pass move count %d\n",moveCount);
        pass->pboard = calloc(m, sizeof(int *));
        for (int i = 0; i < m; i++) {
          pass->pboard[i] = calloc(n, sizeof(int));
          for (int j = 0; j < n; j++) pass->pboard[i][j] = arg.pboard[i][j];
        }

        pass->pboard[pass->KnightRow][pass->KnightCol] = moveCount+1;

        // printBoard(pass->pboard);
        if (paralell_check == 0) storage[i] = pass;
        pthread_mutex_lock(&mutex_next_Thread_id);
        {
          threadNames[i] = next_thread_id;
          pass->id = next_thread_id;
          next_thread_id++;
        }
        pthread_mutex_unlock(&mutex_next_Thread_id);
        pthread_create(&threads[i], NULL, (void *)solverHelper, pass);
#ifdef NO_PARALLEL
        pthread_join(threads[i], &returnValue);  // seg fault here idk why
        if ((size_t)returnValue > maxChildMoves) {
          maxChildMoves = (size_t)returnValue;
        }

        // pthread_mutex_lock(&mutex_next_Thread_id);{
             if(arg.id==0){
                printf("MAIN: T%d joined\n", threadNames[i]);
             }else{
                 printf("T%d: T%d joined\n",arg.id,threadNames[i]);
             }
        //  }
        // pthread_mutex_unlock(&mutex_next_Thread_id);


        // free
        for (int i = 0; i < m; i++) {
          free(pass->pboard[i]);
        }
        free(pass->pboard);
        free(pass);
#endif
        // moveCount--;
      }
    }
    if (paralell_check == 0) {
      for (int i = 0; i < numberofMoves; i++) {
        pthread_join(threads[i], &returnValue);  // seg fault here idk why
        if ((size_t)returnValue > maxChildMoves) {
          maxChildMoves = (size_t)returnValue;
        }

        // pthread_mutex_lock(&mutex_next_Thread_id);{
        if(arg.id==0){
                  printf("MAIN: T%d joined\n", threadNames[i]);
              }else{
                 printf("T%d: T%d joined\n",arg.id,threadNames[i]);
             }
        //  }
    //     pthread_mutex_unlock(&mutex_next_Thread_id);

        for (int j = 0; j < m; j++) {
          free(storage[i]->pboard[j]);
        }
        free(storage[i]);
      }
    }
  }
  if(moveCount>maxChildMoves){
    maxChildMoves = moveCount;
  }
  return (void *)maxChildMoves;
}

int simulate(int argc, char *argv[]) {
  // for submitty
  setvbuf(stdout, NULL, _IONBF, 0);

  int Knightrow, Knightcol;

  if (argc < 3 || argc > 5) {
    fprintf(stderr,
            "ERROR: Invalid arguments\nUSAGE: hw3.out <m> <n> <r> <c>\n");
    return EXIT_FAILURE;
  }

  m = atoi(argv[1]);
  n = atoi(argv[2]);
  if (m <= 2 || n <= 2) {
    fprintf(stderr,
            "ERROR: Invalid argument(s)\nUSAGE: hw2.out <m> <n> <r> <c>\n");
    return EXIT_FAILURE;
  }

  if (argc == 5) {
    if ((atoi(argv[3]) < 0 || atoi(argv[3]) > m - 1) ||
        (atoi(argv[4]) < 0 || atoi(argv[4]) > n - 1)) {
      fprintf(stderr,
              "ERROR: Invalid argument(s)\nUSAGE: hw2.out <m> <n> <r> <c>\n");
      return EXIT_FAILURE;
    } else {
      Knightrow = atoi(argv[3]);
      Knightcol = atoi(argv[4]);
    }
  }

  printf("MAIN: Solving Sonny's knight's tour problem for a %dx%d board\n", m,
         n);
  printf("MAIN: Sonny starts at row %d and column %d (move #1)\n", Knightrow,
         Knightcol);

#ifdef NO_PARALLEL
  paralell_check = 1;
#endif
  // init the board
  board = calloc(m, sizeof(int *));
  for (int i = 0; i < m; i++) {
    board[i] = calloc(n, sizeof(int));
  }
  data arguments;
  arguments.pboard = board;
  arguments.KnightRow = Knightrow;
  arguments.KnightCol = Knightcol;
  arguments.moveCount = 1;
  arguments.id=0;
  solverHelper((void*)&arguments);
  // free the board at the end
  for (int i = 0; i < m; i++) {
    free(board[i]);
  }
  free(board);
  if (total_tours == 0) {
    if (max_squares == 1) {
      printf(
          "MAIN: Search complete; best solution(s) visited %d square out of "
          "%d\n",
          max_squares, m * n);
    } else {
      printf(
          "MAIN: Search complete; best solution(s) visited %d squares out of "
          "%d\n",
          max_squares, m * n);
    }
  } else if (total_tours > 0) {
    printf(
        "MAIN: Search complete; found %d possible paths to achieving a full "
        "knight's tour\n",
        total_tours);
  }

  return EXIT_SUCCESS;
}


