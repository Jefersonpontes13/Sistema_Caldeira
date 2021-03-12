//
// Created by jeferson on 05/03/2021.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "Sensores/sensor_H.h"
#include "Sensores/sensor_T.h"
#include "Sensores/sensor_Ta.h"
#include "Sensores/sensor_Ti.h"
#include "Sensores/sensor_No.h"
#include "Atuadores/atuador_Na.h"
#include "Atuadores/atuador_Ni.h"
#include "Atuadores/atuador_Nf.h"
#include "tela.h"
#include "socket.h"

#include "sensores_bufduplo.h"
#include "tempo_resp_h_bufduplo.h"
#include "tempo_resp_t_bufduplo.h"
#include "referencia_H.h"
#include "referencia_T.h"

#define NSEC_PER_SEC (1000000000)    // Numero de nanosegundos em um segundo

//  Quantidade de amostras a serem coletadas
#define N_AMOSTRAS 1000

//  Retorna o valor absoluto de uma variável
double v_abs(double x) {
    if (x < 0) {
        return -x;
    }
    if (x >= 0) {
        return x;
    }
    return 0;
}

//  função destinada a rotina de controle da tempertura da água no reservatório
_Noreturn void thread_controle_T(void) {

    struct timespec time, time_end;
    long periodo = 50000000;    // 50ms
    int amostras = 1;

    // Le o tempo atual, coloca em time
    clock_gettime(CLOCK_MONOTONIC, &time);

    // Tarefa periodica iniciará em 1 segundo
    time.tv_sec++;

    double t, Q;
    char msg_enviada[1000];

    double R = 0.001;   // resistência térmica do isolamento (2mm madeira) [0.001 Grau / (Joule/segundo)]
    double Qe;          // fluxo de calor através do isolamento do recipiente [Joule/segundo]

    double T_ref = get_ref_T(); //  Temperatura de referência

    while (1) {

        // Espera até inicio do proximo periodo
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);

        //  Calcula Qe
        Qe = (sensor_T_get() - sensor_Ta_get()) / R;

        // define um valor para Q de tal forma que compensa a perda de calor pelo isolamento do recipiente
        if (Qe > 0) {
            if (Qe > 1000000.0) {
                Qe = 1000000.0;
            }
        } else {
            Qe = 0.0;
        }
        Q = Qe;

        // Atualiza o valor do atuador Q
        sprintf(msg_enviada, "aq-%lf", Q);
        msg_socket(msg_enviada);

        /*aloca_tela();
        printf("Q %f\n", Q);
        libera_tela();*/

        t = sensor_T_get();

        //  Ajusta a temperatura caso seja menor que a referência
        if (t < T_ref * 0.999) {
            sprintf(msg_enviada, "aq-%lf", 1000000.0);
            msg_socket(msg_enviada);

            if (atuador_Na_get() < 10) {

                atuador_Ni_put(atuador_Ni_get() + 10 - atuador_Na_get());
                sprintf(msg_enviada, "anf%lf", atuador_Nf_get());
                msg_socket(msg_enviada);

                atuador_Na_put(atuador_Nf_get() + 10 - atuador_Na_get());
                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);

                /*aloca_tela();
                printf("T %.3f T_ref %.3f Na %.3f Ni %.3f Nf %.3f\n",sensor_T_get(), get_ref_T(), atuador_Na_get(),
                       atuador_Ni_get(), atuador_Nf_get());
                libera_tela();*/
            }
        }
        //  Ajusta o valor de Q caso a temperatura não seja menor que a referência
        if (t >= T_ref) {
            sprintf(msg_enviada, "aq-%lf", Q);
            msg_socket(msg_enviada);
        }

        // Le a hora atual, coloca em time_end
        clock_gettime(CLOCK_MONOTONIC, &time_end);

        if (amostras++ <= N_AMOSTRAS) {
            // Calcula o tempo de resposta observado em microsegundos, e insere no buffer
            tempo_resp_t_bufduplo_insereLeitura((1000000 * (time_end.tv_sec - time.tv_sec)) +
                                                ((time_end.tv_nsec - time.tv_nsec) / 1000));
        }

        // Calcula inicio do proximo periodo
        time.tv_nsec += periodo;
        while (time.tv_nsec >= NSEC_PER_SEC) {
            time.tv_nsec -= NSEC_PER_SEC;
            time.tv_sec++;
        }
    }
}

//  função destinada a rotina de controle do nível da água no reservatório
_Noreturn void thread_controle_H(void) {

    struct timespec time, time_end;
    int periodo = 70000000;    // 70ms
    int amostras = 1;

    // Le o tempo atual, coloca em time
    clock_gettime(CLOCK_MONOTONIC, &time);

    // Tarefa periodica iniciará em 1 segundo
    time.tv_sec++;

    double t, ti, Ni, Na, No, Nf, h, l, dh, dNi, dNa;

    char msg_enviada[1000];

    double H_ref = get_ref_H(); //  Nível de referência

    while (1) {

        // Espera até inicio do proximo periodo
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);

        h = sensor_H_get();

        if ((h < H_ref) && ((h / H_ref) < 0.99)) {

            t = sensor_T_get();
            ti = sensor_Ti_get();
            No = sensor_No_get();
            Nf = atuador_Nf_get();
            dh = ((H_ref - h) / H_ref) + 1;
            l = v_abs((ti - t) / (80 - t)); // Fator de proporcionalidade para garantir que não altere a temperatura

            if (No == 0 && Nf == 0) {

                Ni = 50.0 * dh;
                Na = l * Ni;

                //  Verifica se o valor de Ni é permitido (Ni <= 100), e ajusta Ni e Na caso necessário
                if (Ni > 100) {
                    dNi = 100 / Ni;
                    Ni = dNi * Ni;
                    Na = dNi * Na;
                }
                //  Verifica se o valor de Na é permitido (Na <= 10), e ajusta Ni e Na caso necessário
                if (Na > 10) {
                    dNa = 10 / Na;
                    Ni = dNa * Ni;
                    Na = dNa * Na;
                }

                //  Atualiza os valores d Ni e Na em seus respectivos monitores
                atuador_Ni_put(Ni);
                atuador_Na_put(Na);

                //  Atualiza o valor de Ni no simulador
                sprintf(msg_enviada, "ani%lf", atuador_Ni_get());
                msg_socket(msg_enviada);

                //  Atualiza o valor de Na no simulador
                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);

            } else {

                Ni = ((No + Nf) / (l + 1)) * dh * 10;
                Na = l * atuador_Ni_get();

                if (Ni > 100) {
                    dNi = 100 / Ni;
                    Ni = dNi * Ni;
                    Na = dNi * Na;
                }

                if (Na > 10) {
                    dNa = 10 / Na;
                    Ni = dNa * Ni;
                    Na = dNa * Na;
                }
                atuador_Ni_put(Ni);
                atuador_Na_put(Na);

                //  Atualiza os valores dos atuadores Ni e Na no simulador
                sprintf(msg_enviada, "ani%lf", atuador_Ni_get());
                msg_socket(msg_enviada);

                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);
            }
        } else {
            //  Caso H Ultrapassar o limiar de 99% do Nível de Referência

            //  Caso o fluxo de entrada de água seja diferente do fluxo de sáida
            if (atuador_Nf_get() + sensor_No_get() != atuador_Na_get() + atuador_Ni_get()) {
                atuador_Na_put(0.0);
                atuador_Ni_put(0.0);
                atuador_Nf_put(0.0);

                //  Atualiza os valores dos atuadores no simulador
                sprintf(msg_enviada, "ani%lf", atuador_Ni_get());
                msg_socket(msg_enviada);

                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);

                sprintf(msg_enviada, "anf%lf", atuador_Na_get());
                msg_socket(msg_enviada);
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &time_end);

        if (amostras++ <= N_AMOSTRAS) {
            // Calcula o tempo de resposta observado em microsegundos
            tempo_resp_h_bufduplo_insereLeitura((1000000 * (time_end.tv_sec - time.tv_sec)) +
                                                ((time_end.tv_nsec - time.tv_nsec) / 1000));
        }

        // Calcula inicio do proximo periodo
        time.tv_nsec += periodo;
        while (time.tv_nsec >= NSEC_PER_SEC) {
            time.tv_nsec -= NSEC_PER_SEC;
            time.tv_sec++;
        }
    }
}

//  função destinada a rotina de Status (Mostrar os dados(T, Ta, Ti, No e H) no terminal periodicamente)
_Noreturn void thread_status(void) {
    double t, ta, ti, No, h;
    while (1) {

        t = sensor_T_get();
        // Insere a leitura T no buffer duplo destinado aos sensores
        sensores_bufduplo_insereLeitura(t);
        ta = sensor_Ta_get();
        ti = sensor_Ti_get();
        No = sensor_No_get();
        // Insere a leitura No no buffer duplo destinado aos sensores
        sensores_bufduplo_insereLeitura(No);
        h = sensor_H_get();
        // Insere a leitura No no buffer duplo destinado aos sensores
        sensores_bufduplo_insereLeitura(h);

        aloca_tela();
        printf("\33[H\33[2J");
        printf("----------------------------------------\n");
        printf("|Temperatura no interior (T)\t--> %.2lf\n", t);
        printf("|Altura da Coluna(H)\t\t--> %.2lf\n", h);
        printf("|Ambiente em volta (Ta)\t\t--> %.2lf\n", ta);
        printf("|Água que entra (Ti)\t\t--> %.2lf\n", ti);
        printf("|Fluxo de Saída (No)\t\t--> %.2lf\n", No);
        printf("----------------------------------------\n");

        libera_tela();

        //  Espera 1 segundo
        sleep(1);
    }
}

//  função destinada a rotina de Alarme (Quando a temperatura superar a referência de temperatura)
_Noreturn void thread_alarme_T(void) {
    while (1) {
        sensor_T_alarme(get_ref_T());    //  Inicia a função alarme do monitor de T
        aloca_tela();
        puts("\a\n");
        printf("ALARME!!!\nTemperatura maior que %.2lf °C\n", get_ref_T());
        libera_tela();
        sleep(1);   //  Espera 1 segundo
    }
}

//  função da rotina destinada a leitura e armazenamento dos dados dos sensores
_Noreturn void thread_le_sensores(void) {

    struct timespec time;
    double periodo = 10e6; // Periodo de execução do loop: 10ms
    char msg_enviada[1000];

    // Le a hora atual, coloca em time
    clock_gettime(CLOCK_MONOTONIC, &time);

    while (1) {
        // Espera até inicio do proximo periodo
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);

        // Coleta o valor de T, e armazena no monitor do sensor T
        strcpy(msg_enviada, "st-0");
        sensor_T_put(msg_socket(msg_enviada));

        // Coleta o valor de Ta, e armazena no monitor do sensor Ta
        strcpy(msg_enviada, "sta0");
        sensor_Ta_put(msg_socket(msg_enviada));

        // Coleta o valor de Ti, e armazena no monitor do sensor Ti
        strcpy(msg_enviada, "sti0");
        sensor_Ti_put(msg_socket(msg_enviada));

        // Coleta o valor de No, e armazena no monitor do sensor No
        strcpy(msg_enviada, "sno0");
        sensor_No_put(msg_socket(msg_enviada));

        // Coleta o valor de H, e armazena no monitor do sensor H
        strcpy(msg_enviada, "sh-0");
        sensor_H_put(msg_socket(msg_enviada));

        // Calcula inicio do proximo periodo
        time.tv_nsec += periodo;
        while (time.tv_nsec >= NSEC_PER_SEC) {
            time.tv_nsec -= NSEC_PER_SEC;
            time.tv_sec++;
        }
    }
}

int main(int argc, char *argv[]) {

    // Cria a conexão socket, com os argumentos obtidos na execução da função principal (Endereço porta)
    cria_socket(argv[1], atoi(argv[2]));

    char teclado[1000];

    //  Obtem o valor de referencia para H pelo terminal, e guarda no monitor de referência
    printf("Digite um valor de referência para o nível H: ");
    fgets(teclado, 1000, stdin);
    put_ref_H(atof(&teclado[0]));

    //  Obtem o valor de referencia para T pelo terminal, e guarda no monitor de referência
    printf("\nDigite um valor de referência para a temperatura da água T: ");
    fgets(teclado, 1000, stdin);
    put_ref_T(atof(&teclado[0]));

    // Cria elementos do tipo pthread para cada uma das rotinas
    pthread_t t_status, t_le_sensores, t_alarme_T, t_controle_H, t_controle_T, t_sensores_bufduplo, t_tempo_resp_h,
            t_tempo_resp_t;

    // Cria as threads para as rotinas, cujas funções são passadas como argumento
    pthread_create(&t_status, NULL, (void *) thread_status, NULL);
    pthread_create(&t_le_sensores, NULL, (void *) thread_le_sensores, NULL);
    pthread_create(&t_alarme_T, NULL, (void *) thread_alarme_T, NULL);
    pthread_create(&t_controle_H, NULL, (void *) thread_controle_H, NULL);
    pthread_create(&t_controle_T, NULL, (void *) thread_controle_T, NULL);
    pthread_create(&t_sensores_bufduplo, NULL, (void *) sensores_bufduplo_esperaBufferCheio, NULL);
    pthread_create(&t_tempo_resp_h, NULL, (void *) tempo_resp_h_bufduplo_esperaBufferCheio, NULL);
    pthread_create(&t_tempo_resp_t, NULL, (void *) tempo_resp_t_bufduplo_esperaBufferCheio, NULL);

    pthread_join(t_status, NULL);
    pthread_join(t_le_sensores, NULL);
    pthread_join(t_alarme_T, NULL);
    pthread_join(t_controle_H, NULL);
    pthread_join(t_controle_T, NULL);
    pthread_join(t_sensores_bufduplo, NULL);
    pthread_join(t_tempo_resp_h, NULL);
    pthread_join(t_tempo_resp_t, NULL);
}
