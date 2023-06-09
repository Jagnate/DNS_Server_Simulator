#include "struct.h" 

#define MY_SERVER_IP SECONDedu_SERVER_IP
#define MY_TXT "edu.txt"

struct DNS_Header *recv_header;
struct DNS_Query *recv_query;
char recv_buffer[1024] = {0};
int recvMsgSize = 0;
char send_buffer[1024] = {0};
int send_buf_pointer=0;
int recv_buf_pointer=0;
FILE *RR;

void RecvTCP();
void SendTCP();
int FirstFind();
void NotFound();

int main(){
    while(1){
        memset(recv_buffer,0,1024);
        memset(send_buffer,0,1024);
        send_buf_pointer=0;
        recv_buf_pointer=0;
        recvMsgSize = 0;
        // memset(recv_header,0,sizeof(DH));
        // memset(recv_query,0,sizeof(DQ));

        RecvTCP();
        //解析header和query
        recv_header = malloc(sizeof(DH));
        unsigned short buf_len;
        buf_len=Get16Bits(recv_buffer,&recv_buf_pointer);
        DecodeHeader(recv_header,recv_buffer,&recv_buf_pointer);
        recv_query=malloc(sizeof(DQ));
        PrintHeader(recv_header);
        DecodeQuery(recv_query,recv_buffer,&recv_buf_pointer);
        
        //查询本层是否有answer
        RR=fopen(MY_TXT,"a+");
        int flg=FirstFind();
        if(flg==0){// 如果没找到则返回失败头
            NotFound();
        }
        fclose(RR);
        SendTCP();
    }
    return 0;
}

void RecvTCP(){
    //创建流式套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == sockfd){
        printf("socket error \n");
    }
    //填充信息本地信息结构体
    struct sockaddr_in ServAddr = {0}; 
    ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.s_addr = inet_addr(MY_SERVER_IP);
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
    
    //接收socket from local server
    int acceptfd = accept(sockfd, (struct sockaddr *)&ClntAddr, &clntlen);
    if(-1 == acceptfd){
        printf("accept error\n");
    }
    printf("[Connection established]\n");
    if ((recvMsgSize = recv(acceptfd, recv_buffer, sizeof(recv_buffer),0) < 0))
            printf("recvform() failed,\n");
    close(acceptfd);
    close(sockfd);
}

int FirstFind(){
    int find_flg=0;
    struct DNS_RR *fileRR;
    fileRR=malloc(sizeof(DR));
    memset(fileRR,0,sizeof(DR));
    fileRR->name=malloc(MAX_DOMAIN_LEN);
    fileRR->rdata=malloc(MAX_DOMAIN_LEN);

    fseek(RR,0,0);
    while(fscanf(RR,"%s ",fileRR->name)!=EOF){
        fscanf(RR,"%d",&fileRR->ttl);
        char type[10],cls[10];
        fscanf(RR,"%s ",cls);
        fscanf(RR,"%s ",type);
        fileRR->type=TypeToNum(type);
        fscanf(RR,"%s\n",fileRR->rdata);
        if(strcmp(recv_query->name,fileRR->name)==0 && (recv_query->qtype==fileRR->type)){
            printf("Find in edu server.\n");
            CreateRR(fileRR,fileRR->name,fileRR->type,0x0001,fileRR->ttl,0x0000,fileRR->rdata);
            struct DNS_Header *header;
            header=malloc(sizeof(DH));
            unsigned short tag=CreateTag(1,0,1,0,0,0,0,0);
            if(fileRR->type==TYPE_MX){
                CreateHeader(header,recv_header->id,tag,0,1,0,1);
                EncodeHeader(header,send_buffer,&send_buf_pointer);
                EncodeRR(fileRR,send_buffer,&send_buf_pointer);
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
                        EncodeRR(mxRR,send_buffer,&send_buf_pointer);
                    }
                }
            }
            else if(fileRR->type==TYPE_CNAME){
                CreateHeader(header,recv_header->id,tag,0,1,0,1);
                EncodeHeader(header,send_buffer,&send_buf_pointer);
                EncodeRR(fileRR,send_buffer,&send_buf_pointer);
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
                        EncodeRR(cname_RR,send_buffer,&send_buf_pointer);
                    }
                }
                
            }
            else{
                CreateHeader(header,recv_header->id,tag,0,1,0,0);
                EncodeHeader(header,send_buffer,&send_buf_pointer);
                EncodeRR(fileRR,send_buffer,&send_buf_pointer);
            }
            PrintHeader(header);
            PrintRR(fileRR);
            find_flg=1;
            break;
        }
    }
    return find_flg; 
}

void NotFound(){
    printf("Not found.\n");
    struct DNS_Header *header;
    header = malloc(sizeof(DH));
    unsigned short tag = CreateTag(1,0,1,0,0,0,0,1);
    CreateHeader(header,recv_header->id,tag,0,0,0,0);
    EncodeHeader(header,send_buffer,&send_buf_pointer);
    PrintHeader(header);
}

void SendTCP(){
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
    ServAddrTCP.sin_addr.s_addr = inet_addr(MY_SERVER_IP);
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
    //printf("%d",send_buf_pointer);
    memcpy(TCPBuffer,&length,2);
    memcpy(TCPBuffer+2,send_buffer,2+send_buf_pointer);
    ret = send(ServerSocketTCP,TCPBuffer,2+send_buf_pointer,0);
    close(ServerSocketTCP);
}