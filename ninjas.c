#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>

#define MIN(X,Y) (((X)<(Y)) ? (X) : (Y))

//struct to give information to thread
typedef struct thrd_strct {
  int pirate; // 0 means pirate, 1 means ninja
  double muCt; //mean time in changing room
  double muAt; //mean time adventuring
} thrd_strct;

typedef struct cst_strct {
  int visits; // number of visits
  int *t_visit; // time of visit / cost
  int *t_wait; // time waiting
} cst_strct;

// global pointers to array of threads
pthread_t *pirates;
pthread_t *ninjas;

// pointers to cost structs for the threads
cst_strct *p_strcts;
cst_strct *n_strcts;

thrd_strct pstrct;
thrd_strct nstrct;

// index givers
int pcount;
int ncount;

// State variables for departments
int completion;
int pwait;
int nwait;
int indept;

//arrays for department calculations
int *available;
int *busy;
int *lastime;
int *idle;

// Semaphores
sem_t deptsem, psem, nsem, plock, nlock, wlock;

// global variable for team size
int team_size;

//normal distribution function
double gauss(double mean){
  double a = drand48();
  double b = drand48();
  double z = mean+sqrt(-2*log(a))*cos(2*M_PI*b); // normal dist
  if (z < 0) z = 0;
  return z;
}

int runThread(sem_t *my_sem, sem_t *their_sem, sem_t *mylock, thrd_strct *info,
   thrd_strct *them, char *str,int *mywait, int *twait, int *count, cst_strct *my_cst_strct){
  sem_wait(mylock);
  int index = *count;
  printf("Count %d\n",index);
  (*count)++;
  cst_strct *my_cst = &my_cst_strct[index];
  int *tv = malloc(100*sizeof(int));
  int *tw = malloc(100*sizeof(int));
  my_cst->visits = 0;
  sem_post(mylock);
  int visit = 1; // true if visiting department
  int mystaff;
  while (visit){
    struct timeval wt1, wt2, wt3;
    struct timezone tz1, tz2, tz3;
    sleep(gauss(info->muAt));// time adventuring
    gettimeofday(&wt1, &tz1);
    (*mywait)++; // Waiting
    sem_wait(my_sem); // wait until this thread type is allowed
    sem_wait(&deptsem); // Wait until thread can go into dept
    gettimeofday(&wt2, &tz2);
    int time_wait = (int)wt2.tv_sec - (int)wt1.tv_sec;
    sem_wait(&wlock);
    (*mywait)--; // Not waiting
    indept++; // one more in dept
    double tdept = gauss(info->muCt); // time waiting in dept
    for (int i = 0; i < team_size; i++){
      if (available[i]) {
        mystaff = i;
        available[i] = 0;
        break;
      }
    }
    busy[mystaff]+= (int)tdept;
    idle[mystaff]+= (int)wt2.tv_sec - lastime[mystaff];
    sem_post(&wlock);
    printf("I am a %s!\n",str);
    printf("Currently waiting: Pirates %d, Ninjas %d, in department: %d %s\n",pwait,nwait,indept,str);
    printf("Wait for %f secs\n", tdept);
    sleep(tdept); // sleep for that long
    //fill out threads cost struct
    tv[my_cst->visits] = (int)tdept;
    tw[my_cst->visits] = time_wait;
    my_cst->visits++;
    visit = drand48() < 0.25 ; // 25% chance to go back to department

    // check if department should switch types
    sem_wait(&wlock);
    int change = them->muCt*(*twait) + info->muCt*indept > 25 || *twait >= *mywait;
    if ((indept == team_size && change) || (*mywait == 0)){
      completion++; // how many threads have left
      if (completion == indept){ // wait for semaphor to hit 0
        completion = 0;
        indept = 0;
        if (visit && *mywait == 0 && *twait == 0){ // if they go again and there is no one left
          sem_post(&deptsem);
          sem_post(my_sem);
        }
        for(int i = 0; i < team_size; i++) { // increment other semaphor
          sem_post(their_sem);
          sem_post(&deptsem);
        }
      }
    }else { // Let type keep going
        indept--;
        sem_post(&deptsem);
        sem_post(my_sem);
    }
    gettimeofday(&wt3,&tz3);
    lastime[mystaff] = (int)wt3.tv_sec;
    available[mystaff] = 1;
    sem_post(&wlock);
    printf("leaving...\n");
    printf("Currently waiting: Pirates %d, Ninjas %d, in department: %d %s\n",pwait,nwait,indept,str);
    if(visit) printf("going agian hehe!\n");
  }
  my_cst->t_visit = tv;
  my_cst->t_wait  = tw;
  return 0;
}

void *thread(void *var){
  thrd_strct *info = var;
  if (info->pirate) runThread(&psem,&nsem,&plock,info,&nstrct,"pirate",&pwait,&nwait,&pcount,p_strcts);
  else runThread(&nsem,&psem,&nlock,info,&pstrct,"ninja",&nwait,&pwait,&ncount,n_strcts);
  return NULL;
}

// Main simulation
int runSim(int num_team,int num_p, int num_n, double muCpt, double muCnt,
    double muApt, double muAnt){
  // Input check
  if (num_team < 2 || num_team > 4){
    printf("%d is an invalid number of staff, must be between 2 and 4!\n", num_team);
    return -1;
  }
  if (num_p < 10 || num_n < 10 || num_p > 50 || num_n > 50){
    printf("Invalid number of threads, each type must have between 10 and 50 members!\n");
    return -1;
  }
  printf("\n\n ############### Simulation Parameters ##################\n");
  printf("staff size: %d\npirates: %d\nninjas: %d\npirate adventure time: %f\nninja adventure time: %f\npirate change time: %f\nninja change time: %f\n",
  num_team,num_p,num_n,muApt, muAnt, muCpt, muCnt);
  team_size = num_team;
  int pinit, ninit;
  sem_init(&deptsem,0,num_team);
  if (num_p > num_n){
    pinit = num_team;
    ninit = 0;
  } else {
    pinit = 0;
    ninit = num_team;
  }
  sem_init(&psem,0,pinit);
  sem_init(&nsem,0,ninit);
  sem_init(&plock,0,1);
  sem_init(&nlock,0,1);
  sem_init(&wlock,0,1);
  pirates = malloc(num_p*sizeof(pthread_t));
  ninjas = malloc(num_n*sizeof(pthread_t));
  p_strcts = malloc(num_p*sizeof(cst_strct));
  n_strcts = malloc (num_n*sizeof(cst_strct));
  busy = malloc(num_team*sizeof(int));
  idle = malloc(num_team*sizeof(int));
  lastime = malloc(num_team*sizeof(int));
  available = malloc(num_team*sizeof(int));
  struct timeval t;
  struct timezone tz;
  gettimeofday(&t,&tz);
  int start = (int)t.tv_sec;
  for (int i = 0; i < num_team; i++){
    available[i] = 1;
    busy[i]=0;
    lastime[i] = start;
    idle[i]=0;
  }
  completion = 0;
  pwait = 0;
  nwait = 0;
  indept = 0;
  pcount = 0;
  ncount = 0;
  // Create Threads for pirates and ninjas respectively
  pstrct.pirate = 1;
  pstrct.muCt = muCpt;
  pstrct.muAt = muApt;
  nstrct.pirate = 0;
  nstrct.muCt = muCnt;
  nstrct.muAt = muAnt;
  for (int i = 0; i < num_p; i++) if(pthread_create(pirates+i,NULL,thread, &pstrct) != 0) return -1;
  for (int i = 0; i < num_n; i++) if(pthread_create(ninjas+i,NULL, thread, &nstrct) != 0) return -1;

  printf("Running!\n");
  int a,b;
  sem_getvalue(&psem,&a);
  sem_getvalue(&nsem,&b);
  printf("semaphore inital values, psem: %d, nsem: %d\n",a,b);
  //sleep(60);

  // Join the threads for completion
  for (int i = 0; i < num_p; i++) if(pthread_join(pirates[i], NULL) != 0) return -1;
  for (int i = 0; i < num_p; i++) if(pthread_join(ninjas[i], NULL) != 0) return -1;
  gettimeofday(&t,&tz);
  for (int i = 0; i < num_team; i++) lastime[i] = (int)t.tv_sec;
  for (int i = 0; i < num_team; i++) idle[i] += (int)t.tv_sec - lastime[i];
  free(pirates);
  free(ninjas);
  int ptot = 0;
  int ntot = 0;
  int meanwait = 0;
  int count = 0;
  for (int i = 0; i < num_p; i++){
    cst_strct temp_strct = p_strcts[i];
    printf("__Pirate %d__\n",i);
    printf("Number of visits: %d\n",temp_strct.visits);
    int sum = 0;
    for (int j = 0; j < temp_strct.visits; j++){
      if (temp_strct.t_wait[j] < 30) sum += temp_strct.t_visit[j];
      printf("  Visit %d,\n",j);
      printf("    Time waiting: %d\n", temp_strct.t_wait[j]);
      printf("    Time in department: %d\n", temp_strct.t_visit[j]);
      meanwait+=temp_strct.t_wait[j];
      count++;
    }
    free(temp_strct.t_visit);
    free(temp_strct.t_wait);
    printf("  Total cost in department visits %d\n", sum);
    ptot += sum;
  }
  printf("Total pirate bill: %d dollars\n\n",ptot);
  for (int i = 0; i < num_n; i++){
    cst_strct temp_strct = n_strcts[i];
    printf("__Ninja %d__\n",i);
    printf("Number of visits: %d\n",temp_strct.visits);
    int sum = 0;
    for (int j = 0; j < temp_strct.visits; j++){
      if (temp_strct.t_wait[j] < 30) sum += temp_strct.t_visit[j];
      printf("  Visit %d,\n",j);
      printf("    Time waiting: %d\n", temp_strct.t_wait[j]);
      printf("    Time in department: %d\n", temp_strct.t_visit[j]);
      meanwait+=temp_strct.t_wait[j];
      count++;
    }
    free(temp_strct.t_visit);
    free(temp_strct.t_wait);
    printf("  Total cost in deparment visits %d\n\n",sum);
    ntot += sum;
  }
  printf("Total ninja bill: %d dollars\n\n",ntot);
  for (int i = 0; i < num_team; i++){
    printf("__Costume Team %d__\n",i);
    printf("  Time busy: %d\n",busy[i]);
    printf("  Time idle: %d\n",idle[i]);
  }
  printf("\nAverage time waiting: %d\n",meanwait/count);
  printf("Gross revenue: %d\n", ptot+ntot);
  printf("Gold per visit: %d\n", (ptot+ntot)/count);
  printf("Total revenue: %d\n", (ptot+ntot - num_team*5));
  free(p_strcts);
  free(n_strcts);
  return 0;
}

int main (int argc, char **argv) {
  if (argc < 2){
    printf("No values given!\n");
    return -1;
  }
  if (argv[1][0] == 'g') return runSim(2,10,10,5,5,3,3);
  else if (argv[1][0] == 'r') {
    while(1)runSim((int)(2+(2.5*drand48())),(int)(10+(40*drand48())),
    (int)(10+(40*drand48())),gauss(5),gauss(5),gauss(3),gauss(3));
  } else if (argv[1][0] == 'b'){
    while(1)runSim((int)(2+(2.5*drand48())),10,
    10,0,0,0,0);
  }else if (argc != 8){
    printf("Invalid numbe of arguments, program exiting...\n");
    return -1;
  }
  int count = 1;
  do{
    printf("Simulation %d\n",count);
  runSim(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atof(argv[4]),
    atof(argv[5]),atof(argv[6]),atof(argv[7]));
    count++;
  }while(0);
  return 0;
}
