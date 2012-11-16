#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <errno.h>
#include <string.h>
#include "estructuras.h"

#define PORT_TIME       13
#define MY_PORT        9997
#define SERVER_ADDR     "127.0.0.1"     /* localhost */
#define MAXBUF          2048

// Defines y variables para funcion grafica
#define NUMBER_X 10
#define NUMBER_Y 10
#define VALUE 48

char my_matrix[NUMBER_X][NUMBER_Y];
char remote_matrix[NUMBER_X][NUMBER_Y];
int i, j, cantidad_barcos;

char ** parametros;

int sockfd;

// Estructura jugador registrada en el server
struct Jugador jugador;

// Lista de jugadores disponibles (se va actualizando)
struct Jugador jugadoresDisponibles[MAXJUG];

void matrix_init() {
	for (i = 0; i < NUMBER_X; i++) {
		for (j = 0; j < NUMBER_Y; j++) {
			my_matrix[i][j] = 'a';
			remote_matrix[i][j] = 'a';
		}
	}
}

void print_map_line(char value[]) {
	printf("| ");
	for (j = 0; j < NUMBER_Y; j++) {
		if (value[j] == 'a') {
			printf(".");
		} else {
			printf("x");
		}
		printf(" ");
	}
	printf("|");
}

void print_maps() {
	printf("\t\t My map \t\t\t\t\t Remote Map\t\n");
	print_header();
	printf("\t");
	print_header();
	printf("\n");
	for (i = 0; i < NUMBER_X; i++) {
		print_map_line(my_matrix[i]);
		printf("\t");
		print_map_line(remote_matrix[i]);
		printf("\n");
	}
	print_header();
	printf("\t");
	print_header();
	printf("\n");
}

void print_header() {
	printf("+");
	for (i = 0; i < 43; i++) {
		printf("-");
	}
	printf("+");
}

void liberar_recursos() {
	close(sockfd);
}

int iniciarPartida(struct Partida partida) {

	printf("Se inicia la partida entre %s y %s.\n", partida.jugador1.nombre,
			partida.jugador2.nombre);

	matrix_init();

	for (i = 1; i < cantidad_barcos; i++) {
		my_matrix[parametros[i][0] - VALUE][parametros[i][1] - VALUE] = 'b';
	}

	while (1) {
		print_maps();
		printf("enter value\n");
		getchar();
		system("clear");
	}

	while (1) {
		sleep(1);
	}

	return 0;
}

int elegirJugador() {

	int jugadorElegido, s, r;
	char buffer[MAXBUF];
	struct MensajeNIPC * mensajeConfirmacion;

	printf("Elegir jugador: ");
	scanf("%d", &jugadorElegido);

	printf("\n\nJugador elegido: %d - %s.\n", jugadorElegido,
			jugadoresDisponibles[jugadorElegido - 1].nombre);

	// Envio la informacion del jugador elegido al server
	struct MensajeNIPC mensajeJugador;

	mensajeJugador.tipo = Elige_Jugador;
	mensajeJugador.jugadorOrigen = jugador;
	mensajeJugador.jugadorDestino = jugadoresDisponibles[jugadorElegido - 1];

	mensajeJugador.payload_length = sizeof(int);
	memcpy(mensajeJugador.payload, &jugadorElegido, sizeof(jugadorElegido));

	s = send(sockfd, &mensajeJugador, sizeof(struct MensajeNIPC), 0);

	if (s == -1) {
		return -1;
	}

	bzero(buffer, MAXBUF);

	// Espero a recibir del server una confirmacion de que la partida va a iniciar.
	r = recv(sockfd, buffer, sizeof(struct MensajeNIPC), 0);

	if (r == -1) {
		perror("Error al recibir el mensaje.\n");
		return -1;
	}

	mensajeConfirmacion = (struct MensajeNIPC *) buffer;

	if (mensajeConfirmacion->tipo != Confirma_partida) {
		printf("El tipo de mensaje recibido no es valido.\n");
		return -1;
	}

	struct Partida partida;

	partida.jugador1 = jugador;
	partida.jugador2 = jugadoresDisponibles[jugadorElegido - 1];

	iniciarPartida(partida);

	return 0;
}

int main(int argc, char * argv[]) {
	int s, r;
	struct sockaddr_in dest;
	char buffer[MAXBUF];
	struct Mensaje mensaje;

	if (argc < 3) {
		puts("Usage ./cliente [nombre] [pos1] ...");
		return EXIT_FAILURE;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return EXIT_FAILURE;
	}

	cantidad_barcos = argc;
	parametros = argv;

	/*---Initialize server address/port struct---*/
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(MY_PORT);
	if (inet_aton(SERVER_ADDR, &dest.sin_addr.s_addr) == 0) {
		perror(SERVER_ADDR);
		return EXIT_FAILURE;
	}

	/*---Connect to server---*/
	if (connect(sockfd, &dest, sizeof(dest)) != 0) {
		perror("Connect ");
		return EXIT_FAILURE;
	}

	bzero(buffer, MAXBUF);

	// Enviar nombre jugador
	mensaje.tipo = Registra_Nombre;
	strcpy(mensaje.contenido, argv[1]);

	sleep(2);

	s = send(sockfd, &mensaje, sizeof(mensaje), 0);

	if (s == -1) {
		perror("Error al enviar el nombre del jugador");
		return EXIT_FAILURE;
	}

	// Recibo la estructura del jugador registrado.
	r = recv(sockfd, buffer, sizeof(struct MensajeNIPC), 0);

	memcpy(&jugador, buffer + sizeof(TIPO_MENSAJE) + sizeof(int),
			sizeof(struct Jugador));

	printf("Se ha registrado en el servidor como el jugador %s.\n",
			jugador.nombre);

	// Dar la opcion de elegir un jugador de la lista o esperar a que alguien lo elija
	printf("Menu:\n\n");

	printf("\t1. Elegir un contrincante.\n");
	printf("\t2. Esperar a ser elegido.\n");

	int opcion;

	printf("Ingrese opcion:");
	scanf("%d", &opcion);

	if (opcion == 1) {

		// Pido la lista de jugadores

		struct MensajeNIPC mensajeLista;
		mensajeLista.tipo = Lista_Jugadores;

		s = send(sockfd, &mensajeLista, sizeof(struct MensajeNIPC), 0);

		if (s == -1) {
			perror("Error al enviar el mensaje.\n");
			return EXIT_FAILURE;
		}

		// Recibo lista de jugadores
		r = recv(sockfd, buffer, sizeof(struct MensajeNIPC), 0);

		TIPO_MENSAJE tipo;

		memcpy(&tipo, buffer, sizeof(TIPO_MENSAJE));

		int payloadLength;

		memcpy(&payloadLength, buffer + sizeof(TIPO_MENSAJE), sizeof(int));

		memcpy(jugadoresDisponibles,
				buffer + sizeof(TIPO_MENSAJE) + sizeof(int), payloadLength);

		int i = 0;

		char messageBuffer[MAXBUF];

		strcat(messageBuffer, "Lista de jugadores disponibles:\n\n");

		while (jugadoresDisponibles[i].clientfd != 0) {

			char num[5];
			sprintf(num, "%d", i + 1);
			strcat(messageBuffer, num);
			strcat(messageBuffer, ". ");
			strcat(messageBuffer, jugadoresDisponibles[i].nombre);
			strcat(messageBuffer, "(");
			strcat(messageBuffer, jugadoresDisponibles[i].ip);
			strcat(messageBuffer, ")\n");

			i++;
		}

		printf("%s\n", messageBuffer);

		if (elegirJugador() == -1) {
			perror("Error al elegir jugador.\n");
			liberar_recursos();
			return EXIT_FAILURE;
		}
	} else {

		int r;
		struct MensajeNIPC * mensajeConfirmacion;

		printf("Esperando que se inicie una partida.\n");

		r = recv(sockfd, buffer, sizeof(struct MensajeNIPC), 0);

		if (r == -1) {
			perror("Error al recibir el mensaje.\n");
			return EXIT_FAILURE;
		}

		mensajeConfirmacion = (struct MensajeNIPC *) buffer;

		if (mensajeConfirmacion->tipo != Confirma_partida) {
			printf("El tipo de mensaje recibido no es valido.\n");
			return EXIT_FAILURE;
		}

		struct Partida partida;

		partida.jugador1 = mensajeConfirmacion->jugadorOrigen;
		partida.jugador2 = mensajeConfirmacion->jugadorDestino;

		iniciarPartida(partida);
	}

	return EXIT_SUCCESS;
}
