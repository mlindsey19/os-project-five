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

#define PLIMIT 20
#define BILLION 1000000000
#define BUFF_sz 32
#define KEY_PATH "/tmp"
#define SEM_RD "/sem_rd"

#define QUEUE_REL  "/release_queue"
#define QUEUE_AL  "/allocate_queue"



#define PERM 0755




mqd_t openQueRel( int isParent ) ;
mqd_t openQueAl( int isParent ) ;

sem_t * openSem();

char * getClockMem();
void deleteClockMem( char * paddr );

char * getResDescMem();
void deleteResDesripMem( char * paddr );
void deleteSemAndQue() ;

#endif //PROJECT_FIVE_PROCCOMM_H
