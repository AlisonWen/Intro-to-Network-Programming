/* The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){
     int listenfd, connfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     
     listenfd =  socket(AF_INET, SOCK_STREAM, 0);
     if (listenfd < 0) 
        error("ERROR opening socket");
     
     bzero((char *) &serv_addr, sizeof(serv_addr));
     bzero((char *) &cli_addr, sizeof(cli_addr));

     portno = atoi(argv[1]); // get port number

     serv_addr.sin_family = AF_INET;  
     serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
     serv_addr.sin_port = htons(portno);

     if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     // Here, we set the maximum size for the backlog queue to 20.
     listen(listenfd, 20);

     // The accept() call actually accepts an incoming connection
     clilen = sizeof(cli_addr);

     // This accept() function will write the connecting client's address info 
     // into the the address structure and the size of that structure is clilen.
     // The accept() returns a new socket file descriptor for the accepted connection.
     // So, the original socket file descriptor can continue to be used 
     // for accepting new connections while the new socker file descriptor is used for
     // communicating with the connected client.
    for(;;){
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
        if(connfd < 0) error("accept error\n");

        printf("server: got connection from %s port %d\n",inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        printf("connfd = %d\n", connfd);

        pid_t pid;
        pid = fork();
        printf("argv = %s\n", argv[2]);
        if(pid == 0){
            pid = fork();
            if(pid > 0) exit(0);
            int tmp = dup(2); // remember the local stderror
            dup2(connfd, 0); // pipe stdin to client
            dup2(connfd, 1); // pipe the stdout to client
            dup2(connfd, 2);  // pipe the error to client
            if(execvp(argv[2], argv + 2) < 0) {
                dup2(tmp, 2); // pipe the error back if it was execution error of the server
                error("Execution Error");
                exit(0);
                close(connfd);
            }
            exit(0);
        }else wait(NULL);
        close(connfd);
    }
     return 0; 
}
