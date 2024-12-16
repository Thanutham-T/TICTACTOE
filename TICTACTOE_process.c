#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <string.h>

#define SHMSZ sizeof(struct BoardInfo)

struct BoardInfo {
    char board[3][3];
    int countPlayer;
    int countRound;
    char info[50];
    char result[50];
    sem_t semaphore;
};

struct PlayerInfo {
    int shmid;
    char sign;
};

struct PlayerInfo getPlayerInfo(key_t key[2], int n_key);
void initializeBoard(char (*board)[3]);
void showBoard(char (*board)[3]);
int updateBoard(char (*board)[3], char sg, int countRound);
void checkWinner(char (*board)[3], char sg, char *result);
int playTicTacToe(int shmid, char sg);
int createSharedMemory(key_t key, struct BoardInfo **boardInfo, int *shmid);

int main() {
    struct BoardInfo *boardInfo[2];
    struct PlayerInfo info;

    int i, j;
    
    int round = 3;
    int players = 8;
    int n_key = players/2;

    pid_t pid;
    key_t key[n_key];
    int shmid[n_key];

    for (i = 0; i < n_key; i++) {
        key[i] = i + 5555;
        if (createSharedMemory(key[i], &boardInfo[i], &shmid[i]) < 0) {
            perror("createSharedMemory");
            exit(1);
        }
    }

    for (i = 0; i < players; i++) {
        pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
            exit(-1);
        } else if (pid == 0) {
            int i, result;
            struct BoardInfo *shm;

            for (i = 0; i < round; i++) {

                info = getPlayerInfo(key, n_key);
                // printf("[%d][%d]: board[%d], sign[%c] \n", getppid(), getpid(), info.shmid, info.sign);

                result = playTicTacToe(info.shmid, info.sign);
            
                // printf("%d %d\n", result, getpid());
                if (info.shmid == -1) {
                    fprintf(stderr, "[%d] No available player.\n", getpid());
                    exit(1);
                }
                if (result) {
                    // printf("Im left %d \n", getpid());
                    exit(0);
                }

                sleep(3);
            }
            exit(0);
        }
    }

    if (pid != 0) {
        int currentPlayers = players;

        for (j = 0; j < round; j++) {
            printf("\nROUND: %d -----------------------\n", j+1);
            for (i = 0; i < currentPlayers/2; i++) {
                struct BoardInfo *shm;
                if ((shm = (struct BoardInfo *)shmat(shmid[i], NULL, 0)) == (struct BoardInfo *)-1) {
                    perror("shmat");
                    exit(1);
                }
                while (shm->result[0] == '\0') {
                    sleep(1);
                }
                printf("\n+%s \n", shm->info);
                printf("%s \n", shm->result);
                // showBoard(shm->board);
                shmctl(shmid[i], IPC_RMID, NULL);
            }
            for (int i = 0; i < currentPlayers/2; i++) {
                wait(NULL);
            }

            for (i = 0; i < currentPlayers/4; i++) {
                key[i] = i + 5555;
                if (createSharedMemory(key[i], &boardInfo[i], &shmid[i]) < 0) {
                    perror("createSharedMemory");
                    exit(1);
                }
            }

            currentPlayers = currentPlayers/2;
        }
        exit(0);
    }
}

int createSharedMemory(key_t key, struct BoardInfo **boardInfo, int *shmid) {
    *shmid = shmget(key, SHMSZ, IPC_CREAT | 0666);
    if (*shmid < 0) {
        perror("shmget");
        return -1;
    }

    *boardInfo = (struct BoardInfo *)shmat(*shmid, NULL, 0);
    if (*boardInfo == (struct BoardInfo *)-1) {
        perror("shmat");
        return -1;
    }

    initializeBoard((*boardInfo)->board);
    (*boardInfo)->countPlayer = 0;
    (*boardInfo)->countRound = 0;
    sprintf((*boardInfo)->info, "Board[%d] => ", *shmid);
    strcpy((*boardInfo)->result, "");
    sem_init(&((*boardInfo)->semaphore), 1, 1);

    return 0;
}

struct PlayerInfo getPlayerInfo(key_t key[2], int n_key)
{
    int i, shmid;
    key_t keys;
    struct PlayerInfo info;
    struct BoardInfo *shm;

    for (i = 0; i < n_key; i++) {
        keys = key[i];
        if ((shmid = shmget(keys, SHMSZ, 0666)) < 0) {
            perror("shmget");
            exit(1);
        }

        if ((shm = (struct BoardInfo *)shmat(shmid, NULL, 0)) == (struct BoardInfo *)-1) {
            perror("shmat");
            exit(1);
        }

        sem_wait(&shm->semaphore);

        if (shm->countPlayer < 2) {
            shm->countPlayer++;

            if (shm->countPlayer == 1) {
                info.shmid = shmid;
                info.sign = 'X';
                snprintf(shm->info + strlen(shm->info), sizeof(shm->info) - strlen(shm->info), "PID:%d[%c]", getpid(), info.sign);
            } else {
                info.shmid = shmid;
                info.sign = 'O';
                snprintf(shm->info + strlen(shm->info), sizeof(shm->info) - strlen(shm->info), " vs PID:%d[%c]", getpid(), info.sign);
            }
            sem_post(&shm->semaphore);
            return info;
        }

        sem_post(&shm->semaphore);
    }
    info.shmid = -1;
    return info;
}

void initializeBoard(char (*board)[3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

void showBoard(char (*board)[3]) {
    printf("\n\n\t  ");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%c", board[i][j]);
            if (j < 2) {
                printf("  |  ");
            }
        }
        if (i < 2)
            printf("\n\t----------------\n\t  ");
    }
    printf("\n\n\n");
}

int updateBoard(char (*board)[3], char sg, int countRound) {
    srand(time(NULL));

    int isValid = 1;

    while (isValid) {
        int row = (rand() % 9) / 3;
        int col = (rand() % 9) % 3;
        if (board[row][col] == ' ') {
            board[row][col] = sg;
            isValid = 0;
            countRound++;
        }
    }
    return 1;
}

void checkWinner(char (*board)[3], char sg, char *result) {
    if (board[0][0] == sg && board[0][1] == sg && board[0][2] == sg ||
        board[1][0] == sg && board[1][1] == sg && board[1][2] == sg ||
        board[2][0] == sg && board[2][1] == sg && board[2][2] == sg) {
        sprintf(result, "Process [%d][%c] win", getpid(), sg);
    } else if (board[0][0] == sg && board[1][0] == sg && board[2][0] == sg ||
               board[0][1] == sg && board[1][1] == sg && board[2][1] == sg ||
               board[0][2] == sg && board[1][2] == sg && board[2][2] == sg) {
        sprintf(result, "Process [%d][%c] win", getpid(), sg);
    } else if (board[0][0] == sg && board[1][1] == sg && board[2][2] == sg ||
               board[0][2] == sg && board[1][1] == sg && board[2][0] == sg) {
        sprintf(result, "Process [%d][%c] win", getpid(), sg);
    }
    return;
}

int playTicTacToe(int shmid, char sg) {
    struct BoardInfo *shm;

    if ((shm = (struct BoardInfo *)shmat(shmid, NULL, 0)) == (struct BoardInfo *)-1) {
        perror("shmat");
        exit(1);
    }

    while (shm->result[0] == '\0') {
        sleep(1);
        sem_wait(&shm->semaphore);
        if (shm->result[0] != '\0') {
            sem_post(&shm->semaphore);
            return 1;
        }
        if (shm->countRound < 9) {
            updateBoard(shm->board, sg, shm->countRound);
            checkWinner(shm->board, sg, shm->result);
            if (shm->result[0] != '\0') {
                sem_post(&shm->semaphore);
                return 0;
            }
        } else {
            initializeBoard(shm->board);
            shm->countRound = 0;
        }
        sem_post(&shm->semaphore);
    }
    return 1;
}