#include <bits/stdc++.h>
#include <string>
#include <poll.h>
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
#define OPEN_MAX 1005

using namespace std;
using pss = pair<string, string>;
using channel_est = pair<string, pair<string, set<string>>>;
//pair<string, pair<pair<string, string>, pair<string, string>>> usr_info[1005]; // usr_info[connfd] = 
unordered_map<string, int> nicknames;
map <string, pair<string, set<int>> > channels; //channels[channel name] = <second.first, second.second>
struct pollfd		client[OPEN_MAX];
int maxi;

struct Users{
    string nickname, username, hostname, servername, realname;
}usr_info[1005];

void Error(int sockfd, int NNN, string s){
    string err_msg = ":Hogwarts " + to_string(NNN) + " " + usr_info[sockfd].nickname;
    if(NNN == 401) err_msg += " " + s + " :No such nick/channel\n";
    else if(NNN == 403) err_msg += " " + s + " :No such channel\n";
    else if(NNN == 409) err_msg += " :No origin specified\n";
    else if(NNN == 411) err_msg += " :No recipient given (" + s + ")\n";
    else if(NNN == 412) err_msg += " :No text to send\n";
    else if(NNN == 421) err_msg += " " + s + " :Unknown command\n";
    else if(NNN == 431) err_msg += " :No nickname given\n";
    else if(NNN == 436) err_msg += " " + s + " :Nickname collision KILL\n";
    else if(NNN == 442) err_msg += " " + s + " :You're not on that channel\n";
    else if(NNN == 461) err_msg += " " + s + " :Not enough parameters\n";
    write(sockfd, err_msg.c_str(), err_msg.length()); cout << err_msg;
}

void Send_MTOD(int sockfd, string usr_name){
    string buf = ":Hogwarts 001 " + usr_name + " :Welcome to the minimized IRC daemon!\n";
    write(sockfd, buf.c_str(), buf.length());
    buf = ":Hogwarts 375 " + usr_name + " :- Hogwarts Message of the day -\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " : __     __\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :|  |   |  |\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :|  |___|  |                                                   _\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :|         |   ____     ____ _  __        __  _____    ____  _| |_   ___\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :|   ___   |  / __ \\   / __ \\ | | \\  /\\  / | / __  \\  | '_/ |_   _| /   \\ \n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :|  |   |  | | /  \\ | | /  \\  | | | |  | | || /  \\  | | |     | |   \\_ \\/ \n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :|  |   |  | | \\__/ | | |__|  |  \\ \\/  \\/ / | |__/  | | |     | |    _\\ \\ \n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :|__|   |__|  \\____/   \\____| |   \\__/\\__/   \\___/|_| |_|     |_|   \\_/_/ \n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :                           | | \n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :                    /\\_____/ |  \n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :                    \\_______/ \n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    cout << buf;
    buf = ":Hogwarts 375 " + usr_name + " :-  minimized internet relay chat daemon\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    buf = ":Hogwarts 375 " + usr_name + " :-\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
    buf = ":Hogwarts 375 " + usr_name + " :End of message of the day\n";
    send(sockfd, buf.c_str(), buf.length(), MSG_NOSIGNAL);
}

void List_Users(int sockfd){

}

int format_check(int sockfd, string s){
    stringstream ss;
    ss << s;
    string cmd, param;
    ss >> cmd >> param;
    if(cmd.back() == '\n') cmd.pop_back();
    if(param.back() == '\n') param.pop_back();
    if(cmd == "NICK"){
        if(param == ""){
            Error(sockfd, 431, ""); // no nick given
            return 0;
        }
        if(!nicknames.empty() && nicknames[param]){
            Error(sockfd, 436, param); // nick collision
            return 0;
        }
        //nicknames.erase(nicknames.find(usr_info[sockfd].nickname));
        string msg = ":" + usr_info[sockfd].nickname + " NICK " + param + "\n";
        write(sockfd, msg.c_str(), msg.length());
        nicknames[usr_info[sockfd].nickname] = 0;
        usr_info[sockfd].nickname = param;
        nicknames[param] = 1;
    }else if(cmd == "USER"){
        int cnt = 0;
        for(auto i : s){
            if(i == ' ' && cnt < 3) {
                cnt++;
                continue;
            }
            if(i == ':') continue;
            if(i == '\n') break;
            if(cnt == 0) usr_info[sockfd].username.push_back(i);
            else if(cnt == 1) usr_info[sockfd].hostname.push_back(i);
            else if(cnt == 2) usr_info[sockfd].servername.push_back(i);
            else if(cnt == 3) usr_info[sockfd].realname.push_back(i);
        }
        if(cnt < 3){
            Error(sockfd, 461, s); // need more params
            return 0;
        }
        Send_MTOD(sockfd, usr_info[sockfd].nickname);
    }else if(cmd == "PING"){
        if(param == ""){
            Error(sockfd, 409, ""); // no origin specified
            return 0;
        }
        string reply = "PONG " + param + "\n";
        write(sockfd, reply.c_str(), reply.length());
    }else if(cmd == "LIST"){ // list channels and their second.firsts
        string msg = ":Hogwarts 321 " + usr_info[sockfd].nickname + " Channel :Users Name\n";
        write(sockfd, msg.c_str(), msg.length()); cout << msg;

        if(param == ""){ // list all channels
            for(auto &i: channels){
                if(i.second.second.empty()) channels.erase(i.first);
                string msg = ":Hogwarts 322 " +  usr_info[sockfd].nickname + " " + i.first + " #" + to_string(i.second.second.size()) + " :" + i.second.first + "\n";
                write(sockfd, msg.c_str(), msg.length()); cout << msg;
            }
        }else{
            vector <string> names;
            string tmp = "";
            for(int i = 0; i < param.size(); i++){
                if(param[i] == ',') names.emplace_back(tmp);
                else if(param[i] == '#') tmp = "";
                else tmp.push_back(param[i]);
                if(i == param.size() - 1) names.emplace_back(tmp);
            }
            for(auto i : names){
                if(channels.find(i) == channels.end()) continue;
                msg = ":Hogwarts 332 " + usr_info[sockfd].nickname + " " + i + " #" + to_string(channels[i].second.size()) + " :" + channels[i].first + "\n";
                write(sockfd, msg.c_str(), msg.length()); cout << msg;
            }
        }
        msg.clear();
        msg = ":Hogwarts 323 " + usr_info[sockfd].nickname + " :End of /List\n";
        write(sockfd, msg.c_str(), msg.length()); cout << msg;
    }else if(cmd == "JOIN"){
        if(param == "") {
            Error(sockfd, 461, s); // need more parameters
            return 0;
        }
        param.erase(param.begin());
        string msg = ":" + usr_info[sockfd].nickname + " JOIN #" + param + "\n";
        write(sockfd, msg.c_str(), msg.length()); cout << msg;
        channels[param].second.emplace(sockfd);
        if(channels[param].first.empty()){ // no topic is set
            string msg = ":Hogwarts 331 " + usr_info[sockfd].nickname + " #" + param + " :No topic is set\n";
            write(sockfd, msg.c_str(), msg.length()); cout << msg;
        }else{
            string msg = ":Hogwarts 332 " + usr_info[sockfd].nickname + " #" + param + " :" + channels[param].first + "\n";
            write(sockfd, msg.c_str(), msg.length()); cout << msg;
        }
        msg.clear();
        msg = ":Hogwarts 353 " + usr_info[sockfd].nickname + " #" + param + " :";
        for(auto &i : channels[param].second){
            msg += usr_info[i].nickname + " ";
        }
        msg.push_back('\n');
        write(sockfd, msg.c_str(), msg.length()); cout << msg;
        msg.clear();
        msg = ":Hogwarts 366 " + usr_info[sockfd].nickname + " #" + param + " :End of Names List\n";
        write(sockfd, msg.c_str(), msg.length()); cout << msg;
    }else if(cmd == "TOPIC"){ //cout << "debug*************\n";
        if(param.empty()) {
            Error(sockfd, 461, s); // not enough parameters
            return 0;
        }
        stringstream ss;
        ss << s;
        string _channel, _topic, trash;
        ss >>trash >> _channel >> _topic; cout << param <<  " split = " << _channel << " " << _topic << endl;
        if(_channel.empty() || _topic.empty()){
            Error(sockfd, 461, s); // not enough parameters
            return 0;
        }
        _channel.erase(_channel.begin());
        _topic.erase(_topic.begin());
        cout << "members : ";
        for(auto idx : channels[_channel].second){
            cout << usr_info[idx].nickname << ", ";
        }cout << endl;
        if(channels[_channel].second.find(sockfd) == channels[_channel].second.end()){
            Error(sockfd, 442, _channel); // not in the channel
            return 0;
        }else{
            channels[_channel].first = _topic;
            string msg = ":Hogwarts 332 " + usr_info[sockfd].nickname + " #" + _channel + " :" + _topic + "\n";
            for(auto i : channels[_channel].second) write(i, msg.c_str(), msg.length()); cout << msg;
        }
    }else if(cmd == "NAMES"){
        if(param.empty()){
            for(auto _channel : channels){
                string msg = "";
                if(!_channel.second.second.empty()){
                    msg = ":Hogwarts 353 " + usr_info[sockfd].nickname + " #" + _channel.first + " :";
                    for(auto names : _channel.second.second){
                        msg += names + " ";
                    }
                    msg.push_back('\n');
                    write(sockfd, msg.c_str(), msg.length()); cout << msg;
                }
                msg.clear();
                msg = ":Hogwarts 366 " + usr_info[sockfd].nickname + " #" + _channel.first + " :End of Names List\n";
                write(sockfd, msg.c_str(), msg.length()); cout << msg ;
            }
        }else{
            if(param[0] == '#') param.erase(param.begin()); // erase '#'
            string msg = ":Hogwarts 353 " + usr_info[sockfd].nickname + " #" + param + " :";
            for(auto &i : channels[param].second){
                msg += usr_info[i].nickname + " ";
            }
            msg.push_back('\n');
            write(sockfd, msg.c_str(), msg.length()); cout << msg;
            msg.clear();
            msg = ":Hogwarts 366 " + usr_info[sockfd].nickname + " #" + param + " :End of Names List\n";
            write(sockfd, msg.c_str(), msg.length()); cout << msg ;
        }
    }else if(cmd == "PART"){
        if(param == "") {
            Error(sockfd, 461, s);
            return 0;
        }
        if(param[0] == '#') param.erase(param.begin());
        if(channels.find(param) == channels.end()){
            Error(sockfd, 403, param);
            return 0;
        }
        if(channels[param].second.find(sockfd) == channels[param].second.end()){
            Error(sockfd, 442, param);
            return 0;
        }
        channels[param].second.erase(sockfd);
        string msg = ":" + usr_info[sockfd].nickname + " PART :#" + param + "\n";
        write(sockfd, msg.c_str(), msg.length());  cout << msg;
    }else if(cmd == "USERS"){
        string msg = ":Hogwarts 392 " + usr_info[sockfd].nickname + " :";//UserID                           Terminal  Host\n";
        char buf[300];
        string usr_ID = "UserID", terminal = "Terminal", host = "Host";
        sprintf(buf, ":%-8s %-9s %-8s", usr_ID.c_str(), terminal.c_str(), host.c_str());
        msg += string(buf) + "\n";
        write(sockfd, msg.c_str(), msg.length());cout << msg;
        for(int i = 1; i <= maxi; i++){
            if(client[i].fd < 0) continue;
            char buf[300];
            sprintf(buf, ":%-8s %-9s %-8s", usr_info[client[i].fd].nickname.c_str(), "-", usr_info[client[i].fd].hostname.c_str());
            string msg =":Hogwarts 393 " + usr_info[sockfd].nickname + string(buf) + "\n";
            write(sockfd, msg.c_str(), msg.length());cout << msg;
        }
        msg = ":Hogwarts 394 " + usr_info[sockfd].nickname + " :End of users\n";
        write(sockfd, msg.c_str(), msg.length());
    }else if(cmd == "PRIVMSG"){
        stringstream ss;
        ss << s;
        string _channel, message, trash;
        ss >> trash >> _channel >> message;
        if(_channel.empty()) {
            Error(sockfd, 411, "PRIVMSG");
            return 0;
        }
        if(message.empty()) {
            Error(sockfd, 412, "");
            return 0;
        }
        if(_channel[0] == '#') _channel.erase(_channel.begin());
        if(message[0] == ':') message.erase(message.begin());
        if(message.back() == '\n') message.pop_back();
        if(channels.find(_channel) == channels.end()) {
            Error(sockfd, 401, "#"+ _channel);
            return 0;
        }
        string msg = ":" + usr_info[sockfd].nickname + " PRIVMSG #" + _channel + " :" + message + "\n";
        for(auto &i:channels[_channel].second){
            write(i, msg.c_str(), msg.length()); cout << msg;
        }
    }else if(cmd == "QUIT"){
        string msg = "QUIT :Bye\n";
        write(sockfd, msg.c_str(), msg.length()); cout << msg;
        close(sockfd);
        for(int i = 1; i <= maxi; i++) {
            if(client[i].fd == sockfd) {
                client[i].fd = -1;
                break;
            }
        }
        for(auto &i : channels){
            if(i.second.second.find(sockfd) != i.second.second.end()) i.second.second.erase(i.second.second.find(sockfd));
            nicknames.erase(nicknames.find(usr_info[sockfd].nickname));
        }
        usr_info[sockfd].nickname = usr_info[sockfd].username = usr_info[sockfd].hostname = usr_info[sockfd].servername = usr_info[sockfd].realname = "";
    }else{
        Error(sockfd, 421, s);
    }
    return 0;
}

int main(int argc, char **argv){
	int					i;
    int  listenfd, connfd, sockfd;
	int					nready;
	ssize_t				n;
	char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(10004);

    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Binding Error\n");
        exit(0);
    }

	listen(listenfd, 1000);

	client[0].fd = listenfd;
	client[0].events = POLLRDNORM;
	for (i = 1; i < OPEN_MAX; i++) client[i].fd = -1;/* -1 indicates available entry */
	maxi = 0;					/* max index into client[] array */
/* end fig01 */

/* include fig02 */
	for ( ; ; ) {
		nready = poll(client, maxi+1, 20);

		if (client[0].revents & POLLRDNORM) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
            if(connfd < 0){
                perror("Connection failed");
                exit(0);
            }
			// printf("new client: %s\n", inet_ntoa((struct sockaddr *) &cliaddr, clilen));

			for (i = 1; i < OPEN_MAX; i++)
				if (client[i].fd < 0) {
					client[i].fd = connfd;	/* struct sockaddrve descriptor */
					break;
				}
			if (i == OPEN_MAX)
				perror("too many clients");

			client[i].events = POLLRDNORM;
			if (i > maxi) maxi = i;				/* max index in client[] array */
            
			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

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
                    string tmp = "";
                    vector <string> instr;
                    printf("received buffer = %s", buf);
                    for(int i = 0; i < n; i++){
                        if(buf[i] == '\n' || buf[i] == '\r'){
                            instr.emplace_back(tmp);
                            tmp = "";
                        }else tmp.push_back(buf[i]);
                    }
                    if(tmp != "") instr.emplace_back(tmp);
                    for(auto i : instr){
                        cout << "input = " << i  << ", strlen = " << i.size() << ", n = " << n<< endl;
                        if(i.size() > 0){
                            format_check(sockfd, i);
                        }
                    }
                    instr.clear();
                }

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}
