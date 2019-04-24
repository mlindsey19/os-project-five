
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include "procComm.h"


static void increment();
static SimClock nextProcTime();
static void generateProc();
static void communication();
void sigHandle(int);

int processLimit = 1;
int activeLimit = 1;
int active = 0;
int total = 0;
int maxTimeBetweenNewProcsSecs = 1;
int maxTimeBetweenNewProcsNS = 0;
void cleanSHM();



char * clockaddr;
char * resdescpaddr;
char output[BUFF_sz] = "input.txt";
SimClock gentime;
pid_t pids[PLIMIT];
const char user[] = "user";
const char path[] = "./user";

//communication
SimClock * simClock;
ResDesc * resDesc;
sem_t * editResDesc;
mqd_t mq_allocate;
mqd_t mq_realsed;



int main() {
    signal( SIGINT, sigHandle );
    signal( SIGALRM, sigHandle );
    alarm(3);
    communication();


    while(1){
        if( total == processLimit && active == 0)
            break;
        increment();
        if( total == 0)
            generateProc();

    }



    return 0;
}

static void deleteMemory() {
    deleteClockMem(clockaddr);
    deleteResDesripMem(resdescpaddr);
    deleteSemAndQue();
}
static void increment(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec );    //simClock->sec++;
    simClock->ns += rand() % 100000;
    if (simClock->ns > BILLION ){
        simClock->sec++;
        simClock->ns -= BILLION;
        assert( simClock->ns <= BILLION && "too many nanoseconds");
    }
}

void sigHandle(int cc){
    cleanSHM();
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
void sigChild() {
    pid_t pid;
    int i, status;
    for (i = 0; i < processLimit; i++) {
        if (pids[i] > 0) {
            pid = waitpid(pids[i], &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 808 && pid != 0 ) {
                pids[i] = 0;
                printf( "term pid:%u\n", pid );
                active--;
            }
        }
    }
}

static SimClock nextProcTime(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand( (time_t)ts.tv_nsec  );
    int ss = maxTimeBetweenNewProcsSecs * BILLION + maxTimeBetweenNewProcsNS;
    SimClock x;
    int dx = rand() % ss;
    x.sec = ( dx / BILLION ) + gentime.sec;
    x.ns = ( dx % BILLION ) + gentime.ns;
    if (x.ns > BILLION ){
        x.sec++;
        x.ns -= BILLION;
        assert( x.ns <= BILLION && "x - too many nanoseconds");
    }
    printf("OSS: process index %i may generate after %is %ins\n",total, x.sec, x.ns);
    return x;
}

static void generateProc() {
    if ( ( pids[ total ] = fork() ) < 0)
        perror("error forking new process");
    if ( pids[ total ] == 0 )
        execl( path, user, NULL );
    active++, total++;

}
static void makeResources(){

}

static void communication(){
    clockaddr = getClockMem();
    simClock = ( SimClock * ) clockaddr;
    simClock->sec = 0;
    simClock->ns = 0;

    resdescpaddr = getResDescMem();
    resDesc = ( ResDesc * ) resdescpaddr;
    makeResources();

    editResDesc = openSem();

    mq_allocate = openQueAl( 1 ); // 1 for is parent
    mq_realsed = openQueRel( 1 ); // 1 for is parent

}

void sendMSG(){
    char buf[ BUFF_sz  ];

    snprintf(buf, BUFF_sz, "MESSAGE %d",getpid() );

    printf("Parent: Send message... \n");
    mq_send(mq_allocate, buf, BUFF_sz, 0);

    fflush(stdout);
}