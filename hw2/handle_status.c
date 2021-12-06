#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "handleshort.c"

void signhander(int signal){
	if(signal==SIGCLD){
		
	}
}
struct user{
	int userid;
	int status;//0 登出 1登入 2遊戲中
	int socketfd;
};
struct user alllist[1024];
int user_count=0;
int main(int argc , char *argv[]){
    //socket的建立
    int sockfd = 0,clientfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("create socket error\n");
    }
    signal(SIGCLD, signhander);
    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(60001);
    int socket_buffer;
    setsockopt(catchfd,SOL_SOCKET,SO_REUSEADDR,&socket_buffer,sizeof(socket_buffer));
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    if (listen(sockfd,1024)<0){
		printf("listen error\n",);
		exit(0);
	}

    while(1){
    	if((clientfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen))<0){
    		printf("accept error\n");
    	}
        fd=fork();
        if(fd<0){
        	printf("fork error\n");
        }else if(fd==0){//child
        	close(sockfd);
        	char id[6];
        	read(clientfd,id,6);
        	
        	int now=handleshort(id,clientfd);
        	
        	exit(0);
        }else{
		    
        }
        
    }
    return 0;
}
