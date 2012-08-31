// Estructura para el jugador
struct Jugador {
	int clientfd; // File descriptor asociado al cliente
	bool disponible;
	char * nombre;
	char * ip;
};
