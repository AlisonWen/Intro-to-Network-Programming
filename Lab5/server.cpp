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
#define MAXLINE 1000000
using namespace std;
unordered_map <int, string> nickname, addr_port;
// <connfd, nickname>, <connfd, address:port>
string cmd = "";

bool format_check(string s){
	if(s[0] != '/' || s == "/who\n") return true;
	if(s[0] == '/' && s[1] == 'n' && s[2] == 'a' && s[3] == 'm' && s[4] == 'e' && s[5] == ' ' && s.size() > 7) return true;
	return false;
}

int main(int argc, char **argv){
	int					i, maxi, listenfd, connfd, sockfd;
	int					nready;
	ssize_t				n;
	char				buf[MAXLINE];
	socklen_t			clilen;
	struct pollfd		client[1005];
	struct sockaddr_in	cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0){
		perror("Socket Error");
		exit(0);
	}
	cout << "1005 = " << 1005 << endl;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		perror("Binding Error");
		exit(0);
	}

	listen(listenfd, 1005);

	client[0].fd = listenfd;
	client[0].events = POLLRDNORM;
	for (i = 1; i < 1005; i++)
		client[i].fd = -1;		/* -1 indicates available entry */
	maxi = 0;					/* max index into client[] array */
/* end fig01 */

/* include fig02 */
	for ( ; ; ) {
		nready = poll(client, maxi+1, 0);

		if (client[0].revents & POLLRDNORM) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
			if(connfd < 0){
				perror("Accepting Error");
				exit(0);
			}

			printf("client connected from %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));


			for (i = 1; i < 1005; i++)
				if (client[i].fd < 0) {
					client[i].fd = connfd;	/* struct sockaddrve descriptor */
					break;
				}
			if (i == 1005)
				perror("too many clients");

			client[i].events = POLLRDNORM;
			if (i > maxi) maxi = i;				/* max index in client[] array */
			
			//cout << "maxi = " << maxi << endl;
			// send hello message
			time_t t = time(0); // get current time
            tm* cur = localtime(&t);
            char time_arr[100] = {0};
            strftime(time_arr, sizeof(time_arr), "%Y-%m-%d %H:%M:%S", localtime(&t));
			string welcome_1 = string(time_arr) + " *** Welcome to the simple CHAT server\n";
			send(connfd, welcome_1.c_str(), welcome_1.length(), MSG_NOSIGNAL);
			string welcome_2 = string(time_arr) + " *** Total " + to_string(maxi) + " users online now. Your name is " + to_string(maxi) + "\n";
			send(connfd, welcome_2.c_str(), welcome_2.length(), MSG_NOSIGNAL);

			// send broadcast landing message
			string broadcast_msg = string(time_arr) + " *** User " + to_string(maxi) + " has just landed on the server\n";
			for (i = 1; i < 1005; i++){
				if (client[i].fd < 0) continue;;
				send(client[i].fd, broadcast_msg.c_str(), broadcast_msg.length(), MSG_NOSIGNAL);
			}
			// put name and address into list
			nickname[connfd] = to_string(maxi);
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str));
			auto _port = ntohs(cliaddr.sin_port);
			addr_port[connfd] = string(str) + ":" + to_string(_port);

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 1; i <= maxi; i++) {	/* check all clients for data */
			memset(buf, '\0', sizeof(buf));
			queue <int> left;
			if ( (sockfd = client[i].fd) < 0)
				continue;
			if (client[i].revents & (POLLRDNORM | POLLERR)) {
				if ( (n = read(sockfd, buf, MAXLINE)) < 0) {
					if (errno == ECONNRESET) {
							/*4connection reset by client */
#ifdef	NOTDEF
						//printf("client[%d] aborted connection\n", i);
						cout << "* client " << addr_port[sockfd] << " disconnected\n";

#endif
						close(sockfd);
						client[i].fd = -1;
					} else
						perror("read error");
				} else if (n == 0) {
						/*4connection closed by client */
					
					//printf("client[%d] closed connection\n", i);
					cout << "* client " << addr_port[sockfd] << " disconnected\n";
					//left.push(sockfd);
					time_t t = time(0); // get current time
					tm* cur = localtime(&t);
					char time_arr[100] = {0};
					strftime(time_arr, sizeof(time_arr), "%Y-%m-%d %H:%M:%S", localtime(&t));
					string disconnct_msg = string(time_arr) + " *** User <" + nickname[sockfd] + "> has left the server\n";
					for(i = 1; i <= maxi; i++){
						if(client[i].fd == sockfd || client[i].fd < 0) continue;
						send(client[i].fd, disconnct_msg.c_str(), disconnct_msg.length(), MSG_NOSIGNAL);
					}
					close(sockfd);
					client[i].fd = -1;
				} else {
					cmd = string(buf);
					if(!format_check(cmd)){
						time_t t = time(0); // get current time
						tm* cur = localtime(&t);
						char time_arr[100] = {0};
						strftime(time_arr, sizeof(time_arr), "%Y-%m-%d %H:%M:%S", localtime(&t));
						string err_msg = string(time_arr) + " *** Unknown or incomplete command " + cmd;
						send(sockfd, err_msg.c_str(), err_msg.length(), MSG_NOSIGNAL);
						continue;
					}
					if(cmd[0] == '/' && cmd[1] == 'n'){ // rename
						time_t t = time(0); // get current time
						tm* cur = localtime(&t);
						char time_arr[100] = {0};
						strftime(time_arr, sizeof(time_arr), "%Y-%m-%d %H:%M:%S", localtime(&t));

						string new_name = string(cmd.begin() + 6, cmd.end());
						new_name.pop_back();
						string change_name = string(time_arr) + " *** Nickname changed to " + new_name + "\n";
						send(sockfd, change_name.c_str(), change_name.length(), MSG_NOSIGNAL);
						string broadcast_msg = string(time_arr) + " *** User " + nickname[sockfd] + " renamed to " + new_name + "\n";
						for(i = 1; i <= maxi; i++){
							if(client[i].fd < 0 || client[i].fd == sockfd) continue;
							send(client[i].fd, broadcast_msg.c_str(), broadcast_msg.length(), MSG_NOSIGNAL);
						}
						nickname[sockfd] = new_name;
					}else if(cmd[0] == '/' && cmd[1] == 'w'){ // list all users
						string sep = "--------------------------------------------------\n";
						send(sockfd, sep.c_str(), sep.length(), MSG_NOSIGNAL);
						for(i = 1; i <= maxi; i++){
							if(client[i].fd < 0) continue;
							string usr_msg = "";
							if(client[i].fd == sockfd) usr_msg = "* ";
							else usr_msg = "  ";
							usr_msg += nickname[client[i].fd];
							while (usr_msg.size() < 24) usr_msg.push_back(' ');
							usr_msg += addr_port[client[i].fd];
							usr_msg.push_back('\n');
							send(sockfd, usr_msg.c_str(), usr_msg.length(), MSG_NOSIGNAL);
						}
						send(sockfd, sep.c_str(), sep.length(), MSG_NOSIGNAL);
					}else{ // conversation message
						time_t t = time(0); // get current time
						tm* cur = localtime(&t);
						char time_arr[100] = {0};
						strftime(time_arr, sizeof(time_arr), "%Y-%m-%d %H:%M:%S", localtime(&t));
						string text_msg = string(time_arr) + " <" + nickname[sockfd] + "> " + cmd;
						for(i = 1; i <= maxi; i++){
							if(client[i].fd < 0) continue;
							send(client[i].fd, text_msg.c_str(), text_msg.length(), MSG_NOSIGNAL);
						}
					}
				}

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}
