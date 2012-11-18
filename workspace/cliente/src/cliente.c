#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <resolv.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "estructuras.h"

#define PORT_TIME       13

// Defines y variables para funcion grafica
#define NUMBER_X 10
#define NUMBER_Y 10
#define VALUE 48

// Firmas de funciones
int validarPosiciones(char ** argv, char ** posiciones);

struct Conexion leerConfiguracion();

ssize_t readLine(int fd, void *buffer, int n);

int esperarPartida();

void matrix_init();

void print_map_line(char value[]);

void print_maps();

void print_header();

void liberar_recursos();

void * leerJugada(void * args);

void * escucharServidor(void * args);

int iniciarPartida(struct Partida partida);

int elegirJugador();

int handler_ataque(struct MensajeNIPC * mensaje);

// Variables para interfaz grafica
char my_matrix[NUMBER_X][NUMBER_Y];
char remote_matrix[NUMBER_X][NUMBER_Y];
int i, j;

// file descriptor del socket del servidor
int sockfd;

int cantidad_barcos;
char * posiciones_barcos[100];

int cantidad_barcos_hundidos;
char posiciones_barcos_hundidos[100][10];

pthread_t t1, t2;

// Mutex para evitar que los 2 threads actualizen la pantalla al mismo tiempo
pthread_mutex_t mutex_dibujar_pantalla = PTHREAD_MUTEX_INITIALIZER;

// Estructura jugador registrada en el server
struct Jugador jugador;

// Estructura que contiene los datos de la partida en curso
struct Partida partida;

// Flag que va a indicar si es turno del jugador o del rival
int turno = 0;

// Lista de jugadores disponibles (se va actualizando)
struct Jugador jugadoresDisponibles[MAXJUG];
int jugadoresDisponiblesCont;

int main(int argc, char * argv[]) {

	int s, r;
	struct sockaddr_in dest;
	char buffer[MAXBUF];
	struct Mensaje mensaje;

	if (argc < 3) {
		puts("Usage ./cliente [nombre] [pos1] ...");
		return EXIT_FAILURE;
	}

	cantidad_barcos_hundidos = 0;

	// Validacion de las posiciones de la matriz
	cantidad_barcos = argc - 2; // Le resto los primeros 2 argumentos: ejecutable y nombre de jugador

	//char * posiciones[cantidad_barcos];

	if (validarPosiciones(argv, posiciones_barcos) == -1) {
		return EXIT_FAILURE;
	}

	//posiciones_barcos = posiciones;

	// Leo ip y puerto del servidor del archivo de configuracion
	struct Conexion conexion = leerConfiguracion();

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return EXIT_FAILURE;
	}

	/*---Initialize server address/port struct---*/
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(conexion.puerto);
	if (inet_aton(conexion.ip, &dest.sin_addr.s_addr) == 0) {
		perror(conexion.ip);
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

	if (mostrar_menu() == -1) {
		return EXIT_FAILURE;
	}

	desconectar();

	liberar_recursos();
	return EXIT_SUCCESS;
}

struct Conexion leerConfiguracion() {

	int fd;
	struct Conexion conexion;

	fd = open("config.txt", O_RDONLY);

	if (fd == -1) {
		perror("Error al abrir el archivo de configuracion\n");
		abort();
	}

	char buffer[50];

	bzero(buffer, 50);
	int r = readLine(fd, buffer, 50);

	bzero(buffer, 50);
	r = readLine(fd, buffer, 50);

	strcpy(conexion.ip, buffer);

	bzero(buffer, 50);
	r = readLine(fd, buffer, 50);

	bzero(buffer, 50);
	r = readLine(fd, buffer, 50);

	conexion.puerto = atoi(buffer);

	close(fd);

	return conexion;
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

int validarPosiciones(char ** argv, char ** posiciones) {

	int i, j = 2; // j tiene la posicion inicial del primer barco en argv

	for (i = 0; i < cantidad_barcos; i++) {

		if (strlen(argv[j + i]) != 2) {
			printf("La posicion %d ingresada no es valida.\n", i + 1);
			return -1;
		}

		if (argv[j + i][0] < '0' || argv[j + i][0] > '9' || argv[j + i][1] < '0'
				|| argv[j + i][1] > '9') {
			printf("La posicion %d ingresada no es valida.\n", i + 1);
			return -1;
		}

		posiciones[i] = argv[j + i];
	}

	return 0;
}

int mostrar_menu() {

	int s, r;
	char buffer[MAXBUF];

	bzero(buffer, MAXBUF);

	// Dar la opcion de elegir un jugador de la lista o esperar a que alguien lo elija
	printf("Menu:\n\n");

	printf("\t1. Elegir un contrincante.\n");
	printf("\t2. Esperar a ser elegido.\n\n");

	int opcion;

	printf("Ingrese opcion: ");
	scanf("%d", &opcion);

	if (opcion < 1 || opcion > 2) {
		printf("La opcion ingresada no es valida.\n");
		return -1;
	}

	if (opcion == 1) {

		// Pido la lista de jugadores
		struct MensajeNIPC mensajeLista;
		mensajeLista.tipo = Lista_Jugadores;

		s = send(sockfd, &mensajeLista, sizeof(struct MensajeNIPC), 0);

		if (s == -1) {
			perror("Error al enviar el mensaje.\n");
			return -1;
		}

		// Recibo lista de jugadores
		r = recv(sockfd, buffer, sizeof(struct MensajeNIPC), 0);

		TIPO_MENSAJE tipo;

		memcpy(&tipo, buffer, sizeof(TIPO_MENSAJE));

		int payloadLength;

		memcpy(&payloadLength, buffer + sizeof(TIPO_MENSAJE), sizeof(int));

		// Valido que la lista no este vacia, si esta vacia lo pongo a esperar
		// Si la lista esta vacia el fd del primer jugador va a ser 0
		if (payloadLength == 0) {

			printf("No hay jugadores disponibles en este momento.\n");
			esperarPartida();

		} else {

			int i = 0;

			char messageBuffer[MAXBUF];

			memcpy(jugadoresDisponibles,
					buffer + sizeof(TIPO_MENSAJE) + sizeof(int), payloadLength);

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

			jugadoresDisponiblesCont = i;

			printf("%s\n", messageBuffer);

			if (elegirJugador() == -1) {
				perror("Error al elegir jugador.\n");
				liberar_recursos();
				return -1;
			}

		}
	} else {

		if (esperarPartida() == -1) {
			printf("Error iniciando la partida.\n");
			return -1;
		}
	}

	return 0;
}

int esperarPartida() {

	int r;
	char buffer[MAXBUF];
	struct MensajeNIPC * mensajeConfirmacion;

	printf("Esperando que se inicie una partida.\n");

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

	partida.jugadorOrigen = jugador;

	if (strcmp(mensajeConfirmacion->jugadorOrigen.nombre, jugador.nombre) == 0)
		partida.jugadorDestino = mensajeConfirmacion->jugadorOrigen;
	else
		partida.jugadorDestino = mensajeConfirmacion->jugadorOrigen;

	partida.estado = EnProgreso;

	iniciarPartida(partida);

	return 0;
}

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
		} else if (value[j] == 'b') {
			printf("x");
		} else {
			printf("A");
		}
		printf(" ");
	}
	printf("|");
}

void print_maps() {

	pthread_mutex_lock(&mutex_dibujar_pantalla);

	system("clear");

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

	pthread_mutex_unlock(&mutex_dibujar_pantalla);
}

void print_header() {
	printf("+");
	for (i = 0; i < 43; i++) {
		printf("-");
	}
	printf("+");
}

int desconectar() {

	int s;
	struct MensajeNIPC msg_desconectar;

	msg_desconectar.tipo = Desconectar;
	msg_desconectar.jugadorOrigen = jugador;

	s = send(sockfd, &msg_desconectar, sizeof(struct MensajeNIPC), 0);

	if (s == -1) {
		perror("Error al enviar el mensaje.\n");
		return -1;
	}

	return 0;
}

void liberar_recursos() {
	close(sockfd);
}

void * leerJugada(void * args) {

	char pos[10];

	int s, coords_invalidas = 0, inicio = 1;

	bzero(pos, 10);

	while (partida.estado == EnProgreso) {

		print_maps();

		// Muestro mensaje de validacion
		if (coords_invalidas) {

			// Si es el inicio de la partida no muestro ningun mensaje
			if(inicio) {
				inicio = 0;
			}
			else {
				printf("Las coordenadas ingresadas no son validas.\n");
			}
		}

		// Mustro mensaje segun si el jugador tiene el turno o no
		if (turno) {
			printf("Ingrese coordenadas:\n");
		} else {
			printf("Esperando jugada del oponente...\n");
		}

		// Obtengo input (bloqueo)
		gets(pos);

		system("clear");

		// Si no es el turno del jugador no hago nada aun
		if (!turno) {
			continue;
		}

		//Validacion coordenadas
		if (strlen(pos) != 2) {
			coords_invalidas = 1;
			continue;
		}

		if (pos[0] < '0' || pos[0] > '9' || pos[1] < '0' || pos[1] > '9') {
			coords_invalidas = 1;
			continue;
		}

		coords_invalidas = 0;

		// Fin del turno, debera esperar a que el server lo habilite para jugar de nuevo
		turno = 0;

		// Enviar jugada
		struct MensajeNIPC mensajeJugada;
		mensajeJugada.tipo = Jugada;
		mensajeJugada.jugadorOrigen = partida.jugadorOrigen;
		mensajeJugada.jugadorDestino = partida.jugadorDestino;
		mensajeJugada.payload_length = strlen(pos);
		strcpy(mensajeJugada.payload, pos);

		s = send(sockfd, &mensajeJugada, sizeof(struct MensajeNIPC), 0);

		if (s == -1) {
			perror("Error al enviar el mensaje.\n");
			abort();
		}
	}
}

void * escucharServidor(void * args) {

	int r;
	char buffer[MAXBUF];

	while (partida.estado == EnProgreso) {

		bzero(buffer, MAXBUF);

		// Recibe jugadas que le reenvia el servidor
		r = recv(sockfd, buffer, sizeof(struct MensajeNIPC), 0);

		if (r == -1) {
			perror("Error al recibir el mensaje.\n");
			abort();
		}

		struct MensajeNIPC * mensaje;

		mensaje = (struct MensajeNIPC *) buffer;

		switch (mensaje->tipo) {

		case Recibe_Ataque:

			if (handler_ataque(mensaje) == -1) {
				printf("Error al procesar el ataque.\n");
				abort();
			}

			break;

		case Resultado:
			handler_respuesta(mensaje);
			break;

		case Fin_partida:
			handler_fin(mensaje);
			partida.estado = Terminada;

			break;

		case Turno:

			printf("Mensaje turno.\n");
			turno = 1;

			system("clear");

			print_maps();

			printf("Ingrese coordenadas:\n");

			break;
		}
	}
}

int handler_ataque(struct MensajeNIPC * mensaje) {

	int i, hundido = 0, repetido = 0;

	// Mensaje que voy a enviar informando el resultado
	struct Resultado_Ataque resultado_ataq;

	// Se fija si las coordenadas del ataque coinciden con alguna posicion
	for (i = 0; i < cantidad_barcos; i++) {

		if (strcmp(posiciones_barcos[i], mensaje->payload) == 0) {
			hundido = 1;
		}
	}

	// Se fija que ese barco no estuviera hundido
	for (i = 0; i < cantidad_barcos_hundidos; i++) {

		if (strcmp(posiciones_barcos_hundidos[i], mensaje->payload) == 0) {
			repetido = 1;
		}
	}

	if (hundido && !repetido) {

		// Seteo mensaje de respuesta
		resultado_ataq.resultado = Hundido;

		// Guardo la posicion del barco undido e incremento la cantidad de undidos

		strcpy(posiciones_barcos_hundidos[cantidad_barcos_hundidos],
				mensaje->payload);
		cantidad_barcos_hundidos++;

		// Actualizo la interfaz grafica
		int x, y;

		x = mensaje->payload[0] - VALUE;
		y = mensaje->payload[1] - VALUE;

		// Cambio la 'X' por '.'
		my_matrix[x][y] = 'a';

		system("clear");

		print_maps();

		printf("Se recibio un ataque a coordenadas %s... Hundido.\n",
				mensaje->payload);
	} else {

		// Seteo respuesta del mensaje
		resultado_ataq.resultado = Agua;

		system("clear");

		print_maps();

		printf("Se recibio un ataque a coordenadas %s... Agua.\n",
				mensaje->payload);

	}

	if (turno) {
		printf("Ingrese coordenadas:\n");
	} else {
		printf("Esperando jugada del oponente...\n");
	}

	// Envio el mensaje con la respuesta de la jugada

	strcpy(resultado_ataq.coordenadas, mensaje->payload);

	struct MensajeNIPC mensaje_resultado;
	mensaje_resultado.tipo = Resultado;
	mensaje_resultado.payload_length = sizeof(struct Resultado_Ataque);
	memcpy(mensaje_resultado.payload, &resultado_ataq,
			sizeof(struct Resultado_Ataque));
	mensaje_resultado.jugadorOrigen = jugador;
	mensaje_resultado.jugadorDestino = partida.jugadorDestino;

	int s = send(sockfd, &mensaje_resultado, sizeof(struct MensajeNIPC), 0);

	if (s == -1) {
		perror("Error al enviar el mensaje.\n");
		return -1;
	}

	if (cantidad_barcos_hundidos == cantidad_barcos) {
		// Logica de fin de la partida

		struct MensajeNIPC mensaje_fin_partida;
		mensaje_fin_partida.tipo = Fin_partida;
		mensaje_fin_partida.payload_length = sizeof(struct Jugador);

		// Envio el jugador ganador

		memcpy(mensaje_fin_partida.payload, &partida.jugadorDestino,
				sizeof(struct Jugador));

		mensaje_fin_partida.jugadorOrigen = jugador;
		mensaje_fin_partida.jugadorDestino = partida.jugadorDestino;

		int s = send(sockfd, &mensaje_fin_partida, sizeof(struct MensajeNIPC),
				0);

		if (s == -1) {
			perror("Error al enviar el mensaje.\n");
			return -1;
		}
	}

	return 0;
}

int handler_respuesta(struct MensajeNIPC * mensaje) {

	struct Resultado_Ataque resultado;
	char simbolo;

	memcpy(&resultado, mensaje->payload, mensaje->payload_length);

	if (resultado.resultado == Agua) {
		simbolo = 'c';
	} else {
		simbolo = 'b';
	}

	// Actualizo la interfaz grafica remota
	int x, y;

	x = resultado.coordenadas[0] - VALUE;
	y = resultado.coordenadas[1] - VALUE;

	remote_matrix[x][y] = simbolo;

	system("clear");

	print_maps();

	if (resultado.resultado == Agua) {
		printf("Resultado de ataque a coordenadas %s: Agua.\n",
				resultado.coordenadas);
	} else {
		printf("Resultado de ataque a coordenadas %s: Hundido.\n",
				resultado.coordenadas);
	}

	if (turno) {
		printf("Ingrese coordenadas:\n");
	} else {
		printf("Esperando jugada del oponente...\n");
	}

	return 0;
}

int handler_fin(struct MensajeNIPC * mensaje) {

	struct Jugador ganador;

	// Cargo el jugador que gano
	memcpy(&ganador, mensaje->payload, mensaje->payload_length);

	printf("Termino la partida, el ganador es: %s.\n", ganador.nombre);

	return 0;
}

int iniciarPartida(struct Partida partida) {

	int result;

	printf("Se inicia la partida entre %s y %s.\n",
			partida.jugadorOrigen.nombre, partida.jugadorDestino.nombre);

	// Inicializo la matriz grafica
	matrix_init();

	// Cargo las posiciones ingresadas por el jugador
	for (i = 0; i < cantidad_barcos; i++) {
		my_matrix[posiciones_barcos[i][0] - VALUE][posiciones_barcos[i][1]
				- VALUE] = 'b';
	}

	// Voy a separar la ejecucion en 2 threads, uno va a leer lo que ingresa el usuario
	// por consola y otro va a estar escuchando por mensajes que le lleguen desde el server
	result = pthread_create(&t1, NULL, leerJugada, NULL );

	if (result != 0) {
		perror("Error en la creacion del thread\n");
		return EXIT_FAILURE;
	}

	result = pthread_create(&t2, NULL, escucharServidor, NULL );

	if (result != 0) {
		perror("Error en la creacion del thread\n");
		return EXIT_FAILURE;
	}

	// bloqueo hasta que ambos threads terminen
	pthread_join(t2, NULL );
	pthread_join(t1, NULL );

	// Termino la partida
	return 0;
}

int elegirJugador() {

	int jugadorElegido, s, r;
	char buffer[MAXBUF];
	struct MensajeNIPC * mensajeConfirmacion;

	printf("Elegir jugador: ");
	scanf("%d", &jugadorElegido);

	if (jugadorElegido < 1 || jugadorElegido > jugadoresDisponiblesCont) {
		printf("El jugador ingresado no es valido.\n");
		return -1;
	}

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

	partida.jugadorOrigen = jugador;
	partida.jugadorDestino = jugadoresDisponibles[jugadorElegido - 1];
	partida.estado = EnProgreso;

	iniciarPartida(partida);

	return 0;
}
