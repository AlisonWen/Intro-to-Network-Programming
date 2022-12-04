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
#include <unordered_map>

#define err_quit(m) { perror(m); exit(-1); }
using namespace std;

unordered_map <int, int> record;
int _check;
// <seq num, <file name, content>>
pair<int, pair<string, string>> decompose(string content){ // decompose a packet into 1.seq num, 2.checksum, 3.content(string)
	content.erase(content.begin()); // erase '<'
	string fileName = "";
    int seq_num = 0;
	stringstream ss;
    ss << content;
    ss >> seq_num >> _check >> fileName;
	auto start = content.find('>');
	string _content = content.substr(start + 1, content.size() - start);
	return make_pair(seq_num, make_pair(fileName, _content));
}

bool checksum(const char content[]){
    vector <uint64_t> data;
	uint64_t cur = 0;
	char tmp[8];
	for(int i = 0; i < strlen(content) / 8; i++){
		for(int j = i * 8; j <(i + 1) * 8; j++) tmp[j % 8] = content[j];
		sscanf(tmp, "%ld", &cur);
		data.emplace_back(cur);
	}
	if(strlen(content) % 8){
		for(int i = ((int)(strlen(content) / 8)) * 8; i < strlen(content); i++) tmp[i % 8] = content[i];
		sscanf(tmp, "%ld", &cur);
		data.emplace_back(cur);
	}
	uint64_t ans = data[0];
	for(int i = 1; i < data.size(); i++){
		ans ^= data[i];
	}
	if(ans == (uint64_t)_check) return true;
    return false;
}

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
    char fileBuf[1024];

    for(int i = 0; i < file_num;){
        for(int num = 0; num < 10; num++){
            bzero(fileBuf, sizeof(fileBuf));
            if(rlen = recvfrom(s, fileBuf, strlen(fileBuf), 0, (struct sockaddr*) &csin, &csinlen) <= 0){
                printf("Didn't receive the file text!");
                continue;
            }
            pair<int, pair<string, string>> packet = decompose(string(fileBuf));
            if(record[packet.first]) {
                string ack_msg = "<" + to_string(packet.first) + " 0>ACK";
                continue;
            }
            FILE *fp;
            int fsize = packet.second.second.size();
            string _path = string(store_path) + "/" + packet.second.first;
            fp = fopen(_path.c_str(), "w");
            if(fp && checksum(packet.second.second.c_str())){
                fwrite(fileBuf, fsize, 1, fp);
                printf("File received successfully.\n");
            }
            else{
                printf("Cannot create to output file.\n");
            }
            fclose(fp);
            string ack_msg = "<" + to_string(packet.first) + " 0>ACK";
            sendto(s, ack_msg.c_str(), ack_msg.length(), 0, (struct sockaddr *)&csin, csinlen); 
            record[packet.first] = 1;
            i++;
        }
        
        
    }
	close(s);
}
