
//Definicao de Bibliotecas
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "socket.h"
#include "sensor_H.h"
#include "sensor_T.h"
#include "sensor_Ta.h"
#include "sensor_Ti.h"
#include "sensor_No.h"
#include "atuador_Na.h"
#include "atuador_Ni.h"
#include "atuador_Nf.h"
#include "tela.h"

double H_ref;
double T_ref;

_Noreturn void thread_controle_T(void) {

    struct timespec time;
    int periodo = 50000000;    // 50ms
    time.tv_nsec = time.tv_nsec + periodo;
    double t, Q;
    char msg_enviada[1000];

    double R = 0.001;   // resistência térmica do isolamento (2mm madeira) [0.001 Grau / (Joule/segundo)]
    double Qe;          // fluxo de calor através do isolamento do recipiente [Joule/segundo]

    while (1) {

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);

        Qe = (sensor_T_get() - sensor_Ta_get()) / R;

        if (Qe > 1000000.0){
            Q = 1000000.0;
        } else {
            Q = Qe;
        }
        sprintf(msg_enviada, "aq-%lf", Q);
        msg_socket(msg_enviada);

        t = sensor_T_get();

        if (t < T_ref) {
            sprintf(msg_enviada, "aq-%lf", 1000000.0);
            msg_socket(msg_enviada);

            if (atuador_Na_get() < 10){

                atuador_Ni_put(atuador_Ni_get() + 10 - atuador_Na_get());
                sprintf(msg_enviada, "anf%lf", atuador_Nf_get());
                msg_socket(msg_enviada);

                atuador_Na_put(atuador_Nf_get() + 10 - atuador_Na_get());
                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);
            }
        }
        if (t >= T_ref) {
            Q = Qe;
            sprintf(msg_enviada, "aq-%lf", Q);
            msg_socket(msg_enviada);
        }
    }
}

double v_abs(double x) {
    if (x < 0) {
        return -x;
    }
    if (x >= 0) {
        return x;
    }
}

_Noreturn void thread_controle_H(void) {
    struct timespec time;
    int periodo = 70000000;    // 70ms
    time.tv_nsec = time.tv_nsec + periodo - 70000000;
    double t, ti, Ni, Na, No, Nf, h, l, dh, dNi, dNa;

    char msg_enviada[1000];

    sprintf(msg_enviada, "ani%lf", 0.0);
    msg_socket(msg_enviada);

    sprintf(msg_enviada, "ana%lf", 0.0);
    msg_socket(msg_enviada);

    while (1) {

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);

        h = sensor_H_get();

        if ((h < H_ref) && ((h / H_ref) < 0.99)) {
            t = sensor_T_get();
            ti = sensor_Ti_get();
            No = sensor_No_get();
            Nf = atuador_Nf_get();
            dh = ((H_ref - h) / H_ref) + 1;
            l = v_abs((ti - t) / (80 - t));
            if (No == 0 && Nf == 0) {

                Ni = 50.0 * dh;
                Na = l * Ni;

                if (Ni > 100){
                    dNi = 100 / Ni;
                    Ni = dNi * Ni;
                    Na = dNi * Na;
                }

                if (Na > 10){
                    dNa = 10 / Na;
                    Ni = dNa * Ni;
                    Na = dNa * Na;
                }

                atuador_Ni_put(Ni);
                atuador_Na_put(Na);

                sprintf(msg_enviada, "ani%lf", atuador_Ni_get());
                msg_socket(msg_enviada);

                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);

            } else {

                Ni = ((No + Nf) / (l + 1)) * dh * 10;
                Na = l * atuador_Ni_get();

                if (Ni > 100){
                    dNi = 100 / Ni;
                    Ni = dNi * Ni;
                    Na = dNi * Na;
                }

                if (Na > 10){
                    dNa = 10 / Na;
                    Ni = dNa * Ni;
                    Na = dNa * Na;
                }
                atuador_Ni_put(Ni);
                atuador_Na_put(Na);

                sprintf(msg_enviada, "ani%lf", atuador_Ni_get());
                msg_socket(msg_enviada);

                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);
            }
        } else {
            if (atuador_Nf_get() + No != atuador_Na_get() + atuador_Ni_get()) {
                atuador_Na_put(0.0);
                atuador_Ni_put(0.0);
                atuador_Nf_put(0.0);

                sprintf(msg_enviada, "ani%lf", atuador_Ni_get());
                msg_socket(msg_enviada);

                sprintf(msg_enviada, "ana%lf", atuador_Na_get());
                msg_socket(msg_enviada);

                sprintf(msg_enviada, "anf%lf", atuador_Na_get());
                msg_socket(msg_enviada);
            }
        }
    }
}

_Noreturn void thread_status(void) {
    double t, ta, ti, No, h;
    while (1) {
        t = sensor_T_get();
        ta = sensor_Ta_get();
        ti = sensor_Ti_get();
        No = sensor_No_get();
        h = sensor_H_get();
        aloca_tela();
        printf("\33[H\33[2J");
        printf("----------------------------------------\n");
        printf("|Temperatura no interior (T)\t--> %.2lf\n", t);
        printf("|Altura da Coluna(H)\t\t--> %.2lf\n", h);
        printf("|T: Ambiente em volta (Ta)\t--> %.2lf\n", ta);
        printf("|T: Água que entra (Ti)\t\t--> %.2lf\n", ti);
        printf("|Fluxo de Saída (No)\t\t--> %.2lf\n", No);
        printf("----------------------------------------\n");

        libera_tela();
        sleep(1);
        //
    }
}

_Noreturn void thread_alarme_T(void) {
    struct timespec t;
    int periodo = 10000000;    // 10ms
    t.tv_nsec = t.tv_nsec + periodo;
    while (1) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
        sensor_T_alarme(30);
        aloca_tela();
        puts("\a\n");
        printf("ALARME!!!\n");
        libera_tela();
    }
}

_Noreturn void thread_le_sensor(void) {
    char msg_enviada[1000];
    while (1) {
        strcpy(msg_enviada, "st-0");
        sensor_T_put(msg_socket(msg_enviada));

        strcpy(msg_enviada, "sta0");
        sensor_Ta_put(msg_socket(msg_enviada));

        strcpy(msg_enviada, "sti0");
        sensor_Ti_put(msg_socket(msg_enviada));

        strcpy(msg_enviada, "sno0");
        sensor_No_put(msg_socket(msg_enviada));

        strcpy(msg_enviada, "sh-0");
        sensor_H_put(msg_socket(msg_enviada));
    }
}

///Controle



int main(int argc, char *argv[]) {

    cria_socket(argv[1], atoi(argv[2]));

    char teclado[1000];

    printf("Digite um valor de referência para o nível H: ");
    fgets(teclado, 1000, stdin);
    H_ref = atof(&teclado[0]);

    printf("\nDigite um valor de referência para a temperatura da água T: ");
    fgets(teclado, 1000, stdin);
    T_ref = atof(&teclado[0]);

    pthread_t t1, t2, t3, t4, t5;

    pthread_create(&t1, NULL, (void *) thread_status, NULL);
    pthread_create(&t2, NULL, (void *) thread_le_sensor, NULL);
    pthread_create(&t3, NULL, (void *) thread_alarme_T, NULL);
    pthread_create(&t4, NULL, (void *) thread_controle_H, NULL);
    pthread_create(&t5, NULL, (void *) thread_controle_T, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_join(t5, NULL);

}
