//
// Created by jeferson on 04/03/2021.
//

#include "atuador_Na.h"
#include <math.h>
#include <pthread.h>

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t alarme = PTHREAD_COND_INITIALIZER;
static double atuador_lido = 0;
static double limite_atual = HUGE_VAL;

/* Chamado pela thread que le o sensor e disponibiliza aqui o valor lido */
void atuador_Ni_put( double lido) {
    pthread_mutex_lock( &exclusao_mutua);
    atuador_lido = lido;
    if( atuador_lido >= limite_atual)
        pthread_cond_signal( &alarme);
    pthread_mutex_unlock( &exclusao_mutua);
}

/* Chamado por qualquer thread que precisa do valor lido do sensor */
double atuador_Ni_get( void) {
    double aux;
    pthread_mutex_lock( &exclusao_mutua);
    aux = atuador_lido;
    pthread_mutex_unlock( &exclusao_mutua);
    return aux;
}
