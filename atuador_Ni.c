//
// Created by jeferson on 04/03/2021.
//

#include "atuador_Ni.h"
#include <math.h>
#include <pthread.h>

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t alarme = PTHREAD_COND_INITIALIZER;
static double atuador_lido = 0;
static double limite_atual = HUGE_VAL;

void atuador_Ni_put( double lido) {
    pthread_mutex_lock( &exclusao_mutua);
    atuador_lido = lido;
    if( atuador_lido >= limite_atual)
        pthread_cond_signal( &alarme);
    pthread_mutex_unlock( &exclusao_mutua);
}

double atuador_Ni_get( void) {
    double aux;
    pthread_mutex_lock( &exclusao_mutua);
    aux = atuador_lido;
    pthread_mutex_unlock( &exclusao_mutua);
    return aux;
}
