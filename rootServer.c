#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
 
 
int main(){
    
    unsigned short ServPort = 53;
    //创建流式套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sockfd){
        printf("socket error \n");
    }
 
    //填充信息本地信息结构体
    struct sockaddr_in ServAddr = {0}; 
    ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.s_addr = inet_addr("127.2.2.1");
	ServAddr.sin_port = htons(ServPort);
 
    socklen_t serveraddr_len = sizeof(ServAddr);
 
    //绑定
    if(bind(sockfd, (struct sockaddr *)&ServAddr, serveraddr_len)<0){
        printf("bind error\n");
        exit(1);
    }
 
    //监听
    listen(sockfd, 20);
    
    //定义结构体保存对方的信息
    struct sockaddr_in ClntAddr;
    memset(&ClntAddr, 0, sizeof(ClntAddr));
    socklen_t clntlen = sizeof(ClntAddr);

        int acceptfd = accept(sockfd, (struct sockaddr *)&ClntAddr, &clntlen);
        if(-1 == acceptfd){
            printf("accept error\n");
        }

        printf("[Connection established]\n");
        //这里要先获取生成clientAddr内容才能打印，目前只得到了client套接字描述符. getpeername函数用于获取与某个套接字关联的外地协议地址
        // struct sockaddr_in c;
        // socklen_t cLen = sizeof(c);
        // getpeername(acceptfd, (struct sockaddr*) &c, &cLen); 
        // printf("[Client info] %s : %d\n",inet_ntoa(c.sin_addr),ntohs(c.sin_port));	
    
        char ReceiveBuffer[1024] = {0};
        int recvMsgSize = 0;
        if ((recvMsgSize = recv(acceptfd, ReceiveBuffer, sizeof(ReceiveBuffer),0) < 0))
                printf("recvform() failed,\n");

        close(acceptfd);
    
        close(sockfd);
 
    return 0;
}
