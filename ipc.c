#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#define __KERNEL__
#include <linux/linkage.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>


union semun {
   int val; /* used for SETVAL only */
   struct semid_ds *buf; /* for IPC_STAT and IPC_SET */
   ushort *array;  /* used for GETALL and SETALL */
};

int do_shm()
{
	int maxid, shmid, id;
	struct shmid_ds shmseg;
	struct shm_info shm_info;
	struct shminfo shminfo;
	struct ipc_perm *ipcp = &shmseg.shm_perm;
	struct passwd *pw;

	maxid = shmctl (0, SHM_INFO, (struct shmid_ds *) &shm_info);
	if (maxid < 0) {
		printf ("kernel not configured for shared memory\n");
		return 0;
	}
	
	for (id = 0; id <= maxid; id++) 
	{
		shmid = shmctl (id, SHM_STAT, &shmseg);

		if (shmid  < 0) 
			continue;

		if (!shmctl (shmid, IPC_RMID, NULL))
		{
			printf("Deleted shared memory %d\n", shmid);
		}
		else
		perror ("shmctl"); 
	}
	return 0;
}

int do_sem()
{
	int maxid, semid, id;
	struct semid_ds semary;
	struct seminfo seminfo;
	struct ipc_perm *ipcp = &semary.sem_perm;
	struct passwd *pw;
	union semun arg;

	arg.array = (ushort *)  &seminfo;
	maxid = semctl (0, 0, SEM_INFO, arg);
	if (maxid < 0) {
		printf ("kernel not configured for semaphores\n");
		return 0;
	}
	
	for (id = 0; id <= maxid; id++) {
		arg.buf = (struct semid_ds *) &semary;
		semid = semctl (id, 0, SEM_STAT, arg);

		if (semid < 0) 
			continue;

//printf("%d\n", semid);
		if (!semctl (semid, 0, IPC_RMID, arg)) 
		{
			printf("Deleted semaphore %d\n", semid);
		}
		else
		perror ("semctl"); 
	}
	return 0;
}

int do_msg()
{
	int maxid, msqid, id;
	struct msqid_ds msgque;
	struct msginfo msginfo;
	struct ipc_perm *ipcp = &msgque.msg_perm;
	struct passwd *pw;

	maxid = msgctl (0, MSG_INFO, (struct msqid_ds *) &msginfo);
	if (maxid < 0) {
		printf ("kernel not configured for shared memory\n");
		return 0;
	}
	
	for (id = 0; id <= maxid; id++) {
		msqid = msgctl (id, MSG_STAT, &msgque);
		
		if (msqid  < 0) 
			continue;
		
		if (!msgctl (msqid, IPC_RMID, NULL)) 
		{
			printf("Deleted message %d\n", msqid);
		}
		else
		perror ("msgctl"); 
	}
	return 0;
}







int main()
{
	do_shm();
	do_sem();
	do_msg();
	return 0;
}
