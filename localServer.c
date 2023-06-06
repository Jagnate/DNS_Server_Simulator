#include "struct.h"
#define MY_TXT "localcache.txt"

void WriteRR(struct DNS_RR *writeRR);
int FirstFind();

char response_buffer[1024];
int response_buffer_pointer = 0;
char UDP_buffer[1024];
int UDP_buffer_pointer = 0;
char recv_buffer[1024];
char request_buffer[1024];
FILE *RR;


int main(){
	

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

	memset(response_buffer,0,1024);
	memset(UDP_buffer,0,1024);
	memcpy(UDP_buffer,recv_buffer,UDP_msg_size);
	
    RR=fopen(MY_TXT,"a+");
	if(FirstFind()){
		goto out;
	}

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
			struct DNS_Header *response_header = malloc(sizeof(DH));
			struct DNS_Header *temp_header = malloc(sizeof(DH)); //最开始的UDP header
			struct DNS_Query *temp_query = malloc(sizeof(DQ));
			UDP_buffer_pointer = 0;
			CreateHeader(response_header,recv_header->id,recv_header->tag,1,0,0,0);
			DecodeHeader(temp_header,UDP_buffer,&UDP_buffer_pointer);
			DecodeQuery(temp_query,UDP_buffer,&UDP_buffer_pointer);
			MergeRequest(response_header,temp_query,response_buffer,UDP_msg_size);
			break;
		}else if(recv_header->answerNum==num){
			struct DNS_Header *response_header = malloc(sizeof(DH));
			struct DNS_Header *temp_header = malloc(sizeof(DH)); //最开始的UDP header
			struct DNS_Query *temp_query = malloc(sizeof(DQ));
			UDP_buffer_pointer = 0;
			DecodeRR(ans_record,recv_buffer,&recv_buffer_pointer);
			PrintRR(ans_record);
			DecodeHeader(temp_header,UDP_buffer,&UDP_buffer_pointer);
			PrintHeader(temp_header);
			DecodeQuery(temp_query,UDP_buffer,&UDP_buffer_pointer);
			if(temp_query->qtype==TYPE_A){
				CreateHeader(response_header,recv_header->id,recv_header->tag,1,1,1,1);
				EncodeHeader(response_header,response_buffer,&response_buffer_pointer);
				EncodeQuery(temp_query,response_buffer,&response_buffer_pointer);
				EncodeRR(ans_record,response_buffer,&response_buffer_pointer);
				EncodeRR(author_record,response_buffer,&response_buffer_pointer);
				EncodeRR(add_record,response_buffer,&response_buffer_pointer);
				WriteRR(ans_record);
			}else{
				struct DNS_RR *add2_record = malloc(sizeof(DR));
				DecodeRR(add2_record, recv_buffer,&recv_buffer_pointer);
				CreateHeader(response_header,recv_header->id,recv_header->tag,1,1,1,2);
				EncodeHeader(response_header,response_buffer,&response_buffer_pointer);
				EncodeQuery(temp_query,response_buffer,&response_buffer_pointer);
				EncodeRR(ans_record,response_buffer,&response_buffer_pointer);
				EncodeRR(author_record,response_buffer,&response_buffer_pointer);
				EncodeRR(add_record,response_buffer,&response_buffer_pointer);
				EncodeRR(add2_record,response_buffer,&response_buffer_pointer);
				WriteRR(ans_record);
				WriteRR(add_record);
			}
			break;
		}
		DecodeRR(author_record, recv_buffer,&recv_buffer_pointer);
		PrintRR(author_record);
		DecodeRR(add_record, recv_buffer,&recv_buffer_pointer);
		next_server_ip = add_record->rdata;
	}
	out:
	fclose(RR);
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

void WriteRR(struct DNS_RR *writeRR){
	fseek(RR,0,0);
	DecodeDomain(writeRR->name);
    fprintf(RR,"%s ",domain_value);
	fprintf(RR,"%d ",writeRR->ttl);
	fprintf(RR,"IN ");
	fprintf(RR,"%s ",numToType(writeRR->type));
	fprintf(RR,"%s\n",writeRR->rdata);
}


int FirstFind(){
    int find_flg=0;
    struct DNS_RR *fileRR;
    fileRR=malloc(sizeof(DR));
    memset(fileRR,0,sizeof(DR));
    fileRR->name=malloc(MAX_DOMAIN_LEN);
    fileRR->rdata=malloc(MAX_DOMAIN_LEN);
	
	struct DNS_Header *response_header = malloc(sizeof(DH));
	struct DNS_Header *temp_header = malloc(sizeof(DH)); //最开始的UDP header
	struct DNS_Query *temp_query = malloc(sizeof(DQ));
	UDP_buffer_pointer=0;
	DecodeHeader(temp_header,UDP_buffer,&UDP_buffer_pointer);
	DecodeQuery(temp_query,UDP_buffer,&UDP_buffer_pointer);

    fseek(RR,0,0);
    while(fscanf(RR,"%s ",fileRR->name)!=EOF){
        fscanf(RR,"%d",&fileRR->ttl);
        char type[10],cls[10];
        fscanf(RR,"%s ",cls);
        fscanf(RR,"%s ",type);
        fileRR->type=TypeToNum(type);
        fscanf(RR,"%s\n",fileRR->rdata);
        if(strcmp(temp_query->name,fileRR->name)==0 && (temp_query->qtype==fileRR->type)){
            printf("Find in edu server.\n");
            CreateRR(fileRR,fileRR->name,fileRR->type,0x0001,fileRR->ttl,0x0000,fileRR->rdata);
            struct DNS_Header *header;
            header=malloc(sizeof(DH));
            unsigned short tag=CreateTag(1,0,0,0,0,0,0,0);
            if(fileRR->type==TYPE_MX){
                CreateHeader(header,temp_header->id,tag,1,1,0,1);
                EncodeHeader(header,response_buffer,&response_buffer_pointer);
				EncodeQuery(temp_query,response_buffer,&response_buffer_pointer);
                EncodeRR(fileRR,response_buffer,&response_buffer_pointer);
                fseek(RR,0,0);
                struct DNS_RR *mxRR;
                mxRR=malloc(sizeof(DR));
                memset(mxRR,0,sizeof(DR));
                mxRR->name=malloc(MAX_DOMAIN_LEN);
                mxRR->rdata=malloc(MAX_DOMAIN_LEN);
                while (fscanf(RR,"%s ",mxRR->name)!=EOF){
                    fscanf(RR,"%d",&mxRR->ttl);
                    char type[10],cls[10];
                    fscanf(RR,"%s ",cls);
                    fscanf(RR,"%s ",type);
                    mxRR->type=TypeToNum(type);
                    fscanf(RR,"%s\n",mxRR->rdata);
                    if(strcmp(fileRR->rdata,mxRR->name)==0){
                        CreateRR(mxRR,mxRR->name,mxRR->type,0x0001,mxRR->ttl,0x0000,mxRR->rdata);
                        EncodeRR(mxRR,response_buffer,&response_buffer_pointer);
                        PrintRR(mxRR);
                    }
                }
            }
            else if(fileRR->type==TYPE_CNAME){
                CreateHeader(header,temp_header->id,tag,1,1,0,1);
                EncodeHeader(header,response_buffer,&response_buffer_pointer);
				EncodeQuery(temp_query,response_buffer,&response_buffer_pointer);
                EncodeRR(fileRR,response_buffer,&response_buffer_pointer);
                fseek(RR,0,0);
                struct DNS_RR *cname_RR;
                cname_RR=malloc(sizeof(DR));
                memset(cname_RR,0,sizeof(DR));
                cname_RR->name=malloc(MAX_DOMAIN_LEN);
                cname_RR->rdata=malloc(MAX_DOMAIN_LEN);
                while (fscanf(RR,"%s ",cname_RR->name)!=EOF){
                    fscanf(RR,"%d",&cname_RR->ttl);
                    char type[10],cls[10];
                    fscanf(RR,"%s ",cls);
                    fscanf(RR,"%s ",type);
                    cname_RR->type=TypeToNum(type);
                    fscanf(RR,"%s\n",cname_RR->rdata);
                    if(strcmp(fileRR->rdata,cname_RR->name)==0){
                        CreateRR(cname_RR,cname_RR->name,cname_RR->type,0x0001,cname_RR->ttl,0x0000,cname_RR->rdata);
                        EncodeRR(cname_RR,response_buffer,&response_buffer_pointer);
                    }
                }
                
            }
            else{
                CreateHeader(header,temp_header->id,tag,1,1,0,0);
                EncodeHeader(header,response_buffer,&response_buffer_pointer);
				EncodeQuery(temp_query,response_buffer,&response_buffer_pointer);
                EncodeRR(fileRR,response_buffer,&response_buffer_pointer);
            }
            PrintHeader(header);
            PrintRR(fileRR);
            find_flg=1;
            break;
        }
    }
    //回位
    fseek(RR,0,0);
    //MX类型
    if(fileRR->type!=TYPE_A){
        struct DNS_RR *addFileRR;
        addFileRR=malloc(sizeof(DR));
        addFileRR->name=malloc(MAX_DOMAIN_LEN);
        addFileRR->rdata=malloc(MAX_DOMAIN_LEN);
        while(fscanf(RR,"%s ",addFileRR->name)!=EOF){
            fscanf(RR,"%d ",&addFileRR->ttl);
            char type[10],cls[10];
            fscanf(RR,"%s ",cls);
            fscanf(RR,"%s ",type);
            addFileRR->type=TypeToNum(type);
            fscanf(RR,"%s\n",addFileRR->rdata);
            if(strcmp(fileRR->rdata,addFileRR->name)==0){
                printf("find mx rr.\n");
                CreateRR(addFileRR,fileRR->rdata, 1, 1, fileRR->ttl, 0, addFileRR->rdata);
                EncodeRR(addFileRR,response_buffer,&response_buffer_pointer);
                PrintRR(addFileRR);
                break;;
            }
        }
    }
    return find_flg; 
}