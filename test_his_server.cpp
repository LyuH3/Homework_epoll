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
#define LENGTH_OF_LISTEN_QUEUE 20   
#define BUFFER_SIZE 1024   
#define FILE_NAME_MAX_SIZE 512   
static void *Data_handle(void * sock_fd);
int main(void)   
{   
 
////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // 声明并初始化一个服务器端的socket地址结构，sockaddr_in是internet环境下套接字的地址形式
  //sockaddr_in（在netinet / in.h中定义）：
  //	struct  sockaddr_in {
  //	short  int  sin_family;                      /* Address family */
  //	unsigned  short  int  sin_port;       /* Port number */
  //	struct  in_addr  sin_addr;              /* Internet address */
  //	unsigned  char  sin_zero[8];         /* Same size as struct sockaddr */
  //};
  //struct  in_addr {unsigned  long  s_addr;};
  struct sockaddr_in server_addr;   
  bzero(&server_addr, sizeof(server_addr)); 
  //Sa_family: 是地址家族，也成作，协议族，一般都是"AF_XXX"的形式，常用的有
  //AF_INET  Arpa（TCP / IP） 网络通信协议
  //AF_UNIX  UNIX 域协议（文件系统套接字）
  //AF_ISO    ISO标准协议
  //AF_NS    施乐网络体统协议
  //AF_IPX  Novell IPX 协议
  //AF_APPLETALK   Appletalk DDS
  server_addr.sin_family = AF_INET;   
 
  //htons是将整型变量从主机字节顺序转变成网络字节顺序， 就是整数在地址空间存储方式变为高位字节存放在内存的低地址处。
  //INADDR_ANY：0.0.0.0，泛指本机的意思，也就是表示本机的所有IP，监听本机所有网卡
  server_addr.sin_addr.s_addr = htons(INADDR_ANY);   
  server_addr.sin_port = htons(SERVER_PORT);   
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
  // 创建socket，若成功，返回socket描述符   
  //1、domain：即协议域，又称为协议族（family）。AF_INET：TCP/IP协议簇
  //2、type：指定socket类型。SOCK_STREAM（常用）字节流套接字
  //3、protocol：故名思意，就是指定协议。0：IPPROTO_TCP TCP传输协议 
  int server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);   
  if(server_socket_fd < 0)   
  {   
    perror("Create Socket Failed:");   
    exit(1);   
  }   
 
  //int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);
  //sock：将要被设置或者获取选项的套接字。level：选项所在的协议层。
  //optname：需要访问的选项名。optval：对于getsockopt()，指向返回选项值的缓冲。optlen：作为入口参数时，选项值的最大长度。
  // 令SO_REUSEADD==true 允许套接口和一个已在使用中的地址捆绑（参见bind()）。
  int opt = 1;
  setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));   
    
  //bind绑定socket和socket地址结构   
  //三个参数为：socket描述符、协议地址、地址的长度
  if(-1 == (bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))))   
  {   
    perror("Server Bind Failed:");   
    exit(1);   
  }   
  //sockfd：第一个参数即为要监听的socket描述符
  //backlog : 第二个参数为相应socket可以排队的最大连接个数
  //socket()函数创建的socket默认是一个主动类型的，listen函数将socket变为被动类型的，等待客户的连接请求。
  if(-1 == (listen(server_socket_fd, LENGTH_OF_LISTEN_QUEUE)))   
  {   
    perror("Server Listen Failed:");   
    exit(1);   
  }   
  printf("Socket Init Successful!  Begin to listen!\n");
///////////////////////////////////////////////////////////////////////////////////////////////////////////
 
  while(1)   
  {   
    // 定义客户端的socket地址结构   
    struct sockaddr_in client_addr;   
    socklen_t client_addr_length = sizeof(client_addr);   
    
	//int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	//sockfd：第一个参数为服务器的socket描述符
	//addr：，第二个参数为指向struct sockaddr *的指针，用于返回客户端的协议地址
	//addrlen：第三个参数为协议地址的长度
	//返回值 : 如果accpet成功，那么其返回值是由内核自动生成的一个全新的描述字，代表与返回客户的TCP连接。
 
    // 接受连接请求，返回一个新的socket(描述符)，这个新socket用于同连接的客户端通信   
    // accept函数会把连接到的客户端信息写到client_addr中   
    int session_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_length);   
    if(session_fd < 0)   
    {   
      perror("Server Accept Failed:");   
    //  break;   
    }   
	char client_addr_res[20];
	//char *ptr=inet_ntop(AF_INET, &client_addr.sin_addr, client_addr_res, strlen(client_addr_res));
	printf("Get Connected with Client:%s ,Opening a new Thread...\n",inet_ntoa(client_addr.sin_addr) );
	pthread_t thread_id;
	if (pthread_create(&thread_id, NULL, Data_handle, (void *)(&session_fd)) == -1)
	{
		fprintf(stderr, "pthread_create error!\n");
		break;                                  //break while loop
	}
    
 
  }   
  // 关闭监听用的socket   
  close(server_socket_fd);   
  return 0;   
}   
 
static void * Data_handle(void * fd)
{
	int session_fd = *((int *)fd);
	// recv函数通过描述字读取数据到字节流，并存储到地址字符串
	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);
	if (recv(session_fd, buffer, BUFFER_SIZE, 0) < 0)
	{
		perror("Server Recieve Data Failed:");
	}
	char file_name[FILE_NAME_MAX_SIZE + 1];
	bzero(file_name, FILE_NAME_MAX_SIZE + 1);
	strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(buffer));
	printf("Received Filename Successful\n");
	// 通过文件名，打开文件读取数据传输到已连接的client
	FILE *fp = fopen(file_name, "r");
	if (NULL == fp)
	{
		printf("File:%s Not Found\n", file_name);
	}
	else
	{
		bzero(buffer, BUFFER_SIZE);//清空buffer   
		int length = 0;
		//每次读取一个BUFFER_SIZE长度的数据并向client发送
		while ((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
		{
			//ssize_t send(int sockfd, const void *buf, size_t len, int flags);
			//socket：如果是服务端则是accpet()函数的返回值，客户端是connect()函数中的第一个参数
			// buffer：写入或者读取的数据
			// len：写入或者读取的数据的大小
			if (send(session_fd, buffer, length, 0) < 0)
			{
				printf("Send File:%s Failed./n", file_name);
				break;
			}
			bzero(buffer, BUFFER_SIZE);
		}
		fclose(fp);
		printf("Send File:%s To Client Successful!\n", file_name);
	}
	// int close(int fd)  fd:客户端connect()函数的第一个参数,服务端accept()的返回值
	close(session_fd);
	pthread_exit(NULL);   //terminate calling thread!
}