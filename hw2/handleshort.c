#define BUFSIZE 1024
int myhash(char* password){
	int passlen=strlen(password);
	int hashnum=0;
	int table[15]={13,5,7,39,17,29,2,47,41,31,23,19,3,11,43};
	for(int i=0;i<passlen;i++){
		hashnum+=(password[i]-48)*table[i%15]+i;
	}
	return hashnum;
}
int handleshort(int fd){
	char buffer[BUFSIZE];
	int ret = read(fd,buffer,6);
	FILE *user;
	char account[30],password[50],temp[60];
	char* str;
	char* ptr;
	if(strncmp(buffer,"login ",6)==0){//登入
		ret=read(fd,buffer,BUFSIZE);
		str=strstr(buffer," ");//讀帳號
		strncpy(temp,buffer,(int)(str-buffer));
		temp[(int)(str-buffer)]=0;
		strcpy(account,temp);
		str=strstr(buffer," ");//讀密碼
		strncpy(temp,buffer,(int)(str-buffer));
		temp[(int)(str-buffer)]=0;
		strcpy(password,temp);
		
		user=fopen("user.txt",r);
		while(fgets(buffer,1,BUFSIZE,user)!=EOF){
			str=strstr(buffer," ");
			strncpy(temp,buffer,(int)(str-buffer));
			temp[(int)(str-buffer)]=0;
			if(strcmp(temp,account)!=0){
				continue;
			}
			str=strstr(buffer," ");
			strncpy(temp,buffer,(int)(str-buffer));
			temp[(int)(str-buffer)]=0;
			if(atoi(temp)==myhash(password)){
				write(fd,"success",7);
				return 1;
			}else{
				return -1;
			}
		}
	}else if(strncmp(buffer,"logout",6)==0){//登出
		
	}else if(strncmp(buffer,"register",6)==0){//註冊
		fopen("user.txt",w+);
	}else if(strncmp(buffer,"mark  ",6)==0){//畫圈/叉
		
	}else if(strncmp(buffer,"over  ",6)==0){//投降
		
	}else if(strncmp(buffer,"reset ",6)==0){//重新發起挑戰
		
	}else if(strncmp(buffer,"begin ",6)==0){//像人發起挑戰
		
	}else if(strncmp(buffer,"chat  ",6)==0){//聊天
		
	}
	
    write(fd, "Failed to open file", 19);
    
	return ;
}
