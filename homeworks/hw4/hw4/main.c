//
//  main.c
//  hw4
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5

typedef struct {
    int socket;
    char *username;
} client_information;

client_information client_information_array[MAX_CLIENTS];

char* getSecretWord(char** words, int numWordsRead) {
    int secretWordIndex = rand() % numWordsRead;
    
    printf("Dictionary size: %d\n", numWordsRead);
    printf("Secret word: %s\n", words[secretWordIndex]);
    
    return words[secretWordIndex];
}

char** readDictionaryFile(const char* dictionaryFile, int longest, char** words, int numWordsAllocated, int* numWordsRead) {
    char* buffer = (char*)calloc(longest, sizeof(char*));
    
    FILE* fp = fopen(dictionaryFile, "r");
    
    if (!fp) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }
    
    while(fgets(buffer, longest, fp)) {
        if((*numWordsRead) == numWordsAllocated-1) {
            numWordsAllocated *= 2;
            words = realloc(words, sizeof(char*)*numWordsAllocated);
        }
        
        words[*numWordsRead] = calloc(strlen(buffer), sizeof(char*));
        buffer[strlen(buffer)-1] = '\0';
        strcpy(words[*numWordsRead], buffer);
        memset(buffer, 0, strlen(buffer));
        (*numWordsRead)++;
    }
    
    fclose(fp);
    free(buffer);
    
    return words;
    
}

void sendToClient(char* message, int socket) {
    long numBytesSent = send(socket, message, strlen(message), 0);
    if (numBytesSent != strlen(message)) {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }
}


void checkGuess(int* statsCorrect, const char* secretWord, const char* userGuess) {
    int numLettersCorrect = 0;
    int numLettersCorrectlyPlaced = 0;
    
    bool secretWordLettersCounted[strlen(userGuess)];
    bool userGuessLettersCounted[strlen(userGuess)];
    
    for (int i = 0; i < strlen(userGuess); i++) {
        secretWordLettersCounted[i] = false;
        userGuessLettersCounted[i] = false;
    }
    
    if (strlen(secretWord) != strlen(userGuess)) {
        numLettersCorrect = -1;
        numLettersCorrectlyPlaced = -1;
    } else {
        for (int i = 0; i < strlen(secretWord); i++) {
            if (tolower(secretWord[i]) == tolower(userGuess[i])) {
                (numLettersCorrectlyPlaced)++;
            }
        }

        for (int i = 0; i < strlen(secretWord); i++) {
            for (int j = 0; j < strlen(userGuess); j++) {
                if (tolower(secretWord[i]) == tolower(userGuess[j]) && !secretWordLettersCounted[i] && !userGuessLettersCounted[j]) {
                    numLettersCorrect++;
                    secretWordLettersCounted[i] = true;
                    userGuessLettersCounted[j] = true;
                }
            }
        }
    }
    
    statsCorrect[0] = numLettersCorrect;
    statsCorrect[1] = numLettersCorrectlyPlaced;
}

void launchTCPServer(int port, int longestWordLength, char* secretWord, char** words, int numWordsRead) {
    fd_set readfds;
    
    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    
    if ( sock < 0 ) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server;
    
    server.sin_family = PF_INET;
    server.sin_addr.s_addr = htonl( INADDR_ANY );
    
    server.sin_port = htons( port );
    int len = sizeof( server );
    
    if (bind( sock, (struct sockaddr *)&server, len ) < 0 ) {
        perror( "bind()" );
        exit( EXIT_FAILURE );
    }
    
    listen( sock, 5 );
    
    struct sockaddr_in client;
    int fromlen = sizeof(client);
    
    int n;
    int numUsersCurrent = 0;
    
    while(1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        
        for (int i = 0 ; i < 5 ; i++ ) {
            if(client_information_array[i].socket!=0){
                FD_SET(client_information_array[i].socket, &readfds );
            }
        }
        
        select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        
        if(FD_ISSET(sock, &readfds)) {
            int newsock = accept(sock, (struct sockaddr *)&client, (socklen_t *)&fromlen);
            int x = 0;
            while (x < 5) {
                if(client_information_array[x].socket == 0) {
                    client_information_array[x].socket = newsock;
                    numUsersCurrent++;
                    break;
                }
                x++;
            }
            
            char message[BUFFER_SIZE] = "Welcome to Guess the Word, please enter your username.\n";
            sendToClient(message, newsock);
        }
        
        for(int i = 0; i < 5; i++) {
            int fd = client_information_array[i].socket;
            if(FD_ISSET(fd, &readfds)) {
                char* msgFromClient = calloc(longestWordLength, sizeof(char));
                n = recv(fd, msgFromClient, BUFFER_SIZE-1, 0);
                
                if(n < 0) {
                    fprintf(stderr, "recv() failed\n");
                } else if(n == 0) {
                    client_information_array[i].socket = 0;
                    free(client_information_array[i].username);
                    client_information_array[i].username = NULL;
                    numUsersCurrent--;
                } else {
                    msgFromClient[n] = '\0';
                    msgFromClient[n-1] = '\0';
                    if(client_information_array[i].username == NULL) {
                        char messageToClient[BUFFER_SIZE] = "";
                        bool userNameNotTaken = true;
                        int s = 0;
                        while (s < 5) {
                            if(client_information_array[s].username != NULL && strcasecmp(client_information_array[s].username, msgFromClient) == 0) {
                                sprintf(messageToClient, "Username %s is already taken, please enter a different username\n", msgFromClient);
                                sendToClient(messageToClient, fd);
                                userNameNotTaken = false;
                                break;
                            }
                            s++;
                        }
                        if(userNameNotTaken) {
                            
                            client_information_array[i].username = msgFromClient;
                            
                            strcpy(messageToClient, "");
                            sprintf(messageToClient, "Let's start playing, %s\n", msgFromClient);
                            sendToClient(messageToClient, fd);
                            strcpy(messageToClient, "");
                            sprintf(messageToClient, "There are %d player(s) playing. The secret word is %lu letter(s).\n", numUsersCurrent, strlen(secretWord));
                            sendToClient(messageToClient, fd);
                        }
                    } else {
                        bool correct = false;
                        char messageToClient[BUFFER_SIZE] = "";
                        
                        int* result = calloc(2, sizeof(int));
                        checkGuess(result, secretWord, msgFromClient);
                        
                        if (result[0] == -1 && result[1] == -1) {
                            sprintf(messageToClient, "Invalid guess length. The secret word is %lu letter(s).\n", strlen(secretWord));
                            sendToClient(messageToClient, fd);
                            free(result);
                            continue;
                        } else if(result[1] == strlen(secretWord)) {
                            sprintf(messageToClient, "%s has correctly guessed the word %s\n", client_information_array[i].username, secretWord);
                            correct = true;
                        } else {
                            sprintf(messageToClient, "%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed.\n", client_information_array[i].username, msgFromClient, result[0], result[1]);
                            
                        }

                        int j = 0;
                        while (j < 5) {
                            if(client_information_array[j].socket!=0){
                                sendToClient(messageToClient, client_information_array[j].socket);
                            }
                            j++;
                        }
                        free(result);
                        if(correct) {
                            int v = 0;
                            while (v < 5) {
                                if(client_information_array[v].socket!=0){
                                    close(client_information_array[v].socket);
                                    client_information_array[v].socket = 0;
                                    free(client_information_array[v].username);
                                    client_information_array[v].username=NULL;
                                }
                                v++;
                            }
                            numUsersCurrent = 0;
                            secretWord = getSecretWord(words, numWordsRead);
                            break;
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, const char * argv[]) {
    // no buffering for stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    
    // error checking for incorrect command line arguments
    if (argc != 5) {
        fprintf(stderr, "ERROR: Invalid arguments\n");
        fprintf(stderr, "USAGE: a.out <seed> <port> <dictionary_file> <longest_word_length>\n");
        return EXIT_FAILURE;
    }
    
    int seed = atoi(argv[1]);
    int port = atoi(argv[2]);
    const char* dictionaryFile = argv[3];
    int longestWordLength = atoi(argv[4]);
    
    int numWordsAllocated = 1;
    char** words = calloc(numWordsAllocated, sizeof(char*));
    
    int numWordsRead = 0;
    
    words = readDictionaryFile(dictionaryFile, longestWordLength, words, numWordsAllocated, &numWordsRead);
    
    srand(seed);
    char* secretWord = getSecretWord(words, numWordsRead);
    
    launchTCPServer(port, longestWordLength, secretWord, words, numWordsRead);
    
    return EXIT_SUCCESS;
}





