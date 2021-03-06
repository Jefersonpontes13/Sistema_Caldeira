//
// Created by jeferson on 06/03/2021.
//

#include	<pthread.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include "tempo_resp_h_bufduplo.h"

#define TAMBUF 100
static double buffer_0[TAMBUF];
static double buffer_1[TAMBUF];

static int emuso = 0;
static int prox_insercao = 0;
static int gravar = -1;

pthread_t buffer_h;

static pthread_mutex_t exclusao_mutua = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t buffer_cheio = PTHREAD_COND_INITIALIZER;
FILE* dados_arq;

static void tempo_resp_h_insere_dado_arq(int dado){
    dados_arq = fopen("tempo_resp_h.txt", "a");
    if(dados_arq == NULL){
        printf("Erro: Impossível abrir o arquivo de texto\n");
        exit(1);
    }
    fprintf(dados_arq, "%d\n",dado);
    fclose(dados_arq);

}

void tempo_resp_h_bufduplo_insereLeitura( double leitura){
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

void *tempo_resp_h_bufduplo_esperaBufferCheio(void) {
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
        tempo_resp_h_insere_dado_arq(buffer[i]);
    }

    pthread_create(&buffer_h, NULL, (void *) tempo_resp_h_bufduplo_esperaBufferCheio, NULL);
    pthread_join( buffer_h, NULL);
}