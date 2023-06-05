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
// #include<ws2tcpip.h>
// #include<winsock2.h>
// #include <windows.h>
// #pragma comment(lib, "wsock32.lib")

#define SERVER_PORT 53
//local server ip
#define LOCAL_SERVER_IP "127.1.1.1"
//root server ip
#define ROOT_SERVER_IP "127.2.2.1"

#define TLDcnus_SERVER_IP "127.3.3.1"

#define TLDcom_SERVER_IP "127.4.4.1"

#define SECONDedu_SERVER_IP "127.5.5.1"

#define SECONDgov_SERVER_IP "127.6.6.1"

#define MAX_DOMAIN_LEN 100

#define TYPE_A 0x0001
#define TYPE_NS 0x0002
#define TYPE_CNAME 0x0005
#define TYPE_MX 0x000F


char domain_value[MAX_DOMAIN_LEN];

struct DNS_Header{
    unsigned short id;
    unsigned short tag;
    unsigned short queryNum;
    unsigned short answerNum;
    unsigned short authorNum;
    unsigned short addNum;
}DH;

struct DNS_Query{
    int length;
    unsigned char *name;
    unsigned short qtype;
    unsigned short qclass;
}DQ;

struct DNS_RR{
	char *name;   
	unsigned short type;     //请求的域名
	unsigned short _class;      //响应的资源记录的类型 一般为[IN:0x0001]
	unsigned int ttl;        //该资源记录被缓存的秒数。
	unsigned short data_len; //RDATA部分的长度
	unsigned short pre;      //MX特有的优先级 Preference
	char *rdata;	         //[A:32位的IP地址（4字节）] [CNAME/NS/MX:域名]
}DR;

unsigned short CreateTag(   unsigned short qr,       //[1]标示该消息是请求消息（该位为0）还是应答消息（该位为1）
                            unsigned short opcode,   //[4]0000 标准查询为0 反向查询微1
                            unsigned short aa,       //[1]只在响应消息中有效。该位标示响应该消息的域名服务器是该域中的权威域名服务器。因为Answer Section中可能会有很多域名
                            unsigned short tc,       //[1]标示这条消息是否因为长度超过UDP数据包的标准长度512字节，如果超过512字节，该位被设置为1
                            unsigned short rd,       //[1]1 是否递归查询。1为递归查询
                            unsigned short ra,       //[1]1 在响应消息中清除并设置。标示该DNS域名服务器是否支持递归查询。
                            unsigned short z,        //[3]000   冗余res 0
                            unsigned short rcode)    //[4]0000  成功的响应
{
	unsigned short tag = 0;
	if (qr==1)      tag = tag | 0x8000;
    if (opcode==1)  tag = tag | 0x0800;
	if (aa==1)      tag = tag | 0x0400;
	if (tc==1)      tag = tag | 0x0200;
	if (rd==1)      tag = tag | 0x0100;
	if (ra==1)      tag = tag | 0x0080;
	return tag;
}

int CreateHeader(struct DNS_Header *header_section, 
                    unsigned short id,
                    unsigned short tag, 
                    unsigned short queryNum,
                    unsigned short answerNum,
                    unsigned short authorNum,
                    unsigned short addNum)
{
    if(header_section == NULL) return -1;
    memset(header_section, 0, sizeof(struct DNS_Header));
	//ID随机random
	//srandom(time(NULL)); 
    if(queryNum!=0x0000&&answerNum==0x0000){
        header_section->id = htons(random());
    }else{
	    header_section->id = htons(id);        
    }
	header_section->tag = htons(tag); //标准查询
	header_section->queryNum = htons(queryNum);   //只查1个结果
    header_section->answerNum = htons(answerNum); 
    header_section->authorNum = htons(authorNum); 
    header_section->addNum = htons(addNum); 
	return 0;
}

void EncodeDomain(char* qname,const char* domain_name){
    const char delim[2]=".";//分隔符,末尾补个'\0'
    char* domain_name_dup=strdup(domain_name);//复制一份hostname  --->malloc(所以后续要free)
    char* token=strtok(domain_name_dup,delim);
    while(token!=NULL){
        size_t len=strlen(token);//第一个循环token为www,len=3
        *qname=len;//先把长度放上去
        qname++;
        strncpy(qname,token,len+1);//复制www，这里不+1也是可以的，这样是为了把最后的'\0'也复制过来,因为最后也会被覆盖的。(如果这边不+1，最后一步，需要额外加上'\0')
        qname+=len;
        token=strtok(NULL,delim);//因为上一次，token获取还未结束，因此可以指定NULL即可。(注意：要依赖上一次的结果，因此也是线程不安全的)
    }
    free(domain_name_dup);
}

//创建question
//hostname:www.baidu.com
//name:3www5baidu3com'\0'
int CreateQuery(struct DNS_Query* query_section,const char* domain_name,unsigned short qtype,unsigned short qclass){
    if(query_section==NULL||domain_name==NULL) return -1;
    memset(query_section,0,sizeof(struct DNS_Query));
    query_section->name = (unsigned char*)malloc(strlen(domain_name)+2);//因为要判断结尾'\0'，然后再补充一个开头
    if(query_section->name==NULL){//如果内存分配失败
        return -2;
    }
    query_section->length=strlen(domain_name)+2;
    query_section->qtype=htons(qtype);//查询类型，（1表示：由域名获得 IPv4 地址）
    query_section->qclass=htons(qclass);//通常为 1，表明是 Internet 数据
    char* qname = (char *)query_section->name;
    EncodeDomain(qname,domain_name);
    //hostname->name
    return 0;
}

void CreateRR(struct DNS_RR *RR,char* name, unsigned short type, unsigned short _class, unsigned int ttl, unsigned short pre,char *rdata){
    //unsigned short pre为一个MX类型特有的优先级，定长，只有MX类型发送。
	int domain_length = strlen(name);
	//易错点：strlen只读到0但不包含0，所以为了把结束符也复制进去，长度要+1
	RR->name = malloc(domain_length+1);   
	memcpy(RR->name,name,domain_length+1);
	
	RR->type = type;
	RR->_class = _class;
	RR->ttl = ttl;       //data_len
	if (type==TYPE_A) RR->data_len=4;  //对于IP，长度为4 data_len是编码后的长度，length是非编码长度，注意
		else RR->data_len = strlen(rdata) + 2;      //对于域名，生成data_len包含末尾结束符（域名末尾结束符）
	
	//pre
	if (type==TYPE_MX) {
		RR->pre = pre;
		RR->data_len += 2;  //对于邮件类型，由于有pre的存在，多占两个字节
	}
	
	//char* rdata
	int rdata_length = strlen(rdata);  //要加上末尾结束符
	RR->rdata = malloc(rdata_length+1);
	memcpy(RR->rdata,rdata,rdata_length+1);
}

//把上面两个合到request中 返回长度
int MergeRequest(struct DNS_Header* header_section,struct DNS_Query* query_section,char* request,int rlen){
    if(header_section==NULL||query_section==NULL||request==NULL) return -1;
    memset(request,0,rlen);

    //header-->request
    memcpy(request,header_section,sizeof(struct DNS_Header));//把header的数据 拷贝 到request中
    int offset=sizeof(struct DNS_Header);

    //question-->request
    memcpy(request+offset,query_section->name,query_section->length);
    offset+=query_section->length;
    memcpy(request+offset,&query_section->qtype,sizeof(query_section->qtype));
    offset+=sizeof(query_section->qtype);
    memcpy(request+offset,&query_section->qclass,sizeof(query_section->qclass));
    offset+=sizeof(query_section->qclass);
    query_section->length = offset;
    return offset;
}


unsigned short TypeToNum(char* type){
	if (strcmp(type,"A")==0) return TYPE_A;
	if (strcmp(type,"NS")==0) return TYPE_NS;
	if (strcmp(type,"CNAME")==0) return TYPE_CNAME;
	if (strcmp(type,"MX")==0) return TYPE_MX;
	return -1;
}

char* numToType(unsigned short num){
	if (num==0x0001) return "A";
	if (num==0x0002) return "NS";
	if (num==0x0005) return "CNAME";
	if (num==0x000F) return "MX";
	return "ERROR";
}
unsigned short Get16Bits(char *buffer,int *buffer_pointer){
    unsigned short value;
    memcpy(&value,buffer+*buffer_pointer,2);
    *buffer_pointer+=2;
    return ntohs(value);
}

void Put16Bits(char *buffer,int *buffer_pointer, unsigned short value){
	value = htons(value);
	memcpy(buffer + *buffer_pointer,&value,2);
	*buffer_pointer += 2;
}

void Put32Bits(char *buffer,int *buffer_pointer, unsigned short value){
	value = htons(value);
	memcpy(buffer + *buffer_pointer,&value,4);
	*buffer_pointer += 4;
}

void PutDomainName(char *buffer,int *buffer_pointer, char *str){
	memcpy(buffer + *buffer_pointer,str,strlen(str)+1); //末尾0需要一起打印
	*buffer_pointer += strlen(str)+1;
}

void EncodeHeader(struct DNS_Header *header,char *buffer,int *buffer_pointer){
    memcpy(buffer,header,12);
    *buffer_pointer+=12;
}

void EncodeRR(struct DNS_RR *RR,char *buffer, int *buffer_pointer){
    char *domain_name = RR->name;
	// int lengthOfEncodedDomain = strlen(RR->name)+2;
	// domain_name = malloc(lengthOfEncodedDomain);
	 
	EncodeDomain(domain_name,RR->name);
	//memcpy(domain_name,domain_value,lengthOfEncodedDomain);
	
	
    PutDomainName(buffer,buffer_pointer,domain_name); 
	
	Put16Bits(buffer,buffer_pointer,RR->type);
	Put16Bits(buffer,buffer_pointer,RR->_class);
	Put32Bits(buffer,buffer_pointer,RR->ttl);
	Put16Bits(buffer,buffer_pointer,RR->data_len);   
	if (RR->type==0x000F) 
		Put16Bits(buffer,buffer_pointer,RR->pre);
		
	//如果类型为A，发送的是IP，将IP写入缓冲区               
	if(RR->type == 0x0001)         {
		//不能调用get put函数，因为inet_addr自带字节序变换功能
		unsigned int rdata = inet_addr(RR->rdata);
		memcpy(buffer + *buffer_pointer,&rdata,4);
		*buffer_pointer += 4;
	
	}else{          
	//如果类型为MX、CNAME、NS
	//则发送的是域名，则调用域名编码
	//char* rdata
		char *rdata;
		//printf("rdata:[%s]\n",resource_record->rdata); //for test
		int lengthOfEncodedDomain2 = strlen(RR->rdata)+2;
		//printf("length:%d\n",lengthOfEncodedDomain2); //for test
		rdata = malloc(lengthOfEncodedDomain2);
		//printf("encodedomain:[%s]\n",encodeDomain(resource_record->rdata)); //encodeDomain函数周期性抽风 测试文件在test4
		EncodeDomain(rdata,RR->rdata);
		//memcpy(rdata,domain_value,lengthOfEncodedDomain2);   
		//printf("rdata:[%s]\n",rdata);    //这里已经错误
		PutDomainName(buffer,buffer_pointer,rdata); 
	}
}

void DecodeHeader(struct DNS_Header *header,char *buffer,int *buffer_pointer){
    header->id=Get16Bits(buffer,buffer_pointer);
    header->tag=Get16Bits(buffer,buffer_pointer);
    header->queryNum=Get16Bits(buffer,buffer_pointer);
    header->answerNum=Get16Bits(buffer,buffer_pointer);
    header->authorNum=Get16Bits(buffer,buffer_pointer);
    header->addNum=Get16Bits(buffer,buffer_pointer);
}

void DecodeDomain(char* domain){
	memset(domain_value,0,MAX_DOMAIN_LEN);
	int cnt = 0;
	char *p = domain;  
	int count = *p;
	while(count!=0){
		for(int i=0;i<count;i++){
			p += 1;
			domain_value[cnt] = *p;
			cnt++;
		}
		if (*(p+1)!=0) {
			domain_value[cnt] = '.';
			cnt++;
		}
		p += 1;
		count = *p;
	}
	domain_value[cnt]=0;
}

void GetDomainName(char *buffer,int *buffer_pointer,int *lengthOfDomain){
	
	int cnt=0;
	while(buffer[*buffer_pointer]!=0){
		domain_value[cnt] = buffer[*buffer_pointer]; 
		cnt++;
		(*buffer_pointer)++;
	}
	domain_value[cnt] = 0; //末尾为0，写入字符串结束符，方便对字符数组进行字符串操作
	(*buffer_pointer)++; //缓冲区读写下一位指针指示跳过末尾0
	*lengthOfDomain = cnt+1; //包含了末尾结束符 
	//printf("value in function: %s\n",value);
	
}

void DecodeQuery(struct DNS_Query *query, char *buffer,int *buffer_pointer){

    char* domain_name = malloc(MAX_DOMAIN_LEN); 
	memset(domain_name,0,MAX_DOMAIN_LEN);
	int lengthOfDomain=0;
	GetDomainName(buffer,buffer_pointer,&lengthOfDomain);
	memcpy(domain_name,domain_value,lengthOfDomain);
	
	//解码域名
	DecodeDomain(domain_name);
	memcpy(domain_name,domain_value,strlen(domain_name));  
	
	query->name = domain_name;
	query->qtype = Get16Bits(buffer,buffer_pointer);
	query->qclass = Get16Bits(buffer,buffer_pointer);
}

void PrintHeader(struct DNS_Header *header){
    printf("=======DNS HEADER INFOMATION=======\n");
    printf("ID:                   %d\n",ntohs(header->id));
    printf("TAG:                  0x%x\n",ntohs(header->tag));
    printf("QueryNum:             %d\n",ntohs(header->queryNum));
    printf("AnswerNum:            %d\n",ntohs(header->answerNum));
    printf("AuthorNum:            %d\n",ntohs(header->authorNum));
    printf("AddNum:               %d\n",ntohs(header->addNum));
    printf("===================================\n");
}

void PrintRR(struct DNS_RR *resource_record){
	//转码utf8
	//wchar_t *name;
	//HZDomainTransform(name,resource_record->name);
	printf("=========DNS RR INFOMATION=========\n");
	printf("Name:                 [%s]\n",resource_record->name);
	printf("Type:                 [%s]\n",numToType(resource_record->type));
	printf("Class:                [IN]\n");
	printf("TTL:                   %d\n",resource_record->ttl);
	printf("Data_Len:              %d\n",resource_record->data_len);
	if (resource_record->type==0x000F) 
		printf("Pre:                    0x%x\n",resource_record->pre);
	printf("IP/DOMAIN:            [%s]\n",resource_record->rdata);
	printf("===================================\n");
}

void CutDomain(char** domain_pointer){
    while(1){
		(*domain_pointer)++;
		if (**domain_pointer=='.'){
			(*domain_pointer)++;
			break;		
		}
		if (**domain_pointer==0){
			*domain_pointer = NULL;
			break;
		}
	}
}