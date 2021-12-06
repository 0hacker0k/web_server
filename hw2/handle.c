#define BUFSIZE 1024

void handlefd(int fd){
	char buffer[BUFSIZE];
	write(fd, "Please enter your account.", 26);
	int ret = read(fd,buffer,BUFSIZE);
	
    write(fd, "Failed to open file", 19);
    ret = read(fd,buffer,BUFSIZE);
	return ;
}
/*
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define STDIN 0 // standard input 的 file descriptor
int main(void){
  struct timeval tv;
  fd_set readfds;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&readfds);
  FD_SET(STDIN, &readfds);
  // 不用管 writefds 與 exceptfds：
  select(STDIN+1, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(STDIN, &readfds))
    printf("A key was pressed!\n");
  else
    printf("Timed out.\n");
  return 0;
}
*/
