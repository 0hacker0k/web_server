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

int main(){
	char status[10];
	char buffer[12]="hi\n";
	while(1){
		scanf("%s",status);
		sprintf(buffer,"%s\n",status);
		write(1,buffer,strlen(buffer));
		lseek(1, 2, SEEK_END);
		write(1,"0.",strlen(buffer));
		//sleep(15);
		//printf("%d\n",status);
		//status=0;
		//scanf("%d",&status);
		//lseek(1, -1, SEEK_CUR);
	}
	return 0;
}
