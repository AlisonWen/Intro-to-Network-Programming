#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
using namespace std;
char buf[1000000];
char ans[1000000000];

int main(void){

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1) cout << "Failed to create socket\n";
    else cout << "socket fd = " << socket_fd << endl;

    struct sockaddr_in info;
    bzero(&info, sizeof(info));
    info.sin_family = AF_INET;
    info.sin_addr.s_addr = inet_addr((char*)"140.113.213.213");
    info.sin_port = htons(10002);

    int conn_erro = connect(socket_fd, (struct sockaddr*)&info, sizeof(info));
    
    if(conn_erro == -1) cout << "Connection Error\n";
    else cout <<"error number = " <<  conn_erro << endl;
    long long bytes = 0;
    bzero(&buf, sizeof(buf));
    bytes = recv(socket_fd, buf, sizeof(buf), 0);
    for(long long i = 0; i < bytes ; i++) cout << buf[i] << " "; cout << endl;

    char _go[10] = "GO\n";
    send(socket_fd, _go, 3, 0); // 
    cout << "----------------\n";

    bzero(&buf, sizeof(buf));
    long long total_size = 0;
    long long cnt = 0;
    //long long bytes = 0;
    int times = 0;
    int index = 0;

    bool flag = 0;
    while(bytes = recv(socket_fd, buf, sizeof(buf), 0)){
        for(int i = 0; i < bytes; i++) cout << buf[i];
        cnt += bytes;
        if(buf[bytes - 2] == '?') break;
        cout << bytes << endl;
        
    }
    
    string s = to_string(cnt - 86);
    s.push_back('\n');
    cout << "s = " << s << endl;
    printf("%s", s.c_str());
    
    send(socket_fd, s.c_str(), sizeof(s.c_str()), 0);
    bzero(&buf, sizeof(buf));
    bytes = recv(socket_fd, buf, sizeof(buf), 0); // listen to the 
    return 0;
}
