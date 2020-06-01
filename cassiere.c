//cassiere.c
//include l'implementazione delle funzioni del thread cassiere.
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "header/cassiere.h"
#include "header/queue.h"
#include "header/supermercato.h"
#include "header/cliente.h"
#include <assert.h>
#include <pthread.h>


/*il thread cassiere si comporta come consumatore.
**legge una coda di clienti presente ad una cassa, estrae il cliente dalla coda ed elabora i suoi prodotti. 
**in seguito comunica al cliente di essere stato servito, e il cliente termina -> esce dal supermercato.
*/
static struct timespec start;
static struct timespec end;
static struct timespec t_wait;
float tot;
unsigned int fx;
void write_log(char* filename, int id, unsigned int pa, float tot_time, float tot_queue, unsigned int n_queues){
    //puts("Scrivo log");
    FILE* fp = fopen(filename, "a");
    fprintf(fp, "| %.3d | %.3d | %.3f | %.3f | %.3d |\n", id, pa, tot_time, tot_queue, n_queues);
    fclose(fp);
    return;
}

void termina_cassa(cassa_t *cs){
    client_t *c;
    c = pop(cs->coda_clienti);
    while(c->id != -1){
        client_signal(c, CHANGE_QUEUE);
        c = pop(cs->coda_clienti);
    }
    return;
}

void termina_cassa_chiusura(cassa_t *cs, unsigned int *seed){
    puts("termino chiusura");
    struct timespec tw;
    client_t *c;
    c = pop(cs->coda_clienti);
    while(c->id != -1){
        cs->prod_elaborati += c->P;
        cs->clienti_serviti++;
        client_signal(c, LEAVE);
        c = pop(cs->coda_clienti);
    }
    return;
}

void *cassiere_start_thread(void* args){
    sigset_t mask;
	sigemptyset(&mask); 
    sigaddset(&mask, SIGHUP);        
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    cassiere_t      *cast_args = (cassiere_t*)args;
    pthread_t       manager_handler;
    tot = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_create  (&manager_handler, NULL, informa_direttore, args);
    client_t        *c;
    t_wait.tv_nsec = 0;
    //tempo fisso di attesa
    fx = (rand_r(&cast_args->seed) % (CONST_WAIT_10*8 - CONST_WAIT_10*2 + 1)) + CONST_WAIT_10*2; 
    assert(cast_args->cassa_n->desk_open != 0);
    //puts("cassa aperta");
    


    while(cast_args->cassa_n->desk_open){
        t_wait.tv_nsec = fx;
        c = pop(cast_args->cassa_n->coda_clienti);         //la pop è protetta in lettura, estrae solo con una signal da parte di una push.
        if(c != NULL){
            if(c->id == -1){
                //puts("La cassa è chiusa -1");
                return NULL;
            } 
            t_wait.tv_nsec += (c -> P)*CONST_WAIT_10;
            nanosleep(&t_wait, NULL);
            //fprintf(stdout, "Servito il cliente %d\n", c->id);
            get_out(cast_args->client_in);
            client_signal(c, LEAVE);
            cast_args -> cassa_n->clienti_serviti++;
            cast_args -> cassa_n->prod_elaborati += c -> P;
        }
    }
    //c = pop(cast_args->cassa_n->coda_clienti);
    
    if(!*cast_args->s) termina_cassa(cast_args->cassa_n);
    else termina_cassa_chiusura(cast_args->cassa_n, &cast_args->seed);

    clock_gettime(CLOCK_MONOTONIC, &end);

    tot = (end.tv_sec - start.tv_sec) + (float)(end.tv_nsec - start.tv_nsec)/(float)1000000000L;
    cast_args-> cassa_n-> tempo_apertura += tot;
    cast_args->cassa_n->n_chiusure++;
    return NULL;
}

void* informa_direttore(void* args){  //FUNZIONE CHE INFORMA AD INTERVALLI PERIODICI LA CODA DELLA CASSA CHE SERVE.
    struct timespec t_wait;
    cassiere_t      *cast_args = (cassiere_t*)args;
    t_wait.tv_nsec = cast_args->manager_timer*CONST_WAIT_1;
    while(cast_args->cassa_n->desk_open && !*cast_args->s){
        pthread_mutex_lock(&cast_args->qls->ql_mutex);
        while(cast_args->qls->sig&& !*cast_args->s ){
            pthread_cond_wait(&cast_args->qls->ql_cond, &cast_args->qls->ql_mutex);
        }
        cast_args->qls->ql[cast_args->id] = length(cast_args->cassa_n->coda_clienti);
        cast_args->qls->sig = 1;
        pthread_cond_signal(&cast_args->qls->ql_cond);
        pthread_mutex_unlock(&cast_args->qls->ql_mutex);
        nanosleep(&t_wait, NULL);
    }
    return NULL;
}

void client_signal(client_t *c, int v){
    pthread_mutex_lock(&c->c_mutex);
    c -> exit = v;
    pthread_cond_signal(&c->c_cond);
    pthread_mutex_unlock(&c->c_mutex);
}

void get_out(people_t *p){
    pthread_mutex_lock(&p->p_mutex);
    p -> i--;
    //printf("OUT, CLIENT IN:%D", p->i);
    pthread_cond_signal(&p->p_cond);
    pthread_mutex_unlock(&p->p_mutex);
    
}