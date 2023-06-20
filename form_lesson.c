#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
void *printString(void *ptr);
void *printCounter();
void *printCounter2();
int printCount = 0;
pthread_mutex_t count_mutex  = PTHREAD_MUTEX_INITIALIZER;
sem_t mutex_sem, count_sem, printer_sem;
int main(void) {
    pthread_t thread[3];
    int ret;
    if (sem_init(&mutex_sem, 0, 1) != 0) {
        perror("sem_init error");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&count_sem, 0, 0) != 0) {
        perror("sem_init error");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&printer_sem, 0, 1) != 0) {
        perror("sem_init error");
        exit(EXIT_FAILURE);
    }
    if ((ret = pthread_create( &thread[0], NULL, printString,"HELLO WORLD ")) != 0) {
        fprintf (stderr, "thread creation error = %d (%s)\n", ret, strerror (ret));
        exit(EXIT_FAILURE);
    };
    if ((ret = pthread_create( &thread[1], NULL, printString,"Ala ma kota ")) != 0) {
        fprintf (stderr, "thread creation error = %d (%s)\n", ret, strerror (ret));
        exit(EXIT_FAILURE);
    };
    if ((ret = pthread_create( &thread[2], NULL, printCounter,NULL) != 0)) {
        fprintf (stderr, "thread creation error = %d (%s)\n", ret, strerror (ret));
        exit(EXIT_FAILURE);
    };

    // exiting (only) the main thread...
    pthread_exit(0);
}
void *printCounter(){
    while (1){
        //pthread_mutex_lock( &count_mutex ); // ???
        if (sem_wait(&count_sem) == -1) {
            perror("sem_wait: counter_sem");
        }
        //printf("Wydrukowano znakow: %d\n",printCount);
        printf("Wydrukowano liczbę znaków n %% 10 == 0 (?)\n");
        //pthread_mutex_unlock( &count_mutex ); // ??
    }
}
void *printCounter2(){
    // tylko czekanie na odpowiednia liczbe znakow
    if (sem_wait(&count_sem) == -1) {
        perror("sem_wait: counter_sem");
    }
    printf("Wydrukowano odpowiednią liczbę znaków, konczę pracę..\n");
    pthread_exit(NULL);
}
void screenPrinter(char c){
    int ret;
    if ((ret = pthread_mutex_lock( &count_mutex ) != 0)) {
        fprintf (stderr, "count_mutex lock error = %d (%s)\n", ret, strerror (ret));
        exit(EXIT_FAILURE);
    };
    printf("%c\n",c);
    printCount++;
    if ( printCount % 10 == 0 ) {
        if (sem_post(&count_sem) == -1) {
            perror("sem_post: counter_sem");
        }
    }
    if ((ret = pthread_mutex_unlock( &count_mutex ) != 0)) {
        fprintf (stderr, "count_mutex unlock error = %d (%s)\n", ret, strerror (ret));
        exit(EXIT_FAILURE);
    };
}
void *printString( void *ptr ) {
    char *message;
    message = (char *) ptr;
    int len = strlen(message);
    int i = 0;
    while (1){
        if (sem_wait(&printer_sem) == -1) {
            perror("sem_wait: printer_sem");
        }
        for(i=0; i<len; i++){
            screenPrinter(message[i]);
            usleep(200*1000);
        }
        if (sem_post(&printer_sem) == -1) {
            perror("sem_post: printer_sem");
        }
        i=0;
        sleep(1);
    }
}