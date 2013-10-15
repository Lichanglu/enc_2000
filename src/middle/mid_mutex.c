#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
//#include "common.h"
#include "mid_mutex.h"

#include "log_common.h"

mid_mutex_t mid_mutex_create(void)
{
	mid_mutex_t mutex;

	mutex = (mid_mutex_t)malloc(sizeof(struct mid_mutex));

	if(mutex == NULL) {
		ERR_OUT("\n");
	}

	if(pthread_mutex_init(&mutex->id, NULL)) {
		free(mutex);
		ERR_OUT("\n");
	}

	return mutex;
Err:
	return NULL;
}

int mid_mutex_lock(mid_mutex_t mutex)
{
	if(mutex == NULL) {
		ERR_OUT("\n");
	}


	pthread_mutex_lock(&mutex->id);
	return 0;
Err:
	return -1;
}

int mid_mutex_unlock(mid_mutex_t mutex)
{
	if(mutex == NULL) {
		ERR_OUT("\n");
	}

	pthread_mutex_unlock(&mutex->id);

	return 0;
Err:
	return -1;
}

void mid_mutex_destroy(mid_mutex_t *mutex)
{
	if(*mutex == NULL) {
		return ;
	}

	free(*mutex);
	*mutex = NULL;
}
