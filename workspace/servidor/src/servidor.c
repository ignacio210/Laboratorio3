#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h> // gcc miThread.c -o miThread -l pthread
#include "estructuras.h"

// Firmas de funciones
ssize_t readLine(int fd, void *buffer, int n);

int inicializarJugador(struct Jugador nuevoJugador);

struct Jugador buscarJugador(int fdJugador);

struct MensajeNIPC armarListaJugadoresDisponibles();

int eligeJugador(struct MensajeNIPC * mensajeJugador);

int procesarJugada(struct MensajeNIPC * mensajeJugada);

void * handler_jugador(void * args);

int jugadorCount;
struct Jugador jugadores[MAXJUG];

// voy a tener un thread por cliente
pthread_t thread_ids[MAXJUG];

int main(int argc, char argv[]) {

	int sockfd;
	struct sockaddr_in self;
	char buffer[MAXBUF];

	// Se asume que el numero de jugadores maximo es chico, sino deberia gestionarse dinamicamente el tamano del array
	bzero(buffer, MAXBUF);
	jugadorCount = 0;

	// Se lee el puerto en el que va a correr el server de un archivo de configuracion.
	int puerto = leerConfiguracion();

	if (puerto == -1) {
		printf("Error al leer el archivo de configuracion.\n");
		return EXIT_FAILURE;
	}

	printf("Iniciando servicio...\n");

	// Creo el socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error al crear el socket");
		return EXIT_FAILURE;
	}

	//Inicializo la estructura del servidor
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(puerto);
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

	printf("Servicio iniciado en el puerto %d, esperando jugadores...\n",
			puerto);

	// Loop principal para atender clientes
	while (1) {

		int clientfd;
		struct sockaddr_in client_addr;
		int addrlen = sizeof(client_addr);

		// Acepto una nueva conexion
		clientfd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);

		// Cuando se conecta un jugador, armo un struct de tipo jugador con la informacion basica y
		// ejecuto un nuevo thread para que atienda al jugador.
		struct Jugador nuevoJugador;

		nuevoJugador.clientfd = clientfd;
		nuevoJugador.client_addr = client_addr; //TODO: Ver si esto se puede mejorar y solo pasarle el clientfd

		int result = pthread_create(&thread_ids[jugadorCount], NULL,
				handler_jugador, &nuevoJugador);

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

	int r, s;
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

	mensaje = (struct Mensaje *) buffer;

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

	struct MensajeNIPC mensajeRegistro;

	mensajeRegistro.tipo = Jugador_Registrado;
	mensajeRegistro.payload_length = sizeof(struct Jugador);
	memcpy(mensajeRegistro.payload, &nuevoJugador, sizeof(struct Jugador));

	s = send(nuevoJugador.clientfd, &mensajeRegistro,
			sizeof(struct MensajeNIPC), 0);

	if (s == -1) {
		perror("Error al el jugador registrado.\n");
		return -1;
	}

	return 0;
}

int enviarListaJugadoresDisponibles(int fd) {

	struct MensajeNIPC mensajeLista;

	mensajeLista = armarListaJugadoresDisponibles(fd);

	int bytesSent;

	bytesSent = send(fd, &mensajeLista, sizeof(struct MensajeNIPC), 0);

	if (bytesSent == -1) {
		perror("Error al enviar lista de jugadores");
		return -1;
	}

	return 0;
}

struct MensajeNIPC armarListaJugadoresDisponibles(int fd) {

	int i, jugadores_disp_count = 0;
	struct Jugador jugadores_disponibles[MAXJUG];

	struct MensajeNIPC mensaje;
	mensaje.tipo = Lista_Jugadores;
	mensaje.payload_length = 0;

	// Este buffer va a ser el payload del mensaje
	char buffer[MAXBUF];

	bzero(buffer, MAXBUF);

	for (i = 0; i < jugadorCount; i++) {

		if (jugadores[i].estado == Disponible && jugadores[i].clientfd != fd) {

			jugadores_disponibles[jugadores_disp_count] = jugadores[i];

			jugadores_disp_count++;
		}
	}

	if (jugadores_disp_count > 0) {
		// Serializo la lista de jugadores disponibles
		memcpy(mensaje.payload, jugadores_disponibles, sizeof(jugadores_disponibles));
		mensaje.payload_length = sizeof(jugadores_disponibles);
		//memcpy(mensaje.payload, buffer, sizeof(buffer));
	}

	return mensaje;
}

struct Jugador buscarJugador(int fdJugador) {

	int i;

	for (i = 0; i < MAXJUG; i++) {
		if (jugadores[i].clientfd == fdJugador)
			return jugadores[i];
	}
}

int eligeJugador(struct MensajeNIPC * mensajeJugador) {

	int numeroJugador, s;

	memcpy(&numeroJugador, mensajeJugador->payload,
			mensajeJugador->payload_length);

	// Empieza partida
	struct Jugador jugadorOrigen = buscarJugador(
			mensajeJugador->jugadorOrigen.clientfd);

	struct Jugador jugadorDest = buscarJugador(
			mensajeJugador->jugadorDestino.clientfd);

	printf("Empieza partida entre %s y %s.\n", jugadorOrigen.nombre,
			jugadorDest.nombre);

	jugadorOrigen.estado = Jugando;
	jugadorDest.estado = Jugando;

	struct MensajeNIPC mensajeConfirmacion;
	mensajeConfirmacion.tipo = Confirma_partida;
	mensajeConfirmacion.jugadorOrigen = jugadorOrigen;
	mensajeConfirmacion.jugadorDestino = jugadorDest;

	// Le envio al jugador origen una confirmacion de que empieza la partida
	s = send(jugadorOrigen.clientfd, &mensajeConfirmacion,
			sizeof(struct MensajeNIPC), 0);

	if (s == -1) {
		perror("Error al enviar confirmacion.\n");
		return -1;
	}

	// Le envio al jugador destino una confirmacion de que va a empezar la partida
	s = send(jugadorDest.clientfd, &mensajeConfirmacion,
			sizeof(struct MensajeNIPC), 0);

	if (s == -1) {
		perror("Error al el jugador registrado.\n");
		return -1;
	}
}

int procesarJugada(struct MensajeNIPC * mensajeJugada) {

	int s;

	// Envio jugada al jugador destino

	// Le cambio el tipo al mensaje y lo reenvio
	mensajeJugada->tipo = Recibe_Ataque;

	printf("Recibi una jugada de %d, se la envio a %d.\n",
			mensajeJugada->jugadorOrigen.clientfd,
			mensajeJugada->jugadorDestino.clientfd);

	s = send(mensajeJugada->jugadorDestino.clientfd, mensajeJugada,
			sizeof(struct MensajeNIPC), 0);

	if (s == -1) {
		perror("Error al enviar el mensaje.\n");
		return -1;
	}
}

// Funcion principal que ejecuta el thread para cada jugador que se conecta
void * handler_jugador(void * args) {

	int r, s;

	// Al crear el thread se le pasa una estructura de tipo jugador
	struct Jugador jugador = (*(struct Jugador *) args);
	int fdJugador = jugador.clientfd;

	// Muestro informacion del cliente conectado
	printf("%s:%d conectado\n", inet_ntoa(jugador.client_addr.sin_addr),
			ntohs(jugador.client_addr.sin_port));

	// TODO: Ver de agregar semaforos
	if (inicializarJugador(jugador) == -1) {
		printf("Error al inicializar Jugador.\n");
		abort();
	}

	// TODO: Agregar semaforo
	jugadorCount++;

	char buffer[MAXBUF];

	bzero(buffer, MAXBUF);

	TIPO_MENSAJE tipo;

	while (1) {

		r = recv(fdJugador, buffer, sizeof(struct MensajeNIPC), 0);

		if (r == -1) {
			perror("Error al recibir el mensaje.\n");
			abort();
		}

		struct MensajeNIPC * mensajeJugador = (struct MensajeNIPC *) buffer;

		switch (mensajeJugador->tipo) {

		case Elige_Jugador:

			eligeJugador(mensajeJugador);

			break;

		case Lista_Jugadores:

			if (enviarListaJugadoresDisponibles(fdJugador) == -1) {
				printf("Error al enviar lista de jugadores.\n");
				abort();
			}

			break;

		case Juega:

			if (procesarJugada(mensajeJugador) == -1) {
				printf("Error al procesar la jugada.\n");
				abort();
			}
			break;

		case Resultado:

			// Reenvio el mensaje al jugador correspondiente
			s = send(mensajeJugador->jugadorDestino.clientfd, mensajeJugador,
					sizeof(struct MensajeNIPC), 0);

			if (s == -1) {
				perror("Error al enviar el mensaje de resultado.\n");
				abort();
			}

			break;
		case Fin_partida:

			// Envio mensaje de finalizacion a los 2 jugadores
			s = send(mensajeJugador->jugadorDestino.clientfd, mensajeJugador,
					sizeof(struct MensajeNIPC), 0);

			if (s == -1) {
				perror("Error al enviar el mensaje de resultado.\n");
				abort();
			}

			s = send(mensajeJugador->jugadorOrigen.clientfd, mensajeJugador,
					sizeof(struct MensajeNIPC), 0);

			if (s == -1) {
				perror("Error al enviar el mensaje de resultado.\n");
				abort();
			}

			break;
		}
	}
}

int leerConfiguracion() {

	int fd, r;

	fd = open("config.txt", O_RDONLY);

	if (fd == -1) {
		perror("Error al abrir el archivo de configuracion\n");
		return -1;
	}

	char buffer[50];

	bzero(buffer, 50);
	r = readLine(fd, buffer, 50);

	bzero(buffer, 50);
	r = readLine(fd, buffer, 50);

	int puerto = atoi(buffer);

	close(fd);

	return puerto;
}

ssize_t readLine(int fd, void *buffer, int n) {
	ssize_t numRead; /* # of bytes fetched by last read() */
	size_t totRead; /* Total bytes read so far */
	char *buf;
	char ch;

	if (n <= 0 || buffer == NULL ) {
		errno = EINVAL;
		return -1;
	}

	buf = buffer; /* No pointer arithmetic on "void *" */

	totRead = 0;
	for (;;) {
		numRead = read(fd, &ch, 1);

		if (numRead == -1) {
			if (errno == EINTR) /* Interrupted --> restart read() */
				continue;
			else
				return -1; /* Some other error */

		} else if (numRead == 0) { /* EOF */
			if (totRead == 0) /* No bytes read; return 0 */
				return 0;
			else
				/* Some bytes read; add '\0' */
				break;

		} else { /* 'numRead' must be 1 if we get here */

			if (ch == '\n') {
				break;
			}

			if (totRead < n - 1) { /* Discard > (n - 1) bytes */
				totRead++;
				*buf++ = ch;
			}
		}
	}

	*buf = '\0';
	return totRead;
}

