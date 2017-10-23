
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
#include <poll.h>
#include <fcntl.h>

#define MAXCHAR 140 // Maximum character to transfer


int errcheck; // glob error checking variable


int socket_nonblock(int temp)
{
	int flags, s;

	flags = fcntl(temp, F_GETFL, 0); // get fd to set non blocking
	if (flags == -1) {
		perror("server : fcntl");
		return -1;
	}
	s = fcntl(temp, F_SETFL, flags | O_NONBLOCK);
	if (s == -1) {
		perror("server : fcntl");
		return -1;
	}
	return 0;
}
int connecttoServer(char **argv) // Connects server and client
{
	struct addrinfo *p, *info, hints;
	char ipstr[INET_ADDRSTRLEN];
	int tmpsoc;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; //IPv4
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; // AGNOS

	errcheck = getaddrinfo(argv[1], argv[2], &hints, &info); // Takes ip and port infos
	if(errcheck != 0) {
		fprintf(stderr, "getaddrinfo:  %s\n", gai_strerror(errcheck));
		exit(1);
	}

	printf("IP adinfoses for %s: \n\n", argv[1]);

	for(p = info; p != NULL; p = p->ai_next) {

		tmpsoc = socket(PF_INET, p->ai_socktype, p->ai_protocol); //Taking fd from listening port
		if (tmpsoc == -1) {
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

	struct pollfd mypoll = {STDIN_FILENO, POLLIN | POLLPRI };
	int soc;
	int running = 1;
	char *rcvmsg, *sndmsg;

	rcvmsg = calloc(MAXCHAR, sizeof(char));
	if (rcvmsg == NULL) {
		fprintf(stderr, "rcvmsg : calloc ");
		exit(0);
	}




	soc = connecttoServer(argv); // connect server
	errcheck = socket_nonblock(soc);


	while(running) { // event-client loop

		if (poll(&mypoll, 1, 50) > 0) {
			sndmsg = calloc(MAXCHAR, sizeof(char));
			if (sndmsg == NULL) {
				fprintf(stderr, "sndmsg : calloc ");
				exit(0);
			}
			fgets(sndmsg, MAXCHAR, stdin);
			if (sndmsg == NULL) {
				fprintf(stderr, "failed to load msg string");

			}// end string if
			errcheck = send(soc, sndmsg, MAXCHAR, 0);
			 if (errcheck < 0) {
				 if(errno != EWOULDBLOCK ||
					 errno != EAGAIN) {
					 perror("send : ");
				 }// if end
					 continue;
			 }// if end
		}// end stdin send if
		if(poll(&mypoll, soc, 50) > 0) {
			errcheck = recv(soc, rcvmsg, MAXCHAR, 0);
			if (errcheck < 0) {
				if (errno != EAGAIN ||
					errno != EWOULDBLOCK) {
					perror("recv : ");
				}
				continue;
			}
			if (errcheck == 0) {
				puts("Server Disconnected");
				exit(0);
			}
			 if(rcvmsg == NULL) {
				 continue;
			 }
			 else {
				 printf("%s\n", rcvmsg);
			 }
		}
	}// end while


	close(soc);

	return 0;
}
