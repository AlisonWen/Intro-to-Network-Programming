#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <filesystem>
#include <dirent.h>
#include <vector>
#include <unordered_map> 
#include <ctime>
#include <map>

#define PORT 5000
#define MAXLINE 1000
using namespace std;
using pii = pair<int, int>;
char buf[10000000];

map <pii, int> ACKed;
map <pii, string> send_failed;

pair<pii, string> decompose(string content){ // decompose a packet into 1.seq num, 2.checksum, 3.content(string)
	stringstream ss;
	string buf;
	int seq, offset;
	ss << content;
	ss >> seq >> offset >> buf;
	return {{seq, offset}, buf};
}

static int sockfd = -1;
static struct sockaddr_in servin;
static unsigned seq;
static unsigned cnt = 0;

// Driver code
int main(int argc, char *argv[]){
    if(argc < 3) { // not enough parameters
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}
    srand(time(0) ^ getpid());

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
			//skip the directory "."
			if (!strcmp (dir->d_name, "."))
            	continue;
			if (!strcmp (dir->d_name, ".."))    
				continue;
			if (dir->d_name[0] == '.') continue;
			
			if(cnt == num_file) break;
			cnt++;
			
			string path = string(argv[1]) + "/" + string(dir->d_name); //cout << "path = " << path << endl;
			FILE *fp = fopen(path.c_str(), "r");
			bzero(buf, sizeof(buf));

			if(fp){
				fseek(fp, 0, SEEK_END);
				size_t file_size = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				vector <string> content;
				if(file_size < 1000){
					fread(buf, file_size, 1, fp);
					content.emplace_back(string(buf));
				}else{
					int n = file_size / 1000;
					char content_buf[1024];
					for(int i = 0; i < n; i++){
						bzero(content_buf, sizeof(content_buf));
						fread(content_buf, 1000, 1, fp);
						content.emplace_back(string(content_buf));
					}
					if(file_size % 1000) {
						bzero(content_buf, sizeof(content_buf));
						fread(content_buf, file_size - (1000 * n), 1, fp);
						content.emplace_back(string(content_buf));
					}
				}
				
				char ackbuf[1024];
				pair<pii, string> packet = {{-1, -1}, ""};

				for(int offset = 0; offset < content.size(); offset++){
					packet = {{-1, -1}, ""};
					if(ACKed[{seq_num, offset}]) continue;
					string msg = "<" + to_string(seq_num) + " " + to_string(content.size()) + " " + to_string(offset) + " " + to_string(content[offset].size()) + " " + string(dir->d_name) + " >" + string(content[offset]);
					//for(int num = 0; num < 40; num++){
						packet = {{-1, -1}, ""};
						bzero(ackbuf, sizeof(ackbuf));
						int rlen = sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr*)&servin, sizeof(servin));
						usleep(2000);
						socklen_t servinlen = sizeof(servin);
						recvfrom(sockfd, ackbuf, 1024, MSG_DONTWAIT, (struct sockaddr*)&servin, &servinlen);
						packet = decompose(string(ackbuf));
						if(packet.first.first == seq_num && packet.first.second == offset && packet.second == "ACK") {
							ACKed[{seq_num, offset}] = 1;
							break;
						}
					//}
					if(ACKed.find({seq_num, offset}) == ACKed.end()){
						send_failed[{seq_num, offset}] = msg;
					}
				}
			}else{
				std::cout << "Cannot open file\n";
				exit(0);
			}
			fclose(fp);
			seq_num++;
			if(cnt == num_file) break;
		}
		while (!send_failed.empty()){ //cout << "left size = " << send_failed.size() << endl;
			for(auto &f : send_failed){
				string msg = f.second;
				int rlen = sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr*)&servin, sizeof(servin));
				usleep(1000);
				char ackbuf[1024];
				bzero(ackbuf, sizeof(ackbuf));
				socklen_t servinlen = sizeof(servin);
				recvfrom(sockfd, ackbuf, 1024, MSG_DONTWAIT, (struct sockaddr*)&servin, &servinlen);
				pair<pii, string> packet = decompose(string(ackbuf));

				if(packet.second == "ACK") {
					ACKed[{packet.first}] = 1;
					send_failed.erase(f.first);
				}
			}	
		}
		sleep(10);
	}
	// close the descriptor
	close(sockfd);
}
