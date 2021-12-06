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
int sockfd = 0;
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

void sockcreate(){
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
    }
    printf("Connection established.\n");
    return ;
}

void submit(int command,char* content){
	char buffer[bufsize];
	if(command==1){//登入
		sprintf(buffer,"0 %s %s %d",account,"login",myhash(content));
		send(sockfd,buffer,sizeof(buffer),0);
		if(catch()==3){
			printf("Login success.\n");
			status=1;
		}
	}else if(command==2){//註冊
		sprintf(buffer,"0 %s %s %d",account,"signup",myhash(content));
		send(sockfd,buffer,sizeof(buffer),0);
		if(catch()==3){
			printf("Sign up success.\n");
			status=1;
		}
	}else if(command==3){//清單
		sprintf(buffer,"%s %s %s 0",token,account,"list");
		send(sockfd,buffer,sizeof(buffer),0);
		catch();
	}else if(command==4){//挑戰
		sprintf(buffer,"%s %s %s %s",token,account,"play",content);
		send(sockfd,buffer,sizeof(buffer),0);
	}else if(command==5){//登出
		sprintf(buffer,"%s %s %s 0",token,account,"logout");
		send(sockfd,buffer,sizeof(buffer),0);
		if(catch()==2){
			printf("Logout success.\n");
			status=0;
		}
		sockcreate();
	}
}

int catch(){
	char buffer[bufsize];
	recv(sockfd,buffer,sizeof(buffer),0);
	if(buffer[0]=='/'){
		if(buffer[1]=='0'){//失敗：伺服器要求的斷線
			printf("%s",buffer+3);
			close(sockfd);
			printf("server error.\n");
			sockcreate();
		}else if(buffer[1]=='1'){//失敗:不必斷線
			printf("%s",buffer+3);
		}else if(buffer[1]=='2'){//成功
			if(buffer[3]!=0)
				printf("%s",buffer+3);
		}else if(buffer[1]=='3'){//登入特殊狀況:接token
			strcpy(token,buffer+3);
		}return buffer[1]-48;
	}else{
		printf("%s",buffer);
		close(sockfd);
		printf("unknow error.\n");
		sockcreate();
	}
	return -1;
}

int main(int argc , char *argv[]){

    sockcreate();
    
    //Send a message to server
    char buffer[bufsize];
    char inputstr[50];
    int input;
    int count=0;
    while(1){
    	if(status==0){//未登入
    		if(count==0)
    			printf("Hi, may I help you?\n");
    		count=1;
    		printf("[1] login\n");
    		printf("[2] sign up\n");
    		scanf("%s",inputstr);
    		if(inputstr[0]==0)continue;
    		input=atoi(inputstr);
    		if(input==1){
    			printf("Please enter your account.\n");
				scanf("%s",account);
				if(account[0]==0)continue;
				printf("Please enter your password.\n");
				scanf("%s",password);
				if(password[0]==0)continue;
				submit(1,password);
    		}else if(input==2){
    			while(1){
					printf("Please enter your account.\n");
					scanf("%s",account);
					if(account[0]==0)break;
					int len=strlen(account);
					int x=0;
					for(int i=0;i<len;i++){
						if((account[i]>=65 && account[i]<=90) || (account[i]>=97 && account[i]<=122)){
							x=1;
							break;
						}
					}if(x){
						printf("Your account should contain at least one English letter.\n");
					}else{
						break;
					}
    			}if(account[0]==0)continue;
				printf("Please enter your password.\n");
				scanf("%s",password);
				if(password[0]==0)continue;
				submit(2,password);
    		}else{
    			printf("I don't know what your mean.\nPlease enter the number on the list\n");
    		}
    	}else if(status==1){//登入
    		printf("[1] list online players.\n");
    		printf("[2] challenge\n");
    		printf("[3] logout\n");
    		scanf("%s",inputstr);
    		if(inputstr[0]==0)continue;
    		input=atoi(inputstr);
    		if(input==1){
				submit(3,NULL);
    		}else if(input==2){
    			printf("Who do you want to challenge?(id or name)\n");
				scanf("%s",buffer);
				if(buffer[0]==0)continue;
				submit(4,buffer);
    		}else if(input==3){
				submit(5,NULL);
    		}else{
    			printf("I don't know what your mean.\nPlease enter the number on the list\n");
    		}
    	}else if(status==1){//遊戲////////////////////////以下為特殊處理區
    		//將所有scanf和read移到外面 變成select
    		printf("[1] list online players.\n");
    		printf("[2] challenge\n");
    		printf("[3] chat\n");
    		printf("[4] logout\n");
    		scanf("%s",inputstr);
    		input=atoi(inputstr);
    		if(input==1){
				submit(3,password);
    		}else if(input==2){
    			printf("Who do you want to challenge?(id or name)\n");
				scanf("%s",account);
				submit(4,password);
    		}else if(input==3){
    			printf("Who do you want to chat with?(id or name)\n");
				scanf("%s",account);
				printf("What's the content?\n");
				scanf("%s",account);
				submit(4,password);
    		}else if(input==4){
    			printf("Who do you want to challenge?(id or name)\n");
				scanf("%s",account);
				submit(4,password);
    		}else{
    			printf("I don't know what your mean.\nPlease enter the number on the list\n");
    		}
    	}
    }
    printf("bye\n");
    close(sockfd);
    return 0;
}
