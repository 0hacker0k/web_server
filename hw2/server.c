#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#define BUFSIZE 1024
#define MAXUSER 1024
struct user{
	int id;
	char account[30];
	char password[50];
	int status;//0 登出 1登入 2遊戲中
	int socketfd;
	char token[50];
};
struct user *usrlist;
int usercount;
int myhash(char* password){
	int passlen=strlen(password);
	int hashnum=0;
	int table[15]={13,5,7,39,17,29,2,47,41,31,23,19,3,11,43};
	for(int i=0;i<passlen;i++){
		hashnum+=(password[i]-48)*table[i%15]+i;
	}
	return hashnum;
}
int myhash2(char* password){
	int passlen=strlen(password);
	int hashnum=0;
	int table[17]={13,5,53,7,39,17,29,2,47,41,31,67,23,19,3,11,43};
	for(int i=0;i<passlen;i++){
		hashnum+=(password[i]-48)*table[i%17]+i;
	}
	return hashnum;
}
void* child(void* data) {
	pthread_detach(pthread_self());//讓parent不用等待
	int *fd = (int*) data; // 取得輸入資料
	char buffer[BUFSIZE];
	char temp[BUFSIZE];
	int ret=read(*fd,buffer,sizeof(buffer));
	if(ret==0){
		for(int i=0;i<usercount;i++){
			if(*fd==usrlist[i].socketfd){
				usrlist[i].status=0;
				usrlist[i].socketfd=-1;
				usrlist[i].token[0]=0;
				printf("%s has left.\n",usrlist[i].account);
			}
		}
		close(*fd);
		pthread_exit(NULL);
	}
	printf("%s\n",buffer);
	char account[30],content[50],token[50],command[7];
	sscanf(buffer,"%s %s %s %s",token,account,command,content);
	int status=1;
	int id=-1;
	if(strcmp(command,"login")==0){
		for(int i=0;i<usercount;i++){
			if(strcmp(account,usrlist[i].account)==0){
				status=0;
				if(myhash2(content)==atoi(usrlist[i].password)){
					usrlist[i].status=1;
					srand(time(NULL));
					usrlist[i].socketfd=*fd;
					sprintf(usrlist[i].token,"%d",myhash(account)+rand()%323);
					sprintf(buffer,"/3 ");
					strcat(buffer,usrlist[i].token);
					write(*fd,buffer,strlen(buffer));
				}else{
					write(*fd,"/0 Password error.\n",19);
					shutdown(*fd,SHUT_WR);
					close(*fd);
				}
			}
		}if(status==1){
			write(*fd,"/0 Can't find the account.\n",27);
			shutdown(*fd,SHUT_WR);
			close(*fd);
		}
	}else if(strcmp(command,"signup")==0){
		usercount+=1;
		usrlist[usercount-1].id=usercount;
		strcpy(usrlist[usercount-1].account,account);
		sprintf(usrlist[usercount-1].password,"%d",myhash2(content));
		usrlist[usercount-1].status=1;
		usrlist[usercount-1].socketfd==*fd;
		sprintf(usrlist[usercount-1].token,"%d",myhash(account)+rand()%323);
		FILE* wtfd=fopen("user.txt","r+");
		fseek(wtfd,0,SEEK_END);
		fprintf(wtfd,"%d %s %s\n",usrlist[usercount-1].id,usrlist[usercount-1].account,usrlist[usercount-1].password);
		fseek(wtfd,0,SEEK_SET);
		fprintf(wtfd,"%d\n",usercount);
		fclose(wtfd);
		sprintf(buffer,"/3 ");
		strcat(buffer,usrlist[usercount-1].token);
		write(*fd,buffer,strlen(buffer));
	}else{
		for(int i=0;i<usercount;i++){//驗證
			if(token[0]==0){
				status=-2;
				break;
			}
			if(strcmp(usrlist[i].account,account)==0){
				status=-1;
				if(strcmp(usrlist[i].token,token)==0){
					status=0;
					id=i;
					break;
				}
			}
		}
		if(status==-1){//有憑證 有帳號 憑證錯誤
			write(*fd,"/0 Your certificate has expired, or account has been logged in elsewhere.\n",74);
			shutdown(*fd,SHUT_WR);
			close(*fd);
		}else if(status==1){//有憑證 沒帳號
			write(*fd,"/0 No such account.\n",20);
			shutdown(*fd,SHUT_WR);
			close(*fd);
		}else if(status==-2){//沒有憑證
			write(*fd,"/0 You must log in first.\n",26);
			shutdown(*fd,SHUT_WR);
			close(*fd);
		}else{
			if(strcmp(command,"logout")==0){//登出
				write(*fd,"/2 ",3);
				usrlist[id].status=0;
				usrlist[id].socketfd=-1;
				usrlist[id].token[0]=0;
				shutdown(*fd,SHUT_WR);
				close(*fd);
			}else if(strcmp(command,"list")==0){
				sprintf(buffer,"/2 ");
				int x=0;
				for(int i=0;i<usercount;i++){
					if(usrlist[i].socketfd==*fd)continue;
					if(usrlist[i].status==1 || usrlist[i].status==-1){
						x=1;
						sprintf(temp,"\t[%d]\t%s\tonline\n",i,usrlist[i].account);
						strcat(buffer,temp);
					}else if(usrlist[i].status==2 || usrlist[i].status==-2){
						x=1;
						sprintf(temp,"\t[%d]\t%s\tplaying\n",i,usrlist[i].account);
						strcat(buffer,temp);
					}
				}if(x==0){
					sprintf(temp,"No players are online except you.\n");
					strcat(buffer,temp);
					write(*fd,buffer,sizeof(buffer));
				}
				usrlist[id].status=1;
			}else if(strcmp(command,"play")==0){
				int len=strlen(content);
				int x=0;
				for(int i=0;i<len;i++){
					if(content[i]>57 || content[i]<48){
						x=1;
						break;
					}
				}
				usrlist[id].status=-2;
			}
		}
	}
	pthread_exit(NULL); // 離開子執行緒
}
void* check(void* data) {
	pthread_detach(pthread_self());//讓parent不用等待
	
	struct timeval tv;
	fd_set readfds;
	tv.tv_sec = 0;//設定時間
	tv.tv_usec = 0;
	int max=-1;
	char buffer[2];
	while(1){
		FD_ZERO(&readfds);//清除set
		max=-1;
		for(int i=0;i<usercount;i++){
			if(usrlist[i].status!=0 && usrlist[i].status!=-1 && usrlist[i].status!=-2){
				if(max<usrlist[i].socketfd)
					max=usrlist[i].socketfd;
				FD_SET(usrlist[i].socketfd, &readfds);//加入set
			}
		}
		if(max!=-1){//有任何現存的連線才偵測
			int has_rd=select(max+1, &readfds, NULL, NULL, &tv);
			if(has_rd==-1){//監聽set
				printf("error in select.%d\n",errno);
				perror("select");
			}else if(has_rd>0){
				printf("get something.\n");
				for(int i=0;i<usercount;i++){//看看是誰可以讀取了
					if(FD_ISSET(usrlist[i].socketfd, &readfds)){
						usrlist[i].status=-1;
						printf("get %d from %s.\n",usrlist[i].socketfd,usrlist[i].account);
						pthread_t t; // 宣告 pthread 變數
						pthread_create(&t, NULL, child, &usrlist[i].socketfd);
					}
				}
			}
		}
	}
	pthread_exit(NULL); // 離開子執行緒
}
int main(int argc , char *argv[]){
    //socket的建立
    int sockfd = 0,clientfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("create socket error\n");
    }
	signal(SIGCLD, SIG_IGN);
    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(60000);
    int socket_buffer;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&socket_buffer,sizeof(socket_buffer));
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    if (listen(sockfd,1024)<0){
		printf("listen error\n");
		exit(0);
	}
	//讀取帳號密碼
	FILE* rdfd=fopen("user.txt","r");
	fscanf(rdfd,"%d",&usercount);
	usrlist=malloc(sizeof(struct user)*MAXUSER);
	for(int i=0;i<usercount;i++){
		fscanf(rdfd,"%d",&(usrlist[i].id));//id account password
		fscanf(rdfd,"%s",usrlist[i].account);
		fscanf(rdfd,"%s",usrlist[i].password);
		usrlist[i].status=0;
		usrlist[i].socketfd=-1;
		usrlist[i].token[0]=0;
	}
	fclose(rdfd);
	pthread_t t; // 宣告 pthread 變數
	pthread_create(&t, NULL, check, NULL);
    while(1){
    	if((clientfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen))<0){
    		printf("accept error\n");
    	}
    	pthread_create(&t, NULL, child, &clientfd);
    	
        /*fd=fork();
        if(fd<0){
        	printf("fork error\n");
        }else if(fd==0){//child
        	close(sockfd);
        	handlefd(clientfd);
        	shutdown(clientfd,SHUT_WR);
        	close(clientfd);
        	exit();
        }else{
		    
        }*/
        
    }
    return 0;
}
