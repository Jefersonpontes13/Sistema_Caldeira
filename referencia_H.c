/* Monitor referência de Nível de água */
#include <math.h>
#include <pthread.h>
#include "referencia_H.h"

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER; 
static double ref_entrada = 0; 
static double limite_atual = HUGE_VAL;


/* Chamado pela thread que le o valor de referência de Nível de água e disponibiliza aqui o valor lido */
 void put_ref_H( double ref) {
	 pthread_mutex_lock( &exclusao_mutua); 
	 ref_entrada = ref; 
	 pthread_mutex_unlock( &exclusao_mutua); 
 }
 
 /* Chamado por qualquer thread que precisa do valor de referência de Nível de água */
 double get_ref_H( void) {
	 double aux; 
	 pthread_mutex_lock( &exclusao_mutua); 
	 aux = ref_entrada; 
	 pthread_mutex_unlock( &exclusao_mutua); 
	 return aux;
 }
