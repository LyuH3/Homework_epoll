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

int epfd;
epoll_event event, events[5000];
unordered_map<int,sockaddr_in> fdmap;
sockaddr_in serverAddress, clientAddress;
socklen_t clientAddressszie = sizeof(clientAddress);

void *DataTransmit( void * fd)
{
    int session_fd = *((int *)fd);
    char buffer[1024];
    bzero(buffer, 1024);
    if (recv(session_fd, buffer, 1024, 0) < 0)
    {
        perror("Server Recieve Data Failed:");
    }
    char file_name[512+1];
    bzero(file_name, 512+1);
    strncpy(file_name, buffer, ((strlen(buffer)>512) ? 512 : (strlen(buffer))));
    cout << "Received Filename Successful!" << endl;
    FILE *fp = fopen(file_name, "r");
    if (NULL == fp)
    {
        cout << "File:" << file_name << " Not Found" << endl;
    }
    else
    {
        bzero(buffer, 1024);
        int length = 0;
        while ((length = fread(buffer, sizeof(char), 1024, fp)) > 0)
        {
            if (send(session_fd, buffer, length, 0) < 0)
            {
                cout << "Send File " << file_name << " Failed." << endl;
                break;
            }
            bzero(buffer, 1024);
        }
        fclose(fp);
        cout << "Succeed TransMitting" << endl;
    }
    close(session_fd);
    pthread_exit(NULL);
}

int main()
{
    int serverSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&serverAddress, 0 , sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(9999);
    bind(serverSocketFD, (sockaddr *)&serverAddress, sizeof(serverAddress));
    //绑定套接字信息
    unsigned value = 1;

    listen(serverSocketFD, 5000);

    sockaddr_in info_addr;
    socklen_t info_addr_length = sizeof(info_addr);
    int infoFD = accept(serverSocketFD, (struct sockaddr*)&info_addr, &info_addr_length);
    write(infoFD, "HEllO", 6);
    
    epfd = epoll_create(5000);
    event.data.fd = serverSocketFD;
    event.events = EPOLLIN;

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, serverSocketFD, &event);
    if (ret == -1)
    {
        perror("Listening failed");
        exit(-1);
    }
    while (true)
    {
        int nfds = epoll_wait(epfd, events, 5000, -1);
        for (int i = 0; i < nfds; ++i)
        {
            char nfds_str[5];
            sprintf(nfds_str, "%d",nfds);
            write(infoFD, nfds_str, 5);
            if (!(events[ i ].events & EPOLLIN))
            {
                cout << "Server Accept Failed" << endl;
                continue;
            }
            if (events[ i ].data.fd == serverSocketFD)
            {
                int clientSocketFD = accept(serverSocketFD, (sockaddr *)&clientAddress, &clientAddressszie);
                setsockopt(clientSocketFD, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
                fdmap[ clientSocketFD ] = clientAddress;
                cout << "Connected to Client" << endl;
                if (clientSocketFD == -1)
                {
                    cout << "Accept fault" << endl;
                    return 0;
                }
                event.events = EPOLLIN;
                event.data.fd = clientSocketFD;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientSocketFD, &event);
            }
            else
            {
                pthread_t thread_id;
                if (pthread_create(&thread_id, NULL, DataTransmit, (void *)(&events[ i ].data.fd)) == -1)
                {
                    fprintf(stderr, "pthread_create error!\n");
                    break;                                  //break while loop
                }
                int delete_result = epoll_ctl(epfd, EPOLL_CTL_DEL, events[ i ].data.fd, NULL);
                cout << inet_ntoa(fdmap[events[i].data.fd] .sin_addr) << ":" << ntohs(fdmap[events[i].data.fd] .sin_port) << " exit ... " << endl;
                fdmap.erase(events[i].data.fd);
            }
        }
    }
    close(serverSocketFD);
    return 0;
}