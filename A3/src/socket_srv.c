#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>


#define SRV_PORT	7777


int main ( )
{
  int s_tcp, news;			/* socket descriptor */
  struct sockaddr_in sa,
  	 sa_client;			/* socket address structure*/
  int sa_len = sizeof(struct sockaddr_in), n;
  char info[256];


  sa.sin_family = AF_INET;
  sa.sin_port = htons(SRV_PORT);
  sa.sin_addr.s_addr = INADDR_ANY;
  

  if ((s_tcp=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0) {
	perror("TCP Socket");
	exit(1);
	}

  if (bind(s_tcp,(struct sockaddr*) &sa, sa_len) < 0) {
	perror("bind");
	exit(1);
	}

  if (listen(s_tcp,5) < 0) {
  	perror("listen");
	close(s_tcp);
	exit(1);
  	}
  printf("Waiting for TCP connections ... \n");

  while( 1 ) {
	if ((news=accept(s_tcp, (struct sockaddr*) &sa_client, &sa_len)) < 0) {
  		perror("accept");
		close(s_tcp);
		exit(1);
  		}
  	if (recv(news, info,sizeof(info), 0)) {
		printf("Message received: %s \n", info);
		}
	}

  close(s_tcp);
}
