#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#define BUFSIZE 2048  //一次讀取的buffer大小（不可小於1024）

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{"exe","text/plain" },
	{0,0} };

void handle_socket(int fd){
	long int file_fd, buflen, len;
	long ret;
	char * fstr;
	unsigned char buffer[BUFSIZE+1];
	int status=-1;
	int i;
	int browser=0;//0:未知 1:chrome 2:firefox
	bzero(buffer,strlen(buffer));
	ret = recv(fd,buffer,BUFSIZE,0);   //讀取瀏覽器要求 
	//printf("--------------------\n%ld\n%s\n--------------------\n",ret,buffer);
	//while(ret==BUFSIZE){//輸出整個封包
	//	ret = recv(fd,buffer,BUFSIZE,0);   //讀取瀏覽器要求 
	//	printf("--------------------\n%s\n--------------------\n",buffer);
	//}
	if (ret==0||ret==-1) {//網路連線有問題，所以結束程式
		printf("connect error.\n");
		exit(0);
	}
	if(strstr(buffer,"Firefox")){
		browser=2;
	}else if(strstr(buffer,"Chrome")){
		browser=1;
	}
	buffer[BUFSIZE]=0;
	if (ret > 0)
		buffer[ret] = 0;
	else
		buffer[0] = 0;
	unsigned char *ptr,*str;
	char method[10],goal[50],file[50];
	ptr=strstr(buffer," ");
	strncpy(method,buffer,(int)(ptr-buffer));
	method[(int)(ptr-buffer)]=0;

	str=strstr(ptr+1," ");
	strncpy(goal,ptr+1,(int)(str-ptr)-1);
	goal[(int)(str-ptr)-1]=0;
	
	//printf("%s\n",method);
	//printf("%s\n",goal);

	//接受要求 
	if (strcmp(method,"GET")==0 || strcmp(method,"get")==0){
		status=1;//get
	}else if(strcmp(method,"POST")==0 || strcmp(method,"post")==0){
		status=2;//post
	}else{
		status=-1;//can't solve
	}
	if(status==-1){
		printf("can\'t solve this request.");
		write(fd,"can\'t solve this request.",26);
		exit(1);
	}
	
	//擋掉回上層目錄的路徑『..』 
	if(strstr(goal,"..")!=NULL){
		exit(1);
	}

	//處理POST
	
	if(status==2){
		if(access("./file",F_OK)){
			mkdir("./file",509);
		}
		long int file_len;
		char filelenstr[20];
		char boundary[100];
		unsigned char* temptr;
		unsigned char* body_start,*body_end;
		ptr=NULL;
		str=NULL;
		str=strstr(buffer,"Content-Length:");//讀取檔案長度
		str=strstr(str+1," ");
		ptr=strstr(str+1,"\r\n");
		strncpy(filelenstr,str+1,(int)(ptr-str)-1);
		filelenstr[(int)(ptr-str)-1]=0;
		file_len=atoi(filelenstr);
		
		str=strstr(buffer,"----");//讀取boundary（packet辨識碼）
		ptr=strstr(str,"\r\n");
		strncpy(boundary,str,(int)(ptr-str));
		boundary[(int)(ptr-str)]=0;
		
		if(browser==1){//chrome
			if(strstr(ptr,boundary)==NULL){
				ret=recv(fd,buffer,BUFSIZE,0);
				ptr=buffer;
				buffer[ret]=0;
				//printf("======\n%s\n======\n",buffer);
			}
				
		}else if(browser==2){//firefox
			if(strstr(ptr,boundary)==NULL){
				ret=recv(fd,buffer,BUFSIZE,0);
				ptr=buffer;
				buffer[ret]=0;
				//printf("======\n%s\n======\n",buffer);
			}
		}
		
		body_start=strstr(ptr,boundary);
		body_start-=2;
		//printf("%s\n",body_start);
		ptr=strstr(buffer,"filename=\"");//讀取檔名	
		str=strstr(ptr+10,"\"");
		strncpy(file,ptr+10,(int)(str-ptr)-10);
		file[(int)(str-ptr)-10]=0;
		
		
		ptr=strstr(str,"Content-Type:");//ptr指向body內容開頭
		ptr=strstr(ptr+1,"\r\n");
		ptr+=4;
		
		file_len=file_len-(ptr-body_start);//去頭
		file_len-=(strlen(boundary)+8);//去尾 \r\n----\r\n

		long int temp_count=ret-(ptr-buffer);
		char temp_local[100]="./file/";
		int fault_tolerance=0;
		strcat(temp_local,file);
		FILE* write_file=fopen(temp_local,"wb");
		if(strstr(ptr,boundary))temp_count-=(strlen(boundary)+8);
		//ptr[1219]=15;
		fwrite(ptr,1,temp_count,write_file);
		file_len-=temp_count;
		fflush(write_file);
		while(file_len>0 && (ret=recv(fd,buffer,(file_len>BUFSIZE?BUFSIZE:file_len),0))!=0){
			fwrite(buffer,1,ret,write_file);
			file_len-=ret;
			fflush(write_file);
			fault_tolerance=1;
		}
		if(fault_tolerance){
			for(int k=0;k<30;k++){
				ret=recv(fd,buffer,1,0);
				if(buffer[0]!='\r' && ret!=0){
					fwrite(buffer,1,ret,write_file);
					fflush(write_file);
				}else{
					break;
				}
			}
		}
		
		fclose(write_file);
		
	}

	//預設讀取index.html 
	if (strlen(goal)==1){
		strcpy(goal,"/index.html");
	}
	sprintf(buffer,"%s %s",method,goal);
	//檢查客戶端所要求的檔案格式
	buflen = strlen(buffer);
	fstr = (char *)0;

	for(i=0;extensions[i].ext!=0;i++) {
		len = strlen(extensions[i].ext);
		if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr = extensions[i].filetype;
			break;
		}
	}
	//檔案格式不支援 
	if(fstr == 0) {
		fstr = extensions[i-1].filetype;
	}

	//開啟檔案 
	if((file_fd=open(&goal[1],O_RDONLY))==-1){
		write(fd, "Failed to open file", 19);
		printf("open %s error\n\n\n",&goal[1]);
	}
	
	//傳回瀏覽器成功碼 200 和內容的格式 
	sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	//printf("%s\n",buffer);
	write(fd,buffer,strlen(buffer));
	//讀取檔案內容輸出到客戶端瀏覽器 
	while ((ret=read(file_fd, buffer, BUFSIZE))>0) {
		buffer[ret]=0;
		write(fd,buffer,ret);
	}
	close(file_fd);
	//close(fd);
	return ;
	//exit(0);
}

int main(int argc, char **argv){
	int i, pid, catchfd, socketfd;
	size_t length;
	static struct sockaddr_in cli_addr;
	static struct sockaddr_in serv_addr;
	const char socket_buffer[1024];
	//char tmp[50];
	//getcwd(tmp,sizeof(tmp));
	//printf("%s\n",tmp); 
	if(argc>=2){
		if(chdir(argv[1]) == -1){ 
			printf("ERROR: Can't Change to directory %s\n",argv[1]);
			return 0;
		}
	}
	
	printf("%d\n",getpid());
	//處理子程序結束的signal(避免因為沒處理而成為zombie)
	signal(SIGCLD, SIG_IGN);

	//開啟網路 Socket
	if ((catchfd=socket(AF_INET, SOCK_STREAM,0))<0){
		printf("error %d\n",1);
		exit(0);
	}
	//網路連線設定
	serv_addr.sin_family = AF_INET;
	//使用任何在本機的對外 IP
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//使用 8080 Port
	serv_addr.sin_port = htons(8080);

	//綁定serv_addr為"catchfd"
	setsockopt(catchfd,SOL_SOCKET,SO_REUSEADDR,socket_buffer,sizeof(socket_buffer));
	while(1){
		//setsockopt()
		if (bind(catchfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))>=0){
			break;
		}sleep(1);
	}
	
	//允許接收socket queue最多有1024
	if (listen(catchfd,1024)<0){
		printf("error %d\n",3);
		exit(0);
	}
	printf("start server.\n");
	while(1) {
		length = sizeof(cli_addr);
		//等待連線
		while((socketfd = accept(catchfd, (struct sockaddr *)&cli_addr, (socklen_t*)&length))<0){
			printf("error %d\n",4);
			//exit(0);
		}
	   	//測試只有父程序的情狀（debug用）
		/*
		handle_socket(socketfd);
		shutdown(socketfd,SHUT_WR);
		close(socketfd);
		*/
		//分出子程序處理請求
		if ((pid = fork()) < 0) {
			printf("error %d\n",5);
			exit(0);
		} else {
			if (pid == 0) {  //子程序
				close(catchfd);//停止監聽
				handle_socket(socketfd);//處理socket
				shutdown(socketfd,SHUT_WR);
				close(socketfd);
				exit(0);
				//close(socketfd);
				//printf("close\n");
			} else { //父程序
				//printf("c:%d\n",pid);
			}
		}
	}
	return 0;
}
