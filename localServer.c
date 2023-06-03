#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
#include <unistd.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <errno.h>  
#include <netdb.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "struct.h"

int main(int argc, char *argv[]){
	
    char recv_buffer[1024];
	char request_buffer[1024];

    int recvMsgSize;

	memset(request_buffer,0,1024);
    memset(recv_buffer,0,1024);

	int ServerSocket;
    /* Create socket for sending/receiving datagrams*/
	if((ServerSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		printf("socket() failed.\n");
	/* Construct local address structure*/
	struct sockaddr_in ServAddr;
	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.s_addr = inet_addr(LOCAL_SERVER_IP);
	ServAddr.sin_port = htons(SERVER_PORT);
	/* Bind to the local address*/
	if ((bind(ServerSocket, (struct sockaddr *) &ServAddr,
		  sizeof(ServAddr))) < 0){
            printf("bind() failded.\n");
            exit(1);
    }
		
	//for (;;) /* Run forever*/
	//{
		struct sockaddr_in ClntAddr;
		int port = ClntAddr.sin_port;
		memset(recv_buffer,'\0',sizeof(recv_buffer));
		/* Set the size of the in-out parameter */
		unsigned int cliAddrLen = sizeof(ClntAddr);
		/* Block until receive message from a client*/
		if ((recvMsgSize = recvfrom(ServerSocket, recv_buffer, 1024,
			0, (struct sockaddr *) &ClntAddr, &cliAddrLen)) < 0)
                printf("recvform() failed,\n");
		printf("From  %s: %d: %s\n", inet_ntoa(ClntAddr.sin_addr),port,recv_buffer);
		close(ServerSocket);

        /* Send query using TCP */
        /* Create socket for sending/receiving datagrams*/
		int ServerSocketTCP;
		/* Create socket for sending/receiving datagrams*/
		if((ServerSocketTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			printf("socket() failed.\n");
		/* Construct local address structure*/
		struct sockaddr_in ServAddrTCP;
		memset(&ServAddrTCP, 0, sizeof(ServAddrTCP));
		ServAddrTCP.sin_family = AF_INET;
		ServAddrTCP.sin_addr.s_addr = inet_addr(LOCAL_SERVER_IP);
		/* Bind to the local address*/
		if ((bind(ServerSocketTCP, (struct sockaddr *) &ServAddrTCP,
			sizeof(ServAddrTCP))) < 0){
				printf("bind() failded.\n");
				exit(1);
		}
		struct sockaddr_in rootServAddr;
		memset(&rootServAddr, 0, sizeof(rootServAddr));
		rootServAddr.sin_family = AF_INET;
		rootServAddr.sin_addr.s_addr = inet_addr(ROOT_SERVER_IP);
		rootServAddr.sin_port = htons(SERVER_PORT);
		unsigned int rootAddrlen = sizeof(rootServAddr);
		int ret;
		if((ret = connect(ServerSocketTCP, (const struct sockaddr *)&rootServAddr, sizeof(rootServAddr)))==-1){
			printf("Accept Error,\n");
		}
		//send(ServerSocketTCP,&recvMsgSize,2,0);
		char TCPBuffer[1024];
		unsigned short length = ntohs(recvMsgSize);
		memcpy(TCPBuffer,&length,2);
		memcpy(TCPBuffer+2,recv_buffer,2+recvMsgSize);
        ret = send(ServerSocketTCP,TCPBuffer,2+recvMsgSize,0);
		close(ServerSocketTCP);
		
	//Receive from root RR
		//创建流式套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sockfd){
        printf("socket error \n");
    }
 
    //填充信息本地信息结构体
    struct sockaddr_in localServAddr = {0}; 
    localServAddr.sin_family = AF_INET;
	localServAddr.sin_addr.s_addr = inet_addr(LOCAL_SERVER_IP);
	localServAddr.sin_port = htons(SERVER_PORT);
 
    socklen_t localserveraddr_len = sizeof(localServAddr);
 
    //绑定
    if(bind(sockfd, (struct sockaddr *)&localServAddr, localserveraddr_len)<0){
        printf("bind error\n");
        exit(1);
    }
 
    //监听
    listen(sockfd, 20);

    //定义结构体保存对方的信息
    struct sockaddr_in rootClntAddr;
    memset(&rootClntAddr, 0, sizeof(rootClntAddr));
    socklen_t rootclntlen = sizeof(rootClntAddr);

    int acceptfd = accept(sockfd, (struct sockaddr *)&rootClntAddr, &rootclntlen);
    if(-1 == acceptfd){
        printf("accept error\n");
    }

    printf("[Connection established]\n");
    
    if ((recvMsgSize = recv(acceptfd, recv_buffer, sizeof(recv_buffer),0) < 0))
            printf("recvform() failed,\n");

    close(acceptfd);
    close(sockfd);
        
	//}

    return 0;

}