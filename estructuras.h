#ifndef ESTRUCTURAS_H_

#define ESTRUCTURAS_H_

#define NOMBRE_LEN 25 // Longitud maxima del nombre del jugador
#define CONTENIDO_LEN 256 // Longitud maxima para el campo contenido de los mensajes

// Tipos de mensajes que pueden ser intercambiados entre el servidor y los clientes.
typedef enum {
		Registra_Nombre,
		Juega,
		Elige_Jugador,
		Lista_Jugadores
		} TIPO_MENSAJE;

// Estados del jugador
typedef enum {
	Disponible,
	Jugando
} ESTADO;

// Estructura para el jugador
struct Jugador {
	int clientfd; // File descriptor asociado al cliente
	ESTADO estado;
	char nombre[NOMBRE_LEN];
	char ip[15];
};

// Estructura para el mensaje
struct Mensaje {
	struct Jugador jugadorOrigen;
	struct Jugador jugadorDestino;
	TIPO_MENSAJE tipo;
	char contenido[CONTENIDO_LEN];
};


#endif /* ESTRUCTURAS_H_ */
