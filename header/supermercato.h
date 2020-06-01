//include gli header per la funzione supermercato. non so se sia necessario, vedrò in seguito.

#ifndef  SUPERMERCATO_H
#define  SUPERMERCATO_H

#include <pthread.h>
#include "queue.h"
#include <signal.h>
#define  BUF_SIZE   128
//numero massimo clienti supermercato alla volta
//numero cassieri supermercato
#define  CASSIERE_N 6
//numero casse del supermercato
/*#define  E          20
#define  K          10
//capacità supermercato
#define  C          4

#define S1          2
#define S2          5
*/
//Attendi 1 millis 
#define  CONST_WAIT_1  1000000L
//attendi 10 millis
#define  CONST_WAIT_10 10000000L
#define  CONV_NSEC     1000000000L

void write_log(char* filename, int id, unsigned int pa, float tot_time, float tot_queue, unsigned int n_queues);

typedef struct _cassa{
    unsigned short int  desk_open;           //0 if closed, 1 if open.
    queue_t*            coda_clienti;
    unsigned short      len;
    unsigned int        clienti_serviti;
    unsigned short      id;
    unsigned int        prod_elaborati;
    double              tempo_apertura;
    unsigned int        n_chiusure;
    pthread_mutex_t     mutex_cassa;
    pthread_cond_t      cond_cassa;
} cassa_t;

/*typedef struct _casse{
    cassa_t *c;
} casse_t;*/

typedef struct _queue_lenght{
    pthread_mutex_t ql_mutex;
    pthread_cond_t  ql_cond;
    int* ql;
    int  sig;
    int  open_desks_n;
} queue_lenght;

typedef struct _people_in{
    unsigned int i;
    unsigned int tot;
    pthread_mutex_t p_mutex;
    pthread_cond_t  p_cond;
}people_t;

typedef struct _cassiere{
    //unsigned short *keep_alive;
    unsigned int   manager_timer;
    unsigned int   seed;  
    unsigned int   id;
    cassa_t*       cassa_n;                         
    people_t*      client_in;
    queue_lenght*  qls;
    volatile sig_atomic_t* s;
} cassiere_t;


typedef struct _cassiere_args{
    cassiere_t *cassiere_info;
}cassiere_args_t;

typedef struct _config{
    int CASS;
    int K;
    int C;
    int E;
    int T;
    int S;
    int S1;
    int S2;
    int P;
} config_t;

//client_t* create_client(int n);

#endif