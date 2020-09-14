//客户端
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unordered_map>
using namespace std;

#define SERVER_PORT 9999   
#define BUFFER_SIZE 1024   
#define FILE_PATH_SIZE 512   
void find_file_name(char *name, char *path);
int main()
{
	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	client_addr.sin_port = htons(0);
  
	int client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket_fd < 0)
	{
		perror("Create Socket Failed:");
		exit(1);
	}
 
	if (-1 == (bind(client_socket_fd, (struct sockaddr*)&client_addr, sizeof(client_addr))))
	{
		perror("Client Bind Failed:");
		exit(1);
	}
//////////////////////////////////////////////////////////////////////////////////////////////////////
	// 声明一个服务器端的socket地址结构，并用服务器那边的IP地址及端口对其进行初始化，用于后面的连接   
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	//将点分十进制串转换成网络字节序二进制值，此函数对IPv4地址和IPv6地址都能处理。
	//	第一个参数可以是AF_INET或AF_INET6：
	//	第二个参数是一个指向点分十进制串的指针：
	//	第三个参数是一个指向转换后的网络字节序的二进制值的指针。
	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) == 0)
	{
		perror("Server IP Address Error:");
		exit(1);
	}
 
	server_addr.sin_port = htons(SERVER_PORT);
	socklen_t server_addr_length = sizeof(server_addr);
 
	// int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen); 
	// sockfd：第一个参数即为客户端的socket描述字
	//	addr：当前客户端的本地地址，是一个 struct sockaddr_un 类型的变量, 在不同主机中是struct sockaddr_in 类型的变量,
	//	addrlen：表示本地地址的字节长度
	//	返回值 : 成功标志
	if (connect(client_socket_fd, (struct sockaddr*)&server_addr, server_addr_length) < 0)
	{
		perror("Can Not Connect To Server IP:");
		exit(0);
	}
   
	char file_path[FILE_PATH_SIZE + 1];
	bzero(file_path, FILE_PATH_SIZE + 1);
	printf("Input the File Path on Server:\t");
	scanf("%s", file_path);
 
	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);
	strncpy(buffer, file_path, strlen(file_path)>BUFFER_SIZE ? BUFFER_SIZE : strlen(file_path));
 
	//ssize_t send(int sockfd, const void *buf, size_t len, int flags);
	//socket：如果是服务端则是accpet()函数的返回值，客户端是connect()函数中的第一个参数
	// buffer：写入或者读取的数据
	// len：写入或者读取的数据的大小
	if (send(client_socket_fd, buffer, BUFFER_SIZE, 0) < 0)
	{
		perror("Send File Name Failed:");
		exit(1);
	}
 
	//将目标路径转化为本地存储路径
	char save_path[FILE_PATH_SIZE + 1] = {"/home/q/002/"};
	find_file_name(file_path, save_path);
 
	//尝试打开文件
	FILE *fp = fopen(save_path, "w");
	if (NULL == fp)
	{
		printf("File:\t%s Can Not Open To Write\n", save_path);
		exit(1);
	}
 
	// 从服务器接收数据到buffer中   
	// 每接收一段数据，便将其写入文件中，循环直到文件接收完并写完为止   
	bzero(buffer, BUFFER_SIZE);
	int length = 0;
	while ((length = recv(client_socket_fd, buffer, BUFFER_SIZE, 0)) > 0)
	{
		if (fwrite(buffer, sizeof(char), length, fp) < length)
		{
			printf("File:\t%s Write Failed\n", save_path);
			break;
		}
		bzero(buffer, BUFFER_SIZE);
	}
 
	// 接收成功后，关闭文件，关闭socket   
	printf("Receive File:\t%s From Server IP Successful!\n",save_path);
	fclose(fp);
	close(client_socket_fd);
	return 0;
}
 
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