#include "struct.h"

void judgeServer(struct DNS_RR *rr);

int main(int argc, char *argv[]){

	if(argc !=4 && argc!=3){
		printf("Usage: %s <DNS> <Type>\n",
				argv[0]);
		exit(1);
	}else if(argc == 4 && (strcmp(argv[1],"trace")!=0)){
		printf("Usage: %s trace <DNS> <Type>\n",
				argv[0]);
		exit(1);
	}
	

	int sock;
	struct sockaddr_in ServAddr;
	//struct sockaddr_in fromAddr;
	
	char RequestBuffer[1024];
	//unsigned int fromSize;
	memset(RequestBuffer,0,1024);
    /* Create a datagram/UDP socekt */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		printf("socket() failed.\n");
	/* Construct the server address structure */
	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.s_addr = inet_addr(LOCAL_SERVER_IP);
	ServAddr.sin_port = htons(SERVER_PORT);

	/* Create header and query */
	struct DNS_Header header_section = {0};
	//对应: qr,opcode,aa,tc,rd,ra,z,rcode，其中opcode z rcode可以随便填写. 默认不超过512，tc为1
	CreateHeader(&header_section,0,CreateTag(0,0,0,0,0,1,0,0),1,0,0,0);
	struct DNS_Query query_section = {0};
	if(argc==4){
		unsigned short qtype = TypeToNum(argv[3]);
		unsigned short qclass = 0x0001;	//Class in
		CreateQuery(&query_section,argv[2],qtype,qclass);
	}else{
		unsigned short qtype = TypeToNum(argv[2]);
		unsigned short qclass = 0x0001;	//Class in
		CreateQuery(&query_section,argv[1],qtype,qclass);
	}
	int length = MergeRequest(&header_section,&query_section,RequestBuffer,1024);

	/* Send the string to the server*/
	sendto(sock,RequestBuffer, length, 0,
			(struct sockaddr *) &ServAddr, sizeof(ServAddr));
	/* null-terminate the received data */
	close(sock);

	/* Receive a datagram/UDP socekt */
	int ServerSocket;
    /* Create socket for sending/receiving datagrams*/
	if((ServerSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		printf("socket() failed.\n");
	/* Construct local address structure*/
	struct sockaddr_in ansAddr;
	memset(&ansAddr, 0, sizeof(ansAddr));
	ansAddr.sin_family = AF_INET;
	ansAddr.sin_addr.s_addr = inet_addr(CLIENT_IP);
	ansAddr.sin_port = htons(SERVER_PORT);
	/* Bind to the local address*/
	if ((bind(ServerSocket, (struct sockaddr *) &ansAddr,
		  sizeof(ansAddr))) < 0){
            printf("bind() failded.\n");
            exit(1);
    }
	char ans_buffer[1024];
	int ans_buffer_pointer = 0;
	struct sockaddr_in ClntAddr;
	//int port = ClntAddr.sin_port;
	memset(ans_buffer,'\0',1024);
	/* Set the size of the in-out parameter */
	unsigned int cliAddrLen = sizeof(ClntAddr);
	int UDP_msg_size = 0;
	/* Block until receive message from a client*/
	if ((UDP_msg_size = recvfrom(ServerSocket, ans_buffer, 1024,
		0, (struct sockaddr *) &ClntAddr, &cliAddrLen)) < 0)
			printf("recvform() failed,\n");
	close(ServerSocket);

	struct DNS_Header *ans_header = malloc(sizeof(DH));
	struct DNS_Query *ans_query = malloc(sizeof(DQ));
	struct DNS_RR *ans_RR = malloc(sizeof(DR));
	struct DNS_RR *author_RR = malloc(sizeof(DR));
	struct DNS_RR *add_RR = malloc(sizeof(DR));
	DecodeHeader(ans_header,ans_buffer,&ans_buffer_pointer);
	PrintHeader(ans_header);
	DecodeQuery(ans_query,ans_buffer,&ans_buffer_pointer);
	PrintQuery(ans_query);
	if(((ans_header->tag)&(0x0003))==0x0003){
		printf("NOT FOUND!\n");
		return 0;
	}
	for(int i=1;i<=ans_header->answerNum;i++){
		DecodeRR(ans_RR,ans_buffer,&ans_buffer_pointer);
		PrintRR(ans_RR);
	}
	if(((ans_header->tag)&(0x0400))==0x0000){
		printf("Find in cache\n");
		return 0;
	}
	if(argc==4&&strcmp(argv[1],"trace")==0){
		DecodeRR(author_RR,ans_buffer,&ans_buffer_pointer);
		//PrintRR(author_RR);
		DecodeRR(add_RR,ans_buffer,&ans_buffer_pointer);
		//PrintRR(add_RR);
		ans_header->addNum--;
		if(ans_query->qtype!=TYPE_A){
			DecodeRR(add_RR,ans_buffer,&ans_buffer_pointer);
			PrintRR(add_RR);
			ans_header->addNum--;
		}
		for(int i=1;i<=ans_header->addNum;i++){
			DecodeRR(add_RR,ans_buffer,&ans_buffer_pointer);
			//printf("%s\n",add_RR->rdata);
			judgeServer(add_RR);
		}
	}
	
	

	return 0;
}

void judgeServer(struct DNS_RR *rr){
	if(strcmp(rr->rdata,LOCAL_SERVER_IP)==0){
		printf("%s  %d  %s  %s  %s\n",rr->name,rr->ttl,"IN",numToType(rr->type),"local-server");
	}else if(strcmp(rr->rdata,ROOT_SERVER_IP)==0){
		printf("%s  %d  %s  %s  %s\n",rr->name,rr->ttl,"IN",numToType(rr->type),"root-server");
	}else if(strcmp(rr->rdata,TLDcnus_SERVER_IP)==0){
		printf("%s  %d  %s  %s  %s\n",rr->name,rr->ttl,"IN",numToType(rr->type),"TLD-server.cnus");
	}else if(strcmp(rr->rdata,TLDcom_SERVER_IP)==0){
		printf("%s  %d  %s  %s  %s\n",rr->name,rr->ttl,"IN",numToType(rr->type),"TLD-server.com");
	}else if(strcmp(rr->rdata,SECONDedu_SERVER_IP)==0){
		printf("%s  %d  %s  %s  %s\n",rr->name,rr->ttl,"IN",numToType(rr->type),"2ND-server.edu");
	}else if(strcmp(rr->rdata,SECONDgov_SERVER_IP)==0){
		printf("%s  %d  %s  %s  %s\n",rr->name,rr->ttl,"IN",numToType(rr->type),"2ND-server.gov");
	}
}

void printAns(struct DNS_RR *rr){
	printf("");
}