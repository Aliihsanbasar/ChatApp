/*
 * chatServer-Server.c
 *
 *  Created on: Aug 24, 2017
 *      Author: sawakaga
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAXCHAR 140 // max character to comm
#define MAXEVENTS 64 // max event

enum authority {MEMBER, MOD, ADMIN};



struct clients {
	int num;
	char id[25];
	int password;
	enum authority authority;
};



int listener; // listener socket
int errcheck; // global errcheck integer


int initServer(char **argv)
{
	struct addrinfo hints, *tmp, *info; // addr info
	int yes = 1;  // for socket setting

	memset(&hints, 0, sizeof(struct addrinfo)); // init zero
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;


	errcheck = getaddrinfo(NULL, argv[1], &hints, &info); // get address info
	if (errcheck != 0) {
		fprintf(stderr, "getaddrinfo : %s\n ", gai_strerror(errcheck));
		exit(1);
	}

	for(tmp = info; tmp != NULL; tmp = tmp->ai_next) {
		listener = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol); // get socket(s)
		if (listener < 0) {
			perror("server : socket");
			continue;
		}

		errcheck = setsockopt(listener,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); // socket settings
		if (errcheck < 0) {
			perror("setsockopt: ");
			exit(1);
		}

		errcheck = bind(listener, tmp->ai_addr, tmp->ai_addrlen); // take server socket
		if (errcheck < 0) {
			close(listener);
			perror("server : bind");
			continue;

			}

		break;
	}


	if(tmp == NULL) {
		fprintf(stderr, "server : failed to bind");
		exit(1);
	}
	freeaddrinfo(info);

	errcheck = listen(listener, SOMAXCONN); // waits  to connect server
	if (errcheck < 0) {
		perror("listen : ");
		exit(1);
	}

	puts("server: awaits");
	return listener;

}

int socket_nonblock(int temp)
{
	int flags, s;

	flags = fcntl(temp, F_GETFL, 0); // get fd to set non blocking
	if (flags == -1) {
		perror("server : fcntl");
		return -1;
	}
	flags |= O_NONBLOCK;
	s = fcntl(temp, F_SETFL, flags);
	if (s == -1) {
		perror("server : fcntl");
		return -1;
	}
	return 0;
}

void sendMessage(struct epoll_event *events, char *msg, int i, int n)

{
	int count;

	for (int a = 0; a<n; a++) {
		if (events[a].data.fd != listener &&
			events[a].data.fd != events[i].data.fd) {

			count  = send(events[i].data.fd, msg, sizeof msg, 0);
			if (sizeof(msg) - count != 0) {
				printf("Only %d character msg send instead of %lu", count, sizeof(msg));
			}
		} // end if
	} // end for
}

void recieveMessage(struct epoll_event *events, char **msg, int *done, int i)
{
	errcheck = recv(events[i].data.fd, msg, sizeof(msg),0);
	if (errcheck == -1) {
		// if errno == EAGAIN that means we read all data.
		if (errno != EAGAIN) {
			perror("recv");
			*done = 1;
		}


	}
}

void eventHandler(struct epoll_event *events, char **msg, int *done, int i)
{
	if (strcmp(*msg, "quit") == 0) { // EOF means connection closed remotely

		*done = 1;

	}
	/*else if (msg == "kick") {

	}

	else if (msg == "/join") {

	}
	else if (msg == "/monitor") {

	}*/
}

void closedConn(struct epoll_event *events, struct clients *client, char *msg, int i, int n)
{

	printf("Closed Connection on fd : %d\n", events[i].data.fd);
	sprintf(msg, "%d. client disconnected.\n",client[i].num);
	sendMessage(events, msg, i, n);
	// epoll will remove it when we close it
	close(events[i].data.fd);

}

int establishConn()
{
	int tmpfd;
	struct sockaddr client_addr;
	socklen_t client_len;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	client_len = sizeof(client_addr);
	tmpfd = accept(listener, &client_addr, &client_len);
	if (tmpfd == -1 ) {

		if (errno == EAGAIN ||
			errno == EWOULDBLOCK) {
			// all connections established

		}
		else {
			perror("server : accept");

		}
	}

	errcheck = getnameinfo(&client_addr, client_len,
				hbuf, sizeof(hbuf),sbuf, sizeof(sbuf),
				NI_NUMERICHOST | NI_NUMERICSERV);
	if (errcheck == 0) {

		printf("Accepted connection on fd = %d (host = %s, port = %s)\n", tmpfd,hbuf,sbuf);
	}
	errcheck = socket_nonblock(tmpfd);
	if (errcheck == -1) {
		fprintf(stderr, "accept : socket nonblock");
		abort();
	}
	return tmpfd;
}

void addingInstancetoEpoll(struct epoll_event event, int epfd, int tmpfd)
{
	if (!tmpfd) {
		event.data.fd = listener;
	}
	else {
		event.data.fd = tmpfd;
	}
	event.events = EPOLLIN | EPOLLET; // edge-trigg

	errcheck = epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &event); // adding listener to epoll instance
	if (errcheck == -1 ) {
		perror("server : epoll_create");
		abort();
	}

}

int main(int argc, char **argv)
{

	if(argc < 2) {
		fprintf(stderr, "usage: port");
		exit(1);
	}

	initServer(argv);
	errcheck = socket_nonblock(listener);
	if (errcheck == -1) {
		fprintf(stderr, "error : socket_nonblock");
	}


	int epfd, newfd;
	int counter = 1; //clients number
	char *msg;
	struct epoll_event event;
	struct epoll_event *events;
	struct clients *client;

	epfd = epoll_create1(0); // creating epoll instance
	if (epfd == -1) {
		perror("server : epoll_create");
		abort();
	}

	addingInstancetoEpoll(event, epfd, 0);

	events = calloc(MAXEVENTS, sizeof(event)); // allocation for events
	if (events == NULL ) {
		fprintf(stderr, "server : calloc error");
		abort();
	}

	client = calloc(MAXEVENTS, sizeof(struct clients));
	if (client  == NULL) {
		fprintf(stderr, "server : calloc error");
		abort();
	}

	msg = calloc(MAXCHAR, sizeof(char));
	if (msg == NULL) {
		fprintf(stderr, "server : calloc error");
		abort();
	}


	while(1) { //  event loop

		int n = epoll_wait(epfd, events, MAXEVENTS, -1);
		for (int i = 0; i<n ; i++) {
			if ((events[i].events & EPOLLERR) ||
				(events[i].events & EPOLLHUP) ||
				(!(events[i].events & EPOLLIN))) {

				// error occured on I/O trig fd or fd ready for reading
				fprintf(stderr, "server : epoll error\n");
				close(events[i].data.fd);
				continue;

			}
			else if (listener == events[i].data.fd) {
				// event on listener socket means new connections

				while(1) {
					newfd = establishConn(); // handle new conn
					client[i].num = counter;
					counter++;
					sprintf(msg, "%d. client connected", client[i].num);
  					sendMessage(events, msg, i , n); // announce new conn
					addingInstancetoEpoll(event, epfd, newfd); // add new conn to instance
				}
				continue;

			}
			else {
				// That means new data comes to fd
				int done = 0;
				while (1) {
					recieveMessage(events, &msg, &done, i); // recieve message
					eventHandler(events, &msg, &done, i); // handle event
					sprintf(msg, "%d->  %s",client->num, msg); // adding prefix
					sendMessage(events, msg, i, n); // announce new message
				}
				if (done) {

					closedConn(events, client, msg, i, n);

				} // end if
			} // end else
		} // end for
	}// end event loop

	free(events);
	free(client);
	free(msg);
	close(epfd);

	return EXIT_SUCCESS;


	return 0;

}
