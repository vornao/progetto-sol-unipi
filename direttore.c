//direttore.c
//include l'implementazione di direttore.h

#include "header/direttore.h"
#include "header/supermercato.h"
#include "header/cassiere.h"
#include "header/cliente.h"
#include "header/queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

int last_opend;
struct timespec t;



int close_desk(cassa_t *c){
    if(c->desk_open == 0) return 0;
    if(c->id == last_opend) return 0;
    client_t *sc = malloc(sizeof(client_t));
    if(!sc) perror("malloc failed");
    sc->id = -1;
    if(push(c->coda_clienti, sc)==-1) { fprintf(stderr, "failed to close desk"); return 0;}
    c -> desk_open = 0;
    return 1;
}

int open_desk(cassa_t *c, pthread_t* cassiere_thread, cassiere_t* cassiere_args, int K){
    unsigned int seed = 3;
    int r = rand_r(&seed)%K;
    while(c[r].desk_open) r = rand_r(&seed)%K;
    //printf("%d\n", r);
    c[r].desk_open = 1;
    if(pthread_create(&cassiere_thread[r], NULL, cassiere_start_thread, &cassiere_args[r]) == -1){
        perror("pthread create");
        fprintf(stderr, "failed opening desk\n");
        return 0;
    }
    last_opend = r;
    return 1;
}

void* chiudi_cassa_t(void* args){
    direttore_t* arg = (direttore_t*)args;
    t.tv_nsec = 60*CONST_WAIT_1;
    nanosleep(&t, NULL);
    //reads ql values, waits for the shop assistants to write the ql 
    //and wakes up shop assistants waiting for him to read
    //increments or decrements values if necessary.
    while(!*arg->s && !*arg->sigquit){
        pthread_mutex_lock(&arg->ql->ql_mutex);
        while(!arg->ql->sig){
            pthread_cond_wait(&arg->ql->ql_cond ,&arg->ql->ql_mutex);
        }
        for(int i = 0; i < arg->cfg->K; ++i){
            if((arg->ql->ql[i] < arg->cfg->S1 && arg->ql->open_desks_n > 1) && last_opend != i){
                if(close_desk(&arg->desks[i])) arg->ql->open_desks_n--;
            }
            else if(arg->ql->ql[i] > arg->cfg->S2 && arg->ql->open_desks_n < arg->cfg->K){
                if(open_desk(arg->desks, arg->cassieri_thread, arg->cassieri_args, arg->cfg->K)){
                    //printf("coda cassa %d %d, casse aperte %d\n", i, arg->ql->ql[i], arg->ql->open_desks_n);
                    arg->ql->open_desks_n++;
                }       
            }
            //just a little 60 millis sleep to avoid "compulsive" behavior
            //NOT FOR MULTITHREAD SYNCHRONIZATIONS!!
            nanosleep(&t, NULL);
        }
        arg->ql->sig = 0;
        //wakes up all the shop assistants who are waiting to write their ql.
        pthread_cond_broadcast(&arg->ql->ql_cond);
        pthread_mutex_unlock(&arg->ql->ql_mutex);
    }
    //unlock threads before quitting.
    pthread_cond_broadcast(&arg->ql->ql_cond);
    return NULL;
}

void* d_start_thread(void* args){
    direttore_t* arg = (direttore_t*)args;
    pthread_t *desk_handler    =  malloc(sizeof(pthread_t));
    //masking sigquit and sighup, so i'm sure the main thread handles the signals
    sigset_t mask;
	sigemptyset(&mask); 
    sigaddset(&mask, SIGHUP); 
    sigaddset(&mask, SIGQUIT);       
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    //starting desk handler thread, will manage desks opens and closures.
    if(pthread_create(desk_handler, NULL,chiudi_cassa_t, args) != 0){
        fprintf(stderr, "failed creating side thread.\n");
    }
    //starting the zero-prod-customers handler, just signals the client to leave.
    no_prod_process(arg->no_prod_queue, arg->people_in, arg->s, arg->sigquit);
    client_t c;
    c.id = -1;
    push(arg->no_prod_queue, &c);
    pthread_join(desk_handler, NULL);
    return NULL;
}

void no_prod_process(queue_t* q, people_t *p, volatile sig_atomic_t *s, volatile sig_atomic_t *sq){
    struct timespec w;
    client_t *c = pop(q);
    while(c->id != -1){
        if(c != NULL){
            if(c->id == -1){
                return;
            }
            w.tv_nsec = c->P*CONST_WAIT_10;
            nanosleep(&w, NULL);
            if(*s) client_signal(c, LEAVE);
            if(*sq)client_signal(c, SIGQUIT);
            get_out(p);
            c = pop(q);
        }
    }
    return;
}

