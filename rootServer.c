#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdlib.h>  
#include <stdint.h>
#include <unistd.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <errno.h>   
// #include<winsock.h>
// #include<winsock2.h>
// #include <windows.h>
// #pragma comment(lib, "wsock32.lib")

#include "struct.h" 
 
int main(){
    
    struct DNS_Header *recv_header;
    struct DNS_Query *recv_query;
    char recv_buffer[1024] = {0};
    int recvMsgSize = 0;
    char send_buffer[1024] = {0};
    int send_buf_pointer=0;
    int recv_buf_pointer=0;
    int recv_socket;
    struct sockaddr_in root_addr,cli_addr;
    unsigned int len;

    //创建流式套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sockfd){
        printf("socket error \n");
    }
 
    //填充信息本地信息结构体
    struct sockaddr_in ServAddr = {0}; 
    ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.s_addr = inet_addr(ROOT_SERVER_IP);
	ServAddr.sin_port = htons(SERVER_PORT);
 
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
    
    if ((recvMsgSize = recv(acceptfd, recv_buffer, sizeof(recv_buffer),0) < 0))
            printf("recvform() failed,\n");

    close(acceptfd);
    close(sockfd);

    recv_header = malloc(sizeof(DH));
    unsigned short buf_len;
    buf_len=Get16Bits(recv_buffer,&recv_buf_pointer);
    DecodeHeader(recv_header,recv_buffer,&recv_buf_pointer);
    recv_query=malloc(sizeof(DQ));
    PrintHeader(recv_header);
    DecodeQuery(recv_query,recv_buffer,&recv_buf_pointer);
    char *domain = recv_query->name;
    FILE *RR=fopen("rootserver.txt","a+");
    // CutDomain(&recv_query->name);
    while(recv_query->name!=NULL){
        fseek(RR,0,0);  
        //在RR记录中搜索
        struct DNS_RR *nextRR;
        nextRR = malloc(sizeof(DR));
        nextRR->name=malloc(MAX_DOMAIN_LEN);
        nextRR->rdata=malloc(MAX_DOMAIN_LEN);
        while(fscanf(RR,"%s ",nextRR->name)!=EOF){
            fscanf(RR,"%d ",&nextRR->ttl);
            char type[10],cls[10];
            fscanf(RR,"%s ",cls);
            fscanf(RR,"%s ",type);
            nextRR->type = TypeToNum(type);
            fscanf(RR,"%s\n",nextRR->rdata);
            PrintRR(nextRR);
            printf("%s, %s",recv_query->name,nextRR->name);
            if(strcmp(recv_query->name,nextRR->name)==0){//找到之后
                printf("\n[SENT]ASK OTHER SERVER\n");
                //生成头
                struct DNS_Header *header;
                header = malloc(sizeof(DH));
                unsigned short tag = CreateTag(1,0,1,0,0,0,0,0);
                CreateHeader(header,recv_header->id,tag,0,0,1,1);
                EncodeHeader(header,send_buffer,&send_buf_pointer);
                PrintHeader(header);
                
                //生成authority RR  NS记录type=2   此时query_section->name经过cut后已经变成了下一个要去的DNS服务器域名
                struct DNS_RR *authRR;
                authRR = malloc(sizeof(DR));
                CreateRR(authRR, domain, 2, 1, nextRR->ttl, 0, recv_query->name);
                EncodeRR(authRR,send_buffer,&send_buf_pointer);
                PrintRR(authRR);
                
                //生成additon RR   A记录type=1
                struct DNS_RR *addRR;
                addRR = malloc(sizeof(DR));
                CreateRR(addRR, domain, 1, 1, nextRR->ttl, 0, nextRR->rdata);
                EncodeRR(addRR,send_buffer,&send_buf_pointer);
                PrintRR(addRR);
                
                goto out;
            }
        }	
        CutDomain(&recv_query->name);	
    }
    printf("Not found.\n");
    //没找到应该回什么？
    out:
    fclose(RR);


    /* Send RR back to local using TCP */
        /* Create socket for sending/receiving datagrams*/
		int ServerSocketTCP;
		/* Create socket for sending/receiving datagrams*/
		if((ServerSocketTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			printf("socket() failed.\n");
		/* Construct local address structure*/
		struct sockaddr_in ServAddrTCP;
		memset(&ServAddrTCP, 0, sizeof(ServAddrTCP));
		ServAddrTCP.sin_family = AF_INET;
		ServAddrTCP.sin_addr.s_addr = inet_addr(ROOT_SERVER_IP);
		/* Bind to the local address*/
		if ((bind(ServerSocketTCP, (struct sockaddr *) &ServAddrTCP,
			sizeof(ServAddrTCP))) < 0){
				printf("bind() failded.\n");
				exit(1);
		}
		struct sockaddr_in localServerAddr;
		memset(&localServerAddr, 0, sizeof(localServerAddr));
		localServerAddr.sin_family = AF_INET;
		localServerAddr.sin_addr.s_addr = inet_addr(LOCAL_SERVER_IP);
		localServerAddr.sin_port = htons(SERVER_PORT);
		unsigned int localAddrlen = sizeof(localServerAddr);
		int ret;
		if((ret = connect(ServerSocketTCP, (const struct sockaddr *)&localServerAddr, sizeof(localServerAddr)))==-1){
			printf("Accept Error,\n");
		}
		//send(ServerSocketTCP,&recvMsgSize,2,0);
		char TCPBuffer[1024];
		unsigned short length = htons(send_buf_pointer);
        printf("%d",send_buf_pointer);
		memcpy(TCPBuffer,&length,2);
		memcpy(TCPBuffer+2,send_buffer,2+send_buf_pointer);
        ret = send(ServerSocketTCP,TCPBuffer,2+send_buf_pointer,0);
		close(ServerSocketTCP);
 
    return 0;
}
