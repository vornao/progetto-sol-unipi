//cliente.c
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "header/queue.h"
#include "header/cliente.h"
#include "header/direttore.h"
#include "header/supermercato.h"


static struct timespec start_time_supermarket;
static struct timespec end_time_supermarket;
static struct timespec t;
static struct timespec start_time_queue;
static struct timespec end_time_queue;
static double tot_time_supermarket;
static double time_queue;
static unsigned int visited_desks;
static unsigned int bought_items;

//customer choses a random line, checking if checkout is open.
int choose_line(void* args){
    client_t* arg      = (client_t*)args;
    unsigned int *seed = &arg->seed;
    int K              = arg->cfg->K;
    int P              = (rand_r(seed)%arg->cfg->P);
    int T              = arg->cfg->T;
    int r              = rand_r(seed)%K;
    bought_items = P;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_queue);
    if(P){
        t.tv_nsec = CONST_WAIT_10 + P*CONST_WAIT_1 + rand_r(seed)%T*CONST_WAIT_1;
        nanosleep(&t, NULL);
        while(!arg->desks[r].desk_open) r = rand_r(seed)%K;
        if (arg->desks[r].desk_open){
            //fprintf(stdout, "Io sono il cliente %d, ho scelto la cassa: %d\n", arg->id, r);
            if(push(arg->desks[r].coda_clienti, arg) == -1){
                fprintf(stderr, "failed to push %d\n", arg->id); 
                fflush(stdout);
            }  
        }
        //attendo il permesso di uscire       
        return wait_to_exit(arg); 
    }
    else{
        //puts("sono senza prodotti");
        t.tv_nsec = CONST_WAIT_10 + CONST_WAIT_1*(arg->T);
        nanosleep(&t, NULL);
        if(push(arg->d->no_prod_queue, arg) == -1){
            fprintf(stderr, "failed to push %d\n", arg->id); 
            fflush(stdout);
        } 
        return wait_to_exit(arg); 
    }

    //genero numeri casuali fintanto che non trovo una cassa aperta
}
//start customer thread with argument args, needs to be casted to _customer type.
void *cliente_start_thread(void* args){
    client_t* cast_args = (client_t*)args;
    //masking signals in order to be sure main thread handles
    sigset_t mask;
	sigemptyset(&mask); 
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGQUIT);        
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    visited_desks = 0;
    start_time_supermarket.tv_nsec = 0;
    start_time_supermarket.tv_sec  = 0;
    end_time_supermarket.tv_nsec   = 0;
    end_time_supermarket.tv_sec    = 0;
    start_time_queue.tv_nsec         = 0;
    start_time_queue.tv_sec          = 0;
    end_time_queue.tv_nsec = 0;
    end_time_queue.tv_sec = 0;
    time_queue = 0;

    int retval = CHANGE_QUEUE;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time_supermarket);
    retval = choose_line(args);
    while(retval == CHANGE_QUEUE){
        visited_desks++;
        retval = choose_line(args);
    }

    if(retval == SIGQUIT) { 
        pthread_mutex_destroy(&cast_args->c_mutex); 
        pthread_cond_destroy(&cast_args->c_cond);
        return NULL;
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time_queue);
    time_queue += (end_time_queue.tv_sec - start_time_queue.tv_sec)
               +  (double)(end_time_queue.tv_nsec - start_time_queue.tv_nsec)/1000000000L;

    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time_supermarket);
    tot_time_supermarket = (end_time_supermarket.tv_sec - start_time_supermarket.tv_sec)
                         + (double)(end_time_supermarket.tv_nsec - start_time_supermarket.tv_nsec)/1000000000L;


    write_log("client_log.txt", ((client_t*)args)->id, bought_items, tot_time_supermarket, time_queue, visited_desks);
    pthread_mutex_destroy(&cast_args->c_mutex); 
    pthread_cond_destroy(&cast_args->c_cond);
    return NULL;
}

//attendo il permesso per uscire, sincronizzato con cassiere o direttore.
int wait_to_exit(client_t *c){
    int val;
    pthread_mutex_lock(&c->c_mutex);
    while(!c->exit){
        pthread_cond_wait(&c->c_cond,&c->c_mutex);
    }
    val = c->exit;
    pthread_mutex_unlock(&c->c_mutex);
    return val;
}
