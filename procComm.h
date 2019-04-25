//
// Created by mlind on 4/23/2019.
//

#ifndef PROJECT_FIVE_PROCCOMM_H
#define PROJECT_FIVE_PROCCOMM_H
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <stdio.h>

#define BUFF_sz 32

typedef struct {
    unsigned int sec;
    unsigned int ns;
}SimClock;
#define BUFF_clock sizeof( SimClock )

typedef struct {
    int requests;
    int allocated; //total (including special)
    int specAlloc;
    int released;
    int sharable;
    int total;
}ResDesc;
#define BUFF_resdesc sizeof( ResDesc )

typedef struct {
    int ra; // release/allocate 1/0
    int hasBeenRead; // 1 -> yes and ready to be replaced
    pid_t pid; // for send to specific child
    char buf[BUFF_sz];
}MsgQue;
#define BUFF_msgque sizeof( MsgQue )

#define PLIMIT 20
#define BILLION 1000000000
#define KEY_PATH "/tmp"
#define SEM_RD "/sem_rd"
#define SEM_MSG "/sem_msg"



#define PERM 0755




void deleteMsgQueMem( char * paddr );
char * getMsgQueMem();

sem_t * openSemAloRes();
sem_t * openSemResDesc();

char * getClockMem();
void deleteClockMem( char * paddr );

char * getResDescMem();
void deleteResDesripMem( char * paddr );
void deleteSem() ;

#endif //PROJECT_FIVE_PROCCOMM_H
