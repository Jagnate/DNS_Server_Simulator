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

    int sock;
	struct sockaddr_in ServAddr;
	//struct sockaddr_in fromAddr;
	
	char RequestBuffer[1024];
	unsigned short ServPort;
	//unsigned int fromSize;

	memset(RequestBuffer,0,1024);

    if(argc != 3)
	{
		printf("Usage: %s <DNS> <Type>\n",
			       argv[0]);
		exit(1);
	}

    ServPort = 53;

    /* Create a datagram/UDP socekt */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		printf("socket() failed.\n");
	/* Construct the server address structure */
	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.s_addr = inet_addr("127.1.1.1");
	ServAddr.sin_port = htons(ServPort);

	/* Create header and query */
	struct DNS_Header header_section = {0};
	//对应: qr,opcode,aa,tc,rd,ra,z,rcode，其中opcode z rcode可以随便填写. 默认不超过512，tc为1
	CreateHeader(&header_section,0,CreateTag(0,0,0,0,0,1),1,0,0,0);
	struct DNS_Query query_section = {0};
	unsigned short qtype = TypeTrans(argv[2]);
	unsigned short qclass = 0x0001;	//Class in
	CreateQuery(&query_section,argv[1],qtype,qclass);
	int length = MergeRequest(&header_section,&query_section,RequestBuffer,1024);

	/* Send the string to the server*/
	sendto(sock,RequestBuffer, length, 0,
			(struct sockaddr *) &ServAddr, sizeof(ServAddr));
	/* null-terminate the received data */
	close(sock);
	return 0;
}