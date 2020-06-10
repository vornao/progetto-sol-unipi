//direttore.h
//prototipi direttore.c

#ifndef DIRETTORE_H
#define DIRETTORE_H

#include "supermercato.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

typedef struct _direttore{
    volatile sig_atomic_t* s;
    volatile sig_atomic_t* sigquit;
    queue_t         *no_prod_queue;
    cassa_t         *desks;
    people_t        *people_in;
    config_t        *cfg;
    cassiere_t      *cassieri_args;
    pthread_t       *cassieri_thread; 
    queue_lenght    *ql; 
}direttore_t;

//starts "cassa" thread
void* d_start_thread(void* args);
int apri_cassa(int cassa);       
void* chiudi_cassa_t(void* args);
int leggi_coda_cassa(cassa_t *desks);
void no_prod_process(queue_t* q, people_t *p, volatile sig_atomic_t *s, volatile sig_atomic_t *sigquit);

#endif