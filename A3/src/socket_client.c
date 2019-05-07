#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

#define SRV_ADDRESS	"141.22.17.137"
#define SRV_PORT	7777


int main ( )
{
  int s_tcp;			/* socket descriptor */
  struct sockaddr_in sa;	/* socket address structure*/
  int sa_len = sizeof(struct sockaddr_in), n;
  char* msg="Hello World!";


  sa.sin_family = AF_INET;
  sa.sin_port = htons(SRV_PORT);

  if (inet_pton(sa.sin_family,SRV_ADDRESS, &sa.sin_addr.s_addr) <= 0) {
	perror("Address Conversion");
	exit(1);
	}

  if ((s_tcp=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0) {
	perror("TCP Socket");
	exit(1);
	}

  if (connect(s_tcp,(struct sockaddr*) &sa, sa_len) < 0) {
	perror("Connect");
	exit(1);
	}

  if((n=send(s_tcp,msg, strlen(msg),0)) > 0) {
	printf("Message %s sent ( %i Bytes).\n", msg, n); 
	}

  close(s_tcp);
}
