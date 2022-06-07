#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/sched.h>

static char *buffer = NULL;
int buffer_size = 0;
int buffer_index = 0;


pthread_mutex_t semaphore;

void print_sched(int policy)
{
	int priority_min, priority_max;
	//printf("teste %d", SCHED_DEADLINE);

	switch(policy)
	{
		case SCHED_DEADLINE:
			printf("SCHED_DEADLINE");
			break;
		case SCHED_FIFO:
			printf("SCHED_FIFO");
			break;
		case SCHED_RR:
			printf("SCHED_RR");
			break;
		case SCHED_NORMAL:
			printf("SCHED_OTHER");
			break;
		case SCHED_BATCH:
			printf("SCHED_BATCH");
			break;
		case SCHED_IDLE:
			printf("SCHED_IDLE");
			break;
		default:
			printf("unknown\n");
	}
	priority_min = sched_get_priority_min(policy);
	priority_max = sched_get_priority_max(policy);
	printf(" PRI_MIN: %d PRI_MAX: %d\n", priority_min, priority_max);
}

int setpriority(pthread_t *thr, int newpolicy, int newpriority)
{
	int policy, ret;
	struct sched_param param;

	if (newpriority > sched_get_priority_max(newpolicy) || newpriority < sched_get_priority_min(newpolicy))
	{
		printf("Invalid priority: MIN: %d, MAX: %d", sched_get_priority_min(newpolicy), sched_get_priority_max(newpolicy));

		return -1;
	}

	pthread_getschedparam(*thr, &policy, &param);
	printf("current: ");
	print_sched(policy);

	param.sched_priority = newpriority;
	ret = pthread_setschedparam(*thr, newpolicy, &param);
	if (ret != 0)
		perror("perror(): ");

	pthread_getschedparam(*thr, &policy, &param);
	printf("new: ");
	print_sched(policy);

	return 0;
}

void *char_printer(void *input) 
{
	char c = (char)input;

	while(buffer_index < buffer_size-1) 
	{
		// Critica
		pthread_mutex_lock(&semaphore);
			buffer[buffer_index] = c;
			buffer_index++;
		pthread_mutex_unlock(&semaphore);
		
		if(buffer_index>=buffer_size)
			return 0;
	}
}

int main(int argc, char **argv)
{
	//thread_runner <número_de_threads> <tamanho_do_buffer_global_em_kilobytes> <politica> <prioridade>

	if (argc < 5)
	{
		printf("usage: %s <número_de_threads> <tamanho_do_buffer_global_em_kilobytes> <politica> <prioridade>\n\n", argv[0]);
		printf("Politicas:\n");
		printf("%s: %d\n", "SCHED_OTHER", SCHED_OTHER);
		printf("%s: %d\n", "SCHED_FIFO", SCHED_FIFO);
		printf("%s: %d\n", "SCHED_RR", SCHED_RR);
		printf("%s: %d\n", "SCHED_BATCH", SCHED_BATCH);
		printf("%s: %d\n", "SCHED_IDLE", SCHED_IDLE);
		printf("%s: %d\n", "SCHED_DEADLINE", SCHED_DEADLINE );
		return 0;
	}

	pthread_t thr[atoi(argv[1])];
	buffer_size = atoi(argv[2])*1024;
	buffer = (char *)malloc((buffer_size-1)*sizeof(char));
	int thr_policy = atoi(argv[3]);
	int thr_priority = atoi(argv[4]);


	if (pthread_mutex_init(&semaphore, NULL) != 0)
    	{
		printf("\n mutex init failed\n");
		return 1;
    	}

	pthread_mutex_lock(&semaphore);
	for(int i=0; i<atoi(argv[1]); i++) 
	{
		pthread_create(&(thr[i]), NULL, char_printer, (void *)(97+i));
		setpriority(&(thr[i]), thr_policy, thr_priority);
		//thr_priority++;
	}
	pthread_mutex_unlock(&semaphore);

	for(int i=0; i<atoi(argv[1]); i++) 
	{
		pthread_join(thr[i], NULL);
	}

	printf("\n%i\n\n", buffer_size);
	printf("\n%s\n",buffer);


	char *normalized_buffer = (char *)malloc((buffer_size-1)*sizeof(char));
	int normalize_index = 1;
	char last_char = buffer[0];
	normalized_buffer[0] = last_char;
	int occurrences[atoi(argv[1])];

	for (int i = 0; i < atoi(argv[1]); i++)
	{
		occurrences[i] = 0;
	}

	occurrences[last_char-97]++;	

	for(int i=1; i<buffer_size+2; i++) 
	{
		if(buffer[i] != last_char) 
		{
			occurrences[buffer[i]-97]++;
			normalized_buffer[normalize_index] = buffer[i];
			normalize_index++;
		}
		last_char = buffer[i];
	}

	printf("\n\n%s\n\n", normalized_buffer);

	for (int i = 0; i < atoi(argv[1]); i++)
	{
		printf("%c: %d\n", i+97, occurrences[i]);
	}

	return 0;
}
