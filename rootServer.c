#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
#include <stdint.h>
#include <unistd.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <errno.h>  
// #include <netdb.h>  
#include <sys/types.h>  
// #include <sys/socket.h>  
#include<winsock.h>
#include<winsock2.h>
// #include <netinet/in.h>  
// #include <arpa/inet.h>
#include <windows.h>
#pragma comment(lib, "wsock32.lib")

#include "struct.h"

struct DNS_Header *recv_header;
struct DNS_Query *recv_query;
char recv_buf[1024];
char send_buf[1024];
int send_buf_pointer=0;
int recv_buf_pointer=0;
int recv_socket;
struct sockaddr_in root_addr,cli_addr;
unsigned int len;
void RecvSocket(){
    recvfrom(recv_socket,recv_buf,sizeof(recv_buf),0,(struct sockaddr*)&cli_addr,&len);
    printf("Recived socket request.\n");
    recv_header = malloc(sizeof(DH));
    DecodeHeader(recv_header,recv_buf,&recv_buf_pointer);
    recv_query=malloc(sizeof(DQ));
    DecodeQuery(recv_query,recv_buf,&recv_buf_pointer);
}

void FirstFind(){

    FILE *RR=fopen("rootserver.txt","a+");
    struct DNS_RR *fileRR;
    fileRR=malloc(sizeof(DR));
    memset(fileRR,0,sizeof(DR));
    fileRR->name=malloc(MAX_DOMAIN_LEN);
    fileRR->rdata=malloc(MAX_DOMAIN_LEN);

    while(fscanf(RR,"%s ",fileRR->name)!=EOF){
        fscanf(RR,"%d",&fileRR->ttl);
        char type[10],cls[10];
        fscanf(RR,"%s ",cls);
        fscanf(RR,"%s ",type);
        fileRR->type=TypeTrans(type);
        fscanf(RR,"%s\n",fileRR->rdata);
        if(strcmp(recv_query->name,fileRR->name)==0 && (recv_query->qtype==fileRR->type)){
            printf("Find in root cache.\n");
            CreatRR(fileRR,fileRR->name,fileRR->type,0x0001,fileRR->ttl,0x0000,fileRR->rdata);
            struct DNS_Header *header;
            header=malloc(sizeof(DH));
            unsigned short tag=CreateTag(1,0,1,0,0,0,0,0);
            if(strcmp(type,"MX")==0){
                CreateHeader(header,0x0002,tag,0,1,0,1);
            }

        }
    }
    
}

int main(){

    memset(recv_buf,0,1024);
    memset(send_buf,0,1024);
    recv_socket=socket(AF_INET,SOCK_DGRAM,0);
    root_addr.sin_family=AF_INET;
    root_addr.sin_port=htons(ROOT_SERVER_PORT);
    root_addr.sin_addr.s_addr=inet_addr(ROOT_SERVER_IP);
    bind(recv_socket,(struct sockaddr*)&root_addr,sizeof(root_addr));
    len=sizeof(cli_addr);
    while(1){
        RecvSocket();
        struct DNS_RR *RR_answer, *RR_authority, *RR_additional;
        char *domain = recv_query->name;
        FirstFind();

    }

    close(recv_socket);

    return 0;
}