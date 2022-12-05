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

struct Packet{
    int seq_idx;
    string f_name;
    string f_cnt;
};

unordered_map <int, int> record;
int _check;
// <seq num, <file name, content>>
Packet decompose(string content){ // decompose a packet into 1.seq num, 2.checksum, 3.content(string)
	content.erase(content.begin()); // erase '<'
	string fileName = "";
    int seq_num = 0;
	stringstream ss;
    ss << content;
    ss >> seq_num >> _check >> fileName;
	auto start = content.find('>');
	string _content = content.substr(start + 1, content.size() - start);

    Packet packet;
    packet.seq_idx = seq_num;
    packet.f_name = fileName;
    packet.f_cnt = _content;

	return packet;
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
    char fileBuf[100000];

    for(int i = 0; i < file_num;){
        for(int num = 0; num < 10; num++){
            bzero(fileBuf, sizeof(fileBuf));
            if(rlen = recvfrom(s, fileBuf, 100000, MSG_DONTWAIT, (struct sockaddr*) &csin, &csinlen) <= 0){
                //printf("Didn't receive the file text!\n");
                char trash[] = "nono";
                sendto(s, trash, strlen(trash), 0, (struct sockaddr *)&csin, csinlen); 
                continue;
            }
            //pair<int, pair<string, string>> packet = decompose(string(fileBuf));
            Packet packet = decompose(string(fileBuf));
            if(record[packet.seq_idx]) {
                string ack_msg = "<" + to_string(packet.seq_idx) + " 0>ACK";
                continue;
            }

            FILE *fp;
            int fsize = packet.f_cnt.size();
            string _path = string(store_path) + "/" + packet.f_name;
            fp = fopen(_path.c_str(), "w");
            if(fp && checksum(packet.f_cnt.c_str())){
                fwrite(packet.f_cnt.c_str(), fsize, 1, fp);
                printf("File received successfully.\n");
            }
            else{
                printf("Cannot create to output file.\n");
            }
            fclose(fp);

            //told client that the file is received
            string ack_msg = "<" + to_string(packet.seq_idx) + " 0>ACK";
            sendto(s, ack_msg.c_str(), ack_msg.length(), 0, (struct sockaddr *)&csin, csinlen); 
            record[packet.seq_idx] = 1;
            i++;
            if(i >= file_num)break;
        }
    }
	close(s);
}
