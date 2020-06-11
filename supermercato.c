//supermercato.c
#define _POSIX_C_SOURCE  200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "header/queue.h"
#include "header/supermercato.h"
#include "header/cliente.h"
#include "header/cassiere.h"
#include "header/direttore.h"
//codice funzone cliente

int K;
int C;
int CASS;
int E;
int S1;
int S2;
int T;
int P;
int S;
static volatile sig_atomic_t sighup;
static volatile sig_atomic_t sigquit;

static void sighandler(int sig){
    if(sig == SIGHUP){
        sighup = 1;
        sigquit = 0;
        write(1, "***** sighup received!!! *****\n", 32);
    }
    if(sig == SIGQUIT){
        sighup = 0;
        sigquit = 1;
        write(1, "***** sigquit received!!! *****\n", 33);
    }
    return;
}

void write_log_cassa(cassa_t *desks, char* filename){
    FILE* fp = fopen(filename, "a");
    for(int i = 0; i < K; ++i){
        fprintf(fp, "| %.3d | %.3d | %.4d | %.3f | %.3f | %.3d |\n", 
        desks[i].id, 
        desks[i].clienti_serviti, 
        desks[i].prod_elaborati, 
        desks[i].tempo_apertura, 
        (float)(desks[i].prod_elaborati)/(desks[i].clienti_serviti), 
        desks[i].n_chiusure);
    }
    fclose(fp);
    return;
}

client_t* create_client(int n, config_t* cfg, direttore_t *d, cassa_t *desks){
    unsigned int seed = n;
    client_t* c = malloc(sizeof(client_t));
    if(c == NULL)perror("malloc");
    c->id = n;
    c->P  = rand_r(&seed)%cfg->P;
    c->T  = rand_r(&seed)%cfg->T;
    c-> seed = n;
    c-> cfg = cfg;
    c->desks = desks;
    c->d = d;
    c -> sigquit = &sigquit;
    pthread_mutex_init(&c->c_mutex, NULL);
    pthread_cond_init(&c->c_cond, NULL);
    return c;
}

config_t* ps_f(char* fname){

    char *buf;
    char *cp;
    FILE *fd;
    int  i = 0;
    config_t *cfg;

    if((fd = fopen(fname, "r")) == NULL){
        perror("parse_config_file");
        exit(EXIT_FAILURE);
    }

    cfg = malloc(sizeof(config_t));
    buf = malloc(sizeof(char)*BUF_SIZE);
    cp  = malloc(sizeof(char)*BUF_SIZE);

    if(!cfg || !buf || !cp){
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
    while(fgets(buf, BUF_SIZE, fd) != NULL){
        cp = buf;
        while(*buf != '=') buf++;
        buf++;
        switch(i){
            case 0: K = cfg -> K = atoi(buf);
                break;
            case 1: C = cfg ->C = atoi(buf);
                break;
            case 2: E = cfg -> E = atoi(buf);
                break;
            case 3: T = cfg -> T = atoi(buf);
                break;
            case 4: CASS = cfg -> CASS = atoi(buf);
                break;
            case 5: S = cfg-> S = atoi(buf);
                break;
            case 6: S1 = cfg-> S1 = atoi(buf);
                break;
            case 7: S2 = cfg->  S2 = atoi(buf);
                break;
            case 8: P = cfg->P = atoi(buf);
                break;
            default:
                break;    
        }
        i++;
        buf = cp;
    }
    return cfg;
}

config_t *cfg;

int main(int argc, char* argv[]){
    if(argc != 2){
        printf(stderr, "Supermercato: Usage: ./supermercato <path_to_cfg_file>\n");
        return EXIT_FAILURE;
    }

    int    n_clienti = 0;    
    if(signal(SIGHUP, &sighandler) == SIG_ERR) { perror("sigaction"); exit(EXIT_FAILURE);} 
    if(signal(SIGQUIT, &sighandler) == SIG_ERR) { perror("sigaction"); exit(EXIT_FAILURE);} 
    cfg = ps_f(argv[1]);
    people_t *client_in                    =  malloc(sizeof(people_t));
    //array che contiene le K casse;
    cassa_t            *desks              =  malloc(sizeof(cassa_t)*K);
    //array che contiene i thread clienti iniziali;
    pthread_t          *thread_clienti     =  malloc(sizeof(pthread_t)*C);
    client_t           *customers          =  malloc(sizeof(client_t)*C);
    queue_lenght       *ql                 =  malloc(sizeof(queue_lenght));
    //array che contiene i thread cassieri
    pthread_t          *thread_cassieri    =  malloc(sizeof(pthread_t)*K); 
    //array che contiene gli argomenti da passare al cassiere        
    cassiere_t         *cassiere_args      =  malloc(sizeof(cassiere_t)*K);  
    //argomenti direttore
    direttore_t        *d_args             =  malloc(sizeof(direttore_t));   
    pthread_t          *d_thread           =  malloc(sizeof(pthread_t));   

    //set client in
    client_in -> i = C;
    pthread_mutex_init(&client_in->p_mutex, NULL);
    pthread_cond_init(&client_in->p_cond, NULL);

    //set array lunghezze client
    ql->ql = calloc(K, sizeof(int));
    ql -> open_desks_n = 0;
    pthread_mutex_init(&ql->ql_mutex, NULL);
    pthread_cond_init(&ql->ql_cond, NULL);
    

    if(!desks || !thread_cassieri || !cassiere_args || !thread_clienti)                    
        fprintf(stderr, "Malloc fallita\n");
    /**initializing manager thread args and starting his thread.*/

    //inizializzo gli attributi dei clienti, tempo permanenza e acquisti
    for(int i = 0; i < C; ++i){

        //numero prodotti random
        customers[i] = *create_client(n_clienti + i, cfg, d_args, desks);
        n_clienti++;
    }
    puts("Inizializzo clienti...\n");

    /*inizliaizzo le casse*/
    for(int i = 0; i < K; ++i){
        desks[i].coda_clienti = initQueue();
        desks[i].clienti_serviti = 0;
        desks[i].id = i;
        desks[i].prod_elaborati = 0;
        desks[i].tempo_apertura = 0;
        desks[i].desk_open = 1;
        ql -> open_desks_n++;
    }
    
    fprintf(stdout, "Inizializzo casse...\n");

    /*inizializzo gli argomenti da passare ai thread cassieri: tempo esecuzione e cassa assegnata.*/
    for(int i = 0; i < K; ++i){
        cassiere_args[i].id = i;
        cassiere_args[i].cassa_n   = &desks[i];
        cassiere_args[i].client_in = client_in;
        cassiere_args[i].seed = i;
        cassiere_args[i].manager_timer = 60;
        cassiere_args[i].qls = ql;
        cassiere_args[i].s = &sighup;
        cassiere_args[i].sigquit = &sigquit;
    }

    d_args -> desks         = desks;
    d_args -> no_prod_queue = initQueue();
    d_args -> people_in     = client_in;
    d_args -> cfg           = cfg;
    d_args -> cassieri_args =    cassiere_args;
    d_args -> cassieri_thread =  thread_cassieri;
    d_args -> ql = ql;
    d_args -> s  = &sighup;
    d_args -> sigquit = &sigquit;

    if(pthread_create(d_thread, NULL, d_start_thread, d_args)!=0){
       fprintf(stderr, "Failed creating manager thread.");
       exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Inizializzati argomenti thread cassieri...\n");

    for(int i = 0; i < K; ++i){
        if(desks[i].desk_open){
            if(pthread_create(&thread_cassieri[i], NULL, cassiere_start_thread, &cassiere_args[i]) != 0){
                fprintf(stderr, "failed creating employee thread");
             exit(EXIT_FAILURE);
            }
        }
    }
    //let first C customers to get in
    for(int i = 0; i < C; ++i){
        if(pthread_create(&thread_clienti[i], NULL, cliente_start_thread, &customers[i]) != 0){
            fprintf(stderr, "failed creating client thread");
            exit(EXIT_FAILURE);
        }
        n_clienti++;
    }
    //creo i cassieri -> consumatori rispetto ai clienti
    
    fprintf(stdout, "Inizializzati thread cassieri...\n");

    struct timespec t;
    client_t        *c;
    pthread_t *new_client;
    t.tv_nsec = CONST_WAIT_10;
    
    //waiting loop until a managed signal is received.
    while(!sighup && !sigquit){
        pthread_mutex_lock(&client_in->p_mutex);
        //waiting loop until customers inside the shop drops below C-E
        while(client_in->i > C-E){
            pthread_cond_wait(&client_in->p_cond, &client_in->p_mutex);
        }
        client_in->i += E;
        pthread_cond_signal(&client_in->p_cond);
        pthread_mutex_unlock(&client_in->p_mutex);
        for(int i = 0; i < E; ++i){
            new_client = malloc(sizeof(pthread_t));
            c = create_client(n_clienti, cfg, d_args, desks);
            if(pthread_create(new_client, NULL, cliente_start_thread, c) != 0){
                fprintf(stderr, "failed creating client thread\n");
                exit(EXIT_FAILURE);                
            } 
            n_clienti++;   
        }  
    }

    client_t term;
    term.id = -1;
    //sending special termination client to all the desks, pushing it in the queue.
    for(int i = 0; i < K; ++i){
        desks[i].desk_open = 0;
        push(desks[i].coda_clienti, &term);
    }
    //waking up threads waiting for the queue lenght to be signaled;
    pthread_cond_broadcast(&ql->ql_cond);

    //waiting 
    for(int i = 0; i < K; ++i){
        pthread_join(thread_cassieri[i], NULL);
    }

    write_log_cassa(desks, "cassa_log.log");
    free(thread_cassieri);
    free(thread_clienti);
    free(desks);
    free(d_args);
    free(d_thread);
    free(cassiere_args);
    free(ql);
    return 0;
}


