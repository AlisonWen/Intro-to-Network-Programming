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
	setvbuf(stderr, NULL, _IONBF, 0);// udp client driver program// udp client driver program
// #include <bits/stdc++.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
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
#include <filesystem>
#include <dirent.h>
#include <vector>

#define PORT 5000
#define MAXLINE 1000
using namespace std;
char buf[10000000];

pair<int, string> decompose(string content){ // decompose a packet into 1.seq num, 2.checksum, 3.content(string)
	content.erase(content.begin()); // erase '<'
	string seq_str = "";
	for(int i = 0; i < content.size() && content[i] != ' '){
		seq_str.push_back(content[i]);
	}
	int seq_num = strtol(seq_str.c_str(), nullptr, 10);
	auto start = content.find('>');
	string ack = content.substr(start, content.size() - start);
	return make_pair(seq_num, ack);
}

uint64_t checksum(string content){
	vector <uint64_t> data;
	uint64_t cur = 0;
	char tmp[8];
	for(int i = 0; i < content.size() / 8; i++){
		for(int j = i * 8; j <(i + 1) * 8; j++) tmp[j % 8] = content[j];
		sscanf(tmp, "%d", &cur);
		data.emplace_back(cur);
	}
	if(content.size() % 8){
		for(int i = ((int)(content.size() / 8)) * 8; i < content.size(); i++) tmp[i % 8] = content[i];
		sscanf(tmp, "%d", &cur);
		data.emplace_back(cur);
	}
	uint64_t ans = data[0];
	for(int i = 1; i < data.size(); i++){
		ans ^= data[i];
	}
	return ans;
}

double tv2ms(struct timeval *tv) {
	return 1000.0*tv->tv_sec + 0.001*tv->tv_usec;
}

static int sockfd = -1;
static struct sockaddr_in servin;
static unsigned seq;
static unsigned cnt = 0;

// Driver code
int main(int argc, char *argv[]){
	cout << argc << endl;
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
	servin.sin_port = htons(strtol(argv[3], NULL, 0));
	if(inet_pton(AF_INET, argv[4], &servin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[4]);
	}

    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("Socket Error\n");
        exit(0);
    }
	
	DIR *d = opendir(argv[1]) ;
	int num_file = strtol(argv[2], nullptr, 10);
	struct dirent *dir;
	int cnt = 0;
	int seq_num = 0;
	if(d){
		while ((dir = readdir(d)) != NULL){
			if(cnt == num_file) break;
			cnt++;
			cout << "cnt = " << cnt << endl;
			
			cout << "file name = " << dir->d_name << endl;
			string path = string(argv[1]) + "/" + string(dir->d_name);
			FILE *fp = fopen(path.c_str(), "r");
			cout << &fp << endl;
			bzero(buf, sizeof(buf));
			if(fp){
				std::cout << "Reading file contents\n";
				cout << "seek end = " << fseek(fp, 0, SEEK_END) << endl;
				size_t file_size = ftell(fp);
				cout << "file size = " << file_size << endl;
				cout << "seek set = " << fseek(fp, 0, SEEK_SET) << endl;
				if(fread(buf, file_size, 1, fp) <= 0){
					std::cout << "Unable to copy file into buffer or empty file\n";
					exit(1);
				}else{
					string fileName_msg = "<" + to_string(seq_num++) + to_string(checksum(string(dir->d_name))) + ">" + string(dir->d_name);
					char ackbuf[1024];
					bzero(ackbuf, sizeof(ackbuf));
					while (1){
						pair<int, string> packet;
						if(seq_num == packet.first && packet.second == "ACK") break;
						sendto(sockfd, fileName_msg.c_str(), fileName_msg.length(), 0, (struct sockaddr*)&servin, sizeof(servin));
						recvfrom(sockfd, ackbuf, sizeof(ackbuf), 0, (struct sockaddr *)&servin, sizeof(servin));
					}
					string msg = "<" + to_string(seq_num++) + to_string(checksum(string(buf))) + ">" + string(buf);
					sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr *) &servin, sizeof(servin));
				}
			}else{
				std::cout << "Cannot open file\n";
				exit(0);
			}
			fclose(fp);
			if(cnt == num_file) break;
		}
		
	}

	// request to send datagram
	// no need to specify server address in sendto
	// connect stores the peers IP and port
	// sendto(sockfd, buf, MAXLINE, 0, (struct sockaddr*)NULL, sizeof(servaddr));
	
	// waiting for response
	recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL);
	puts(buffer);

	// close the descriptor
	close(sockfd);
}

// #include <bits/stdc++.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
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
#include <filesystem>
#include <dirent.h>

#define PORT 5000
#define MAXLINE 1000
using namespace std;
char buf[10000000];

double tv2ms(struct timeval *tv) {
	return 1000.0*tv->tv_sec + 0.001*tv->tv_usec;
}

static int sockfd = -1;
static struct sockaddr_in servin;
static unsigned seq;
static unsigned cnt = 0;

// Driver code
int main(int argc, char *argv[]){
	cout << argc << endl;
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
	servin.sin_port = htons(strtol(argv[3], NULL, 0));
	if(inet_pton(AF_INET, argv[4], &servin.sin_addr) != 1) {
		return -fprintf(stderr, "** cannot convert IPv4 address for %s\n", argv[4]);
	}

    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("Socket Error\n");
        exit(0);
    }
		

	char buffer[100];
	// char *message = "Hello Server";
	//int sockfd, n;
	//struct sockaddr_in servaddr;
	
	// clear servaddr
	
    //string faddr = string(argv[0]);
	DIR *d = opendir(argv[1]) ;
	// printf("file path = %s", argv[1]);
	int num_file = strtol(argv[2], nullptr, 10);
	struct dirent *dir;
	int cnt = 0;
	if(d){
		while ((dir = readdir(d)) != NULL){
			if(cnt == num_file) break;
			cnt++;
			cout << "cnt = " << cnt << endl;
			
			cout << "file name = " << dir->d_name << endl;
			string path = string(argv[1]) + "/" + string(dir->d_name);
			FILE *fp = fopen(path.c_str(), "r");
			cout << &fp << endl;
			if(fp){
				std::cout << "Reading file contents\n";
				cout << "seek end = " << fseek(fp, 0, SEEK_END) << endl;
				size_t file_size = ftell(fp);
				cout << "file size = " << file_size << endl;
				cout << "seek set = " << fseek(fp, 0, SEEK_SET) << endl;
				if(fread(buf, file_size, 1, fp) <= 0){
					std::cout << "Unable to copy file into buffer or empty file\n";
					exit(1);
				}else{
					
					cout  << "send file name " << sendto(sockfd, dir->d_name, strlen(dir->d_name), 0, (struct sockaddr*)&servin, sizeof(servin)) << endl;
					sendto(sockfd, buf, file_size, 0, (struct sockaddr *) &servin, sizeof(servin));
				}
			}else{
				std::cout << "Cannot open file\n";
				exit(0);
			}
			fclose(fp);
			if(cnt == num_file) break;
		}
		
	}

	// request to send datagram
	// no need to specify server address in sendto
	// connect stores the peers IP and port
	// sendto(sockfd, buf, MAXLINE, 0, (struct sockaddr*)NULL, sizeof(servaddr));
	
	// waiting for response
	recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL);
	puts(buffer);

	// close the descriptor
	close(sockfd);
}

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
