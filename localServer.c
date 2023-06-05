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

int main(){
	
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
		int UDP_msg_size = 0;
		/* Block until receive message from a client*/
		if ((UDP_msg_size = recvfrom(ServerSocket, recv_buffer, 1024,
			0, (struct sockaddr *) &ClntAddr, &cliAddrLen)) < 0)
                printf("recvform() failed,\n");
		printf("From  %s: %d: %s\n", inet_ntoa(ClntAddr.sin_addr),port,recv_buffer);
		close(ServerSocket);
		char* next_server_ip = ROOT_SERVER_IP;

		char response_buffer[1024];
		memset(response_buffer,0,1024);
		int response_buffer_pointer = 0;

		char UDP_buffer[1024];
		memset(UDP_buffer,0,1024);
		int UDP_buffer_pointer = 0;
		memcpy(UDP_buffer,recv_buffer,UDP_msg_size);

	struct DNS_Header *recv_header = malloc(sizeof(DH));
	struct DNS_RR *author_record = malloc(sizeof(DR));
	struct DNS_RR *add_record = malloc(sizeof(DR));
	struct DNS_RR *ans_record = malloc(sizeof(DR));
	while(1){
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
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(next_server_ip);
		server_addr.sin_port = htons(SERVER_PORT);
		unsigned int Addrlen = sizeof(server_addr);
		int ret;
		if((ret = connect(ServerSocketTCP, (const struct sockaddr *)&server_addr, Addrlen))==-1){
			printf("Accept Error,\n");
		}
		//send(ServerSocketTCP,&recvMsgSize,2,0);
		char TCPBuffer[1024];
		unsigned short length = htons(UDP_msg_size);
		memcpy(TCPBuffer,&length,2);
		memcpy(TCPBuffer+2,UDP_buffer,2+UDP_msg_size);
		ret = send(ServerSocketTCP,TCPBuffer,2+UDP_msg_size,0);
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
		struct sockaddr_in next_server_addr;
		memset(&next_server_addr, 0, sizeof(next_server_addr));
		socklen_t next_clntlen = sizeof(next_server_addr);

		int acceptfd = accept(sockfd, (struct sockaddr *)&next_server_addr, &next_clntlen);
		if(-1 == acceptfd){
			printf("accept error\n");
		}

		printf("[Connection established]\n");
		
		if ((recvMsgSize = recv(acceptfd, recv_buffer, sizeof(recv_buffer),0) < 0))
				printf("recvform() failed,\n");

		close(acceptfd);
		close(sockfd);
		printf("Receive Successfully\n");
		
		int recv_buffer_pointer = 0;
		//判断RR如果不成功则再建立TCP到下一级
		unsigned short buf_len;
		buf_len=Get16Bits(recv_buffer,&recv_buffer_pointer);
		DecodeHeader(recv_header,recv_buffer,&recv_buffer_pointer);
		PrintHeader(recv_header);
		unsigned short num = 1;
		if(((recv_header->tag)&(0x0003))==0x0003){
			printf("if1\n");
			struct DNS_Header *response_header = malloc(sizeof(DH));
			struct DNS_Header *temp_header = malloc(sizeof(DH)); //最开始的UDP header
			struct DNS_Query *temp_query = malloc(sizeof(DQ));
			CreateHeader(response_header,recv_header->id,recv_header->tag,1,0,0,0);
			//EncodeHeader(response_header,response_buffer,&response_buffer_pointer);
			DecodeHeader(temp_header,UDP_buffer,&UDP_buffer_pointer);
			DecodeQuery(temp_query,UDP_buffer,&UDP_buffer_pointer);
			MergeRequest(response_header,temp_query,response_buffer,UDP_msg_size);
			break;
		}else if(recv_header->answerNum==num){
			printf("else2\n");
			struct DNS_Header *response_header = malloc(sizeof(DH));
			struct DNS_Header *temp_header = malloc(sizeof(DH)); //最开始的UDP header
			struct DNS_Query *temp_query = malloc(sizeof(DQ));
			UDP_buffer_pointer = 0;
			DecodeRR(ans_record,recv_buffer,&recv_buffer_pointer);
			PrintRR(ans_record);
			DecodeHeader(temp_header,UDP_buffer,&UDP_buffer_pointer);
			PrintHeader(temp_header);
			DecodeQuery(temp_query,UDP_buffer,&UDP_buffer_pointer);
			printf("Query!!!\n");
			if(temp_query->qtype==TYPE_A){
				printf("==A\n");
				CreateHeader(response_header,recv_header->id,recv_header->tag,1,1,1,1);
				EncodeHeader(response_header,response_buffer,&response_buffer_pointer);
				EncodeQuery(temp_query,response_buffer,&response_buffer_pointer);
				EncodeRR(ans_record,response_buffer,&response_buffer_pointer);
				EncodeRR(author_record,response_buffer,&response_buffer_pointer);
				EncodeRR(add_record,response_buffer,&response_buffer_pointer);
			}else{
				printf("==else\n");
				struct DNS_RR *add2_record = malloc(sizeof(DR));
				DecodeRR(add2_record, recv_buffer,&recv_buffer_pointer);
				CreateHeader(response_header,recv_header->id,recv_header->tag,1,1,1,2);
				EncodeHeader(response_header,response_buffer,&response_buffer_pointer);
				EncodeQuery(temp_query,response_buffer,&response_buffer_pointer);
				EncodeRR(ans_record,response_buffer,&response_buffer_pointer);
				EncodeRR(author_record,response_buffer,&response_buffer_pointer);
				EncodeRR(add_record,response_buffer,&response_buffer_pointer);
				EncodeRR(add2_record,response_buffer,&response_buffer_pointer);
			}
			break;
		}
		DecodeRR(author_record, recv_buffer,&recv_buffer_pointer);
		PrintRR(author_record);
		DecodeRR(add_record, recv_buffer,&recv_buffer_pointer);
		next_server_ip = add_record->rdata;
		//memcpy(next_server_ip,add_record->rdata,sizeof(add_record->rdata));
		printf("END\n");
	}
	//成功，建立UDP发回RR给client
	int sock;
	struct sockaddr_in client_addr;
	struct sockaddr_in local_addr = {0}; 
    local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = inet_addr(LOCAL_SERVER_IP);
 
    socklen_t local_serveraddr_len = sizeof(local_addr);
	/* Create a datagram/UDP socekt */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		printf("socket() failed.\n");
    //绑定
    if(bind(sock, (struct sockaddr *)&local_addr, local_serveraddr_len)<0){
        printf("bind error\n");
        exit(1);
    }
	// char response_buffer[1024];
	// //unsigned int fromSize;
	// memset(response_buffer,0,1024);
	
	/* Construct the server address structure */
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
	client_addr.sin_port = htons(SERVER_PORT);

	/* Create Response */
	/* Send the string to the server*/
	sendto(sock,response_buffer, response_buffer_pointer, 0,
			(struct sockaddr *) &client_addr, sizeof(client_addr));
	/* null-terminate the received data */
	close(sock);
    return 0;

}