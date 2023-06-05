#include "struct.h"

int main(int argc, char *argv[]){

    

    if(argc != 3)
	{
		printf("Usage: %s <DNS> <Type>\n",
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
	unsigned short qtype = TypeToNum(argv[2]);
	unsigned short qclass = 0x0001;	//Class in
	CreateQuery(&query_section,argv[1],qtype,qclass);
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
	DecodeHeader(ans_header,ans_buffer,&ans_buffer_pointer);
	DecodeQuery(ans_query,ans_buffer,&ans_buffer_pointer);
	DecodeRR(ans_RR,ans_buffer,&ans_buffer_pointer);
	PrintHeader(ans_header);
	PrintRR(ans_RR);
	return 0;
}