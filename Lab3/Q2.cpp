#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <ctime>
#include <sys/time.h>
#include <sys/signal.h>
using namespace std;
static unsigned long long bytesent = 0;

double tv2s(struct timeval *ptv) { // timeval to second
	return 1.0 * (ptv -> tv_sec) + 0.000001 * (ptv -> tv_usec);
}

char buf[1000000000];

int main(int argc, char *argv[]){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1) cout << "Failed to create socket\n";
    else cout << "socket fd = " << socket_fd << endl;

    struct sockaddr_in info;
    bzero(&info, sizeof(info));
    info.sin_family = AF_INET;
    info.sin_addr.s_addr = inet_addr((char*)"140.113.213.213");
    info.sin_port = htons(10003);
    int conn_erro = connect(socket_fd, (struct sockaddr*)&info, sizeof(info));
    if(conn_erro == -1) cout << "Connection Error\n";
    else cout <<"error number = " <<  conn_erro << endl;
    // get the first message
    bzero(&buf, sizeof(buf));
    int bytes = recv(socket_fd, buf, sizeof(buf), 0);

    int cnt = 0;
    cout << "Bit Rate assign = " <<stod(string( argv[1])) << "\n";
    while(1){
        struct timeval t_0, t_1;
        gettimeofday(&t_0, NULL);
        double _size = stod(string(argv[1]));
	    for(int i = 0; i < 100000; i += 1000) send(socket_fd, buf,(uint64_t)(_size * 1000.0), 0);
        gettimeofday(&t_1, NULL);
        double t0 = tv2s(&t_0);
    	double t1 = tv2s(&t_1);    
    	usleep(100000 - (int)(1000000.0*(t1 - t0)));
    }

    return 0;
}
