/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define err_quit(m) { perror(m); exit(-1); }

int main(int argc, char *argv[]) {
	int s;
	struct sockaddr_in sin;

	if(argc < 4) {
		return -fprintf(stderr, "usage: %s ... <port>\n", argv[0]);
	}

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-1], NULL, 0));

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");

    int file_num = argv[argc-2];
    string store_path(argv[argc-3]);

    struct sockaddr_in csin;
    socklen_t csinlen = sizeof(csin);
    int rlen;
    char fileName[1024];
    char fileBuf[1024];#include <iostream>
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

#define err_quit(m) { perror(m); exit(-1); }
using namespace std;
int main(int argc, char *argv[]) {
	int s;
	struct sockaddr_in sin;

	if(argc < 4) {
		return -fprintf(stderr, "usage: %s ... <port>\n", argv[0]);
	}

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-1], NULL, 0));

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");

    int file_num = atoi(argv[argc-2]);
    char* store_path = argv[argc-3];

    struct sockaddr_in csin;
    socklen_t csinlen = sizeof(csin);
    int rlen;
    char fileName[1024];
    char fileBuf[1024];

    for(int i = 0; i < file_num; i++){
        
        if(rlen = recvfrom(s, fileName, strlen(fileName), 0, (struct sockaddr*) &csin, &csinlen) <= 0)
            printf("Didn't receive the file name!\n");
        else
            printf("NAME OF TEXT FILE RECEIVED : %s , %d\n",fileName, rlen);
        
        FILE *fp;
        
        if(rlen = recvfrom(s, fileBuf, strlen(fileBuf), 0, (struct sockaddr*) &csin, &csinlen) <= 0)
            printf("Didn't receive the file text!");
        else{
            printf("Contents in the received text file : \n");
            printf("%s\n",fileBuf);
        }
        
        int fsize = strlen(fileBuf);

        //strcat(store_path, fileName);
        string _path = string(store_path) + "/" + string(fileName);

        fp = fopen(_path.c_str(), "w");
        if(fp){
            fwrite(fileBuf, fsize, 1, fp);
            printf("File received successfully.\n");
        }
        else{
            printf("Cannot create to output file.\n");
        }

        memset(fileName, '\0', sizeof(fileName));
        fclose(fp);
    }

	// while(1) {
	// 	struct sockaddr_in csin;
	// 	socklen_t csinlen = sizeof(csin);
	// 	char buf[2048];
	// 	int rlen;
		
	// 	if((rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
	// 		perror("recvfrom");
	// 		break;
	// 	}

	// 	sendto(s, buf, rlen, 0, (struct sockaddr*) &csin, sizeof(csin));
	// }

	close(s);
}


    for(int i = 0; i < file_num; i++){
        
        if(rlen = recvfrom(s, fileName, sizeof(fileName), 0, (struct sockaddr*) &csin, &csinlen) < 0)
            printf("Didn't receive the file name!");
        else
            printf("NAME OF TEXT FILE RECEIVED : %s\n",fileName);
        
        FILE *fp;

        
        if(rlen = recvfrom(s, fileBuf, sizeof(fileBuf), 0, (struct sockaddr*) &csin, &csinlen) < 0)
            printf("Didn't receive the file text!");
        else{
            printf("Contents in the received text file : \n");
            printf("%s\n",fileBuf);
        }
        
        int fsize = strlen(fileBuf);

        fp = fopen(fileName, "w");
        if(fp){
            fwrite(fileBuf, fsize, 1, fp);
            printf("File received successfully.\n");
        }
        else{
            printf("Cannot create to output file.\n");
        }

        memset(fileName, '\0', sizeof(fileName));
        fclose(fp);
    }

	// while(1) {
	// 	struct sockaddr_in csin;
	// 	socklen_t csinlen = sizeof(csin);
	// 	char buf[2048];
	// 	int rlen;
		
	// 	if((rlen = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
	// 		perror("recvfrom");
	// 		break;
	// 	}

	// 	sendto(s, buf, rlen, 0, (struct sockaddr*) &csin, sizeof(csin));
	// }

	close(s);
}
