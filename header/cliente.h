//cliente.h

#ifndef CLIENTE_H
#define CLIENTE_H
#include <stdlib.h>
#include "supermercato.h"
#include "direttore.h"

#define CHANGE_QUEUE 2
#define LEAVE 1
#define WAIT_AUTH 0;

typedef struct _cliente{
    unsigned int seed;
    unsigned int id;
    short exit;
    //numero prodotti da acquistare.
    size_t P;    
    //tempo fisso da spendere nel supermercato in millis       
    size_t T;
    FILE* log_file;
    config_t* cfg;
    pthread_mutex_t c_mutex;
    pthread_cond_t  c_cond; 
    cassa_t *    desks;
    direttore_t* d; 
    volatile sig_atomic_t *sigquit;

}client_t;

/*typedef struct _client_args{
    client_t*    client_info;
    cassa_t *    desks;
    direttore_t* d;
    config_t* cfg;
} client_args_t;*/

void *cliente_start_thread(void* args);
int  choose_line(void* args);
int   wait_to_exit(client_t *c);

#endif
