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
#include <set>
#include <map>
#include <utility>

#define err_quit(m) { perror(m); exit(-1); }
using namespace std;

struct Packet{
    int seq_idx;
    int num_off;
    int offset;
    int checksum;
    int frag_size;
    string f_name;
};

vector<int> packet_list;                    //record how many offset of a file should be received
vector<bool> received_list;
vector<set<int>> record_list;               //record whether the packet is received
vector<vector<string>> fcnt_list;   //record the content of the packets


bool checkIsExist(int f_idx, int p_idx){
    if(record_list[f_idx].find(p_idx) == record_list[f_idx].end())
        return 0;
    else
        return 1;
}

string MergePackets(int idx){
    string cnt = "";
    for(auto &frag : fcnt_list[idx]){
        cnt += frag;
    }
    return cnt;
}

Packet decompose(string content){ // decompose a packet 
	content.erase(content.begin()); // erase '<'
	string fileName = "";
    int seq_num = 0;
    int num_packet;
    int off;
    int _frag_size;
	stringstream ss;
    ss << content;
    ss >> seq_num >> num_packet >> off >> _frag_size >> fileName;
	auto start = content.find('>');
	string fcnt = content.substr(start + 1, content.size() - start);
    
    //record how many offset 
    packet_list[seq_num] = num_packet;
    //if the content list hasn't been established
    if(fcnt_list[seq_num].empty())
        fcnt_list[seq_num].resize(num_packet);
    //record the content to the list
    if(!checkIsExist(seq_num, off))
        fcnt_list[seq_num][off] = fcnt;

    Packet packet;
    packet.seq_idx = seq_num;
    packet.num_off = num_packet;
    packet.offset = off;
    //packet.checksum = _check;
    packet.frag_size = _frag_size;
    packet.f_name = fileName;

	return packet;
}

int main(int argc, char *argv[]) {
	int s;
	struct sockaddr_in sin;

	if(argc < 4) {
		return -fprintf(stderr, "usage: %s ... <port>\n", argv[0]);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-1], NULL, 0));

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");

    int file_num = atoi(argv[argc-2]);
    char* store_path = argv[argc-3];

    //initialize the tables
    fcnt_list.resize(file_num);
    record_list.resize(file_num);
    packet_list.resize(file_num);
    received_list.resize(file_num);

    struct sockaddr_in csin;
    socklen_t csinlen = sizeof(csin);
    int rlen;
    char fileBuf[100000];

    int i = 0;
    while(i < file_num){
        bzero(fileBuf, sizeof(fileBuf));
        if(rlen = recvfrom(s, fileBuf, 100000, 0, (struct sockaddr*) &csin, &csinlen) <= 0){
            //printf("Didn't receive the file text!\n");
            char trash[] = "nono";
            sendto(s, trash, strlen(trash), 0, (struct sockaddr *)&csin, csinlen); 
            continue;
        }
        //printf("received = %s\n", fileBuf);
        Packet packet = decompose(string(fileBuf));


        string f_cnt = fcnt_list[packet.seq_idx][packet.offset];
        if(f_cnt.size() != packet.frag_size){   //checksum 不對
            //printf("not right checksum!\n");
            char trash[] = "nono";
            sendto(s, trash, strlen(trash), 0, (struct sockaddr *)&csin, csinlen); 
            continue;
        }

        //told client that the packet is received
        string ack_msg = to_string(packet.seq_idx) + " " + to_string(packet.offset) + " ACK";
        sendto(s, ack_msg.c_str(), ack_msg.length(), 0, (struct sockaddr *)&csin, csinlen); 
        //cout<<"ack sent!!\n";

        //deal with the received packets
        if(!checkIsExist(packet.seq_idx, packet.offset))    //第一次收到packet
            record_list[packet.seq_idx].insert(packet.offset);

        if(record_list[packet.seq_idx].size() == packet_list[packet.seq_idx] && !received_list[packet.seq_idx]){  
            //有任何一個file收到了所有的packet
            //write file
            i++;
            received_list[packet.seq_idx] = true;
            FILE *fp;
            string cnt = MergePackets(packet.seq_idx);
            string f_path = string(store_path) + "/" + packet.f_name;

            fp = fopen(f_path.c_str(), "w");
            if(fp){
                fwrite(cnt.c_str(), cnt.size(), 1, fp);
                printf("%s File received successfully.\n", packet.f_name.c_str());
            }
            else{
                printf("Cannot create to output file.\n");
            }
            fclose(fp);
        }
    }

	close(s);
}
