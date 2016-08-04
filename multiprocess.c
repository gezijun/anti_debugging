#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>  
#include <signal.h>

#define DBG
void facker (){}

#ifdef DBG
#define XXX_DEBUG_LOG(x) puts(x)
#else 
#define XXX_DEBUG_LOG(x) faker();
#endif

union semun  
{  
	int val;  
	struct semid_ds *buf;  
	unsigned short *array;  
};  

void init_sem (int sem_id, int init_value)  
{  
	union semun sem_union;  

	sem_union.val = init_value;  

	if (semctl (sem_id, 0, SETVAL, sem_union) < 0) {  
		perror ("failed to init_sem");  
		exit (-1);  
	}  

	return ;  
}  

void delete_sem (int sem_id)  
{  
	union semun sem_union;  

	if (semctl(sem_id, 0, IPC_RMID, sem_union) < 0) {  
		perror ("failed to delete_sem");  
		exit (-1);  
	}  

	return ;  
}  

void sem_p (int sem_id)  
{  
	struct sembuf sem_b;  

	sem_b.sem_num = 0;  
	sem_b.sem_op = -1;  
	sem_b.sem_flg = SEM_UNDO;  

	if (semop(sem_id, &sem_b, 1) < 0) {  
		perror ("failed to sem_p");  
		exit (-1);  
	}  

	return;  
}  

void sem_v (int sem_id)  
{  
	struct sembuf sem_b;  

	sem_b.sem_num = 0;  
	sem_b.sem_op = 1;  
	sem_b.sem_flg = SEM_UNDO;  

	if (semop(sem_id, &sem_b, 1) < 0) {  
		perror ("failed to sem_v");  
		exit (-1);  
	}  

	return ;  
}  

pid_t *getTids(const pid_t pid, size_t *const countptr)
{
	char dirbuf[128];
	DIR *dir;
	struct dirent *ent;

	pid_t *data = NULL, *temp;
	size_t size = 0;
	size_t used = 0;

	int tid;
	char dummy;

	if ((int)pid < 2) {
		errno = EINVAL;
		return NULL;
	}

	if (snprintf (dirbuf, sizeof dirbuf, "/proc/%d/task/", (int)pid) >= (int)sizeof dirbuf) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	dir = opendir (dirbuf);
	if (!dir)
		return NULL;

	while (1) {
		errno = 0;
		ent = readdir(dir);
		if (!ent)
			break;

		if (sscanf(ent->d_name, "%d%c", &tid, &dummy) != 1)
			continue;

		if (tid < 2)
			continue;

		if (used >= size) {
			size = (used | 127) + 129;
			temp = realloc (data, size * sizeof data[0]);
			if (!temp) {
				free (data);
				closedir (dir);
				errno = ENOMEM;
				return NULL;
			}
			data = temp;
		}

		data[used++] = (pid_t)tid;
	}
	if (errno) {
		free (data);
		closedir (dir);
		errno = EIO;
		return NULL;
	}
	if (closedir (dir)) {
		free (data);
		errno = EIO;
		return NULL;
	}

	if (used < 1) {
		free (data);
		errno = ENOENT;
		return NULL;
	}

	size = used + 1;
	temp = realloc (data, size * sizeof data[0]);
	if (!temp) {
		free (data);
		errno = ENOMEM;
		return NULL;
	}
	data = temp;

	data[used] = (pid_t)0;

	if (countptr)
		*countptr = used;

	errno = 0;
	return data;
}


static pid_t g_pPid = -1;
static pid_t g_childPid = -1;

void *srvThread ()
{
	printf ("srvThread started\n");
	pid_t child = fork();

	if (child == (pid_t)-1) {
		printf ("fork() failed: %s\n", strerror(errno));
		exit (11);
	}

	if (!child) {	
		printf ("Child started, pid = %d\n", getpid ());

		pid_t 	*tid = NULL;
		size_t	tids = 0;
		key_t 	key;  
		char 	*p = NULL;  
		int 	shmid;  
		int 	create_flag = 0;  
		int 	sem_id;  

		ptrace (PTRACE_TRACEME, 0, NULL, NULL);
		g_pPid = getppid ();
		tid = getTids (g_pPid, &tids);
		if (!tid) {
			printf ("getTids(): %s\n", strerror (errno));
		}
		printf ("Parent process %d has %d tasks.\n", (int)g_pPid, (int)tids);

		if ((key = ftok(".", 'a')) < 0) {  
			perror("failed to get key");  
			exit(-1);  
		}  

		if ((sem_id = semget (key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {  
			if (errno == EEXIST) {  
				if ((sem_id = semget (key, 1, 0666)) < 0) {  
					perror ("failed to semget");  
					exit (-1);  
				}  
			}  
		}  

		init_sem(sem_id, 0);  

		if ((shmid = shmget (key, 64, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {  
			if (errno == EEXIST) {         
				if ((shmid = shmget (key, 64, 0666)) < 0) {  
					perror ("failed to shmget memory");  
					exit (-1);  
				}  
			}  
			else {  
				perror ("failed to shmget");  
				exit (-1);  
			}  
		}  
		else { 
			create_flag = 1;
		}

		if ((p = shmat (shmid, NULL, 0)) == (void *)(-1)) {  
			perror ("failed to shmat memory");  
			exit (-1);  
		}  		

		for (;;) {
			if (ifPStatusNormal (tid, tids, p)) break;
		}

		if (create_flag == 1) {  
			if (shmdt (p) < 0) {  
				perror ("failed to shmdt memory");  
				exit (-1);  
			}  

			if (shmctl (shmid, IPC_RMID, NULL) == -1) {     
				perror ("failed to delete share memory");  
				exit (-1);  
			}  

			delete_sem (sem_id);  
		} 

		exit (11);
	}

	int stat;
	g_childPid = child;
	while (waitpid (g_childPid, &stat, 0) ) {
		if (WIFEXITED (stat) || WIFSIGNALED(stat)) {
			XXX_DEBUG_LOG ("waitpid : child died\n");
			exit (11);
		}
		ptrace (PTRACE_CONT, g_childPid, NULL, NULL);
	}
}

int ifPStatusNormal (pid_t *tid, int tids, char *p) 
{
	//XXX_DEBUG_LOG ("Enter ifPStatusNormal");
	int 	k;
	FILE	*fp = NULL;
	char 	pathName [256];
	char 	readLine [256];
	for (k = 0; k < (int)tids; k++) {
		const pid_t t = tid[k];
		snprintf (pathName, 256-1, "/proc/%d/task/%d/status", g_pPid, t);
		fp = fopen (pathName, "r");
		if (fp == NULL) {
			printf ("can't open status file : %s\n", pathName);
		}
		while (fgets (readLine, 256-1, fp)) {
			if (strncmp (readLine, "State", 5) == 0) {
				if (strstr (readLine, "stop") != 0) {
					XXX_DEBUG_LOG ("DEBUG : TID status error, state");
					memcpy (p, "quit", 4); // not use sem_v
					kill (g_pPid, SIGKILL);
					fclose (fp);
					return 1;
				}
				break;
			}
			if (strncmp (readLine, "TracerPid", 9) == 0) {
				if (0 != atoi (&readLine[10])) {
					printf ("DEBUG : TID status error, TracerPid : %d\n", atoi (&readLine[10]));
					memcpy (p, "quit", 4);
					kill (g_pPid, SIGKILL);
					fclose (fp);
					return 1;
				}
				break;
			}
		}
		fclose(fp);
		fp = NULL;
	}
	snprintf (pathName, 256-1, "/proc/%d/status", g_pPid);
	fp = fopen (pathName, "r");
	if (fp == NULL) {
		printf ("can't open status file : %s\n", pathName);
	}
	while (fgets (readLine, 256-1, fp)) {
		if (strncmp (readLine, "TracerPid", 9) == 0) {
			if (0 != atoi (&readLine[10])) {
				XXX_DEBUG_LOG ("DEBUG : PPID debugged");
				memcpy (p, "quit", 4);
				kill (g_pPid, SIGKILL);
				fclose (fp);
				return 1;
			}
			break;
		}
	}
	fclose(fp);

	return 0;
}

void *ifChildExited ()
{
	printf ("ifChildExited started\n");

	key_t 	key;
	char 	*p;
	int 	shmid;
	int 	sem_id; 
	int 	create_flag = -1;
	if ((key = ftok (".", 'a')) < 0) {  
		perror ("failed to get key");  
		exit (-1);  
	}  

	if ((sem_id = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {  
		if (errno == EEXIST) {  
			if ((sem_id = semget(key, 1, 0666)) < 0) {  
				perror("failed to semget");  
				exit(-1);  
			}  
		}  
	}  

	init_sem(sem_id, 0);  
	if ((shmid = shmget (key, 64, 0666 | IPC_CREAT | IPC_EXCL)) < 0) {  
		if (errno == EEXIST) {         
			if ((shmid = shmget (key, 64, 0666)) < 0) {  
				perror("failed to shmget memory");  
				exit(-1);  
			}  
		}  
		else {  
			perror ("failed to shmget");  
			exit (-1);  
		}  
	}  
	else { 
		create_flag = 1;  
	}
	if ((p = shmat (shmid, NULL, 0)) == (void *)(-1)) {  
		perror("failed to shmat memory");  
		exit(-1);  
	} 

	memset (p, 0x0, 64);
	char fileName [256];
	while (1) {	
		if (g_childPid == -1) continue;
		if (strncmp(p, "quit", 4) == 0) {
			XXX_DEBUG_LOG ("ifChildExited : DEBUG");
			break;
		}
		snprintf (fileName, 255, "/proc/%d/status", g_childPid);
		if (-1 == access (fileName, F_OK) ) {
			XXX_DEBUG_LOG ("ifChildExited : child died");
			break;
		}
	}

	if (create_flag == 1) {  
		if (shmdt(p) < 0) {  
			perror("failed to shmdt memory");  
			exit(-1);  
		}  

		if (shmctl(shmid, IPC_RMID, NULL) == -1) {	  
			perror("failed to delete share memory");  
			exit(-1);  
		}  

		delete_sem(sem_id);  
	}  
	exit (11);
}

int main ()
{
	pthread_t pt_m, pt_d;
	if (pthread_create (&pt_m, NULL, srvThread, NULL)) {
		printf ("Error creating thread\n");
		return 1;
	}

	if (pthread_create (&pt_d, NULL, ifChildExited, NULL)) {
		printf ("Error creating thread\n");
		return 2;
	}

	if (pthread_join (pt_m, NULL) || pthread_join (pt_d, NULL) ) {
		printf ("Error joining thread\n");
		return 3;
	}
	for (;;);
	return 0;
}

