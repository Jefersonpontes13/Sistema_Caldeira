/* Monitor referência de Temperatura */
#include <math.h>
#include <pthread.h>
#include "referencia_T.h"

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER; 
static double ref_entrada = 0; 
static double limite_atual = HUGE_VAL;


/* Chamado pela thread que le o valor de referência de Temperatura e disponibiliza aqui o valor lido */
 void put_ref_T( double ref) {
	 pthread_mutex_lock( &exclusao_mutua); 
	 ref_entrada = ref; 
	 pthread_mutex_unlock( &exclusao_mutua); 
 }
 
 /* Chamado por qualquer thread que precisa do valor de referência de Temperatura */
 double get_ref_T( void) {
	 double aux; 
	 pthread_mutex_lock( &exclusao_mutua); 
	 aux = ref_entrada; 
	 pthread_mutex_unlock( &exclusao_mutua); 
	 return aux;
 }
