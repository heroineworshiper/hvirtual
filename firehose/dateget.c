#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("Date Get Usage: dateget host:port.\n");
		exit(1);
	}

	int sock;
	struct sockaddr_in name;
	int port;
	struct hostent *hostinfo;
	char *port_string = strchr(argv[1], ':');
	char *hostname = strdup(argv[1]);
	char *hostname_end = strchr(hostname, ':');
	int64_t result = 0;
	time_t time_result;
	struct timeval time_val;
	struct tm *broken_time;
	char *string;
	int total;

	if(!hostname_end)
	{
		printf("No port specified after the server.\n");
		exit(1);
	}

	*hostname_end = 0;
	port_string++;
	port = atol(port_string);

	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	hostinfo = gethostbyname(hostname);
	if(!hostinfo) 
	{
		perror("gethostbyname");
		exit(1);
	}
	name.sin_addr = *(struct in_addr *) hostinfo->h_addr;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(connect(sock, (struct sockaddr *)&name, sizeof(name)) < 0)
	{
		perror("connect");
		exit(1);
	}

	for(total = 0; total < sizeof(result) && total >= 0; )
	{
		total += read(sock, ((unsigned char*)&result) + total, sizeof(result));
		if(total < 0) 
		{
			perror("read");
			exit(1);
		}
	}
	close(sock);

/*
 * printf("%02x %02x %02x %02x %02x %02x %02x %02x\n",
 * *((unsigned char*)&result + 0),
 * *((unsigned char*)&result + 1),
 * *((unsigned char*)&result + 2),
 * *((unsigned char*)&result + 3),
 * *((unsigned char*)&result + 4),
 * *((unsigned char*)&result + 5),
 * *((unsigned char*)&result + 6),
 * *((unsigned char*)&result + 7));
 */

	time_result = result;
	time_val.tv_sec = time_result;
	time_val.tv_usec = 0;




	broken_time = localtime(&time_result);

	string = asctime(broken_time);
	printf("When this program crashes, the time will be: %s\n", string);
	settimeofday(&time_val, 0);
}






