#include <bits/stdc++.h>
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
#include <cstring>
#include <regex>
using namespace std;

vector <string> domain_names;
vector <string> zone_file_paths;
uint8_t recvBuf[100000];

struct domain_info{
	string host_label;
	int ttl;
	string record_class; // IN
	string record_type;  // NS, MX, A, ...
	string record_data;
};

struct HEADER{
	uint16_t ID;
	uint QR:1;
	uint Opcode:4;
	uint AA:4;
	uint TC:1;
	uint RD:1;
	uint RA:1;
	uint Z:3;
	uint RCODE:4;
	uint16_t QDCOUNT;
	uint16_t ANCOUNT;
	uint16_t NSCOUNT;
	uint16_t ARCOUNT;
};

map <string, vector<domain_info>> Domain_info;
map <int, map<string, pair<string, int>>> type_list; // type_list[Qtype] = map<input string, pair<domain name, idx in domain name>>
vector <string> SOA;
unordered_map <int, string> RR_value_to_type;
unordered_map <string, int> RR_type_to_value;

void init_domain(domain_info* tmp){
	tmp -> host_label = "";
	tmp -> ttl = 0;
	tmp -> record_class = "";
	tmp -> record_type = "";
	tmp -> record_data = "";
}

bool isNum(char c){
	if(c - '0' >= 0 && c - '0' <= 9) return true;
	return false;
}

void setRR(){
	RR_type_to_value["A"] = 1;
	RR_type_to_value["NS"] = 2;
	RR_type_to_value["CNAME"] = 5;
	RR_type_to_value["SOA"] = 6;
	RR_type_to_value["MX"] = 15;
	RR_type_to_value["TXT"] = 16;
	RR_type_to_value["AAAA"] = 28;

	RR_value_to_type[1] = "A";
	RR_value_to_type[2] = "NS";
	RR_value_to_type[5] = "CNAME";
	RR_value_to_type[6] = "SOA";
	RR_value_to_type[15] = "MX";
	RR_value_to_type[16] = "TXT";
	RR_value_to_type[28] = "AAAA";
}

vector <string> Decode_Qname(string Qname){
	vector <string> encoding_Qname;
	string temp = "";
	for(int j = 0; j < Qname.size(); j++){
		if(Qname[j] == '.'){
			encoding_Qname.push_back(temp);
			temp = "";
		}else{
			temp.push_back(Qname[j]);
		}
	}
	return encoding_Qname;
}

vector <int> handle_IP(string s){ // 140.113.123.1 -> 8c 71 82 01
	vector <int> ans;
	ans.resize(4);
	for(int i = 0; i < s.size(); i++) if(s[i] == '.') s[i] = ' ';
	stringstream ss;
	ss << s;
	ss >> ans[0] >> ans[1] >> ans[2] >> ans[3];
	return ans;
}

vector <uint8_t> Split_IPv6(string s){
	int cnt = 0, pos = 0;
	for(int i = 0; i < s.size(); i++){
		if(s[i] == ':') cnt++;
		if(s[i] == ':' && s[i + 1] == ':' && i < s.size() - 1) pos = i;
	}
	while (cnt < 7){
		s.insert(pos, ":");
		cnt++;
	}
	cout << s << endl;
	string tmp = "";
	vector <string> first_split;
	for(int i = s.size() - 1; i >= 0; i--){
		if(s[i] == ':'){
			while(tmp.size() < 4) tmp.push_back('0');
			reverse(tmp.begin(), tmp.end());
			first_split.emplace_back(tmp);
			tmp = "";
		}else{
			tmp.push_back(s[i]);
		}
	}
	reverse(tmp.begin(), tmp.end());
	first_split.emplace_back(tmp);
	tmp = "";
	reverse(first_split.begin(), first_split.end());
	vector <uint8_t> ans;
	for(auto cur : first_split){
		string s1 = "";
		s1.push_back(cur[0]);
		s1.push_back(cur[1]);
		string s2 = "";
		s1.push_back(cur[2]);
		s2.push_back(cur[3]);
		ans.push_back(uint8_t(stoi(s1, 0, 16)));
		ans.push_back(uint8_t(stoi(s2, 0, 16)));
	}
	return ans;
}

string int_to_hex_string_TTL(int ttl){
	vector <char> ans ;
	for(int i = 0; i < 8; i++){
		ans.push_back((char)(ttl % 16));
		ttl /= 16;
	}
	reverse(ans.begin(), ans.end());
	string tmp = "";
	for(auto i : ans) tmp.push_back(i);
	return tmp;
}

string int_to_hex_string_DATA_LEN(int len){ // cout << "in function\n";
	vector <char> ans;
	for(int i = 0; i < 4; i++){
		ans.push_back((char)(len % 16)); 
		len /= 16;
	}
	reverse(ans.begin(), ans.end());
	string tmp = "";
	for(auto i : ans) tmp.push_back(i);
	return tmp;
}

int main(int argc, char **argv){
	int					sockfd;
	struct sockaddr_in	servaddr, cliaddr;

	setRR();

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("Socket Error\n");
		exit(0);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
		perror("Bind Error\n");
		exit(0);
	}

	ifstream fin;
    fin.open(argv[2]);
    string forwardIP = "";
    getline(fin, forwardIP);
	string s = "";
	while (getline(fin, s)){
		for(int i = 0; i < s.size(); i++) if(s[i] == ',') s[i] = ' ';
		string domain_name, zone_file_;
		stringstream ss;
		ss << s;
		ss >> domain_name >> zone_file_;
		domain_names.emplace_back(domain_name);
		zone_file_paths.emplace_back(zone_file_);
	}
	fin.close();
	cout << "Reading Zone files\n";
    for(int i = 0; i < domain_names.size(); i++){
		fin.open(zone_file_paths[i]);
		string s = "";
		getline(fin, s); // domain name
		cout << "domain name " << s << endl;
		// getline(fin, s); // SOA
		// cout << "SOA = " << s << endl; 
		// SOA.emplace_back(s);
		domain_info tmp;
		init_domain(&tmp);
		while (getline(fin, s)){
			init_domain(&tmp);
			int j = 0;
			for(; j < s.size() && s[j] != ','; j++) tmp.host_label.push_back(s[j]);
			j++;
			for(; j < s.size() && s[j] != ','; j++) tmp.ttl = tmp.ttl * 10 + (s[j] - '0');
			j++;
			for(; j < s.size() && s[j] != ','; j++) tmp.record_class.push_back(s[j]);
			j++;
			for(; j < s.size() && s[j] != ','; j++) tmp.record_type.push_back(s[j]);
			j++;
			for(; j < s.size(); j++) tmp.record_data.push_back(s[j]);

			if(tmp.host_label == "@"){
				type_list[RR_type_to_value[tmp.record_type]][domain_names[i]] = make_pair(domain_names[i], Domain_info[domain_names[i]].size());
			}else{
				type_list[RR_type_to_value[tmp.record_type]][tmp.host_label + "." + domain_names[i]] = make_pair(domain_names[i], Domain_info[domain_names[i]].size());
			}
			Domain_info[domain_names[i]].push_back(tmp);
		}
		fin.close();
	}

	socklen_t cliaddr_len = sizeof(cliaddr);

	cout << "listing type list\n";
    for(auto i : type_list){
        cout << "Record Type : "<< i.first << endl;
        for(auto j : i.second){
            cout << " "<< j.first << ", " << j.second.first <<", " << j.second.second<< endl;
        }
    }

	while (true){
		memset(recvBuf, 0, sizeof(recvBuf));
		int n = recvfrom(sockfd, recvBuf, 100000, 0, (struct sockaddr*)&cliaddr, &cliaddr_len);
		char buf[10000];
		bzero(buf, sizeof(buf));
		memcpy(buf, recvBuf, n);
		for(int i = 0; i < n; i++) printf("recvBuf[%d] = %x, ", i, recvBuf[i]), cout << (char)recvBuf[i];
		int i;
		string Qname = "";
		for(i = 12; i < n && recvBuf[i] != 0; i++){
			int len = (int)recvBuf[i];
			string s = "";
			for(int j = i + 1; j <= i + len; j++){
				s.push_back(recvBuf[j]);
			}
			Qname += s + ".";
			i += len;
		} cout << "end address, i = " << i << endl;
		i++;
		
		cout << "name " << Qname << endl;
		int Qtype = 0;
		Qtype = 10*Qtype + (int)recvBuf[i++];
		Qtype = 10*Qtype + (int)recvBuf[i++];
		i += 2; // bypass the Q_CLASS
		cout << "Qtype = " << Qtype << endl;
		cout << "i = " << i << endl;
		//bool found = false;
		/*******************/
		//if(regex_match(Qname, /^([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})\.([0-9a-zA-Z]{1,61}\.)*{YOUR DOMAIN}$/))
		string found_domain = "";
		string matching_str;
		int pos;
		for(auto d:domain_names){
			if((pos = Qname.find(d)) != std::string::npos){
				found_domain = d;
				matching_str = Qname.substr(0, pos);
				break;
			}
		}
		cout << "matching str = " <<matching_str << endl;
		std::regex regex_expr("^([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})\\.([0-9a-zA-Z]{1,61}\\.)$");
		if(regex_match(matching_str, regex_expr)){
			cout << "matched regex\n";
			recvBuf[7] = uint8_t((int)recvBuf[7] + 1); // ans += 1
			int cur_pos = i;
			for(int j = 12; j < cur_pos - 4; j++){
				recvBuf[i++] = recvBuf[j];
			}
			// A
			recvBuf[i++] = uint8_t(0);
			recvBuf[i++] = uint8_t(1);
			// IN
			recvBuf[i++] = uint8_t(0);
			recvBuf[i++] = uint8_t(1);
			// TTL = 1
			recvBuf[i++] = uint8_t(0);
			recvBuf[i++] = uint8_t(0);
			recvBuf[i++] = uint8_t(0);
			recvBuf[i++] = uint8_t(1);
			// data len = 4
			recvBuf[i++] = uint8_t(0);
			recvBuf[i++] = uint8_t(4);
			// IP address
			string raw_IP = "";
			int dot_cnt = 0;
			for(int j = 0; j < Qname.size(); j++){
				if(Qname[j] == '.') dot_cnt++;
				if(dot_cnt == 4) break;
				raw_IP.push_back(Qname[j]);
			}
			vector <int> IP = handle_IP(raw_IP);
			for(auto ip : IP){
				recvBuf[i++] = uint8_t(ip);
			}
			// AUTHORITY
			for(auto serv : Domain_info[found_domain]){
				if(serv.record_type != "NS") continue;
				recvBuf[9] = uint8_t((int)recvBuf[9] + 1);
				vector <string> encoding_domain = Decode_Qname(found_domain);
				for(auto frag : encoding_domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = 0;
				// NS
				recvBuf[i++] = 0;
				recvBuf[i++] = 2;
				// IN
				recvBuf[i++] = 0;
				recvBuf[i++] = 1;
				// TTL
				string ttl = int_to_hex_string_TTL(serv.ttl);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)ttl[j];
					num = 16 * num + (int)ttl[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				// data len
				string data_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
				for(int j = 0; j < 4; j += 2){
					int num = 0;
					num = 16 * num + (int)data_len[j];
					num = 16 * num + (int)data_len[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				vector <string> encoding_data = Decode_Qname(serv.record_data);
				for(auto frag : encoding_data){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
			}
			cout << "sent msg " << i << endl;
			for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (char)recvBuf[j] << "\n";
			cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			continue;
		}
		if(type_list[Qtype].find(Qname) != type_list[Qtype].end()){ // is within the domain
			string domain = type_list[Qtype][Qname].first;
			int idx = type_list[Qtype][Qname].second;
			struct domain_info cur = Domain_info[domain][idx];
			
			vector <string> encoding_Qname = Decode_Qname(Qname);
			string ttl;
			vector <int> IP;
			string resp_len;
			vector <pair<int, string>> mail_servers;
			
			if(Qtype == 1){ // A ANS + Authority
				// address
				for(auto frag : encoding_Qname){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = 0;
				// ANS SECTION
				// response type
				recvBuf[7] = uint8_t((int)recvBuf[7] + 1); // ans cnt += 1
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				// response class, IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				// TTL
				ttl = int_to_hex_string_TTL(cur.ttl);
				for(int j = 0; j < 8; j++) cout << (int)ttl[j] ; cout << endl;
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)ttl[j];
					num = 16 * num + (int)ttl[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				// response len
				recvBuf[i++] = (uint8_t)0;
				recvBuf[i++] = (uint8_t)4;
				// address
				IP = handle_IP(cur.record_data);
				for(auto j : IP) recvBuf[i++] = uint8_t(j);
				// AUTHORITY SECTION
				// Domain Name
				vector <string> encoding_domain = Decode_Qname(domain);
				for(auto serv:Domain_info[domain]){
					if(serv.record_type == "NS"){
						recvBuf[9] = uint8_t((int)recvBuf[9] + 1); // authority cnt += 1

						for(auto frag : encoding_domain){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						// response type NS
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(2);
						// response class, IN
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(1);
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						string data_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
						for(int j = 0; j < 4; j += 2){
							int num = 0;
							num = 16 * num + (int)data_len[j];
							num = 16 * num + (int)data_len[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						vector <string> encoding_domain = Decode_Qname(serv.record_data);
						for(auto frag : encoding_domain){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
					}
				}
				// Domain Name
				// vector <string> encoding_domain = Decode_Qname(domain);
				// for(auto frag : encoding_domain){
				// 	recvBuf[i++] = (uint8_t)(frag.size());
				// 	for(int j = 0; j < frag.size(); j++){
				// 		recvBuf[i++] = frag[j];
				// 	}
				// }
				
				// recvBuf[i++] = uint8_t(0);
				// // response type NS
				// recvBuf[i++] = uint8_t(0);
				// recvBuf[i++] = uint8_t(2);
				// // response class, IN
				// recvBuf[i++] = uint8_t(0);
				// recvBuf[i++] = uint8_t(1);
				// // TTL
				// for(auto serv : Domain_info[domain]){
				// 	if(serv.record_type == "NS" && cur.host_label == serv.host_label){
				// 		ttl = int_to_hex_string_TTL(serv.ttl);
				// 		for(int j = 0; j < 8; j += 2){
				// 			int num = 0;
				// 			num = 16 * num + (int)ttl[j];
				// 			num = 16 * num + (int)ttl[j + 1];
				// 			recvBuf[i++] = (uint8_t)num;
				// 		}
				// 		break;
				// 	}
				// }
				
				// // Data len, id est, the number of bytes needed to transfer the query name
				// string data_len = int_to_hex_string_DATA_LEN(1 + Qname.size());
				// for(int j = 0; j < 4; j += 2){
				// 	int num = 0;
				// 	num = 16 * num + (int)data_len[j];
				// 	num = 16 * num + (int)data_len[j + 1];
				// 	recvBuf[i++] = (uint8_t)num;
				// }
				// // Domain Name Server
				// for(auto data : Domain_info[domain]){
				// 	if(data.record_type == "NS"){
				// 		vector <string> encoding_domain = Decode_Qname(data.record_data);
				// 		for(auto frag : encoding_domain){
				// 			recvBuf[i++] = (uint8_t)(frag.size());
				// 			for(int j = 0; j < frag.size(); j++){
				// 				recvBuf[i++] = frag[j];
				// 			}
				// 		}
				// 		break;
				// 	}
				// }
				// recvBuf[i++] = uint8_t(0);
				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (char)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}
			else if(Qtype == 2){ // NS
				// ANS 
				vector <pair<string, string>> sub_domains;
				for(auto serv : Domain_info[domain]){
					if(serv.record_type == "NS" && serv.host_label == cur.host_label){
						recvBuf[7] = (uint8_t(int(recvBuf[7])+1));

						encoding_Qname.clear();
						encoding_Qname = Decode_Qname(domain);
						for(auto frag : encoding_Qname){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						// NS
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(2);
						// IN
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(1);
						// TTL 
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						// Response len
						string response_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
						for(int j = 0; j < 4; j += 2){
							int num = 0;
							num = 16 * num + (int)response_len[j];
							num = 16 * num + (int)response_len[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						// Response Data
						vector <string> encoding_domain = Decode_Qname(serv.record_data);
						for(auto frag : encoding_domain){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						string sub = "";
						for(int k = 0; k < serv.record_data.size() && serv.record_data[k] != '.'; k++) sub.push_back(serv.record_data[k]);
						sub_domains.emplace_back(sub, serv.record_data);
					}
				}
				for(auto label : sub_domains){
					for(auto serv:Domain_info[domain]){
						if(label.first == serv.host_label && serv.record_type == "A"){
							recvBuf[11] = (uint8_t(int(recvBuf[11])+1));
							vector <string> encoding_Domain = Decode_Qname(label.second);
							for(auto frag : encoding_Domain){
								recvBuf[i++] = (uint8_t)(frag.size());
								for(int j = 0; j < frag.size(); j++){
									recvBuf[i++] = frag[j];
								}
							}
							recvBuf[i++] = uint8_t(0);
							// A
							recvBuf[i++] = uint8_t(0);
							recvBuf[i++] = uint8_t(1);
							// IN
							recvBuf[i++] = uint8_t(0);
							recvBuf[i++] = uint8_t(1);
							ttl = int_to_hex_string_TTL(serv.ttl);
							for(int j = 0; j < 8; j += 2){
								int num = 0;
								num = 16 * num + (int)ttl[j];
								num = 16 * num + (int)ttl[j + 1];
								recvBuf[i++] = (uint8_t)num;
							}
							// IP
							cout << "IP len loc = " << i << endl;
							recvBuf[i++] = uint8_t(0);
							recvBuf[i++] = uint8_t(4);
							IP = handle_IP(serv.record_data);
							for(auto j : IP) recvBuf[i++] = uint8_t(j);
							break;
						}
					}
				}
				// ANS Query
				// for(auto frag : encoding_Qname){
				// 	recvBuf[i++] = (uint8_t)(frag.size());
				// 	for(int j = 0; j < frag.size(); j++){
				// 		recvBuf[i++] = frag[j];
				// 	}
				// }
				// recvBuf[i++] = uint8_t(0);
				// // NS
				// recvBuf[i++] = uint8_t(0);
				// recvBuf[i++] = uint8_t(2);
				// // IN
				// recvBuf[i++] = uint8_t(0);
				// recvBuf[i++] = uint8_t(1);
				// // TTL
				// ttl = int_to_hex_string_TTL(cur.ttl);
				// // for(int j = 0; j < 8; j++) cout << (int)ttl[j] ; cout << endl;
				// for(int j = 0; j < 8; j += 2){
				// 	int num = 0;
				// 	num = 16 * num + (int)ttl[j];
				// 	num = 16 * num + (int)ttl[j + 1];
				// 	recvBuf[i++] = (uint8_t)num;
				// }
				// // Response len
				// string response_len = int_to_hex_string_DATA_LEN(cur.record_data.size());
				// for(int j = 0; j < 4; j += 2){
				// 	int num = 0;
				// 	num = 16 * num + (int)response_len[j];
				// 	num = 16 * num + (int)response_len[j + 1];
				// 	recvBuf[i++] = (uint8_t)num;
				// }
				// // response data
				// for(auto data : Domain_info[domain]){
				// 	if(data.record_type == "NS"){
				// 		recvBuf[7] = (uint8_t(int(recvBuf[7])+1));
				// 		vector <string> encoding_domain = Decode_Qname(data.record_data);
				// 		for(auto frag : encoding_domain){
				// 			recvBuf[i++] = (uint8_t)(frag.size());
				// 			for(int j = 0; j < frag.size(); j++){
				// 				recvBuf[i++] = frag[j];
				// 			}
				// 		}
				// 		recvBuf[i++] = uint8_t(0);
				// 	}
				// }
				// cout << "correct IP = " << Domain_info[domain][idx].record_data << endl;
				// string sub_domain = "";
				// for(auto j : Domain_info[domain][idx].record_data){
				// 	if(j == '.') break;
				// 	sub_domain.push_back(j);
				// }
				// cout << "sub domain = " << sub_domain << endl;
				// for(auto data : Domain_info[domain]){
				// 	cout << "cur type " << data.record_type << ", label = " << data.host_label << " : " << data.record_data << endl;
				// 	if(data.record_type == "A" && data.host_label == sub_domain){
				// 		cout << "found IP " << data.record_data << endl;
				// 		vector <string> encoding_domain = Decode_Qname(Domain_info[domain][idx].record_data);
				// 		for(auto frag : encoding_domain){
				// 			recvBuf[i++] = (uint8_t)(frag.size());
				// 			for(int j = 0; j < frag.size(); j++){
				// 				recvBuf[i++] = frag[j];
				// 			}
				// 		}
				// 		recvBuf[i++] = uint8_t(0);
				// 		// A
				// 		recvBuf[i++] = uint8_t(0);
				// 		recvBuf[i++] = uint8_t(1);
				// 		// IN
				// 		recvBuf[i++] = uint8_t(0);
				// 		recvBuf[i++] = uint8_t(1);
				// 		// TTL
				// 		ttl = int_to_hex_string_TTL(data.ttl);
				// 		for(int j = 0; j < 8; j += 2){
				// 			int num = 0;
				// 			num = 16 * num + (int)ttl[j];
				// 			num = 16 * num + (int)ttl[j + 1];
				// 			recvBuf[i++] = (uint8_t)num;
				// 		}
				// 		// IP
				// 		cout << "IP len loc = " << i << endl;
				// 		recvBuf[i++] = uint8_t(0);
				// 		recvBuf[i++] = uint8_t(4);
				// 		IP = handle_IP(data.record_data);
				// 		for(auto j : IP) recvBuf[i++] = uint8_t(j);
				// 		break;
				// 	}
				// }
				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (char)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}
			else if(Qtype == 5){ // CNAME
				recvBuf[7] = uint8_t(int(recvBuf[7]) + 1);
				recvBuf[9] = uint8_t(int(recvBuf[9]) + 1);
				// ANS
				vector <string> encoding_Domain = Decode_Qname(Qname);
				for(auto frag : encoding_Domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// CNAME
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(5);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				// TTL
				ttl = int_to_hex_string_TTL(cur.ttl);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)ttl[j];
					num = 16 * num + (int)ttl[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				// Response Len
				string resp_len = int_to_hex_string_DATA_LEN(cur.record_data.size() + 1);
				for(int j = 0; j < 4; j += 2){
					int num = 0;
					num = 16 * num + (int)resp_len[j];
					num = 16 * num + (int)resp_len[j + 1];
					recvBuf[i++] = num;
				}
				vector <string> encoding_cname = Decode_Qname(cur.record_data);
				for(auto frag : encoding_cname){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// AUTHORITY
				for(auto serv : Domain_info[domain]){
					if(serv.record_type == "NS"){
						encoding_Domain.clear();
						encoding_Domain = Decode_Qname(domain);
						for(auto frag : encoding_Domain){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						// NS
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(2);
						// IN
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(1);
						//TTL
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						string data_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
						for(int j = 0; j < 4; j += 2){
							int num = 0;
							num = 16 * num + (int)data_len[j];
							num = 16 * num + (int)data_len[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						vector <string> encoding_server = Decode_Qname(serv.record_data);
						for(auto frag : encoding_server){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						break;
					}
				}
				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (char)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}
			else if(Qtype == 6){ // SOA
				recvBuf[7] = uint8_t(int(recvBuf[7]) + 1);
				recvBuf[9] = uint8_t(int(recvBuf[9]) + 1);
				vector <string> encoding_Domain = Decode_Qname(domain);
				for(auto frag : encoding_Domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = (uint8_t)frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// SOA
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(6);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				// TTL
				ttl = int_to_hex_string_TTL(cur.ttl);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)ttl[j];
					num = 16 * num + (int)ttl[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				stringstream ss ;
				ss << cur.record_data;
				string Domain_Server, admin_mailbox;
				int serial, refresh, retry, expire, minimum;
				ss >> Domain_Server >> admin_mailbox >> serial >> refresh >> retry >> expire >> minimum;
				// SOA len
				string resp_len = int_to_hex_string_DATA_LEN(Domain_Server.size()+1 + admin_mailbox.size()+1 + 20);
				for(int j = 0; j < 4; j += 2){
					int num = 0;
					num = 16 * num + (int)resp_len[j];
					num = 16 * num + (int)resp_len[j + 1];
					recvBuf[i++] = num;
				}
				// SOA content
				vector <string> info = Decode_Qname(Domain_Server);
				for(auto frag : info){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				info.clear();
				info = Decode_Qname(admin_mailbox);
				for(auto frag : info){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);

				string tmp = int_to_hex_string_TTL(serial);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)tmp[j];
					num = 16 * num + (int)tmp[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				tmp = int_to_hex_string_TTL(refresh);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)tmp[j];
					num = 16 * num + (int)tmp[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				tmp = int_to_hex_string_TTL(retry);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)tmp[j];
					num = 16 * num + (int)tmp[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				tmp = int_to_hex_string_TTL(expire);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)tmp[j];
					num = 16 * num + (int)tmp[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				tmp = int_to_hex_string_TTL(minimum);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)tmp[j];
					num = 16 * num + (int)tmp[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				// AUTHORITY
				info.clear();
				for(auto frag : encoding_Domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// NS
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(2);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				for(auto serv : Domain_info[domain]){
					if(serv.record_type == "NS"){
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						string data_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
						for(int j = 0; j < 4; j += 2){
							int num = 0;
							num = 16 * num + (int)data_len[j];
							num = 16 * num + (int)data_len[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						info.clear();
						info = Decode_Qname(serv.record_data);
						for(auto frag : info){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						break;
					}
				}
				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (uint8_t)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}
			else if(Qtype == 0xf){ // MX
				recvBuf[7] = uint8_t((int)recvBuf[7] + 1); // ans cnt += 1
				recvBuf[9] = uint8_t((int)recvBuf[9] + 1); // authority cnt += 1
				for(auto frag : encoding_Qname){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// MX
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(15);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				// TTL
				ttl = int_to_hex_string_TTL(cur.ttl);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)ttl[j];
					num = 16 * num + (int)ttl[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				
				for(auto i : Domain_info[domain]){
					if(i.record_type == "MX"){
						stringstream ss;
						ss << i.record_data;
						int prefer;
						string serv_name;
						ss >> prefer >> serv_name;
						mail_servers.emplace_back(prefer, serv_name);
					}
				}
				sort(mail_servers.begin(), mail_servers.end());

				for(auto j:mail_servers) cout << j.first << ", " << j.second << endl;
				cout << mail_servers[0].second.size() << endl;

				// Response len 
				resp_len = int_to_hex_string_DATA_LEN(mail_servers[0].second.size() + 3);
				for(int j = 0; j < 4; j += 2){
					int num = 0;
					num = 16 * num + (int)resp_len[j];
					num = 16 * num + (int)resp_len[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				// preference number
				recvBuf[i++] = mail_servers[0].first >> 8;
				recvBuf[i++] = mail_servers[0].first & 0xFF;
				// string preference = int_to_hex_string_DATA_LEN(mail_servers[0].first);
				// for(int j = 0; j < 4; j += 2){
				// 	int num = 0;
				// 	num = 16 * num + (int)preference[j];
				// 	num = 16 * num + (int)preference[j + 1];
				// 	recvBuf[i++] = (uint8_t)num;
				// }
				vector <string> encoding_ans = Decode_Qname(mail_servers[0].second);
				for(auto frag : encoding_ans){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// AUTHORITY, the Name Server of the Domain
				vector <string> encoding_Domain = Decode_Qname(domain);
				for(auto frag : encoding_Domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// NS
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(2);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				for(auto serv : Domain_info[domain]){
					if(serv.record_type == "NS"){
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						string data_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
						for(int j = 0; j < 4; j+=2){
							int num = 0;
							num = 16 * num + (int)data_len[j];
							num = 16 * num + (int)data_len[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						vector <string> encoding_ns = Decode_Qname(serv.record_data);
						for(auto frag : encoding_ns){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						break;
					}
				}
				string sub_domain = "";
				for(int j = 0; j < mail_servers[0].second.size() && mail_servers[0].second[j] != '.'; j++) sub_domain.push_back(mail_servers[0].second[j]);
				cout << "sub domain = " << sub_domain << endl;
				for(auto serv:Domain_info[domain]){
					if(serv.record_type == "A" && serv.host_label == sub_domain){
						vector <string> encoding_serv = Decode_Qname(mail_servers[0].second);
						for(auto frag : encoding_serv){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						// A
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(1);
						// IN
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(1);
						// TTL
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(4);
						IP = handle_IP(serv.record_data);
						for(auto j : IP) recvBuf[i++] = uint8_t(j);
						break;
					}
				}
				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %c ,", j, recvBuf[j]), cout << hex << (int)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}
			else if(Qtype == 16){ // TXT
				recvBuf[7] = uint8_t(int(recvBuf[7]) + 1);
				recvBuf[9] = uint8_t(int(recvBuf[9]) + 1);
				// ANS
				vector <string> encoding_Domain = Decode_Qname(Qname);
				for(auto frag : encoding_Domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// TXT
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(16);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				// TTL
				ttl = int_to_hex_string_TTL(cur.ttl);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)ttl[j];
					num = 16 * num + (int)ttl[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				// Response Len
				string resp_len = int_to_hex_string_DATA_LEN(cur.record_data.size() + 1);
				for(int j = 0; j < 4; j += 2){
					int num = 0;
					num = 16 * num + (int)resp_len[j];
					num = 16 * num + (int)resp_len[j + 1];
					recvBuf[i++] = num;
				}
				recvBuf[i++] = (uint8_t)cur.record_data.size();
				for(int j = 0; j < cur.record_data.size(); j++){
				 		recvBuf[i++] = cur.record_data[j];
				}
				recvBuf[i++] = uint8_t(0);
				// AUTHORITY
				for(auto serv : Domain_info[domain]){
					if(serv.record_type == "NS" && cur.host_label == serv.host_label){
						encoding_Domain.clear();
						encoding_Domain = Decode_Qname(domain);
						for(auto frag : encoding_Domain){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						// NS
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(2);
						// IN
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(1);
						//TTL
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						string data_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
						for(int j = 0; j < 4; j += 2){
							int num = 0;
							num = 16 * num + (int)data_len[j];
							num = 16 * num + (int)data_len[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						vector <string> encoding_server = Decode_Qname(serv.record_data);
						for(auto frag : encoding_server){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
					}
				}
				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (char)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}else if(Qtype == 28){ // AAAA
				// ANS
				recvBuf[7] = uint8_t((int)recvBuf[7] + 1);
				vector <string> encoding_Domain = Decode_Qname(Qname);
				for(auto frag : encoding_Domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// AAAA
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(28);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				// TTL
				string ttl = int_to_hex_string_TTL(cur.ttl);
				for(int j = 0; j < 8; j += 2){
					int num = 0;
					num = 16 * num + (int)ttl[j];
					num = 16 * num + (int)ttl[j + 1];
					recvBuf[i++] = (uint8_t)num;
				}
				// Data len
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(16);
				vector <uint8_t> IP = Split_IPv6(cur.record_data);
				for(auto ip : IP){
					recvBuf[i++] = ip;
				}

				// AUTHORITY
				for(auto serv : Domain_info[domain]){
					if(serv.record_type == "NS"){
						recvBuf[9] = uint8_t((int)recvBuf[9] + 1);
						encoding_Domain.clear();
						encoding_Domain = Decode_Qname(domain);
						for(auto frag : encoding_Domain){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						// NS
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(2);
						// IN
						recvBuf[i++] = uint8_t(0);
						recvBuf[i++] = uint8_t(1);
						//TTL
						ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						string data_len = int_to_hex_string_DATA_LEN(serv.record_data.size());
						for(int j = 0; j < 4; j += 2){
							int num = 0;
							num = 16 * num + (int)data_len[j];
							num = 16 * num + (int)data_len[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						vector <string> encoding_server = Decode_Qname(serv.record_data);
						for(auto frag : encoding_server){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						//break;
					}
				}

				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (char)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}
			
		}else{
			bool found_domain = false;
			string domain;
			for(auto d : domain_names){
				if(Qname.find(d) != std::string::npos){
					found_domain = true;
					domain = d;
					break;
				}
			}
			if(found_domain){ // return SOA
				recvBuf[9] = uint8_t((int)recvBuf[9] + 1);
				vector <string> encoding_Domain = Decode_Qname(domain);
				for(auto frag : encoding_Domain){
					recvBuf[i++] = (uint8_t)(frag.size());
					for(int j = 0; j < frag.size(); j++){
						recvBuf[i++] = frag[j];
					}
				}
				recvBuf[i++] = uint8_t(0);
				// SOA
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(6);
				// IN
				recvBuf[i++] = uint8_t(0);
				recvBuf[i++] = uint8_t(1);
				for(auto serv : Domain_info[domain]){
					if(serv.record_type == "SOA"){
						string ttl = int_to_hex_string_TTL(serv.ttl);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)ttl[j];
							num = 16 * num + (int)ttl[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						stringstream ss ;
						ss << serv.record_data;
						string Domain_Server, admin_mailbox;
						int serial, refresh, retry, expire, minimum;
						ss >> Domain_Server >> admin_mailbox >> serial >> refresh >> retry >> expire >> minimum;
						// SOA len
						string resp_len = int_to_hex_string_DATA_LEN(Domain_Server.size()+1 + admin_mailbox.size()+1 + 20);
						for(int j = 0; j < 4; j += 2){
							int num = 0;
							num = 16 * num + (int)resp_len[j];
							num = 16 * num + (int)resp_len[j + 1];
							recvBuf[i++] = num;
						}
						// SOA content
						vector <string> info = Decode_Qname(Domain_Server);
						for(auto frag : info){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);
						info.clear();
						info = Decode_Qname(admin_mailbox);
						for(auto frag : info){
							recvBuf[i++] = (uint8_t)(frag.size());
							for(int j = 0; j < frag.size(); j++){
								recvBuf[i++] = frag[j];
							}
						}
						recvBuf[i++] = uint8_t(0);

						string tmp = int_to_hex_string_TTL(serial);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)tmp[j];
							num = 16 * num + (int)tmp[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						tmp = int_to_hex_string_TTL(refresh);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)tmp[j];
							num = 16 * num + (int)tmp[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						tmp = int_to_hex_string_TTL(retry);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)tmp[j];
							num = 16 * num + (int)tmp[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						tmp = int_to_hex_string_TTL(expire);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)tmp[j];
							num = 16 * num + (int)tmp[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						tmp = int_to_hex_string_TTL(minimum);
						for(int j = 0; j < 8; j += 2){
							int num = 0;
							num = 16 * num + (int)tmp[j];
							num = 16 * num + (int)tmp[j + 1];
							recvBuf[i++] = (uint8_t)num;
						}
						break;
					}
				}
				cout << "sent msg " << i << endl;
				for(int j = 0; j < i; j++) printf("sendBuf[%d] = %x ,", j, recvBuf[j]), cout << (char)recvBuf[j] << "\n";
				cout << "bytes sent " << sendto(sockfd, recvBuf, i, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}else{ // ask forward IP
				// Send the data to forward IP
				cout << "Forward IP " << forwardIP << endl;

				struct sockaddr_in forward_addr;
				bzero(&forward_addr, sizeof(forward_addr));
				forward_addr.sin_family      = AF_INET;
				cout << "inet pton " << inet_pton(AF_INET, forwardIP.c_str(), &forward_addr.sin_addr) << endl;
				forward_addr.sin_port        = htons(53);

				int forwardFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if(forwardFD < 0){
					perror("Socket Error\n");
					exit(0);
				}else cout << "Forward fd " << forwardFD << endl;
				
				cout << "Forwarding\n";
				int slen = 0;
				if((slen = sendto(forwardFD, recvBuf, n, 0, (struct sockaddr*)&forward_addr, sizeof(forward_addr))) < 0){
					perror("Send Error\n");
					exit(0);
				}else cout << "slen = " << slen << endl;
				socklen_t forward_addr_len = sizeof(forward_addr);
				bzero(recvBuf, sizeof(recvBuf));
				int rlen = recvfrom(forwardFD, recvBuf, 100000, 0, (struct sockaddr*)&forward_addr, &forward_addr_len);
				cout << "rlen = " << rlen << endl;
				for(int i = 0; i < rlen; i++) {printf("%02x ", recvBuf[i]) ; if((i%16 == 0) && i) cout << endl;}
				cout << "to client "<< sendto(sockfd, recvBuf, rlen, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) << endl;
			}
		}
	}
	
	
    
    exit(0);
}
