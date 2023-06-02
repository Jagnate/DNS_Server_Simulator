#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> 
#include <stdlib.h>


struct DNS_Header{
    unsigned short id;
    unsigned short tag;
    unsigned short queryNum;
    unsigned short answerNum;
    unsigned short authorNum;
    unsigned short addNum;
};

struct DNS_Query{
    int length;
    unsigned char *name;
    unsigned short qtype;
    unsigned short qclass;
};

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
	if (qr==1)  tag = tag | 0x8000;
	if (aa==1)  tag = tag | 0x0400;
	if (tc==1)  tag = tag | 0x0200;
	if (rd==1)  tag = tag | 0x0100;
	if (ra==1)  tag = tag | 0x0080;
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
    if(queryNum!=0x0000&answerNum==0x0000){
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

//创建question
//hostname:www.baidu.com
//name:3www5baidu3com'\0'
int CreateQuery(struct DNS_Query* query_section,const char* domain_name,unsigned short qtype,unsigned short qclass){
    if(query_section==NULL||domain_name==NULL) return -1;
    memset(query_section,0,sizeof(struct DNS_Query));
    query_section->name = (char*)malloc(strlen(domain_name)+2);//因为要判断结尾'\0'，然后再补充一个开头
    if(query_section->name==NULL){//如果内存分配失败
        return -2;
    }
    query_section->length=strlen(domain_name)+2;
    query_section->qtype=htons(qtype);//查询类型，（1表示：由域名获得 IPv4 地址）
    query_section->qclass=htons(qclass);//通常为 1，表明是 Internet 数据

    //hostname->name
    const char delim[2]=".";//分隔符,末尾补个'\0'
    char* qname = query_section->name;
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
    return 0;
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

unsigned short TypeTrans(char* type)
{
	if (strcmp(type,"A")==0) return 0x0001;
	if (strcmp(type,"NS")==0) return 0x0002;
	if (strcmp(type,"CNAME")==0) return 0x0005;
	if (strcmp(type,"MX")==0) return 0x000F;
	return -1;
}