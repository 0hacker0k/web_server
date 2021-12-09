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
	int status;//0 登出 1登入 2遊戲中 3等待回應中
	int socketfd;
	char token[50];
};
struct playerpair{
	struct user *p1;
	struct user *p2;
};
struct user *usrlist;
int usercount;
FILE* logfd;
time_t now;
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
void* playinroom(void *d){
	struct playerpair *data=(struct playerpair*)d;
	pthread_detach(pthread_self());//讓parent不用等待
	struct user *p1=data->p1;
	struct user *p2=data->p2;
	
	int p1fd=p1->socketfd,p2fd;
	if(-1==p2){
		p2fd=-1;
	}else{
		p2fd=p2->socketfd;
	}
	char buffer[BUFSIZE];
	char buffer2[BUFSIZE];
	char temp[BUFSIZE];
	char token[50],content[BUFSIZE];
	int table[3][3];
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			table[i][j]=-1;
		}
	}
	int max=(p1fd>p2fd)?p1fd:p2fd;
	srand(time(NULL));
	int first=rand()%2;
	int turn=(first==0)?0:1;
	sprintf(buffer,"/2 ");
	sprintf(temp,"\n\nYou are the first.\n");
	strcat(buffer,temp);
	sprintf(temp,"Enter the corresponding id to mark.(Your mark is O)\n");
	strcat(buffer,temp);
	
	sprintf(buffer2,"/2 ");
	sprintf(temp,"\n\nYou are the second.\n");
	strcat(buffer2,temp);
	sprintf(temp,"Enter the corresponding id to mark.(Your mark is X)\n");
	strcat(buffer2,temp);
	
	for(int i=0;i<3;i++){
		for(int j=0;j<3;j++){
			if(table[i][j]==-1){
				sprintf(temp,"%d",i*3+j+1);
				strcat(buffer,temp);
				strcat(buffer2,temp);
			}
			if(j<2){
				sprintf(temp,"|");
				strcat(buffer,temp);
				strcat(buffer2,temp);
			}
		}sprintf(temp,"\n");
		strcat(buffer,temp);
		strcat(buffer2,temp);
	}
	if(p2fd!=(first==0)?p2fd:p1fd || p2fd!=-1){
		write((first==0)?p1fd:p2fd,buffer,strlen(buffer));
	}
	if(p2fd!=(first==0)?p2fd:p1fd || p2fd!=-1){
		write((first==0)?p2fd:p1fd,buffer2,strlen(buffer2));
	}
	
	fd_set readfds;
	int ret;
	int status=0;//遊玩中
	int ainum;
	while(1){
		FD_ZERO(&readfds);
		FD_SET(p1fd, &readfds);
		if(p2fd!=-1){
			FD_SET(p2fd, &readfds);
		}
		int has_rd;
		printf("%d %d\n",turn,p2fd);
		if(turn==1 && p2fd==-1){
			has_rd=1;
			//AI
			int m=rand()%3;
			int n=rand()%3;
			int countnum=0;
			while(table[m][n]!=-1 && countnum<=9){
				countnum++;
				n++;
				if(n==3){
					n=0;
					m++;
				}if(m==3){
					m=0;
				}
			}
			if(countnum<=9){
				ainum=n*3+m+1;
			}content[0]=0;
		}else{
			has_rd=select(max+1, &readfds, NULL, NULL, 0);
		}
		
		if(has_rd==-1){//監聽set
			fprintf(logfd,"%s %s %d\n",ctime(&now),"error in select",errno);
			perror("select");
		}else if(has_rd>0){
			int getfd,anofd;
			struct user* get;
			struct user* ano;
			if(FD_ISSET(p1fd, &readfds)){
				get=p1;
				ano=p2;
				getfd=p1fd;
				anofd=p2fd;
			}else if(p2fd!=-1 && FD_ISSET(p2fd, &readfds)){
				get=p2;
				ano=p1;
				getfd=p2fd;
				anofd=p1fd;
			}else if(p2fd==-1){
				get=p1;
				ano=p1;
				getfd=p1fd;
				anofd=p1fd;
			}
			if(!(turn==1 && p2fd==-1)){
				if(p2fd==-1)
					ret=read(p1fd,buffer,sizeof(buffer));
				else
					ret=read(getfd,buffer,sizeof(buffer));
			}else{
				ret=1;
			}
			if(ret==0){
				sprintf(buffer,"/1 Your opponent is offline.\n");
				if(p2fd!=-1){
					write(anofd,buffer,sizeof(buffer));
				}
				close(getfd);
				get->status=0;
				get->socketfd=-1;
				get->token[0]=0;
				if(p2fd!=-1){
					ano->status=1;
				}printf("ret:%d\n",ret);
				break;
			}
			if(!(turn==1 && p2fd==-1))sscanf(buffer,"%s %s",token,content);
			if(strcmp(token,get->token)!=0 && !(turn==1 && p2fd==-1)){
				sprintf(buffer,"/0 Your certificate has expired.\n");
				write(getfd,buffer,sizeof(buffer));
				shutdown(getfd,SHUT_WR);
				close(getfd);
				sprintf(buffer,"/1 Your opponent is offline.\n");
				if(p2fd!=-1){
					write(anofd,buffer,sizeof(buffer));
				}
				get->status=0;
				get->socketfd=-1;
				get->token[0]=0;
				if(p2fd!=-1){
					ano->status=1;
				}
				
				break;
			}
			if((status==1 || status==2) && (strcmp(content,"N")==0 || strcmp(content,"n")==0)){
				sprintf(buffer,"/1 Ok bey~\n");
				write(getfd,buffer,sizeof(buffer));
				sprintf(buffer,"/1 Your opponent has left.\n");
				write(anofd,buffer,sizeof(buffer));
				p1->status=1;
				if(p2fd!=-1){
					p2->status=1;
				}
				break;
			}else if(status==1 && (strcmp(content,"Y")==0 || strcmp(content,"y")==0)){
				status++;
				continue;
			}else if(status==2 && (strcmp(content,"Y")==0 || strcmp(content,"y")==0)){//遊戲重起
				status=0;
				for(int i=0;i<3;i++){
					for(int j=0;j<3;j++){
						table[i][j]=-1;
					}
				}
				first=rand()%2;
				turn=(first==0)?0:1;
				sprintf(buffer,"/2 ");
				sprintf(temp,"\n\nYou are the first.\n");
				strcat(buffer,temp);
				sprintf(temp,"Enter the corresponding id to mark.(Your mark is O)\n");
				strcat(buffer,temp);
				
				sprintf(buffer2,"/2 ");
				sprintf(temp,"\n\nYou are the second.\n");
				strcat(buffer2,temp);
				sprintf(temp,"Enter the corresponding id to mark.(Your mark is X)\n");
				strcat(buffer2,temp);
				
				for(int i=0;i<3;i++){
					for(int j=0;j<3;j++){
						if(table[i][j]==-1){
							sprintf(temp,"%d",i*3+j+1);
							strcat(buffer,temp);
							strcat(buffer2,temp);
						}
						if(j<2){
							sprintf(temp,"|");
							strcat(buffer,temp);
							strcat(buffer2,temp);
						}
					}sprintf(temp,"\n");
					strcat(buffer,temp);
					strcat(buffer2,temp);
				}
				if(p2fd!=(first==0)?p2fd:p1fd || p2fd!=-1){
					write((first==0)?p1fd:p2fd,buffer,strlen(buffer));
				}
				if(p2fd!=(first==0)?p2fd:p1fd || p2fd!=-1){
					write((first==0)?p2fd:p1fd,buffer2,strlen(buffer2));
				}
				continue;
			}
			int len=strlen(content);
			int x=0;
			for(int i=0;i<len;i++){
				if(content[i]>57 || content[i]<49){
					x=1;
					break;
				}
			}int num;
			if(x==0){
				if(content[0]!=0)
					num=atoi(content);
				else
					num=ainum;
				if(num>9 || num<1) x=1;
			}
			if(x==1){//有其他文字 或數字不合理 故判斷為聊天
				if(p2fd!=-1){
					sprintf(buffer,"/2 %s: %s\n",ano->account,content);
					write(anofd,buffer,sizeof(buffer));
				}else{
					sprintf(buffer,"/2 %s: %s\n","AI",content);
					write(getfd,buffer,sizeof(buffer));
				}
				continue;
			}else{//下棋
				if(status!=0)continue;
				if((turn==0 && get==p2) || (turn==1 && get==p1 && p2fd!=-1)){
					sprintf(buffer,"/2 This is not your turn.\n");
					write(getfd,buffer,sizeof(buffer));
					continue;
				}
				int m=(num-1)/3;
				int n=(num-1)%3;
				if(table[m][n]!=-1){
					sprintf(buffer,"/2 Has been marked.\n");
					write(getfd,buffer,sizeof(buffer));
				}else{
					table[m][n]=(turn==first)?0:1;
					sprintf(buffer,"/2 ");
					for(int i=0;i<3;i++){
						for(int j=0;j<3;j++){
							if(table[i][j]==-1){
								sprintf(temp," ");
								strcat(buffer,temp);
							}else if(table[i][j]==0){
								sprintf(temp,"O");
								strcat(buffer,temp);
							}else if(table[i][j]==1){
								sprintf(temp,"X");
								strcat(buffer,temp);
							}
							if(j<2){
								sprintf(temp,"|");
								strcat(buffer,temp);
							}
						}sprintf(temp,"\n");
						strcat(buffer,temp);
					}
					sprintf(temp,"\n");
					strcat(buffer,temp);
					turn=(turn==0)?1:0;
				}
			}x=0;
			if(table[0][0]!=-1){
				if(table[0][0]==table[0][1] && table[0][0]==table[0][2])
					x=1;
				else if(table[0][0]==table[1][0] && table[0][0]==table[2][0])
					x=1;
				else if(table[0][0]==table[1][1] && table[0][0]==table[2][2])
					x=1;
			}if(table[1][1]!=-1){
				if(table[1][1]==table[1][0] && table[1][1]==table[1][2])
					x=2;
				else if(table[1][1]==table[0][1] && table[1][1]==table[2][1])
					x=2;
				else if(table[1][1]==table[0][2] && table[1][1]==table[2][0])
					x=2;
			}if(table[2][2]!=-1){
				if(table[2][2]==table[2][1] && table[2][2]==table[2][0])
					x=3;
				else if(table[2][2]==table[0][2] && table[2][2]==table[1][2])
					x=3;
			}strcat(buffer2,buffer);
			if(x!=0){//有勝負
				x-=1;
				if(table[x][x]==first){//p1勝
					status=1;//遊玩暫時結束
					sprintf(temp,"/2 You win the game!\nDo you want to play again?(type \'Y\' to accept, \'N\' to refuse)\n");
					strcat(buffer,temp);
					write(p1fd,buffer,sizeof(buffer));
					sprintf(temp,"/2 You lost the game...\nDo you want to play again?(type \'Y\' to accept, \'N\' to refuse)\n");
					strcat(buffer2,temp);
					if(p2fd!=-1){
						write(p2fd,buffer2,sizeof(buffer2));
					}else{
						status++;
					}
				}else{//p2勝
					status=1;//遊玩暫時結束
					sprintf(temp,"You win the game!\nDo you want to play again?(type \'Y\' to accept, \'N\' to refuse)\n");
					strcat(buffer,temp);
					if(p2fd!=-1){
						write(p2fd,buffer,sizeof(buffer));
					}else{
						status++;
					}
					if(p2fd==-1)buffer2[0]=0;
					sprintf(temp,"You lost the game...\nDo you want to play again?(type \'Y\' to accept, \'N\' to refuse)\n");
					strcat(buffer2,temp);
					write(p1fd,buffer2,sizeof(buffer2));
				}
			}else{//沒有勝負
				x=0;
				for(int i=0;i<3;i++){
					for(int j=0;j<3;j++){
						if(table[i][j]==-1){
							x=1;
							break;
						}
					}
				}if(x==0){//平手
					status=1;//遊玩暫時結束
					sprintf(temp,"There is no winner in this game.\nDo you want to play again?(type \'Y\' to accept, \'N\' to refuse)\n");
					strcat(buffer,temp);
					write(p1fd,buffer,sizeof(buffer));
					if(p2fd!=-1){
						write(p2fd,buffer,sizeof(buffer));
					}
				}else{
					write(p1fd,buffer,sizeof(buffer));
					if(p2fd!=-1){
						write(p2fd,buffer,sizeof(buffer));
					}
				}
			}
		}
	}
	pthread_exit(NULL);
}
void* child(void* data) {
	pthread_detach(pthread_self());//讓parent不用等待
	int fd = *((int*) data); // 取得輸入資料
	char buffer[BUFSIZE];
	char temp[BUFSIZE];
	int ret=read(fd,buffer,sizeof(buffer));
	if(ret==0){
		for(int i=0;i<usercount;i++){
			if(fd==usrlist[i].socketfd){
				usrlist[i].status=0;
				usrlist[i].socketfd=-1;
				usrlist[i].token[0]=0;
				fprintf(logfd,"%s %s %s\n",ctime(&now),usrlist[i].account,"has left");
				close(fd);
			}
		}
		pthread_exit(NULL);
	}
	buffer[ret]=0;
	//printf("%d\n",fd);
	//printf("%s\n",);
	fprintf(logfd,"%s get %s from %d\n",ctime(&now),buffer,fd);
	char account[30],content[50],token[50],command[7];
	sscanf(buffer,"%s %s %s %s",token,account,command,content);
	int status=1;
	int id=-1;
	if(strcmp(command,"login")==0){
		for(int i=0;i<usercount;i++){
			if(strcmp(account,usrlist[i].account)==0){
				status=0;
				if(myhash2(content)==atoi(usrlist[i].password)){
					int tempnum;
					if(usrlist[i].socketfd!=-1){//已被登入
						sprintf(buffer,"/0 Your account has been logged in elsewhere.\n");
						tempnum=usrlist[i].socketfd;
						write(usrlist[i].socketfd,buffer,strlen(buffer));
					}
					usrlist[i].status=-1;
					srand(time(NULL));
					usrlist[i].socketfd=fd;
					sprintf(usrlist[i].token,"%d",myhash(account)+rand()%323);
					sprintf(buffer,"/3 ");
					strcat(buffer,usrlist[i].token);
					write(fd,buffer,strlen(buffer));
					
					shutdown(tempnum,SHUT_WR);
					close(tempnum);
					usrlist[i].status=1;
				}else{
					write(fd,"/0 Password error.\n",19);
					shutdown(fd,SHUT_WR);
					close(fd);
				}
			}
		}if(status==1){
			write(fd,"/0 Can't find the account.\n",27);
			shutdown(fd,SHUT_WR);
			close(fd);
		}
	}else if(strcmp(command,"signup")==0){
		int x=1;
		for(int i=0;i<usercount;i++){
			if(strcmp(usrlist[i].account,account)==0){
				x=0;
				break;
			}
		}
		if(x){
			usercount+=1;
			usrlist[usercount-1].id=usercount;
			strcpy(usrlist[usercount-1].account,account);
			sprintf(usrlist[usercount-1].password,"%d",myhash2(content));
			usrlist[usercount-1].status=1;
			usrlist[usercount-1].socketfd=fd;
			sprintf(usrlist[usercount-1].token,"%d",myhash(account)+rand()%323);
			FILE* wtfd=fopen("user.txt","r+");
			fseek(wtfd,0,SEEK_END);
			fprintf(wtfd,"%d %s %s\n",usrlist[usercount-1].id,usrlist[usercount-1].account,usrlist[usercount-1].password);
			fseek(wtfd,0,SEEK_SET);
			fprintf(wtfd,"%d\n",usercount);
			fclose(wtfd);
			sprintf(buffer,"/3 ");
			strcat(buffer,usrlist[usercount-1].token);
			write(fd,buffer,strlen(buffer));
		}
		
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
			write(fd,"/0 Your certificate has expired.\n",74);
			shutdown(fd,SHUT_WR);
			close(fd);
		}else if(status==1){//有憑證 沒帳號
			write(fd,"/0 No such account.\n",20);
			shutdown(fd,SHUT_WR);
			close(fd);
		}else if(status==-2){//沒有憑證
			write(fd,"/0 You must log in first.\n",26);
			shutdown(fd,SHUT_WR);
			close(fd);
		}else{
			if(strcmp(command,"logout")==0){//登出
				write(fd,"/2 ",3);
				usrlist[id].status=0;
				usrlist[id].socketfd=-1;
				usrlist[id].token[0]=0;
				shutdown(fd,SHUT_WR);
				close(fd);
			}else if(strcmp(command,"list")==0){
				sprintf(buffer,"/2 ");
				int x=0;
				int count=0;
				for(int i=0;i<usercount;i++){
					if(id==i)continue;
					if(usrlist[i].status==1 || usrlist[i].status==-1){
						x=1;
						count++;
						sprintf(temp,"\t[%d]\t%s\tonline\n",count,usrlist[i].account);
						strcat(buffer,temp);
					}else if(usrlist[i].status==2){
						x=1;
						count++;
						sprintf(temp,"\t[%d]\t%s\tplaying\n",count,usrlist[i].account);
						strcat(buffer,temp);
					}
				}if(x==0){
					sprintf(temp,"No players are online except you.\n");
					strcat(buffer,temp);
				}write(fd,buffer,sizeof(buffer));
				usrlist[id].status=1;
			}else if(strcmp(command,"plist")==0){
				sprintf(buffer,"/2 ");
				int x=0;
				for(int i=0;i<usercount;i++){
					if(usrlist[i].socketfd==fd)continue;
					if(usrlist[i].status==1 || usrlist[i].status==-1){
						if(x==0){
							sprintf(temp,"These are the players you can challenge.\n");
							strcat(buffer,temp);
						}
						x=1;
						sprintf(temp,"\tid:[ %d ]\t%s\tonline\n",i,usrlist[i].account);
						strcat(buffer,temp);
					}
				}
				if(x==0){
					sprintf(buffer,"/2 ");//////////////////////////////////////////////////////////////////////
					sprintf(temp,"No players are online except you and AI.\n");
					strcat(buffer,temp);
				}write(fd,buffer,sizeof(buffer));
				usrlist[id].status=1;
			}else if(strcmp(command,"play")==0){
				int len=strlen(content);
				int x=0;
				for(int i=0;i<len;i++){
					if(i==0 && content[i]=='-')continue;
					if(content[i]>57 || content[i]<48){
						x=1;
						break;
					}
				}
				if(x==1){//以帳號搜尋
					for(int i=0;i<usercount;i++){
						if(strcmp(usrlist[i].account,content)==0){
							x=3;
							if(usrlist[i].status==0){//玩家不在線上
								sprintf(buffer,"/1 ");
								sprintf(temp,"%s is not online.\n",usrlist[i].account);
								strcat(buffer,temp);
								write(fd,buffer,sizeof(buffer));
							}else if(usrlist[i].status==2){//玩家已在遊戲中
								sprintf(buffer,"/1 ");
								sprintf(temp,"%s is already in another game.\n",usrlist[i].account);
								strcat(buffer,temp);
								write(fd,buffer,sizeof(buffer));
							}else if(strcmp(usrlist[i].account,account)==0){//自己挑戰自己
								sprintf(buffer,"/1 ");
								sprintf(temp,"You can't challenge yourself.\n");
								strcat(buffer,temp);
								write(fd,buffer,sizeof(buffer));
							}else{
								sprintf(buffer,"/4 ");
								sprintf(temp,"%s",account);
								strcat(buffer,temp);
								write(usrlist[i].socketfd,buffer,sizeof(buffer));
							}
							break;
						}
					}
				}else{//以id搜尋
					int idtemp=atoi(content);
					for(int i=0;i<usercount;i++){
						if(i==idtemp){
							x=3;
							if(usrlist[i].status==0){//玩家不在線上
								sprintf(buffer,"/1 ");
								sprintf(temp,"%s is not online.\n",usrlist[i].account);
								strcat(buffer,temp);
								write(fd,buffer,sizeof(buffer));
							}else if(usrlist[i].status==2){//玩家已在遊戲中
								sprintf(buffer,"/1 ");
								sprintf(temp,"%s is already in another game.\n",usrlist[i].account);
								strcat(buffer,temp);
								write(fd,buffer,sizeof(buffer));
							}else if(strcmp(usrlist[i].account,account)==0){//自己挑戰自己
								sprintf(buffer,"/1 ");
								sprintf(temp,"You can't challenge yourself.\n");
								strcat(buffer,temp);
								write(fd,buffer,sizeof(buffer));
							}else{
								sprintf(buffer,"/4 ");
								sprintf(temp,"%s",account);
								strcat(buffer,temp);
								write(usrlist[i].socketfd,buffer,sizeof(buffer));
							}
							break;
						}
					}
				}
				if(x!=3 && (strcmp(content,"AI")==0 || atoi(content)!=-1)){//沒有這個玩家
					sprintf(buffer,"/1 ");
					sprintf(temp,"No such players found.\n");
					strcat(buffer,temp);
					write(fd,buffer,sizeof(buffer));
				}
				if(strcmp(content,"AI")==0 || atoi(content)==-1){
					sprintf(buffer,"/2 ");
					write(fd,buffer,sizeof(buffer));
					pthread_t t; // 宣告 pthread 變數
					struct playerpair player;
					player.p1=&(usrlist[id]);
					player.p2=-1;
					pthread_create(&t, NULL, playinroom, &player);
					usrlist[id].status=2;
				}
				usrlist[id].status=3;
			}else if(strcmp(command,"no")==0){//拒絕遊戲:不需要理會是否有回應
				int x=0;
				for(int i=0;i<usercount;i++){
					if(strcmp(usrlist[i].account,content)==0){
						x=1;
						sprintf(buffer,"/1 ");
						sprintf(temp,"%s",account);
						strcat(buffer,temp);
						write(usrlist[i].socketfd,buffer,sizeof(buffer));
						usrlist[i].status=1;
						break;
					}
				}sprintf(buffer,"/2 ");
				write(fd,buffer,sizeof(buffer));
				usrlist[id].status=1;
			}else if(strcmp(command,"ok")==0){//同意遊戲
				int x=0;
				struct user *p2;
				for(int i=0;i<usercount;i++){
					if(strcmp(usrlist[i].account,content)==0){
						if(usrlist[i].status==3){
							x=1;
							sprintf(buffer,"/2 ");
							write(usrlist[i].socketfd,buffer,sizeof(buffer));
							usrlist[i].status=2;
							p2=&(usrlist[i]);
						}
						break;
					}
				}
				if(x==0){//玩家消失
					sprintf(buffer,"/1 ");
					sprintf(temp,"Invitation expired.\n");
					strcat(buffer,temp);
					write(fd,buffer,sizeof(buffer));
					usrlist[id].status=1;
				}else{
					sprintf(buffer,"/2 ");
					write(fd,buffer,sizeof(buffer));
					pthread_t t; // 宣告 pthread 變數
					struct playerpair player;
					player.p1=&(usrlist[id]);
					player.p2=p2;
					pthread_create(&t, NULL, playinroom, &player);
					usrlist[id].status=2;
				}
			}else if(strcmp(command,"cancel")==0){//cancel 取消邀請
				int len=strlen(content);
				int x=0;
				for(int i=0;i<len;i++){
					if(content[i]>57 || content[i]<48){
						x=1;
						break;
					}
				}
				for(int i=0;i<usercount;i++){
					if((x==1 && strcmp(usrlist[i].account,content)==0) || (x==0 && i==atoi(content))){
						sprintf(buffer,"/2 ");
						sprintf(temp,"%s has cancelled the challenge.\n",account);
						strcat(buffer,temp);
						write(usrlist[i].socketfd,buffer,sizeof(buffer));
						break;
					}
				}
				usrlist[id].status=1;
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
	char temp[1024];
	while(1){
		FD_ZERO(&readfds);//清除set
		max=-1;
		for(int i=0;i<usercount;i++){
			if(usrlist[i].status==1 || usrlist[i].status==3){
				if(max<usrlist[i].socketfd)
					max=usrlist[i].socketfd;
				FD_SET(usrlist[i].socketfd, &readfds);//加入set
				//printf("%d online\n",usrlist[i].socketfd);
			}
		}
		if(max!=-1){//有任何現存的連線才偵測
			int has_rd=select(max+1, &readfds, NULL, NULL, &tv);
			if(has_rd==-1){//監聽set
				fprintf(logfd,"%s %s %d\n",ctime(&now),"error in select",errno);
				//perror("select");
			}else if(has_rd>0){
				//printf("get something.\n");
				for(int i=0;i<usercount;i++){//看看是誰可以讀取了
					if(FD_ISSET(usrlist[i].socketfd, &readfds)){
						usrlist[i].status=-1;
						//printf("get %d from %s.\n",usrlist[i].socketfd,usrlist[i].account);
						pthread_t t; // 宣告 pthread 變數
						pthread_create(&t, NULL, child, &usrlist[i].socketfd);
					}
				}
			}
		}//sleep(2);
	}
	pthread_exit(NULL); // 離開子執行緒
}
int main(int argc , char *argv[]){
    //socket的建立
    logfd=fopen("log.txt","r+");
	fseek(logfd,0,SEEK_END);
	
    int sockfd = 0,clientfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("create socket error\n");
    }
	signal(SIGCLD, SIG_IGN);
	signal(SIGPIPE,SIG_IGN);
    //socket的連線
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));
	
    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(60000);
    int socket_buffer;
    int err=0;
    do{
		setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&socket_buffer,sizeof(socket_buffer));
		err=bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    }while(err==-1);
    
    if (listen(sockfd,1024)<0){
		fprintf(logfd,"%s %s\n",ctime(&now),"listen error");
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
	pthread_t t1; // 宣告 pthread 變數
	pthread_create(&t1, NULL, check, NULL);
	time(&now);
	
	fprintf(logfd,"%s %s\n",ctime(&now),"server operation access");
	int cfd;
    while(1){
    	if((clientfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen))<0){
    		fprintf(logfd,"%s %s\n",ctime(&now),"accept error");
    	}
    	cfd=clientfd;
    	//printf("%d\n",clientfd);
    	//close(cfd);
    	pthread_t t;
    	pthread_create(&t, NULL, child, &cfd);
    	pthread_detach(t);
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
