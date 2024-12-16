#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define NUM_THREAD 8
#define NUM_BOARD (NUM_THREAD / 2)
#define BOARD_SIZE 3

struct Board {
    char board[BOARD_SIZE][BOARD_SIZE];
    int boardPlayer;
    int boardRound;
    char boardInfo[55];
    char boardResult[50];
    sem_t boardKey;
};

struct Player {
    int boardNumber;
    char playerSign;
};

pthread_t tid[NUM_THREAD];
sem_t getBnS, Round;

void *tournament(void *param);
void defaultBoard(struct Board *board);
void initializeBoard(char board[BOARD_SIZE][BOARD_SIZE]);
struct Player getBoardandSign();
void showBoard(char board[BOARD_SIZE][BOARD_SIZE]);
int updateBoard(struct Player player);
void checkWinner(struct Player player);
int playTicTacToe(struct Player player);

struct Board boardArr[NUM_BOARD];

int main() {
    int i, j, k, time = 14;

    int numRounds = (int)ceil(log2(NUM_THREAD));
    int currentPlayers = NUM_THREAD;
    int currentBoards = NUM_BOARD;

    sem_init(&getBnS, 0, 1);
    sem_init(&Round, 0, 1);

    for (i = 0; i < NUM_BOARD; i++) {
        defaultBoard(&boardArr[i]);
    }

    for (i = 0; i < NUM_THREAD; i++) {
        pthread_create(&tid[i], NULL, tournament, NULL);
    }

    for (j = 0; j < numRounds; j++) {
        printf("Round: %d ------------\n", j + 1);

        sleep(2);
        sem_wait(&Round);
        for (k = 0; k < currentBoards; k++) {
            while (boardArr[k].boardResult[0] == '\0') {
                sleep(1);
            }
            sleep(1);
            sem_wait(&boardArr[k].boardKey);
            printf("Board[%d]: %s\n", k, boardArr[k].boardInfo);
            printf("Board[%d]: %s\n", k, boardArr[k].boardResult);
            // showBoard(boardArr[k].board);
            defaultBoard(&boardArr[k]);
            sem_post(&boardArr[k].boardKey);
        }

        currentPlayers = currentPlayers / 2;
        currentBoards = currentBoards / 2;
        printf("\n");
        sem_post(&Round);
    }

    printf("Good bye.\n");
    return 0;
}

void *tournament(void *param) {
    struct Player playerinfo;
    int i, result;
    int numRounds = (int)ceil(log2(NUM_THREAD));
    
    for (int i = 0; i < numRounds; i++) {
        sleep(1);
        sem_wait(&getBnS);

        playerinfo = getBoardandSign();

        sem_post(&getBnS);

        sleep(4);
        result = playTicTacToe(playerinfo);

        // printf("%d=>%ld\n", result, pthread_self());
        if (result) {
            pthread_exit(NULL);
        }
        sem_wait(&Round);
            // printf("wait %ld\n", pthread_self());
        sem_post(&Round);
    }
    pthread_exit(NULL);
}

void defaultBoard(struct Board *board) {
    initializeBoard(board->board);
    board->boardPlayer = 0;
    board->boardRound = 0;
    board->boardInfo[0] = '\0';
    board->boardResult[0] = '\0';
    sem_init(&board->boardKey, 0, 1);
}

void initializeBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = ' ';
        }
    }
}

struct Player getBoardandSign() {
    struct Board *board;
    struct Player player;

    for (int i = 0; i < NUM_BOARD; i++) {
        board = &boardArr[i];
        if (board->boardPlayer < 2) {
            sem_wait(&board->boardKey);
            board->boardPlayer++;
            if (board->boardPlayer == 1) {
                player.boardNumber = i;
                player.playerSign = 'X';
                sprintf(board->boardInfo, "%sThread:%ld[%c]", board->boardInfo, pthread_self(), player.playerSign);
            } else {
                player.boardNumber = i;
                player.playerSign = 'O';
                sprintf(board->boardInfo, "%s vs Thread:%ld[%c]", board->boardInfo, pthread_self(), player.playerSign);
            }
            sem_post(&board->boardKey);
            return player;
        }
    }
}

void showBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    printf("\n\n\t  ");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("%c", board[i][j]);
            if (j < BOARD_SIZE - 1) {
                printf("  |  ");
            }
        }
        if (i < BOARD_SIZE - 1)
            printf("\n\t----------------\n\t  ");
    }
    printf("\n\n\n");
}

int updateBoard(struct Player player) {
    struct Board *board;
    char (*PlayBoard)[BOARD_SIZE];

    srand(time(NULL));
    board = &boardArr[player.boardNumber];

    PlayBoard = board->board;

    int isValid = 1;

    while (isValid) {
        int row = (rand() % 9) / 3;
        int col = (rand() % 9) % 3;
        if (PlayBoard[row][col] == ' ') {
            PlayBoard[row][col] = player.playerSign;
            isValid = 0;
            board->boardRound++;
        }
    }
    return 1;
}

void checkWinner(struct Player player) {
    struct Board *board;
    char (*PlayBoard)[BOARD_SIZE], sg;

    board = &boardArr[player.boardNumber];
    PlayBoard = board->board;
    sg = player.playerSign;

    if (PlayBoard[0][0] == sg && PlayBoard[0][1] == sg && PlayBoard[0][2] == sg ||
        PlayBoard[1][0] == sg && PlayBoard[1][1] == sg && PlayBoard[1][2] == sg ||
        PlayBoard[2][0] == sg && PlayBoard[2][1] == sg && PlayBoard[2][2] == sg) {
        sprintf(board->boardResult, "Pthread [%ld][%c] win", pthread_self(), sg);
    } else if (PlayBoard[0][0] == sg && PlayBoard[1][0] == sg && PlayBoard[2][0] == sg ||
               PlayBoard[0][1] == sg && PlayBoard[1][1] == sg && PlayBoard[2][1] == sg ||
               PlayBoard[0][2] == sg && PlayBoard[1][2] == sg && PlayBoard[2][2] == sg) {
        sprintf(board->boardResult, "Pthread [%ld][%c] win", pthread_self(), sg);
    } else if (PlayBoard[0][0] == sg && PlayBoard[1][1] == sg && PlayBoard[2][2] == sg ||
               PlayBoard[0][2] == sg && PlayBoard[1][1] == sg && PlayBoard[2][0] == sg) {
        sprintf(board->boardResult, "Pthread [%ld][%c] win", pthread_self(), sg);
    }
}

int playTicTacToe(struct Player player) {
    struct Board *board;

    board = &boardArr[player.boardNumber];

    while (board->boardResult[0] == '\0') {
        sleep(1);
        sem_wait(&board->boardKey);
        if (board->boardResult[0] != '\0') {
                sem_post(&board->boardKey);
                return 1;
        }
        if (board->boardRound < 9) {
            updateBoard(player);
            checkWinner(player);
            // showBoard(board->board);
            if (board->boardResult[0] != '\0') {
                sem_post(&board->boardKey);
                return 0;
            }
        } else {
            initializeBoard(board->board);
            board->boardRound = 0;
        }
        sem_post(&board->boardKey);
    }
    return 1;
}
