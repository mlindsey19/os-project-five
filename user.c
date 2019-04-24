
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include "procComm.h"


static void communication();
static void getMSG();


char * clockaddr;
char * resdescpaddr;

//communication
SimClock * simClock;
ResDesc * resDesc;
sem_t * editResDesc;
mqd_t mq_allocate;
mqd_t mq_realsed;



int main() {

    communication();
    sleep(1);
    getMSG();


    return 0;
}

void sigHandle(int cc){

}

static void communication(){
    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;
    simClock->sec = 0;
    simClock->ns = 0;

    resdescpaddr = getResDescMem();
    resDesc = ( ResDesc * ) resdescpaddr;

    editResDesc = openSem();

    mq_allocate = openQueAl( 0 ); // 0 -> not parent
    mq_realsed = openQueRel( 0 ); // 0 -> not parent

}


static void getMSG(){
    char buf[ BUFF_sz + 1 ];
    ssize_t bytes_read;
    memset(buf, 0x00, sizeof(buf));
    bytes_read = mq_receive(mq_allocate, buf, BUFF_sz, NULL);
    if(bytes_read >= 0) {
        printf("child: Received message: %s\n", buf);
    } else {
        printf("child: None \n");
    }

    fflush(stdout);
}