//
// Created by mlind on 4/23/2019.
//

#include "procComm.h"
 enum shmkeyID{
    clockshmkey = 'a',
    resdescshmkey = 'r',
    msgqueshmkey = 'm'
};
char * getMsgQueMem(){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , msgqueshmkey)) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    char * paddr;
    int shmid = shmget ( shmkey, MAX_MSGS * BUFF_msgque, PERM | IPC_CREAT );

    if ( shmid == -1 )
        perror( "msgque get mem - error shmid" );

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}
void deleteMsgQueMem( char * paddr ){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , msgqueshmkey)) == (key_t) -1) {
        perror("IPC error: ftok");
    }

    int shmid = shmget ( shmkey, MAX_MSGS * BUFF_msgque, PERM );

    shmctl(shmid, IPC_RMID, NULL);

    if(  shmdt( paddr )  == -1 ){
        perror("err shmdt clock");
    }
}

sem_t * openSemResDesc(){
    sem_t * sem = sem_open(SEM_RD, O_CREAT, PERM, 1);
    return sem;
}
sem_t * openSemAloRes(){
    sem_t * sem = sem_open(SEM_MSG, O_CREAT, PERM, 1);
    return sem;
}

char * getClockMem(){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , clockshmkey)) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    char * paddr;
    int shmid = shmget ( shmkey, BUFF_clock, PERM | IPC_CREAT );

    if ( shmid == -1 )
        perror( "get clock mem - error shmid" );

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}
void deleteClockMem( char * paddr ){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , clockshmkey)) == (key_t) -1) {
        perror("IPC error: ftok");
    }

    int shmid = shmget ( shmkey, BUFF_clock, PERM );


    if(  shmdt( paddr )  == -1 ){
        perror("err shmdt clock");
    }
    shmctl(shmid, IPC_RMID, NULL);

}
char * getResDescMem(){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , resdescshmkey ) ) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    char * paddr;
    int shmid = shmget ( shmkey, 20 * BUFF_resdesc, PERM | IPC_CREAT );
    if ( shmid == -1 )
        perror( "error shmid  res desc" );

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}
void deleteResDesripMem( char * paddr ){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , resdescshmkey ) ) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    int shmid = shmget ( shmkey, BUFF_resdesc, PERM  );

    shmctl(shmid, IPC_RMID, NULL);

    if(  shmdt( paddr )  == -1 ){
        perror("err shmdt clock");
    }
}
void deleteSem() {

    sem_unlink(SEM_RD);
    sem_unlink(SEM_MSG);

    // unlink message ques

}