#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LEN 20

pthread_t *threads; //threads
pthread_mutex_t init; // the lock for initializing and assigning all threads
pthread_mutex_t qlock[4]; // 0:NW 1:NE 2:SW 3:SE
pthread_mutex_t cardlock[4]; // locks for each cardinality
pthread_cond_t card_cond[4]; // conditional variables for each direction to queue up the cars

int count = 0; // assigns indeces for threads
int cardid[4] = {0,0,0,0}; // for condition variables to enforce order

char *tstr[3] = {"right", "straight", "left"}; // strings for direction
char *cstr[4] = {"north", "east", "south", "west"}; // strings for cardinality

int *direction(int card, int dir){ //returns array in format of locks to aquire in order
  int options[4][3] = {{0,2,3},{1,0,2},{3,1,0},{2,3,1}};
  int *temp = malloc((dir+1)*sizeof(int));
  for (int i = 0; i < dir + 1; i ++) {
    temp[i]= options[card][i];
  }
  return temp;
}

// returns new direction
char* get_new_card(int card, int dir){
  int options[4][3] = {{3,2,1},{0,3,2},{1,0,3},{2,1,0}};
  return cstr[options[card][dir]];
}

// Thread function pointer
void *car(void* info){
  int index; // car index for queue management
  pthread_mutex_lock(&init);
  index = count; // Assign each thread  a unique id
  count++;
  pthread_mutex_unlock(&init);
  do{
    int card = rand()%4;//randomly assign direction 0:N 1:E 2:S 3:W
    int dir = rand()%3; //randomly assign turn 0:right 1:straight 2:left
    printf("Thread %d approaching from %s going %s to %s\n",index,cstr[card],tstr[dir],get_new_card(card,dir));
    sleep(LEN*0.02);
    int *locks_to_aquire = direction(card,dir);
    pthread_mutex_lock(&cardlock[card]);
    while (cardid[card])
      pthread_cond_wait(&card_cond[card],&cardlock[card]);
    cardid[card] = 1;
    int save[dir+1]; // save order of lock aquiring in this array
    int num = 0;
    int go = 0; // always grab locks in order to prevent mutex error
    for (int i = 0; i < 4; i++){ //aquire locks for each quadrant
      for (int j = 0; j < dir + 1; j++) if (locks_to_aquire[j] == i) go = 1;
      if (go){
        pthread_mutex_lock(&qlock[i]);
        save[num] = i;
        num++;
        go = 0;
      }
    }
    printf("Thread %d entering from %s going %s to %s\n", index,cstr[card],tstr[dir],get_new_card(card,dir));
    sleep(LEN*0.02);
    for (int i = 0; i < dir + 1; i++) pthread_mutex_unlock(&qlock[save[i]]);
    cardid[card] = 0;
    pthread_mutex_unlock(&cardlock[card]);
    pthread_cond_signal(&card_cond[card]);
    printf("Thread %d leaving from %s going %s to %s\n", index,cstr[card],tstr[dir],get_new_card(card,dir));
    free(locks_to_aquire);
  } while (0);
  return NULL;
}

int main(){
  srand(time(0));
  //for(int i = 0; i < 4; i ++) occupied[i] = 0;
  threads = malloc(LEN*sizeof(pthread_t));
  pthread_mutex_init(&init,NULL);
  for (int i = 0; i < 4; i++) pthread_cond_init(&card_cond[i],NULL);
  for (int i = 0; i < 4; i++) pthread_mutex_init(&cardlock[i],NULL);
  for (int i = 0; i < 4; i ++) pthread_mutex_init(&qlock[i],NULL);
  for (int i = 0; i < LEN; i++) pthread_create(threads+i,NULL,car,NULL);
  printf("Running\n");
  for (int i = 0; i < LEN; i++) pthread_join(threads[i],NULL);
  free(threads);
  return 0;
}
