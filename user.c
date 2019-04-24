
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
char * msgqueaddr;

//communication
SimClock * simClock;
ResDesc * resDesc;
sem_t * editResDesc;


MsgQue * msgQue;


int main() {
printf("hi\n");
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

    msgqueaddr = getMsgQueMem();
    msgQue = ( MsgQue *) msgqueaddr;


}


static void getMSG(){
    char buf[ BUFF_sz ];
    ssize_t bytes_read;
    memset(buf, 0, BUFF_sz);
    strcpy(buf, msgQue->buf);
    printf("child: Received message: %s\n", buf);


//    fflush(stdout);
}