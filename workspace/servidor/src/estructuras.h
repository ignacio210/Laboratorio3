#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#define NOMBRE_LEN 25 // Longitud maxima del nombre del jugador
#define CONTENIDO_LEN 256 // Longitud maxima para el campo contenido de los mensajes

#define MAXJUG 16
#define MAXBUF 4096
#define BIGBUF 4096

// Tipos de mensajes que pueden ser intercambiados entre el servidor y los clientes.
typedef enum {
		Registra_Nombre,
		Jugada,
		Elige_Jugador,
		Lista_Jugadores,
		Jugador_Registrado,
		Confirma_partida,
		Recibe_Ataque,
		Resultado,
		Fin_partida,
		Desconectar
		} TIPO_MENSAJE;

// Estados del jugador
typedef enum {
	Disponible,
	Jugando
} ESTADO;

// Resultados de una jugada
typedef enum {
	Agua,
	Hundido
} RESULTADO;

// Estados de la partida
typedef enum {
	EnProgreso,
	Terminada
} ESTADO_PARTIDA;

// Estructura para la configuracion de conexion
struct Conexion {
	char ip[15];
	int puerto;
};

// Estructura para el jugador
struct Jugador {
	int clientfd; // File descriptor asociado al cliente
	ESTADO estado;
	char nombre[NOMBRE_LEN];
	char ip[15];
	struct sockaddr_in client_addr;
};

// Estructura para el mensaje
struct Mensaje {
	TIPO_MENSAJE tipo;
	struct Jugador jugadorOrigen;
	struct Jugador jugadorDestino;
	char contenido[CONTENIDO_LEN];
};

// Estructura para el mensaje
struct MensajeNIPC {
	TIPO_MENSAJE tipo;
	int payload_length;
	char payload[2048];
	struct Jugador jugadorOrigen;
	struct Jugador jugadorDestino;
};

// Estructura que representa la partida
struct Partida {
	struct Jugador jugadorOrigen;
	struct Jugador jugadorDestino;
	ESTADO_PARTIDA estado;
};

// Estructura que representa la partida
struct Resultado_Ataque {
	RESULTADO resultado;
	char coordenadas[10];
};

#endif /* ESTRUCTURAS_H_ */

