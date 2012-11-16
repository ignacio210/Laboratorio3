#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h> // gcc miThread.c -o miThread -l pthread
#include "estructuras.h"

#define MAXBUF		2048
#define PUERTO		9998 // TODO: Sacar el puerto de un archivo de configuracion
int jugadorCount;
struct Jugador jugadores[MAXJUG];

// voy a tener un thread por cliente
pthread_t thread_ids[MAXJUG];

// TODO: Agregar firmas de funciones
int inicializarJugador(struct Jugador nuevoJugador);

struct MensajeNIPC armarListaJugadoresDisponibles();

void eligeJugador(struct MensajeNIPC * mensajeJugador) {

	int numeroJugador;

	memcpy(&numeroJugador, mensajeJugador->payload, mensajeJugador->payload_length);

	printf("Se ha elegido al jugador %d, %s.\n", numeroJugador, jugadores[numeroJugador - 1].nombre);
}

void * handler_jugador(void * args) {

	int r;

	struct Jugador jugador = (*(struct Jugador *)args);

	// Muestro informacion del cliente conectado
	printf("%s:%d conectectado\n", inet_ntoa(jugador.client_addr.sin_addr),
			ntohs(jugador.client_addr.sin_port));

	// TODO: Ver de agregar semaforos en esta funcion donde corresponda
	if (inicializarJugador(jugador) == -1) {
		printf("Error al inicializar Jugador.\n");
		abort();
	}

	// TODO: Agregar semaforo
	jugadorCount++;

	struct MensajeNIPC mensajeLista;

	mensajeLista = armarListaJugadoresDisponibles();

	// Imprimo la lista de jugadores para info de debug
	//printf("%s", buffer);

	if (enviarListaJugadoresDisponibles(jugador.clientfd, mensajeLista) == -1) {
		printf("Error al enviar lista de jugadores.\n");
		abort();
	}

	char buffer[MAXBUF];
	TIPO_MENSAJE tipo;

	while(1) {

		r = recv(jugador.clientfd, buffer, sizeof(struct MensajeNIPC), 0);

		if(r == -1) {
			perror("Error al recibir el mensaje.\n");
			abort();
		}

		struct MensajeNIPC * mensajeJugador;

		mensajeJugador = (struct MensajeNIPC *)buffer;

		switch(mensajeJugador->tipo) {

			case Elige_Jugador:

				eligeJugador(mensajeJugador);

				break;
		}
	}
}





int main(int argc, char argv[]) {

	int sockfd;
	struct sockaddr_in self;
	char buffer[MAXBUF];

	// Se asume que el numero de jugadores maximo es chico, sino deberia gestionarse dinamicamente el tamano del array
	bzero(buffer, MAXBUF);
	jugadorCount = 0;

	printf("Iniciando servicio...\n");

	// Creo el socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error al crear el socket");
		return EXIT_FAILURE;
	}

	//Inicializo la estructura del servidor
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(PUERTO);
	self.sin_addr.s_addr = INADDR_ANY;

	// Asigno el numero de puerto al socket creado
	if (bind(sockfd, (struct sockaddr*) &self, sizeof(self)) != 0) {
		perror("Error en bind del servidor");
		return EXIT_FAILURE;
	}

	// Hago que el socket escuche en el puerto especificado
	if (listen(sockfd, 20) != 0) {
		perror("Error en listen del servidor");
		return EXIT_FAILURE;
	}

	printf("Servicio iniciado en el puerto %d, esperando jugadores...\n", PUERTO);

	// Loop principal para atender clientes
	while (1) {
		int clientfd;
		struct sockaddr_in client_addr;
		int addrlen = sizeof(client_addr);

		// Acepto una nueva conexion
		clientfd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);

		struct Jugador nuevoJugador;

		nuevoJugador.clientfd = clientfd;
		nuevoJugador.client_addr = client_addr;

		int result = pthread_create(&thread_ids[jugadorCount], NULL, handler_jugador, &nuevoJugador);

		if (result != 0) {
			perror("Error en la creacion del thread\n");
			return EXIT_FAILURE;
		}

		// TODO: ver cuando se cierra el clientfd
		//close(clientfd);
	}

	// Cierro el socket
	close(sockfd);
	return EXIT_SUCCESS;
}

int inicializarJugador(struct Jugador nuevoJugador) {

	int r;
	char buffer[MAXBUF];
	struct Mensaje * mensaje;

	// Validacion maximo jugadores
	// TODO: Semaforo
	if (jugadorCount >= MAXJUG)
		return -1;

	bzero(buffer, MAXBUF);

	// Recibo el nombre del jugador
	r = recv(nuevoJugador.clientfd, buffer, sizeof(struct Mensaje), 0);

	if (r == -1) {
		printf("Error al recibir el nombre del jugador.\n");
		return -1;
	}

	mensaje = (struct Mensaje *)buffer;

	if (mensaje->tipo != Registra_Nombre)
		return -1;

	strcpy(nuevoJugador.nombre, mensaje->contenido);

	// Inicio al jugador como disponible
	nuevoJugador.estado = Disponible;
	strcpy(nuevoJugador.ip, inet_ntoa(nuevoJugador.client_addr.sin_addr));

	// TODO: Aca abria que agregar un semaforo y antes validar que no se haya
	// alterado el jugadorCount, posiblemente la secci'on critica sea casi todo
	// el metodo
	jugadores[jugadorCount] = nuevoJugador;

	return 0;
}

int enviarListaJugadoresDisponibles(int fd, struct MensajeNIPC mensaje) {

	int bytesSent;

	bytesSent = send(fd, &mensaje, sizeof(struct MensajeNIPC), 0);

	if (bytesSent == -1) {
		perror("Error al enviar lista de jugadores");
		return -1;
	}

	return 0;
}

struct MensajeNIPC armarListaJugadoresDisponibles() {

	int i, jugadores_disp_count = 0;
	struct Jugador jugadores_disponibles[MAXJUG];

	struct MensajeNIPC mensaje;

	// Este buffer va a ser el payload del mensaje
	char buffer[MAXBUF];

	bzero(buffer, MAXBUF);

	for (i = 0; i < jugadorCount; i++) {

		if (jugadores[i].estado == Disponible) {

			jugadores_disponibles[jugadores_disp_count] = jugadores[i];

			jugadores_disp_count++;
		}
	}

	// Serializo la lista de jugadores disponibles
	memcpy(buffer, jugadores_disponibles, sizeof(jugadores_disponibles));

	mensaje.tipo = Lista_Jugadores;
	mensaje.payload_length = sizeof(jugadores_disponibles);
	memcpy(mensaje.payload, buffer, sizeof(buffer));

	return mensaje;
}

