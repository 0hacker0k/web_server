#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define bufsize 1024
char account[30],password[50],token[50];
int sockfd=-1;
int status=0;
struct sockaddr_in info;
int catch();
int myhash(char* password){
	int passlen=strlen(password);
	int hashnum=0;
	int table[15]={13,5,7,39,17,29,2,47,41,31,23,19,3,11,43};
	for(int i=0;i<passlen;i++){
		hashnum+=(password[i]-48)*table[i%15]+i;
	}
	return hashnum;
}

void sockcreate(char *buffer){
	sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.\n");
    }

    //socket的連線

    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_port = htons(60000);

	int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error");
        return ;
    }
	send(sockfd,buffer,strlen(buffer),0);
    printf("Connection established.\n");
    return ;
}
int listen_all(char *output){
	fd_set readfds;
	FD_ZERO(&readfds);
	if(sockfd!=-1)FD_SET(sockfd, &readfds);
	if(output!=NULL)FD_SET(0, &readfds);
	int has_rd;
	struct timeval tv;
	if(output!=NULL && output[0]==0){
		tv.tv_sec = 30;//設定時間
		tv.tv_usec = 0;
		has_rd=select((sockfd==-1)?1:sockfd+1, &readfds, NULL, NULL, &tv);
	}else{
		has_rd=select((sockfd==-1)?1:sockfd+1, &readfds, NULL, NULL, 0);
	}
	if(has_rd==-1){//監聽set
		perror("select");
	}else if(has_rd>0){
		if(FD_ISSET(sockfd, &readfds)){
			if(output!=NULL)output[0]=0;
			int c=catch();
			//printf("get socket, return %d\n",c);
			return c;
		}else if(output!=NULL && FD_ISSET(0, &readfds)){
			//printf("get type\n");
			fgets(output,1024,stdin);
			if(output[strlen(output)-1]=='\n')output[strlen(output)-1]='\0';
			return -2;
		}
	}else if(has_rd==0){
		return -1;
	}
}
int submit(int command,char* content){
	char buffer[bufsize];
	if(command==1){//登入
		sprintf(buffer,"0 %s %s %d",account,"login",myhash(content));
		sockcreate(buffer);
		if(catch()==3){
			printf("Login success.\n");
			status=1;
		}
	}else if(command==2){//註冊
		sprintf(buffer,"0 %s %s %d",account,"signup",myhash(content));
		sockcreate(buffer);
		if(catch()==3){
			printf("Sign up success.\n");
			status=1;
		}
	}else if(command==3){//清單
		sprintf(buffer,"%s %s %s 0",token,account,"list");
		send(sockfd,buffer,sizeof(buffer),0);
		catch();
	}else if(command==4){//準備挑戰
		sprintf(buffer,"%s %s %s 0",token,account,"plist");
		send(sockfd,buffer,sizeof(buffer),0);
		return catch();
	}else if(command==5){//登出
		sprintf(buffer,"%s %s %s 0",token,account,"logout");
		send(sockfd,buffer,sizeof(buffer),0);
		if(catch()==2){
			printf("Logout success.\n");
			status=0;
		}
	}else if(command==6){//發起挑戰
		sprintf(buffer,"%s %s %s %s",token,account,"play",content);
		send(sockfd,buffer,sizeof(buffer),0);
		buffer[0]=0;
		int x=listen_all(buffer);
		if(x==1 || x==-1){
			status=1;
			submit(10,content);
			return 0;
		}else if(x==2){
			return 1;
		}
	}else if(command==7){//回應ok
		sprintf(buffer,"%s %s %s %s",token,account,"ok",content);
		send(sockfd,buffer,sizeof(buffer),0);
		if(catch()==2){
			printf("Teleport you to the game room...\n");
			status=2;
		}
	}else if(command==8){//回應no
		sprintf(buffer,"%s %s %s %s",token,account,"no",content);
		send(sockfd,buffer,sizeof(buffer),0);
	}else if(command==9){//聊天 or 遊戲
		sprintf(buffer,"%s %s",token,content);
		send(sockfd,buffer,sizeof(buffer),0);
	}else if(command==10){//取消遊戲請求
		sprintf(buffer,"%s %s %s %s",token,account,"cancel",content);
		send(sockfd,buffer,sizeof(buffer),0);
	}
	return 0;
}

int catch(){
	char buffer[bufsize];
	int ret=recv(sockfd,buffer,sizeof(buffer),0);
	buffer[ret]=0;
	if(ret==0){
		printf("Disconnected.\n");
		close(sockfd);
		sockfd=-1;
		status=0;
		return -1;
	}
	if(buffer[0]=='/'){
		if(buffer[1]=='0'){//失敗：伺服器要求的斷線
			status=0;
			printf("%s",buffer+3);
			close(sockfd);
			sockfd=-1;
			printf("server error.\n");
		}else if(buffer[1]=='1'){//失敗:不必斷線
			printf("%s",buffer+3);
		}else if(buffer[1]=='2'){//成功
			if(buffer[3]!=0)
				printf("%s",buffer+3);
		}else if(buffer[1]=='3'){//登入特殊狀況:接token
			strcpy(token,buffer+3);
		}else if(buffer[1]=='4'){//特殊狀況:接到挑戰:插隊處理
			printf("You got a challenge from %s.(Type \'Y\' to accept, other key to refuse)\n",buffer+3);
			if(listen_all(buffer)!=2){
				if(strcmp(buffer,"Y")==0 || strcmp(buffer,"y")==0){
					submit(7,buffer+3);
				}else{
					submit(8,buffer+3);
				}
			}
		}return buffer[1]-48;
	}else{
		printf("%s",buffer);
		close(sockfd);
		printf("unknow error.\n");
	}
	return -1;
}

int main(int argc , char *argv[]){
    //Send a message to server
    char buffer[bufsize];
    char inputstr[50];
    int input;
    int count=0;
    while(1){
    	if(status==0){//未登入狀態
    		if(count==0)
    			printf("Hi, may I help you?\n");
    		count=1;
    		printf("[1] login\n");
    		printf("[2] sign up\n");
    		listen_all(inputstr);
    		if(inputstr[0]==0)continue;
    		input=atoi(inputstr);
    		if(input==1){
    			printf("Please enter your account.\n");
				listen_all(account);
				if(account[0]==0)continue;
				printf("Please enter your password.\n");
				listen_all(password);
				if(password[0]==0)continue;
				submit(1,password);
    		}else if(input==2){
    			while(1){
					printf("Please enter your account.\n");
					listen_all(account);
					if(account[0]==0)break;
					int len=strlen(account);
					int x=0;
					for(int i=0;i<len;i++){
						if((account[i]>=65 && account[i]<=90) || (account[i]>=97 && account[i]<=122)){
							x=1;
							break;
						}
					}if(x==0){
						printf("Your account should contain at least one English letter.\n");
					}else{
						break;
					}
    			}if(account[0]==0)continue;
				printf("Please enter your password.\n");
				listen_all(password);
				if(password[0]==0)continue;
				submit(2,password);
    		}else{
    			printf("I don't know what your mean.\nPlease enter the number on the list\n");
    		}
    	}else if(status==1){//登入狀態
    		printf("[1] list online players.\n");
    		printf("[2] challenge\n");
    		printf("[3] logout\n");
    		if(listen_all(inputstr)==4)continue;
    		if(inputstr[0]==0)continue;
    		input=atoi(inputstr);
    		if(input==1){
				submit(3,NULL);
    		}else if(input==2){
    			if(submit(4,NULL)!=2){
    				continue;
    			}
				printf("Who do you want to challenge?(id or name)\n");
    			if(listen_all(buffer)==4)continue;
    			if(buffer[0]==0)continue;
				if(submit(6,buffer)==1){
					printf("The player accepts your challenge.\n");
					printf("Teleport you to the game room...\n");
					status=2;
				}else{
					printf("The player refuses or response time expired.\n");
				}
    		}else if(input==3){
				submit(5,NULL);
    		}else{
    			printf("I don't know what your mean.\nPlease enter the number on the list\n");
    		}
    	}else if(status==2){//遊戲中
    		listen_all(NULL);
    		int x;
    		while((x=listen_all(buffer))!=1 && x!=-1){//回傳值是1代表 對方跳線 遊戲結束且不再繼續 -1為伺服器斷線
    			if(x==-2 || buffer[0]!='\0'){//收到鍵入
    				submit(9,buffer);
       			}
    		}
    		if(x!=-1)
    			status=1;
    	}
    }
    printf("bye\n");
    close(sockfd);
    return 0;
}
