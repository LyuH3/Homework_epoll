#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

#define BUFFER_SIZE 1024
#define FILE_PATH_SIZE 512

struct thread_data
{
    char *fpath;
    sockaddr_in address;
};


void find_file_name(char *name, char *path)
{
	char *name_start = NULL;
	int sep = '/';
	if (NULL == name) {
		printf("the path name is NULL\n");
		exit(-1);
	}
    name_start = strrchr(name, sep);
	if (NULL == name_start)
	{
		strcat(path, name_start);
	}
	else
		strcat(path, name_start + 1);
}

void *client(void *threaddata)
{
    thread_data client_data = *((thread_data *)threaddata);
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int result = connect(clientSocket, (struct sockaddr*)&(client_data.address), sizeof(client_data.address));
    //*(client_data.thread_count)++;
    if (result == -1)
    {
        perror("Oops,an error for yout client.");
        exit(1);
    }
    cout << "Start to find file" << endl;
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    strncpy(buffer, client_data.fpath, strlen(client_data.fpath)>BUFFER_SIZE ? BUFFER_SIZE : strlen(client_data.fpath));
    if (send(clientSocket, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Send File Name Failed:");
        exit(1);
    }
    char save_path[FILE_PATH_SIZE + 1] = {"/home/q/002/"};
    find_file_name(client_data.fpath, save_path);
    FILE *fp = fopen(save_path, "w");
    if (fp == NULL)
    {
        cout << "File:" << save_path << "Can not open to write" << endl;
        exit(1);
    }
    bzero(buffer, BUFFER_SIZE);
    int length = 0;
    while ((length = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if (fwrite(buffer, sizeof(char), length, fp) < length)
        {
            cout << "File:" << save_path << " Write Failed" << endl;
            break;
        }
        bzero(buffer, BUFFER_SIZE);
    }
    cout << "Receive File:" << save_path << " Successful!" << endl;
    fclose(fp);
    //close(clientSocket);
    //*(client_data.thread_count)--;
    pthread_exit(NULL);
}

void *info_listen(void *address)
{
    sockaddr_in server_Addr = *((sockaddr_in *)address);
    int infoSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    connect(infoSocket, (struct sockaddr*)&(server_Addr), sizeof(server_Addr));
    char info[BUFFER_SIZE];
    int j = 0;
    while ((j = recv(infoSocket, info, BUFFER_SIZE, 0)) > 0)
    {
        cout << "目前连接的客户端: " ;
        cout << info ;
        cout << " 个" << endl;
    }
    pthread_exit(NULL);
}

int main()
{
    int count;
    char ServerAddress[15];
    int ServerPort = 0;
    char input_path[ FILE_PATH_SIZE + 1];
    bzero(input_path, FILE_PATH_SIZE + 1);

    sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    cout << "输入服务器地址：" << endl;
    cin >> ServerAddress;
    serverAddress.sin_addr.s_addr = inet_addr(ServerAddress);
    cout << "输入服务器正在监听的端口号：" << endl;
    cin >> ServerPort;
    serverAddress.sin_port = htons(ServerPort);
    cout << "输入要下载的文件地址：" << endl;
    cin >> input_path;
    cout << "请输入用于测试的线程数：" << endl;
    cin >> count;

    thread_data data;
    data.fpath = input_path;
    data.address = serverAddress;

    pthread_t info_thread;
    pthread_create(&info_thread, NULL, info_listen, (void *)(&serverAddress));
    sleep(1);

    for (int i = 0; i < count; i++)
    {
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client, (void *)(&data)) == -1)
        {
            perror("Create Thread Error");
            break;
        }
        usleep(50);
    }
    sleep(10);
    return 0;
}