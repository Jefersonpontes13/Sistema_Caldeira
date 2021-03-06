cmake_minimum_required(VERSION 3.19)
project(Sistema_Caldeira C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Sistema_Caldeira
        atuador_Na.c
        atuador_Na.h
        atuador_Nf.c
        atuador_Nf.h
        atuador_Ni.c
        atuador_Ni.h
        main.c
        sensor_H.c
        sensor_H.h
        sensor_No.c
        sensor_No.h
        sensor_T.c
        sensor_T.h
        sensor_Ta.c
        sensor_Ta.h
        sensor_Ti.c
        sensor_Ti.h
        sensores_bufduplo.c
        sensores_bufduplo.h
        socket.c
        socket.h
        tela.c
        tela.h)
