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
    //printf("apro cassa %d\n", r);
    return 1;
    //attivare un thread cassiere su quella cassa.
    //al direttoree devo passare anche l'elenco dei cassieri.
}

void* chiudi_cassa_t(void* args){
    direttore_t* arg = (direttore_t*)args;
    t.tv_nsec = 60*CONST_WAIT_1;
    nanosleep(&t, NULL);
    while(!*arg->s){
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
            nanosleep(&t, NULL);
        }
        arg->ql->sig = 0;
        pthread_cond_broadcast(&arg->ql->ql_cond);
        pthread_mutex_unlock(&arg->ql->ql_mutex);
    }
    pthread_cond_broadcast(&arg->ql->ql_cond);
    return NULL;
}

void* d_start_thread(void* args){
    direttore_t* arg = (direttore_t*)args;
    pthread_t *desk_handler    =  malloc(sizeof(pthread_t));
    sigset_t mask;
	sigemptyset(&mask); 
    sigaddset(&mask, SIGHUP);        
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    fprintf(stdout, "Sono il direttore\n");
    fflush(stdout);    
    if(pthread_create(desk_handler, NULL,chiudi_cassa_t, args) != 0){
        fprintf(stderr, "failed creating side thread.\n");
    }
    no_prod_process(arg->no_prod_queue, arg->people_in, arg->s);
    pthread_join(desk_handler, NULL);
    fprintf(stdout, "Termina direttore\n");
    fflush(stdout);
    return NULL;
}

void no_prod_process(queue_t* q, people_t *p, volatile sig_atomic_t *s){
    struct timespec w;
    client_t *c;
    while(!*s){
        c = pop(q);
        if(c != NULL){
            if(c->id == -1){
                return;
            }
            w.tv_nsec = c->P*CONST_WAIT_10;
            nanosleep(&w, NULL);
            client_signal(c, LEAVE);
            get_out(p);
            //fprintf(stdout, "Sono il direttore, faccio uscire il cliente %d, senza prodotti\n", c->id);
        }
    }
    return;
}

