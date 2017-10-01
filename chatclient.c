
/*
 ============================================================================
 Name        : chatServer.c
 Author      : Mert Anıl Gören
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <unistd.h>

#define MAXCHAR 140 // Maximum character to transfer


int errcheck; // glob error checking variable

int connecttoServer(struct addrinfo hints, char **argv, int *soc) // Connects server and client
{
	struct addrinfo *p, *info;
	char ipstr[INET_ADDRSTRLEN];
	int tmpsoc;

	errcheck = getaddrinfo(argv[1], argv[2], &hints, &info); // Takes ip and port infos
	if(errcheck != 0) {
		fprintf(stderr, "getaddrinfo:  %s\n", gai_strerror(errcheck));
		exit(1);
	}

	printf("IP adinfoses for %s: \n\n", argv[1]);

	for(p = info; p != NULL; p = p->ai_next) {

		tmpsoc = socket(PF_INET, p->ai_socktype, p->ai_protocol); //Taking fd from listening port
		if (*soc == -1) {
			perror("client : socket");
			continue;
		}

		errcheck = connect(tmpsoc, p->ai_addr,p->ai_addrlen); //Connects with server via fd
		if (errcheck < 0) {
			perror("connect : ");
			continue;
		}

		break;
	}

	if (p == NULL) {  // That means failed to connect
		close(tmpsoc);
		fprintf(stderr, "client: failed to connect");
	}

	inet_ntop(p->ai_family, (struct sockaddr_in*)p->ai_addr,ipstr, sizeof(ipstr));

	printf("client: connecting to: %s\n", ipstr);

	return tmpsoc;
}





int main(int argc, char **argv)
{


	if(argc < 3) {
		fprintf(stderr, "usage: ip/hostname port\n");
		return 1;
	}

	struct addrinfo hints;
	int soc;
	int running = 1;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; //IPv4
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; // AGNOS

	soc = connecttoServer(hints, argv); // connect server

	while(running) { // event-client loop


	}


	close(soc);



	return 0;
}
