
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
static void sendMSG( pid_t, int );
static int getIndexFromPid(int);
static void createResources();
static void checkMSG();
static void giveResources();
static void deadlock();

static pid_t pids[ PLIMIT ];
static int processLimit = 20;
static int activeLimit = 18;
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

// vectors
static int aboutToAllocate [ 20 ];
static int availableVector[ 20 ];
static int maxVector[ 20 ];
static int requests[ 20 ][ 20 ];
static int allocatedMatrix[ 20 ][ 20 ];


//slowdown to prevent starvation
static unsigned int slowdown;


int main() {
    total = 0;
    slowdown = 0;
    signal( SIGINT, sigHandle );
    //   signal( SIGCHLD, childHandle );
    signal( SIGALRM, sigHandle );
    // ualarm(900000,0);
      alarm(114);
    communication();
    createResources();

    nextProcTime();
    int deadLockCheck =0;
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

        childrenStatus();
        giveResources();
        if(130 <  deadLockCheck++){
            deadLockCheck = 0;
            deadlock();
        }
    }

    cleanSHM();
    return 0;
}
static void slowDown(){
    slowdown = ( slowdown > 30000 ) ? 0 : slowdown;
}
static void printReqArray() {
    int i, j;
    for (i = 0; i < processLimit; i++) {
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
    simClock->ns += rand() % BILLION;
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
    for (i = 0; i < 20; i++) {
        if(pids[i] > 0){
            pid = waitpid(pids[i], &status, WNOHANG);
            if (WIFEXITED(status)
                && pid == pids[i]) {
                i = getIndexFromPid(pid);
                pids[i] = 0;
                printf("term pid:%u  status: %i \n", pid, WEXITSTATUS(status));
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
static void adjustSharable(){
    int i;
    for ( i = 0; i < 20; i++  ) {
        if (resDesc[i].sharable)
            availableVector[i] = maxVector[i];
    }
}

static void adjustResVectors(pid_t pid){
    int i, j;
    i = getIndexFromPid(pid);
    for ( j = 0 ; j < 20; j++ ){
        allocatedMatrix[i][j] += aboutToAllocate[j];
        availableVector[j] -= aboutToAllocate[j];
        requests[i][j]  = 0 ;
        aboutToAllocate[j] = 0;

    }
    adjustSharable();
}

static void sendMSG( pid_t pid, int fl ){
    //enter critical
    //   printf("p - try enter crit to send\n");
    if(pid)
        if (!sem_trywait(semMsgA)) {

            char buf[BUFF_sz - 1];
            memset(buf, 0, BUFF_sz - 1);
            int i;

            for (i = 0; i < 20; i++) {
                sprintf(buf, "%s%d ", buf, aboutToAllocate[i]);
            }

            //       printf("p    -    enter crit to send\n");

            for (i = 0; i < MAX_MSGS; i++) {
                if (msgQueA[i].hasBeenRead) {
                    memset(msgQueA[i].buf, 0, BUFF_sz);
                    strcpy(msgQueA[i].buf, buf);
                    msgQueA[i].hasBeenRead = 0; // has not been read
                    msgQueA[i].rra =  0 - fl;
                    msgQueA[i].pid = pid;
                    if(!fl)
                    adjustResVectors(pid);
             //       printf("p - sig %u\n", pid);
               //     kill(pid,SIGUSR1);
                    break;
                }
            }


            sem_post(semMsgA);

            //   printf("p - leave crit to send\n");

            //leave critical
      //      printf("Parent: Send message...%s \n", buf);
        }
}

//static void printReqAlloAvail(pid_t pid){
//    int i, j;
//    printf("pid %i\n", pid);
//    i = getIndexFromPid(pid);
//
//    printf("req - ");
//    for ( j = 0; j < 20; j++ ) {
//        printf("%i ", requests[i][j]);
//    }
//    printf("\n");
//
//    printf("has - ");
//    for ( j = 0; j < 20; j++ ) {
//        printf("%i ", allocatedMatrix[i][j]);
//    }
//    printf("\n");
//
//    printf("available - ");
//    for ( j = 0; j < 20; j++ ) {
//        printf("%i ", availableVector[j]);
//    }
//    printf("\n");
//}

static void appendAvailableVector(char * buf, pid_t pid){
    int temp;
    int i,j;
    i = getIndexFromPid(pid);
    for ( j = 0; j < 20; j++ ){
        sscanf(buf, "%d %[^\n]", &temp, buf);
        availableVector[ j ] += temp;
        allocatedMatrix[ i ][j] -= temp;

    }
    adjustSharable();
    //   printReqArray();
}
static void appendRequestVector(char * buf, pid_t pid){
    int temp;
    int i = getIndexFromPid(pid);
    int j;
    for ( j = 0; j < 20; j++ ){
        sscanf(buf, "%d %[^\n]", &temp, buf);
        requests[ i ][j] += temp;
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
        pid_t pid;


        //   printf("p- enter crit to check\n");
        for (i = 0; i < MAX_MSGS; i++) {
            if (msgQueG[i].rra && !msgQueG[i].hasBeenRead) {
                strcpy(buf, msgQueG[i].buf);
                pid = msgQueG[i].pid;
                if (msgQueG[i].rra - 1) {  //1-> release 0->request
          //          printf("released : %s\n", buf);
                    msgQueG[i].hasBeenRead = 1; //true
                    appendAvailableVector(buf, pid);
                } else {
         //           printf("request : %s\n", buf);
                    msgQueG[i].hasBeenRead = 1; //true
                    appendRequestVector(buf, pid);
               //     printReqAlloAvail(pid);
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
    q = 0;
    for( j  = 0 ; j < 20 ; j++ ) {
        q += requests[i][j];
        if ( ( requests[i][j] + allocatedMatrix[i][j] ) > availableVector[j]) {
            a = 0;
        }
    }

    a = ( q == 0 ) ? 0  : a ;//if q is 0 a<-0
    //   a = !( q + z ) ? 1 : a;
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
                aboutToAllocate[j] = requests[i][j];
            sendMSG( pids[i] , 0);
            break;
        }
    }
}

static void deadlock(){
    int i,j, allReq, availReq,allocated;
    allReq = availReq =allocated = 0;
    for (i = 0; i< 20 ; i++){
        int isAvail = 1;
        int thisReq =0 ;
        for(j=0; j < 20; j++) {
            allReq += requests[i][j];
            thisReq += requests[i][j];
            allocated += allocatedMatrix[i][j];
            if ( ( requests[i][j] + allocatedMatrix[i][j] ) > availableVector[j]) {
                isAvail = 0;
            }
        }
        isAvail = ( thisReq) ? isAvail: thisReq;
        availReq += isAvail;
    }
    // if sum of request == 0 -> do nothing
    // if requests avail > 0 -> do nothing
    // if allocated == 0 -> do nothing
    // no deadlock
    int deadlockcheck = 0;
    if(310 <  deadlockcheck++) {
        deadlockcheck = 0;
        printf("deadlock  - ra %i - sumR %i - allo %i\n", availReq, allReq, allocated);
    }
    int indexOfMost = 0;
    int max = 0;
    if (allReq && !availReq && allocated){
        // restart proc with most rec

        for (i = 0; i < 20 ; i++) {
            int numRes = 0;
            for (j = 0; j < 20; j++) {
                numRes += allocatedMatrix[i][j];
            }
            if (max < numRes) {
                max = numRes;
                indexOfMost = i;
            }
        }
      if(max) {
         // kill(pids[indexOfMost], SIGUSR2);
          printReqArray();
          printf("**************************deadlock %i\n", pids[indexOfMost]);
          sendMSG(pids[ indexOfMost ],1);

        //  sleep(1);
      }
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
        int p = 20;
        int dart = ( rand() % 99 ) + 1; //20%
        resDesc[ i ].sharable = ( dart < p ) ? 1 : 0; //1->true
    }
    //set all msg to empty
    for ( i = 0; i < MAX_MSGS; i ++){
        msgQueA[i].hasBeenRead = 1;
        msgQueG[i].hasBeenRead = 1;
    }



}