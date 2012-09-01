#include <include.h>

// Estructura para el jugador
struct Jugador {
	int clientfd; // File descriptor asociado al cliente
	int disponible;
	char * nombre;
	char * ip;
};

// Tipos de mensajes que pueden ser intercambiados entre el servidor y los clientes.
typedef enum {
		Registra_Jugador,
		Juega,
		Elige_Jugador,
		Lista_Jugadores
		} TIPO_MENSAJE;

// Estructura para el mensaje
struct Mensaje {
	struct Jugador jugador;
	TIPO_MENSAJE tipo;
	char * contenido;
};

int main(int argc, char argv[]) {

	int sockfd, jugadorCount;
	struct sockaddr_in self;

	// TODO: El verctor de jugadores tiene que asignarse dimanicamente y no ser estatico
	struct Jugador jugadores[MAXJUG];
	char buffer[MAXBUF];

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
		printf("%s:%d connectectado\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		//HAY QUE DISPARAR UNL THREAD PARA ATENDER AL JUGADOR
		if(inicializarJugador(clientfd, client_addr, jugadores, jugadorCount) == -1) {
			perror("inicializarJugador");
			return EXIT_FAILURE;
		}

		jugadorCount++;

		armarListaJugadoresDisponibles(jugadores, buffer, jugadorCount);

		printf("%s", buffer);

		if(enviarListaJugadoresDisponibles(clientfd, buffer) == -1) {
			perror("Error al enviar lista de jugadores");
			return EXIT_FAILURE;
		}

		//escuchar - jugador1 envia el jugador a conectarse
		//actualizar estado
		//enviar mensaje a jugador 2
		/*jugar() {
			envia mensaje a jugaor 1 para que juegue
			recibe el respuesta
			comunica a jugador2
			jugador 2 devuelve el resultado
			comunica a jugador1
			(semaforo)
			envia mensaje a jugador 2 para que juegue

			jugador informa que perdio
			servidor actualiza estado y VECTOR DE PARTIDAS
	    }

		/*---Echo back anything sent---*/
		//send(clientfd, buffer, recv(clientfd, buffer, MAXBUF, 0), 0);

		/*---Close data connection---*/
		//close(clientfd);
	}

	// Cierro el socket
	close(sockfd);
	return EXIT_SUCCESS;
}

int inicializarJugador(int fd, struct sockaddr_in client_addr, struct Jugador jugadores[], int count) {

	char buffer[MAXBUF];
	int r;
	struct Jugador nuevoJugador;

	// Recibo el nombre del jugador
	r = recv(fd, buffer, sizeof(buffer), 0);

	if(r == -1) {
		perror("recv");
		return -1;
	}

	nuevoJugador.nombre = malloc(strlen(buffer));
	strcpy(nuevoJugador.nombre, buffer);

	nuevoJugador.clientfd = fd;
	nuevoJugador.disponible = 1;
	nuevoJugador.ip = inet_ntoa(client_addr.sin_addr);

	jugadores[count] = nuevoJugador;

	return EXIT_SUCCESS;
}

int enviarListaJugadoresDisponibles(int fd, char buffer[]) {

	int bytesSent;

	bytesSent = send(fd, buffer, strlen(buffer), 0);

	if(bytesSent == -1) {
		perror("Error al enviar lista de jugadores");
		return -1;
	}

	return 0;
}

int armarListaJugadoresDisponibles(struct Jugador jugadores[], char * buffer, int count) {

	int i;

	strcat(buffer, "Lista de jugadores disponibles:\n\n");

	for(i=0; i<count; i++) {

		if(jugadores[i].disponible) {

			strcat(buffer, "1"); // Todo, ver como castear int a string
			strcat(buffer, ". ");
			strcat(buffer, jugadores[i].nombre);
			strcat(buffer, "(");
			strcat(buffer, jugadores[i].ip);
			strcat(buffer, ")\n");
		}
	}

	return 0;
}


