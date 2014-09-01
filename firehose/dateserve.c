#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>



int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("Date Server Usage: dateserve port.\n");
		exit(1);
	}

	int sock, new_sock;
	struct sockaddr_in name;
	int port = atol(argv[1]);
	struct sockaddr_in clientname;
	socklen_t size = sizeof(clientname);
	int64_t result;
	time_t time_result;
	struct tm *broken_time;
	char *string;
	int pid;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0)
	{
		perror("bind");
		exit(1);
	}

	pid = fork();
	if(pid) exit(0);

	while(1)
	{
		if(listen(sock, 1) < 0)
		{
			perror("listen");
			exit(1);
		}

		if((new_sock = accept(sock, (struct sockaddr*)&clientname, &size)) < 0)
		{
			perror("accept");
			exit(1);
		}


		time(&time_result);
		broken_time = localtime(&time_result);
		string = asctime(broken_time);
		printf("Sending time: %s\n", string);

		result = time_result;

		write(new_sock, &result, sizeof(result));
		close(new_sock);
	}
	
}






