//
// Created by jeferson on 03/03/2021.
//
/* Monitor sensor, no arquivo sensor_H.c */


#include "sensor_H.h"
#include <math.h>
#include <pthread.h>

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t alarme = PTHREAD_COND_INITIALIZER;
static double sensor_lido = 0;
static double limite_atual = HUGE_VAL;

/* Chamado pela thread que le o sensor e disponibiliza aqui o valor lido */
void sensor_H_put( double lido) {
    pthread_mutex_lock( &exclusao_mutua);
    sensor_lido = lido;
    if( sensor_lido >= limite_atual)
        pthread_cond_signal( &alarme);
    pthread_mutex_unlock( &exclusao_mutua);
}

/* Chamado por qualquer thread que precisa do valor lido do sensor */
double sensor_H_get( void) {
    double aux;
    pthread_mutex_lock( &exclusao_mutua);
    aux = sensor_lido;
    pthread_mutex_unlock( &exclusao_mutua);
    return aux;
}

/* Thread fica bloqueada ate o valor do sensor chegar em limite */
void sensor_H_alarme( double limite) {
    pthread_mutex_lock( &exclusao_mutua);
    limite_atual = limite;
    while( sensor_lido < limite_atual)
        pthread_cond_wait( &alarme, &exclusao_mutua);
    limite_atual = HUGE_VAL;
    pthread_mutex_unlock( &exclusao_mutua);
}
