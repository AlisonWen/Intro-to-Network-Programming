// udp client driver program
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <filesystem>
#include <dirent.h>
#include <vector>
#include <unordered_map>
#include <sys/types.h> 
#include <ctime>
#include <map>

#define PORT 5000
#define MAXLINE 1000
using namespace std;
using pii = pair<int, int>;
char buf[10000000];

map <pii, int> ACKed;




pair<pii, string> decompose(string content){ // decompose a packet into 1.seq num, 2.checksum, 3.content(string)
	stringstream ss;
	string buf;
	int seq, offset;
	ss << content;
	ss >> seq >> offset >> buf;
	return {{seq, offset}, buf};
}

uint64_t checksum(string content){
	vector <uint64_t> data;
	uint64_t cur = 0;
	char tmp[8];
	for(int i = 0; i < content.size() / 8; i++){
		for(int j = i * 8; j <(i + 1) * 8; j++) tmp[j % 8] = content[j];
		sscanf(tmp, "%ld", &cur);
		data.emplace_back(cur);
	}
	if(content.size() % 8){
		for(int i = ((int)(content.size() / 8)) * 8; i < content.size(); i++) tmp[i % 8] = content[i];
		sscanf(tmp, "%ld", &cur);
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
    if(argc < 3) { // not enough parameters
		return -fprintf(stderr, "usage: %s ... <port> <ip>\n", argv[0]);
	}
	
    srand(time(0) ^ getpid());

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

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
			
			if(cnt == num_file) break;
			cnt++;
			
			//cout << "cnt = " << cnt << endl;
			//cout << "file name = " << dir->d_name << endl;

			string path = string(argv[1]) + "/" + string(dir->d_name);
			FILE *fp = fopen(path.c_str(), "r");
			//cout << &fp << endl;
			// sleep(2);
			bzero(buf, sizeof(buf));
			if(fp){
				cout << "Reading file " << seq_num << " contents\n";
				fseek(fp, 0, SEEK_END);
				size_t file_size = ftell(fp);
				//cout << "file size = " << file_size << endl;
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
						fread(content_buf, file_size - 1000 * n, 1, fp);
						content.emplace_back(string(content_buf));
					}
				}
				
				//cout << "content size = " << content.size() << endl;
				//for(auto i : content) cout << i << endl;
				// string msg = "<" + to_string(seq_num) + " " + to_string(checksum(string(buf))) + " " + string(dir->d_name) + " >" + string(buf);
				char ackbuf[1024];
				
				//for(int num = 0; num < 100; num++){ // 最多送 10 次避免卡住
					
				pair<pii, string> packet = {{-1, -1}, ""};
				for(int offset = 0; offset < content.size(); offset++){
					packet = {{-1, -1}, ""};
					//cout<<seq_num<<" "<<offset<<endl;
					if(ACKed[{seq_num, offset}]) continue;
					string msg = "<" + to_string(seq_num) + " " + to_string(content.size()) + " " + to_string(offset) + " " + to_string(checksum(string(content[offset]))) + " " + string(dir->d_name) + " >" + string(content[offset]);
					//cout << "msg = " << msg << endl;
					for(int num = 0; num < 10; num++){
						packet = {{-1, -1}, ""};
						bzero(ackbuf, sizeof(ackbuf));
						int rlen = sendto(sockfd, msg.c_str(), msg.length(), 0, (struct sockaddr*)&servin, sizeof(servin));
						//cout << "sending the msg " << rlen << endl;
						usleep(500);
						socklen_t servinlen = sizeof(servin);
						recvfrom(sockfd, ackbuf, 1024, MSG_DONTWAIT, (struct sockaddr*)&servin, &servinlen);
						//cout << "ack buf = " << string(ackbuf) << endl;
						packet = decompose(string(ackbuf));
						if(packet.first.first == seq_num && packet.first.second == offset && packet.second == "ACK") {
							ACKed[{seq_num, offset}] = 1;
							//cout << "ACK received!" << endl;
							break;
						}
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
		
	}

	// close the descriptor
	close(sockfd);
}
