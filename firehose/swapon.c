#include <unistd.h>
#include <asm/page.h>
#include <sys/swap.h>


int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("Usage: swapon device.\n");
		exit(1);
	}

	printf("Enabling swap space on %s\n", argv[1]);
	int result = swapon(argv[1], 0);
	if(result)
	{
		perror("swapon");
	}
	return ;
}









