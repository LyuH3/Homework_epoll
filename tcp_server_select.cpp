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
using namespace std;

int main()
{
    int serverSocketFD, clientSocketFD;

    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    fd_set readfds, testfds;

    serverSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&serverAddress, 0 , sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(9999);
    bind(serverSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    //绑定套接字信息
    unsigned value = 1;

    listen(serverSocketFD, 5000);
    FD_ZERO(&readfds);
    FD_SET(serverSocketFD, &readfds);

    while (true)
    {
        char strBuffer[1024];
        int nread;
        testfds = readfds;
        cout << "server waiting..." << endl;

        int result = select(FD_SETSIZE, &testfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0);
        if (result < 1)
        {
            perror("server error 20");
            exit(1);
        }
        
        for (int fd = 0; fd < FD_SETSIZE; fd++)
        {
            if (FD_ISSET(fd, &testfds))
            {
                if (fd == serverSocketFD)
                {
                    struct sockaddr_in clientAddress;
                    socklen_t clientAddresssize = sizeof(clientAddress);
                    clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, &clientAddresssize);
                    setsockopt(clientSocketFD, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
                    FD_SET(clientSocketFD, &readfds);
                    //cout << "Adding client on fd " << clientSocketFD << endl;
                }
                else
                {
                    ioctl(fd, FIONREAD, &nread);

                    if (nread == 0)
                    {
                        close(fd);
                        FD_CLR(fd, &readfds);
                        //cout << "removing client on fd " << fd << endl;
                    }
                    else
                    {
                        int datalength = read(fd, strBuffer ,sizeof(strBuffer)-1);
                        cout << "serving client on fd " << fd+1 << endl;
                        //cout << "Already read data to the client" << endl;
                        cout << strBuffer << endl;
                    }
                    
                }
                
            }
            
        }
        
    }

    return 0;
}