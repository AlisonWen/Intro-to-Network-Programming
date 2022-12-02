// udp client driver program
#include <bits/stdc++.h>
#include <iostream>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5000
#define MAXLINE 1000
using namespace std;
char buf[10000000];

double tv2ms(struct timeval *tv) {
	return 1000.0*tv->tv_sec + 0.001*tv->tv_usec;
}

static int s = -1;
static struct sockaddr_in servin;
static unsigned seq;
static unsigned cnt = 0;

// Driver code
int main(int argc, char *argv[])
{
    if(argc < 3) { // not enough parameters
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}

    srand(time(0) ^ getpid());
    /*
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
    */

	memset(&servin, 0, sizeof(servin));
	servin.sin_family = AF_INET;
	servin.sin_port = htons(strtol(argv[argc-2], NULL, 0));
	if(inet_pton(AF_INET, argv[argc-1], &servin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[1]);
	}

    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("Socket Error\n");
        exit(0);
    }
		

	char buffer[100];
	char *message = "Hello Server";
	int sockfd, n;
	struct sockaddr_in servaddr;
	
	// clear servaddr
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(PORT);
	servaddr.sin_family = AF_INET;
	
	// create datagram socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	// connect to server
	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("\n Error : Connect Failed \n");
		exit(0);
	}
    //string faddr = string(argv[0]);
    FILE *fp = fopen(argv[0], "r"); // read file
    if(fp){
        std::cout << "Reading file contents\n";
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if(fread(buf, file_size, 1, fp) <= 0){
            std::cout << "Unable to copy file into buffer or empty file\n";
            exit(1);
        }
    }else{
        std::cout << "Cannot open file\n";
        exit(0);
    }

	// request to send datagram
	// no need to specify server address in sendto
	// connect stores the peers IP and port
	sendto(sockfd, buf, MAXLINE, 0, (struct sockaddr*)NULL, sizeof(servaddr));
	
	// waiting for response
	recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL);
	puts(buffer);

	// close the descriptor
	close(sockfd);
}
