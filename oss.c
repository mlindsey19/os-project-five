
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include "procComm.h"


static void increment();
static void nextProcTime();
static void generateProc();
static void communication();
static void sigHandle(int);
static void childHandle(int);
static void sendMSG( pid_t );
static int getIndexFromPid(int);
static void createResources();
static void checkMSG();
static void giveResources();


static pid_t pids[ PLIMIT ];
static int processLimit = 4;
static int activeLimit = 4;
static int active = 0;
static int total = 0;
static int maxTimeBetweenNewProcsSecs = 5;
static int maxTimeBetweenNewProcsNS = BILLION;
static void cleanSHM();
void childrenStatus();


static char * clockaddr;
static char * resdescpaddr;
static char * msgqueAaddr;
static char * msgqueGaddr;
//static char output[BUFF_sz] = "input.txt";
static SimClock gentime;
const char user[] = "user";
const char path[] = "./user";

//communication
static SimClock * simClock;
static ResDesc * resDesc;
static MsgQue * msgQueG;
static MsgQue * msgQueA;
static sem_t * semMsgA;
static sem_t * semMsgG;
static int spinlock;

// vectors
static int allocateVector [ 20 ];
static int availableVector[ 20 ];
static int maxVector[ 20 ];
static int requests[ 20 ][ 20 ];
static int allocateMartix[ 20 ][ 20 ];


//slowdown to prevent starvation
static unsigned int slowdown;


int main() {
    total = 0;
    slowdown = 0;
    signal( SIGINT, sigHandle );
    signal( SIGCHLD, childHandle );
    signal( SIGALRM, sigHandle );
    // ualarm(900000,0);
    alarm(7);
    communication();
    createResources();

    nextProcTime();
    while(1){
        if( total == processLimit && active == 0)
            break;
        increment();
        if ( ( active < activeLimit ) && ( total < processLimit ) &&
             ( simClock->sec >= gentime.sec && simClock->ns > gentime.ns ) ) {

            generateProc();
            printf("p -proc gen\n");
        }
        checkMSG();

        //       childrenStatus();
        giveResources();

    }

    cleanSHM();
    return 0;
}
static void slowDown(){
    slowdown = ( slowdown > 30000 ) ? 0 : slowdown;
}
static void printReqArray() {
    int i, j;
    for (i = 0; i < 20; i++) {
        printf("%i -> ", pids[i]);

        for (j = 0; j < 20; j++) {
            printf("%i ", requests[i][j]);
        }
        printf("\n");
    }
}
static void deleteMemory() {
    deleteClockMem(clockaddr);
    deleteResDesripMem(resdescpaddr);
    deleteSem();
    deleteMsgQueAMem(msgqueAaddr);
    deleteMsgQueGMem(msgqueGaddr);
}
static void increment(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec );    //simClock->sec++;
    simClock->ns += rand() % 100000000;
    if (simClock->ns > BILLION ){
        simClock->sec++;
        simClock->ns -= BILLION;
        assert( simClock->ns <= BILLION && "too many nanoseconds");
    }


}

static void sigHandle( int cc ){
    cleanSHM();
}
static void childHandle( int cc ){
    childrenStatus();
}
void cleanSHM(){
    int i;
    for (i =0; i < processLimit; i++) {
        if (pids[i] > 0) {

            kill(pids[i], SIGTERM);
        }
    }
    deleteMemory();
    //   fclose(ofpt);

}
void childrenStatus() {
    pid_t pid;
    int i, status;
    for (i = 0; i < processLimit; i++) {
        if (pids[i] > 0) {
            printf("p - checking children\n");
            pid = waitpid(pids[i], &status, WNOHANG);
            if ( WIFEXITED(status) && WEXITSTATUS(status) == 808 ) {
                pids[i] = 0;
                printf( "term pid:%u\n", pid );
                active--;
            }
        }
    }
}

static void nextProcTime(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec  );
    int ss = maxTimeBetweenNewProcsSecs * BILLION + maxTimeBetweenNewProcsNS;
    SimClock x;
    int dx = (rand() % 50) + 50 ;
    dx = dx * ( ss / 100 );
    x.sec = ( dx / BILLION ) + gentime.sec;
    x.ns = ( dx % BILLION ) + gentime.ns;
    if (x.ns > BILLION ){
        x.sec++;
        x.ns -= BILLION;
        assert( x.ns <= BILLION && "x - too many nanoseconds");
    }
    printf("OSS: process index %i may generate after %is %ins\n",total, x.sec, x.ns);
    gentime.sec =  x.sec;
    gentime.ns = x.ns;
}

static void generateProc() {
    if ( ( pids[ total ] = fork() ) < 0)
        perror("error forking new process");
    if ( pids[ total ] == 0 )
        execl( path, user, NULL );
    active++;
    total++;
    nextProcTime();

}
static void communication(){
    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;
    simClock->sec = 0;
    simClock->ns = 0;


    resdescpaddr = getResDescMem();
    resDesc = ( ResDesc * ) resdescpaddr;

    semMsgG = openSemResDesc();
    semMsgA = openSemAloRes();

    msgqueGaddr = getMsgQueGMem();
    msgQueG = ( MsgQue *) msgqueGaddr;

    msgqueAaddr = getMsgQueAMem();
    msgQueA = ( MsgQue *) msgqueAaddr;

}
static int getIndexFromPid(int pid){
    int i;
    for ( i = 0; i < PLIMIT; i++)
        if ( pid == pids[ i ] )
            return i;
    return 0;
}
static void clearallovec(){
    int i;
    for ( i = 0 ; i < 20; i++ ){
        allocateVector[i] = 0;
    }
}

static void sendMSG( pid_t pid ){
    //enter critical
    //   printf("p - try enter crit to send\n");
    if (!sem_trywait(semMsgA)) {

        char buf[BUFF_sz - 1];
        memset(buf, 0, BUFF_sz - 1);
        int i;
        for (i = 0; i < 20; i++) {
            sprintf(buf, "%s%d ", buf, allocateVector[i]);
        }

        //       printf("p    -    enter crit to send\n");

        for (i = 0; i < MAX_MSGS; i++) {
            if (msgQueA[i].hasBeenRead) {
                memset(msgQueA[i].buf, 0, BUFF_sz);
                strcpy(msgQueA[i].buf, buf);
                msgQueA[i].hasBeenRead = 0; // has not been read
                msgQueA[i].rra = 0;
                msgQueA[i].pid = pid;
                clearallovec();
                break;
            }
        }
        printf("p - sig %u\n", pid);
        kill(pid,SIGUSR1);

        sem_post(semMsgA);

        //   printf("p - leave crit to send\n");

        //leave critical
        printf("Parent: Send message...%s \n", buf);
    }
}


static void createResources(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec  );

    int i;
    for ( i =0; i <20 ; i++){
        resDesc[i].total = (rand() % 9) + 1;
        maxVector[i] = resDesc[i].total;
        availableVector[i] = maxVector[i];
        assert(resDesc[i].total >= 0 && resDesc[i].total <= 10 && "created too many resources");
        int p = 2000;
        int dart = ( rand() % 99999 ) + 1; //2000/10000 -> 20% with reasonable range
        resDesc[ i ].sharable = ( dart < p ) ? 1 : 0; //1->true
    }
    //set all msg to empty
    for ( i = 0; i < MAX_MSGS; i ++){
        msgQueA[i].hasBeenRead = 1;
        msgQueG[i].hasBeenRead = 1;
    }



}
static void adjustSharable(){
    int i;
    for ( i = 0; i < 20; i++  ) {
        if (resDesc[i].sharable)
            availableVector[i] = maxVector[i];
    }
}
static void appendAvailableVector(char * buf){
    int temp;
    int i;
    for ( i = 0;i < 20; i++ ){
        sscanf(buf, "%d %[^\n]", &temp, buf);
        availableVector[ i ] += temp;
    }
    adjustSharable();
    //   printReqArray();
}
static void appendRequestVector(char * buf, pid_t pid){
    int temp;
    int pi = getIndexFromPid(pid);
    int i;
    for ( i = 0;i < 20; i++ ){
        sscanf(buf, "%d %[^\n]", &temp, buf);
        requests[ pi ][i] += temp;
    }
}
static void checkMSG(){
    //enter critical

//    printf("p         - try enter crit to check\n");
    if (!sem_trywait(semMsgG)) {
        int i;
        char buf[BUFF_sz];
        memset(buf, 0, BUFF_sz);
        adjustSharable();


        //   printf("p- enter crit to check\n");
        for (i = 0; i < MAX_MSGS; i++) {
            if (msgQueG[i].rra && !msgQueG[i].hasBeenRead) {
                strcpy(buf, msgQueG[i].buf);
                if (msgQueG[i].rra - 1) {  //1-> release 0->request
                    printf("released : %s\n", buf);
                    msgQueG[i].hasBeenRead = 1; //true
                    appendAvailableVector(buf);
                } else {
                    printf("request : %s\n", buf);
                    msgQueG[i].hasBeenRead = 1; //true
                    appendRequestVector(buf, msgQueG[i].pid);
                }
            }
        }
//        printf("p- leave crit to check\n");

        sem_post(semMsgG);
    }//leave critical
}

static int isRequestAvailable( int i ){
    //check requests vs available
//    printf("p- checking if res avail \n");
    int j, a, q,z;
    a = 1;
    for( j = q  = z = 0 ; j < 20 ; j++ ) {
        q += requests[i][j];
        z += allocateMartix[i][j];
        if (requests[i][j] > availableVector[j]) {
            a = 0;
            break;
        }
    }

    a = ( q ) ? a  : q ;//if q is 0 a<-0
    a = ( q + z ) ? 1 : a;
    // if (!a)
    //      printf("not avail : %i, %i\n", a, q);
    return a;
}

static void giveResources(){
    int i;
    for ( i =0 ; i < total ; i++){
        if ( isRequestAvailable( i ) ){
            int j;
            for ( j =0; j < 20; j++ )
                allocateVector[j] = requests[i][j];
            sendMSG( pids[i] );
            for ( j =0; j < 20; j++ ) {
                requests[i][j] = 0;
                availableVector[j] -= allocateVector[j];
                allocateMartix[i][j] = allocateVector[j];
            }
            adjustSharable();
            break;
        }
    }
}






