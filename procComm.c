//
// Created by mlind on 4/23/2019.
//

#include "procComm.h"
 enum shmkeyID{
    clcokshmkey = 'a',
    resdesc = 'r'
};

mqd_t openQueRel( int isParent ) {

    static struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 50;
    attr.mq_msgsize = BUFF_sz;
    attr.mq_curmsgs = 0;
    mqd_t mq;
    if( isParent )
        mq = mq_open(QUEUE_REL, O_CREAT | O_RDONLY | O_NONBLOCK, PERM, &attr);
    else
        mq = mq_open(QUEUE_REL, O_WRONLY );
    return mq;
}
mqd_t openQueAl( int isParent ) {

    static struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 50;
    attr.mq_msgsize = BUFF_sz;
    attr.mq_curmsgs = 0;
    mqd_t mq;
    if( isParent )
        mq = mq_open(QUEUE_AL, O_CREAT | O_WRONLY | O_NONBLOCK, PERM, &attr);
    else
        mq = mq_open(QUEUE_AL,  O_RDONLY ) ;
    return mq;
}

sem_t * openSem(){
    sem_t * sem = sem_open(SEM_RD, O_CREAT, PERM, 1);
    return sem;
}
char * getClockMem(){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , clcokshmkey)) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    char * paddr;
    int shmid = shmget ( shmkey, BUFF_clock, PERM | IPC_CREAT );

    if ( shmid == -1 )
        perror( "parent - error shmid" );

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}
void deleteClockMem( char * paddr ){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , clcokshmkey)) == (key_t) -1) {
        perror("IPC error: ftok");
    }

    int shmid = shmget ( shmkey, BUFF_clock, PERM );

    shmctl(shmid, IPC_RMID, NULL);

    if(  shmdt( paddr )  == -1 ){
        perror("err shmdt clock");
    }
}
char * getResDescMem(){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , resdesc ) ) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    char * paddr;
    int shmid = shmget ( shmkey, 20 * BUFF_resdesc, PERM | IPC_CREAT );

    if ( shmid == -1 )
        perror( " error shmid  res desc" );

    paddr = ( char * ) ( shmat ( shmid, 0,0));

    return paddr;
}
void deleteResDesripMem( char * paddr ){
    key_t shmkey;

    if ((shmkey = ftok( KEY_PATH , resdesc ) ) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    int shmid = shmget ( shmkey, BUFF_resdesc, PERM  );

    shmctl(shmid, IPC_RMID, NULL);

    if(  shmdt( paddr )  == -1 ){
        perror("err shmdt clock");
    }
}
void deleteSemAndQue() {


    sem_unlink(SEM_RD);

    // unlink message ques
    mq_unlink(QUEUE_REL);
    mq_unlink(QUEUE_AL);

}