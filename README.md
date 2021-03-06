# Sistema_Caldeira

java -jar aquecedor2008_1.jar

gcc -o caldeira main.c Sensores/sensor_H.c Sensores/sensor_T.c Sensores/sensor_Ta.c Sensores/sensor_Ti.c Sensores/sensor_No.c Atuadores/atuador_Na.c Atuadores/atuador_Ni.c Atuadores/atuador_Nf.c tela.c socket.c sensores_bufduplo.c tempo_resp_h_bufduplo.c tempo_resp_t_bufduplo.c -lpthread

./caldeira localhost 4545
