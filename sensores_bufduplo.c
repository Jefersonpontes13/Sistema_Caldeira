//
// Created by jeferson on 05/03/2021.
//

/* Monitor buffer duplo, no arquivo bufduplo.c */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sensores_bufduplo.h"


#define TAMBUF 12
static double buffer_0[TAMBUF];
static double buffer_1[TAMBUF];

static int emuso = 0;
static int prox_insercao = 0;
static int gravar = -1;

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t buffer_cheio = PTHREAD_COND_INITIALIZER;
static FILE* dados_arq;
static int cont = 0;

void sensores_bufduplo_insereLeitura( double leitura) {
    //printf("\n\n%f\n\n", leitura);
    pthread_mutex_lock( &exclusao_mutua);
    if( emuso == 0)
        buffer_0[prox_insercao] = leitura;
    else
        buffer_1[prox_insercao] = leitura;
    ++prox_insercao;
    if( prox_insercao == TAMBUF){
        gravar = emuso;
        emuso = (emuso + 1)% 2;
        prox_insercao = 0;
        pthread_cond_signal( &buffer_cheio);
    }
    pthread_mutex_unlock( &exclusao_mutua);
}

void sensores_bufduplo_esperaBufferCheio(void) {
    double *buffer;
    pthread_mutex_lock( &exclusao_mutua);
    while( gravar == -1)
        pthread_cond_wait( &buffer_cheio, &exclusao_mutua);
    if(gravar==0)
        buffer = buffer_0;
    else
        buffer = buffer_1;
    gravar = -1;
    pthread_mutex_unlock( &exclusao_mutua);
    //alterar para ao inves de retornar no buffer ja gravar no

    for( int i=0; i<TAMBUF; ++i){
        //printf("\n\n%f\n\n", buffer[i]);
        insere_dados_sensores_arq(buffer[i]);
    }

    pthread_t buffer_s;
    pthread_create(&buffer_s, NULL, (void *) sensores_bufduplo_esperaBufferCheio, NULL);
    pthread_join( buffer_s, NULL);
}

static void insere_dados_sensores_arq(float dado){

    dados_arq = fopen("sensores_T_No_H.txt", "a");
    if(dados_arq == NULL){
        printf("Erro: Impossível abrir o arquivo de texto\n");
    }
    if(cont == 2){
        fprintf(dados_arq, "%f\n",dado);
        cont = 0;
    }
    else{
        cont++;
        fprintf(dados_arq, "%f\t",dado);
    }

    fclose(dados_arq);

}