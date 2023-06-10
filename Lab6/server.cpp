#include <bits/stdc++.h>
#include <string>
#include <poll.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <limits.h>		/* for 1005 */
#include <ctime>
#define MAXLINE 1000000
#define OPEN_MAX 1005
using namespace std;
using ll = long long;

int main(int argc, char **argv){
	int					i, maxi, max_si, listenfd1, listenfd2, connfd1, connfd2, sockfd, cnt;
	int					nready;
	ssize_t				n;
	char				buf[MAXLINE];
	socklen_t			clilen, _sink;
	struct pollfd		client[OPEN_MAX], sink[OPEN_MAX];
	struct sockaddr_in	cliaddr, servaddr, sinkaddr;
    ll bytes = 0;
    struct timeval pre;
    pre.tv_sec = 0;
    pre.tv_usec = 0;

	listenfd1 = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

    if(bind(listenfd1, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Binding Error 1\n");
        exit(0);
    }

	listen(listenfd1, 5);

    listenfd2 = socket(AF_INET, SOCK_STREAM, 0);

	servaddr.sin_port        = htons(atoi(argv[1]) + 1);

    if(bind(listenfd2, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Binding Error 2\n");
        exit(0);
    }

	listen(listenfd2, 5);

	client[0].fd = listenfd1;
	client[0].events = POLLRDNORM;
	for (i = 1; i < OPEN_MAX; i++) client[i].fd = -1;		/* -1 indicates available entry */
	maxi = 0;					/* max index into client[] array */

    sink[0].fd = listenfd2;
    sink[0].events = POLLRDNORM;
    for (i = 1; i < OPEN_MAX; i++) sink[i].fd = -1;
    max_si = 0;
    bytes = 0;
/* end fig01 */

/* include fig02 */
	for ( ; ; ) {
		nready = poll(client, maxi+1, 0);

		if (client[0].revents & POLLRDNORM) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd1 = accept(listenfd1, (struct sockaddr *) &cliaddr, &clilen);
            if(connfd1 < 0){
                perror("Accept Error\n");
            }

			//printf("new client: %s\n", inet_ntoa((sockaddr*) &cliaddr, clilen));

			for (i = 1; i < OPEN_MAX; i++)
				if (client[i].fd < 0) {
					client[i].fd = connfd1;	/* struct sockaddrve descriptor */
					break;
				}
			if (i == OPEN_MAX)
				perror("too many clients");

			client[i].events = POLLRDNORM;
			if (i > maxi) maxi = i;				/* max index in client[] array */
			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}
        nready = poll(sink, max_si+1, 0);
        if (sink[0].revents & POLLRDNORM) {	/* new client connection */
            _sink = sizeof(sinkaddr);
			connfd2 = accept(listenfd2, (struct sockaddr *) &sinkaddr, &_sink);
            cout << "new client " << cnt << endl;
			//printf("new client: %s\n", inet_ntoa((sockaddr*) &cliaddr, clilen));

			for (i = 1; i < OPEN_MAX; i++)
				if (sink[i].fd < 0) {
					sink[i].fd = connfd2;	/* struct sockaddrve descriptor */
					break;
				}
			if (i == OPEN_MAX)
				perror("too many clients");

			sink[i].events = POLLRDNORM;
            cnt++;
			if (i > max_si) max_si = i;				/* max index in client[] array */
			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}
        // handle user commands
		for (i = 1; i <= maxi; i++) {	/* check all clients for data */
            memset(buf, '\0', sizeof(buf));
			if ( (sockfd = client[i].fd) < 0)
				continue;
			if (client[i].revents & (POLLRDNORM | POLLERR)) {
				if ( (n = read(sockfd, buf, MAXLINE)) < 0) {
					if (errno == ECONNRESET) {
							/*4connection reset by client */
						printf("client[%d] aborted connection\n", i);
						close(sockfd);
						client[i].fd = -1;
					} else
						perror("read error");
				} else if (n == 0) {
						/*4connection closed by client */
					printf("client[%d] closed connection\n", i);
					close(sockfd);
					client[i].fd = -1;
				} else{
                    string cmd = string(buf);
                    timeval t;
                    gettimeofday(&t, NULL);
                    if(cmd == "/reset\n"){
                        string msg = to_string(t.tv_sec) + "." + to_string(t.tv_usec) + " RESET " + to_string(bytes) + "\n";
                        write(connfd1, msg.c_str(), msg.length());
                        bytes = 0;
                        pre.tv_sec = t.tv_sec, pre.tv_usec = t.tv_usec;
                        cout << "reset\n";
                    }else if(cmd == "/ping\n"){
                        string msg = to_string(t.tv_sec) + "." + to_string(t.tv_usec) + " PONG\n";
                        write(connfd1, msg.c_str(), msg.length());
                    }else if(cmd == "/report\n"){
                        double elapse_time = (double)(t.tv_sec - pre.tv_sec) + (double)(t.tv_usec - pre.tv_usec)/1000000.0;
                        string msg = to_string(t.tv_sec) + "." + to_string(t.tv_usec) + " REPORT " + to_string(elapse_time) + " " + to_string((8.0 * (double)bytes)/1000000.0/elapse_time) + "Mbps\n";
                        cout << "report = " << msg;
                        write(connfd1, msg.c_str(), msg.length());
                    }else if(cmd == "/clients\n"){
                        string msg = to_string(t.tv_sec) + "." + to_string(t.tv_usec) + " CLIENTS " + to_string(cnt) + "\n";
                        write(connfd1, msg.c_str(), msg.length());
                    }
                }
				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
        // hand sink servers
        for (i = 1; i <= max_si; i++) {	/* check all clients for data */
            memset(buf, '\0', sizeof(buf));
			if ( (sockfd = sink[i].fd) < 0)
				continue;
			if (sink[i].revents & (POLLRDNORM | POLLERR)) {
				if ( (n = read(sockfd, buf, MAXLINE)) < 0) {
					if (errno == ECONNRESET) {
							/*4connection reset by client */
						printf("sink[%d] aborted connection\n", i);
						close(sockfd);
						sink[i].fd = -1;
                        cnt--;
					} else
						perror("read error");
				} else if (n == 0) {
						/*4connection closed by client */
					printf("sink[%d] closed connection\n", i);
					close(sockfd);
					sink[i].fd = -1;
                    cnt--;
				} else{
                    bytes += n;
                    cout << "bytes = " << bytes << endl;
                }
				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}   
}