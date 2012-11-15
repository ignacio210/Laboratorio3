#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <errno.h>
#include <string.h>
#include "estructuras.h"

#define PORT_TIME       13
#define MY_PORT        9999
#define SERVER_ADDR     "127.0.0.1"     /* localhost */
#define MAXBUF          1024

int main(int argc, char * argv[])
{
	int sockfd, s, r;
    struct sockaddr_in dest;
    char buffer[MAXBUF];
    struct Mensaje mensaje;

    if(argc < 3) {
    	puts("Usage ./cliente [nombre] [pos1] ...");
    	return EXIT_FAILURE;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket");
        return EXIT_FAILURE;
    }

    /*---Initialize server address/port struct---*/
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(MY_PORT);
    if ( inet_aton(SERVER_ADDR, &dest.sin_addr.s_addr) == 0 )
    {
        perror(SERVER_ADDR);
        return EXIT_FAILURE;
    }

    /*---Connect to server---*/
    if (connect(sockfd, &dest, sizeof(dest)) != 0)
    {
        perror("Connect ");
        return EXIT_FAILURE;
    }

    bzero(buffer, MAXBUF);

    // Enviar nombre jugador

    mensaje.tipo = Registra_Nombre;
    strcpy(mensaje.contenido, argv[1]);

    s = send(sockfd, &mensaje, sizeof(mensaje), 0);

    if(s == -1) {
    	perror("Error al enviar el nombre del jugador");
    	return EXIT_FAILURE;
    }

    // Recibo lista de jugadores
    r = recv(sockfd, buffer, sizeof(struct Mensaje), 0);

    struct Mensaje * mensajeLista;

    if(r == -1) {
        	perror("Error al recibir lista de jugadores");
        	return EXIT_FAILURE;
    }

    mensajeLista = buffer;

    if(mensajeLista->tipo != Lista_Jugadores)
    	return EXIT_FAILURE;

    printf("%s", mensajeLista->contenido);

    close(sockfd);
    return EXIT_SUCCESS;
}