#include <bits/stdc++.h>
#include <fcntl.h>
#include <unistd.h>
#define MAX 1000000000
using namespace std;
using ll = long long;
struct FILE_E
{
    uint32_t name_offset;
    uint32_t file_size;
    uint32_t content_offset;
    uint64_t checksum;
};
vector<FILE_E> files;
int fd;
off_t name_off, content_off;
uint64_t total_size = 0, num;
char content[MAX];
char cur_content[MAX];
bool CHECK_SUM(int i)
{
    vector<uint64_t> data;
    lseek(fd, content_off + files[i].content_offset, SEEK_SET);
    int n = files[i].file_size / 8;
    //cout << n << " " << files[i].file_size % 8 << endl;
    for (int i = 0; i < n; i++)
    {
        uint64_t cur;
        read(fd, &cur, 8);
        data.emplace_back(cur);
    }
    //cout << data.size() << endl;
    if (files[i].file_size % 8)
    {
        uint64_t cur = 0;
        uint64_t r = files[i].file_size % 8;
        read(fd, &cur, r);
        data.emplace_back(cur);
    }
    uint64_t ans = data[0];
    for (int i = 1; i < data.size(); i++)
        ans ^= data[i];

//    cout << "ans = " << ans << endl;
//    cout << "Ori = " << files[i].checksum << endl;

    if (ans == files[i].checksum)
        return true;
    return false;
}

int main(int argc, char *argv[])
{
    char buf[128];
    memset(buf, '\0', sizeof(buf));
    // printf("%s\n", argv[1]);
    fd = open(argv[1], O_RDONLY);
    // cout << fd << '\n';
    read(fd, buf, 4); // Read P A K O
    read(fd, &name_off, 4);
    read(fd, &content_off, 4);
    read(fd, &num, 4);
    files.resize(num);
    cout << "number of files = " << num << endl;
    
    for (int i = 0; i < num; i++)
    {
        read(fd, &files[i].name_offset, 4);
        //read(fd, buf, 4);
        /*
        for (int j = 0; j < 4; j++)
            files[i].file_size = files[i].file_size * 10 + (int)buf[j];
            */
        read(fd, &files[i].file_size, 4);
        files[i].file_size = __builtin_bswap32(files[i].file_size);
        total_size += files[i].file_size;
        read(fd, &files[i].content_offset, 4);
        read(fd, &files[i].checksum, 8);
        files[i].checksum = __builtin_bswap64(files[i].checksum);
    }

    for (auto i : files)
        cout << hex << i.name_offset << " " << i.file_size << " " << i.content_offset << " " << i.checksum << endl;
    uint64_t tail = lseek(fd, 0, SEEK_END);    
    lseek(fd, content_off, SEEK_SET);
    cout << "total size = " << tail - content_off << " " << content_off << " " << tail << endl; 
    //char *content = (char *)malloc(total_size);
    read(fd, content, tail - content_off);
    uint64_t cur_pos = 0; // position in content
    int cnt = 0;
    for (int i = 0; i < num; i++)
    {
        lseek(fd, name_off + files[i].name_offset, SEEK_SET);
        string file_name = "";
        while (true)
        {
            char tmp;
            read(fd, &tmp, 1);
            if ((int)tmp == 0)
                break;
            file_name.push_back(tmp);
        }
        cout << file_name << '\n';
        memset(cur_content, '\0', sizeof(cur_content));
        cur_pos = files[i].content_offset;
        for (int j = 0; j < files[i].file_size; j++)
        {
            cur_content[j] = content[cur_pos + j];
	   // assert(cur_pos + j < total_size);
        }
        if (CHECK_SUM(i)) {
            string path = string(argv[2]);
            path = path + "/" + file_name;
            int fd_save = open(path.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
           // cout << cur_content << '\n';
	   //cout <<"file size = " <<(int)files[i].file_size <<  "\n";
	   printf("file size = %d\n", files[i].file_size);
            write(fd_save, cur_content, files[i].file_size);
            close(fd_save);
	    cnt++;
	    cout << "O" << endl;
        }else cout << "X" << endl;
	
        cout << "------------------------\n";
    }
    return 0;
}
