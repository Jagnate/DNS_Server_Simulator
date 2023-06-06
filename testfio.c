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
int main(){
    FILE *RR=fopen("rootserver.txt","a+");
    fseek(RR,0,0);  
    char* name = malloc(20);
    unsigned short ttl;
    char* rdata = malloc(20);
    //在RR记录中搜索
    while(fscanf(RR,"%s",name)!=EOF){
        fscanf(RR,"%d",&ttl);
        char type[10],cls[10];
        fscanf(RR,"%s ",cls);
        fscanf(RR,"%s ",type);
        fscanf(RR,"%s\n",rdata);
        printf("%s\n",name);
        printf("%s\n",cls);
        printf("%s\n",type);
        printf("%s\n",rdata);
        printf("%d\n",ttl);
    }
}