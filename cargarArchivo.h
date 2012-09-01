/*

 * cargarArchivo.h
 *
 *  Created on: Sep 1, 2012
 *      Author: santi
 */
/*#include "include.h"

#ifndef CARGARARCHIVO_H_
#define CARGARARCHIVO_H_

/*#ifndef NULL
#define NULL   ((void *) 0)
#endif*/
// Funciones para leer los archivos de configuracion
/*
int LeerArchConfJugador(const char *arch, Jugador *cfg);
int CargaDatosJugador(char *fields, char *valor, Jugador* cfg);

int LeerArchConfJugador(const char *arch, Jugador *cfg) {
	int numgay;
	char *ptr;
	char linea[80 + 1], fields[80 + 1];
	FILE *fd;

	if ((fd = fopen(arch, "r+")) == NULL) {
		printf("\nERROR AL ABRIR EL ARCHIVO %s !!!!!\n", arch);
		exit(0);
	}
	while (fgets(linea, 80 + 1, fd) != NULL)
	if (linea[0] != '*') {
		ptr = strtok(linea, "=");
		if (ptr == NULL)
		return 0;
		strcpy(fields, ptr);
		ptr = strtok(NULL, "\n");
		if (ptr == NULL) {
			printf("\nNO HAY NINGUN DATO EN EL CAMPO -FIN LECTURA %s\n",
					fields);
			return 0;
		}

		numgay = CargaDatosJugador(fields, ptr, cfg);
	}
	fclose(fd);
	return 0;
}

/*Jugador
 IP_JUGADOR
 IP_SERVIDOR
 PUERTO_SERVIDOR
 PUERTO_JUGADOR

#include "include.h"

int CargaDatosJugador(char *fields, char *valor, Jugador* cfg) {
	int c = 0;

	if (strcmp(fields, "IP_JUGADOR") == 0) {
		cfg->ip_Jugador.s_addr = inet_addr(valor);
		fflush (stdout);
		fflush (NULL);
	} else
	c++;

	if (strcmp(fields, "IP_SERVIDOR") == 0) {
		cfg->ip_Servidor.s_addr = inet_addr(valor);
		fflush (stdout);
		fflush (NULL);
	} else
	c++;

	if (strcmp(fields, "PUERTO_SERVIDOR") == 0) {
		cfg->iPuerto_Servidor = atol(valor);
		fflush (NULL);
	} else
	c++;

	if (strcmp(fields, "PUERTO_JUGADOR") == 0) {
		cfg->iPuerto_Jugador = atol(valor);
		fflush (NULL);
	} else
	c++;
	if (c == 5) {
		printf("\nERROR NO EXISTE EL CAMPO %s\n", fields);
		return 1;
	}

	return 0;
}
/*
#endif /* CARGARARCHIVO_H_ */
