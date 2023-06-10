#include <bits/stdc++.h>
#include <cstring>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#define SA struct sockaddr
using namespace std;
int sinkfd[1005]; // the sockfd of connected to the sink server


int main(int argc, char const *argv[]){
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("socket creation failed...\n");
		exit(0);
	}else printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(atoi(argv[2]));

	// connect the client socket to server socket
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) < 0) {
		printf("connection with the server failed...\n");
		exit(0);
	}
	else
		printf("connected to the server..\n");
    // interact with the server

    // creat connections to the sink server
    servaddr.sin_port = htons(atoi(argv[2]) + 1);
    for(int i = 0; i < 10; i++){
        sinkfd[i] = socket(AF_INET, SOCK_STREAM, 0);
    }
    for(int i = 0; i < 10; i++){ // establish 10 connections to sink server
            connect(sinkfd[i], (SA*)&servaddr, sizeof(servaddr));
    }
    string cmd = "/reset\n";
    write(sockfd, cmd.c_str(), cmd.length());
    char buf[10005];
    int n = read(sockfd, buf, sizeof(buf));
    string reply = string(buf);
    cout << reply;
    for(int i = 0; i < 10000; i++){
        int tmp[1005];
        memset(tmp, 0, sizeof(tmp));
        for(int j = 0; j < 10; j++) write(sinkfd[j], tmp, sizeof(tmp));
    }
    //cout << "debug1\n" ;
    // close connections to sink server
    for(int i = 0; i < 10; i++){
        close(sinkfd[i]);
        sinkfd[i] = -1;
    }
    cmd = "/report\n";
    write(sockfd, cmd.c_str(), cmd.length());
    bzero(buf, sizeof(buf));
	n = read(sockfd, buf, sizeof(buf));
    reply = string(buf);
    cout << reply ;
	// close the socket
	close(sockfd);
    sockfd = -1;
}
