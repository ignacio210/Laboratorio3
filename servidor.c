#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "estructuras.h"

#define MAXBUF		1024
#define MAXJUG		16
#define PUERTO		9999 // TODO: Sacar el puerto de un archivo de configuracion

int jugadorCount;
struct Jugador jugadores[MAXJUG];
char buffer[MAXBUF];

int main(int argc, char argv[]) {

	int sockfd;
	struct sockaddr_in self;

	// Se asume que el numero de jugadores maximo es chico, sino deberia gestionarse dinamicamente el tamano del array
	bzero(buffer, MAXBUF);
	jugadorCount = 0;

	// Creo el socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("Error al crear el socket");
		return EXIT_FAILURE;
	}

	//Inicializo la estructura del servidor
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(PUERTO);
	self.sin_addr.s_addr = INADDR_ANY;

	// Asigno el numero de puerto al socket creado
	if(bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0)
	{
		perror("Error en bind del servidor");
		return EXIT_FAILURE;
	}

	// Hago que el socket escuche en el puerto especificado
	if (listen(sockfd, 20) != 0)
	{
		perror("Error en listen del servidor");
		return EXIT_FAILURE;
	}

	// Loop principal para atender clientes
	while (1)
	{
		int clientfd;
		struct sockaddr_in client_addr;
		int addrlen = sizeof(client_addr);

		// Acepto una nueva conexion
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);

		// Muestro informacion del cliente conectado
		printf("%s:%d conectectado\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		// Aca arrancaria el thread
		if(inicializarJugador(clientfd, client_addr) == -1) {
			perror("Error al inicializar Jugador");
			return EXIT_FAILURE;
		}

		jugadorCount++;

		armarListaJugadoresDisponibles();

		// Imprimo la lista de jugadores para info de debug
		printf("%s", buffer);

		if(enviarListaJugadoresDisponibles(clientfd) == -1) {
			perror("Error al enviar lista de jugadores");
			return EXIT_FAILURE;
		}

		// TODO: ver cuando se cierra el clientfd
		//close(clientfd);
	}

	// Cierro el socket
	close(sockfd);
	return EXIT_SUCCESS;
}

int inicializarJugador(int fd, struct sockaddr_in client_addr) {

	int r;
	struct Jugador nuevoJugador;
	struct Mensaje * mensaje;

	// Validacion maximo jugadores
	if(jugadorCount >= MAXJUG)
		return -1;

	bzero(buffer, MAXBUF);

	// Recibo el nombre del jugador
	r = recv(fd, buffer, sizeof(struct Mensaje), 0);

	if(r == -1) {
		perror("Error al recibir el nombre del jugador");
		return -1;
	}

	mensaje = buffer;

	if(mensaje->tipo != Registra_Nombre)
		return -1;

	strcpy(nuevoJugador.nombre, mensaje->contenido);

	nuevoJugador.clientfd = fd;
	nuevoJugador.estado = Disponible;
	strcpy(nuevoJugador.ip, inet_ntoa(client_addr.sin_addr));

	jugadores[jugadorCount] = nuevoJugador;

	return 0;
}

int enviarListaJugadoresDisponibles(int fd) {

	int bytesSent;
	struct Mensaje mensaje;

	mensaje.tipo = Lista_Jugadores;
	strcpy(mensaje.contenido, buffer);

	bytesSent = send(fd, &mensaje, sizeof(struct Mensaje), 0);

	if(bytesSent == -1) {
		perror("Error al enviar lista de jugadores");
		return -1;
	}

	return 0;
}

int armarListaJugadoresDisponibles() {

	int i;

	bzero(buffer, MAXBUF);

	strcat(buffer, "Lista de jugadores disponibles:\n\n");

	for(i=0; i<jugadorCount; i++) {

		if(jugadores[i].estado == Disponible) {

			char num[5];
			sprintf(num, "%d", i+1);
			strcat(buffer, num); // Todo, ver como castear int a string
			strcat(buffer, ". ");
			strcat(buffer, jugadores[i].nombre);
			strcat(buffer, "(");
			strcat(buffer, jugadores[i].ip);
			strcat(buffer, ")\n");
		}
	}

	return 0;
}


