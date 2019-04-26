
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include "procComm.h"


static void communication();
static void getMSG();
static void sendMSG( int );
static void initUserParams();
void askForMore();
void giveUpSome();
static void nextResTime();
static void incrAlive();
static void giveAllBack( );


char * clockaddr;
char * resdescpaddr;
char * msgqueaddr;

//communication
static SimClock * simClock;
static ResDesc * resDesc;
static sem_t * semDesc;
static sem_t * semMsg;
static MsgQue * msgQue;
sigset_t sigset;
int sig;



// resource vectors
static int maxRequestVector[ 20 ];
static int requestVector[ 20 ];
static int acquiredVector[ 20 ];
static int releaseVector [ 20 ];
//other params
static int requestOrReleaseRate;
SimClock nextRes;
SimClock minTimeAlive;
void sigHandle(int );



int main() {
    signal(SIGUSR1, sigHandle);
    communication();
    initUserParams();
    nextResTime();
    printf("hi %i\n", getpid());
    askForMore();
    while (1) {

        if ( simClock->sec >= nextRes.sec && simClock->ns > nextRes.ns ){
            if( rand()%2 ) {
                askForMore();
                sigwait(&sigset, &sig);
            }else
                giveUpSome();
        }
        if (simClock->sec >= minTimeAlive.sec && simClock->ns > minTimeAlive.ns) {
            if( rand() % 2 )
                incrAlive();
            else
                break;
        }

    }

    giveAllBack();
    printf("bye %i\n", getpid());
    exit(808);
}
static void incrAlive(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec );    //simClock->sec++;
    minTimeAlive.ns += BILLION / 4;
    if (minTimeAlive.ns > BILLION ){
        minTimeAlive.sec++;
        minTimeAlive.ns -= BILLION;
        assert( simClock->ns <= BILLION && "too many nanoseconds");
    }
}
void sigHandle(int cc){
    //   printf("c - sig\n");
    getMSG();
}

static void initUserParams(){

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t )ts.tv_nsec ^ getpid()  );
    int i, max;
    for ( i = 0; i <20 ; i++ ) {
        max = ( resDesc[ i ].total * 65 ) / 100;
        maxRequestVector[ i ] = ( max ) ? ( rand() % max ) + 1 : max;
        acquiredVector[ i ] = 0;
    }
    requestOrReleaseRate = rand() % ( BILLION / 10 );

    minTimeAlive.sec = ( ( rand() % 10 ) + 1 ) +  simClock->sec;
    minTimeAlive.ns =  ( rand() % BILLION )  + simClock->ns;
    printf("%i - %is %ins \n ",getpid(), minTimeAlive.sec, minTimeAlive.ns  );

    nextRes.sec = simClock->sec;
    nextRes.ns = simClock->ns;

}
static void communication(){
    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;

    resdescpaddr = getResDescMem();
    resDesc = ( ResDesc * ) resdescpaddr;

    semDesc = openSemResDesc();
    semMsg = openSemAloRes();

    msgqueaddr = getMsgQueMem();
    msgQue = ( MsgQue *) msgqueaddr;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);


}
static void appendAcquiredVector(char * buf){
    int temp;
    int i;
    for ( i = 0;i < 20; i++ ){
        sscanf(buf, "%d %[^\n]", &temp, buf);
        acquiredVector[ i ] = temp;
        //    printf("%i-\n s %s", temp, buf);
    }
}

static void getMSG() {
    //enter critical
    //   printf("c - try enter crit to get\n");
    if( !sem_trywait(semMsg)){

        int i, f;
        char buf[BUFF_sz];
        memset(buf, 0, BUFF_sz);
        pid_t pid = getpid();


        //       printf("c - enter crit to get\n");
        f = 0;
        for (i = 0; i < MAX_MSGS; i++) {
            //         printf("c - rra: %i - mpid %i hsb %i %s\n", msgQue[i].rra,
            //                msgQue[i].pid, msgQue[i].hasBeenRead, buf);

            if (!msgQue[i].rra && msgQue[i].pid == pid && !msgQue[i].hasBeenRead) {
                strncpy(buf, msgQue[i].buf, (BUFF_sz - 1));
                //            printf("child: Received message: %s\n", buf);
                msgQue[i].hasBeenRead = 1;//true
                f = 1;
                break;
            }
        }
        sem_post(semMsg);
        //   printf("c - leave crit to get\n");

        //leave critical
        if (f)
            appendAcquiredVector(buf);
    }
}
static void sendMSG( int fl ){   // fl 1-> release 0->request
    //enter critical
    //   printf("c - try enter crit to send\n");
    if (! sem_trywait(semMsg) ){

        char buf[BUFF_sz];
        memset(buf, 0, BUFF_sz);

        int i;
        for (i = 0; i < 20; i++) {
            if (fl)
                sprintf(buf, "%s%d ", buf, releaseVector[i]);
            else
                sprintf(buf, "%s%d ", buf, requestVector[i]);
        }
        //     printf("c - enter crit to send\n");

        for (i = 0; i < MAX_MSGS; i++) {
            if (msgQue[i].hasBeenRead) {
                memset(msgQue[i].buf, 0, BUFF_sz);
                strcpy(msgQue[i].buf, buf);
                msgQue[i].hasBeenRead = 0; // has not been read
                msgQue[i].rra = 1 + fl;
                msgQue[i].pid = getpid();
                break;
            }
        }
        //   printf("child %i: Send message..  %s - fl %i\n", getpid(), buf,fl);

        sem_post(semMsg);
        //   printf("c - leave crit to send\n");

        //leave critical

    }

}
void askForMore(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec ^ getpid()  );
    int max,i;
    for ( i = 0; i <20 ; i++){
        max = maxRequestVector[ i ] - acquiredVector[ i ];
        requestVector[ i ] = ( max ) ? ( rand() % max ) + 1 : max ;  // if max is 0 set to max
    }
    sendMSG( 0 );
}
void giveUpSome(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec ^ getpid()  );
    int max,i;
    for ( i = 0; i <20 ; i++){
        max = acquiredVector[ i ];
        releaseVector[ i ] = ( max ) ? ( rand() % max ) + 1 : max ;  // if max is 0 set to max
    }
    sendMSG( 1 );
}
static void nextResTime(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec  ^ getpid() );
    int ss = requestOrReleaseRate;
    SimClock x;
    int dx = (rand() % 50) + 50 ;
    dx = dx * ( ss / 100 );
    x.sec = ( dx / BILLION ) + nextRes.sec;
    x.ns = ( dx % BILLION ) + nextRes.ns;
    if (x.ns > BILLION ){
        x.sec++;
        x.ns -= BILLION;
        assert( x.ns <= BILLION && "x - too many nanoseconds");
    }
    //  printf("child %i give or ask at  %is %ins\n", getpid(), x.sec, x.ns);

    nextRes =  x;
}
static void giveAllBack( ){
    //enter critical
    //   printf("c - try enter crit to send all\n");

    if ( !sem_trywait( semMsg ) ) {
        char buf[ BUFF_sz  ];
        memset(buf, 0, BUFF_sz);

        int i;
        for ( i =0 ; i < 20; i++ )
            sprintf( buf, "%s%d ", buf , acquiredVector[ i ] );


//        printf("c - enter crit to send all\n");

        for (i = 0; i < MAX_MSGS; i++) {
            if (msgQue[i].hasBeenRead) {
                memset(msgQue[i].buf, 0, BUFF_sz);
                strncpy(msgQue[i].buf, buf, (BUFF_sz - 1));
                //           msgQue[i].buf[ BUFF_sz -1 ] = 0;
                msgQue[i].hasBeenRead = 0; // has not been read
                msgQue[i].rra = 2;
                msgQue[i].pid = getpid();
                break;
            }
        }
        sem_post(semMsg);
        //       printf("c - leave crit to send all\n");

        //leave critical

        printf("child %i: Send all back..  %s \n", getpid(), buf);

    }
}
