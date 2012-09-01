/*LIBRERIAS */

/*Resultados Posibles*/

#define AGUA          0
#define BARCO         1
#define HUNDIDO       2

/*Datos servidor*/
#define MAXBUF		1024
#define MAXJUG		200
#define PUERTO		9999 // TODO: Ver de donde sale el puerto

/*DRAW MATRIX CONST*/
#define NUMBER_X 10
#define NUMBER_Y 10
#define VALUE 48

/*MODIFICAR PARA TOMAR LA IP DE ARCHIVO DE CONFIGURACION*/
#define PORT_TIME       13
#define MY_PORT        9999
#define SERVER_ADDR     "127.0.0.1"     /* localhost */
#define MAXBUF          1024

#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "estructuras.h"
#include "cargarArchivo.h"
