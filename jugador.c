#include "include.h"


int main(int argc, char * argv[]) {
	int sockfd, s, r;
	struct sockaddr_in dest;
	char buffer[MAXBUF];

	/*MATRIX VARS*/
	char my_matrix[NUMBER_X][NUMBER_Y];
	char remote_matrix[NUMBER_X][NUMBER_Y];
	int i, j;

	if (argc < 3) {
		puts("Usage ./cliente [nombre] [pos1] ...");
		return EXIT_FAILURE;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return EXIT_FAILURE;
	}

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

	s = send(sockfd, argv[1], strlen(argv[1]), 0);

	if (s == -1) {
		perror("Error en envio");
		return EXIT_FAILURE;
	}

	r = recv(sockfd, buffer, sizeof(buffer), 0);

	if (r == -1) {
		perror("Error al recibir lista de jugadores");
		return EXIT_FAILURE;
	}

	printf("%s", buffer);

	/*---Draw screen---*/

	/*matrix_init();
	for (i = 1; i < argc; i++) {
		my_matrix[argv[i][0] - VALUE][argv[i][1] - VALUE] = 'b';
	}
	while (1) {
		print_maps();
		printf("Enter value\n");
		getchar();
		system("clear");
	}*/

	close(sockfd);
	return EXIT_SUCCESS;
}

void matrix_init() {
	int i,j;
	for (i = 0; i < NUMBER_X; i++) {
		for (j = 0; j < NUMBER_Y; j++) {
			//my_matrix[i][j] = 'a';
			//remote_matrix[i][j] = 'a';
		}
	}
}
void print_map_line(char value[]) {
	int j;
	printf("|   ");
	for (j = 0; j < NUMBER_Y; j++) {
		if (value[j] == 'a') {
			printf(".");
		} else {
			printf("x");
		}
		printf("   ");
	}
	printf("|");
}
void print_header() {
	int i;
	printf("+");
	for (i = 0; i < 43; i++) {
		printf("-");
	}
	printf("+");
}
void print_maps() {
	int i;
	printf("\t\t My map \t\t\t\t\t Remote Map\t\n");
	print_header();
	printf("\t");
	print_header();
	printf("\n");
	for (i = 0; i < NUMBER_X; i++) {
		//print_map_line(my_matrix[i]);
		printf("\t");
		//print_map_line(remote_matrix[i]);
		printf("\n");
	}
	print_header();
	printf("\t");
	print_header();
	printf("\n");
}

