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

// int FirstFind(){
//     int find_flg=0;
//     struct DNS_RR *fileRR;
//     fileRR=malloc(sizeof(DR));
//     memset(fileRR,0,sizeof(DR));
//     fileRR->name=malloc(MAX_DOMAIN_LEN);
//     fileRR->rdata=malloc(MAX_DOMAIN_LEN);

//     while(fscanf(RR,"%s ",fileRR->name)!=EOF){
//         fscanf(RR,"%d",&fileRR->ttl);
//         char type[10],cls[10];
//         fscanf(RR,"%s ",cls);
//         fscanf(RR,"%s ",type);
//         fileRR->type=TypeTrans(type);
//         fscanf(RR,"%s\n",fileRR->rdata);
//         if(strcmp(recv_query->name,fileRR->name)==0 && (recv_query->qtype==fileRR->type)){
//             printf("Find in root cache.\n");
//             CreatRR(fileRR,fileRR->name,fileRR->type,0x0001,fileRR->ttl,0x0000,fileRR->rdata);
//             struct DNS_Header *header;
//             header=malloc(sizeof(DH));
//             unsigned short tag=CreateTag(1,0,1,0,0,0,0,0);
//             if(strcmp(type,"MX")==0){
//                 CreateHeader(header,0x0002,tag,0,1,0,1);
//             }
//             else{
//                 CreateHeader(header,0x0002,tag,0,1,0,0);
//             }
//             EncodeHeader(header,send_buf,&send_buf_pointer);
//             PinrtHeader(header);
//             EncodeRR(fileRR,send_buf,&send_buf_pointer);
//             PrintRR(fileRR);
//             find_flg=1;
//             break;
//         }
//     }
//     //回位
//     fseek(RR,0,0);
//     //MX类型
//     if(fileRR->type==TYPE_MX){
//         struct DNS_RR *addFileRR;
//         addFileRR=malloc(sizeof(DR));
//         addFileRR->name=malloc(MAX_DOMAIN_LEN);
//         addFileRR->rdata=malloc(MAX_DOMAIN_LEN);
//         while(fscanf(RR,"%s ",addFileRR->name)!=EOF){
//             fscanf(RR,"%d ",&addFileRR->ttl);
//             char type[10],cls[10];
//             fscanf(RR,"%s ",cls);
//             fscanf(RR,"%s ",type);
//             addFileRR->type=TypeTrans(type);
//             fscanf(RR,"%s\n",addFileRR->rdata);
//             if(strcmp(fileRR->rdata,addFileRR->name)==0){
//                 printf("find mx rr.\n");
//                 CreateRR(addFileRR,fileRR->rdata, 1, 1, fileRR->ttl, 0, addFileRR->rdata);
//                 EncodeRR(addFileRR,send_buf,send_buf_pointer);
//                 PrintRR(addFileRR);
//                 break;;
//             }
//         }
//     }
//     return find_flg; 
// }

    printf("[Connection established]\n");
    
    if ((recvMsgSize = recv(acceptfd, recv_buffer, sizeof(recv_buffer),0) < 0))
            printf("recvform() failed,\n");

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
                nextRR->type = TypeTrans(type);
                fscanf(RR,"%s\n",nextRR->rdata);
                if(strcmp(recv_query->name,nextRR->name)==0){//找到之后
                    printf("\n[SENT]ASK OTHER SERVER\n");
                    //生成头
                    struct DNS_Header *header;
                    header = malloc(sizeof(DH));
                    unsigned short tag = CreateTag(1,0,1,0,0,0,0,0);
                    CreateHeader(header,0x1235,tag,0,0,1,1);
                    EncodeHeader(header,send_buf,&send_buf_pointer);
                    PrintHeader(header);
                    
                    //生成authority RR  NS记录type=2   此时query_section->name经过cut后已经变成了下一个要去的DNS服务器域名
                    struct DNS_RR *authRR;
                    authRR = malloc(sizeof(DR));
                    CreateRR(authRR, domain, 2, 1, nextRR->ttl, 0, recv_query->name);
                    EncodeRR(authRR,send_buf,&send_buf_pointer);
                    PrintRR(authRR);
                    
                    //生成additon RR   A记录type=1
                    struct DNS_RR *addRR;
                    addRR = malloc(sizeof(DR));
                    CreateRR(addRR, domain, 1, 1, nextRR->ttl, 0, nextRR->rdata);
                    EncodeRR(addRR,send_buf,&send_buf_pointer);
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
        //发送
        sendto(recv_socket,send_buf,send_buf_pointer,0,(struct sockaddr*)&cli_addr,len);
        send_buf_pointer=0;
        recv_buf_pointer=0;
        memset(send_buf,0,1024);
        memset(recv_buf,0,1024);
    }
    printf("Not found.\n");
    //没找到应该回什么？
    out:
    fclose(RR);
    //
    char TCPBuffer[1024];
	unsigned short length = ntohs(len);
	memcpy(TCPBuffer,&length,2);
	memcpy(TCPBuffer+2,send_buffer,2+len);
    send(acceptfd,send_buffer,len+2,0);

    close(acceptfd);
    close(sockfd);
 
    return 0;
}
