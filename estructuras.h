#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

// Estructura para el jugador
typedef struct {
	int clientfd; // File descriptor asociado al cliente
	int disponible;
	char * nombre;
	char * ip;
//	struct in_addr ip_Jugador;/*el IP de la pc donde levanto el proceso*/
//	unsigned long int iPuerto_Jugador;/*el puerto donde escuchara  conexiones*/
//	struct in_addr ip_Servidor;/*el IP del router al que me conecto*/
//	unsigned long int iPuerto_Servidor;/*el puerto del servidor a donde se tiene que conectar */
//	int sock_servidor;/* socket que utilizo para hablar con el servidor. El q va al connect*/

} Jugador;

// Tipos de mensajes que pueden ser intercambiados entre el servidor y los clientes.
typedef enum {
	Registra_Jugador, Juega, Elige_Jugador, Lista_Jugadores
} TIPO_MENSAJE;

// Estructura para el mensaje
typedef struct {
	Jugador jugador;
	TIPO_MENSAJE tipo;
	char * contenido;
} Mensaje;



#endif /* ESTRUCTURAS_H_ */
