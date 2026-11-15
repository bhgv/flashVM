/* Copyright (c) 2026 bhgv
 * EN: Verify native lib9 formatter, error and hardware FP state on concurrent threads.
 * RU: Проверить форматтер, ошибки lib9 и аппаратное FP-состояние параллельных потоков.
 */
#include <lib9.h>
#include <pthread.h>

static pthread_barrier_t rendezvous; /* Eight native workers synchronize their observations. */
static int failures[8]; /* Each worker owns exactly one result slot. */

/* EN: Exercise independent error/FP state and shared formatter registration.
 * RU: Проверить независимые ошибки/FP и общий реестр форматтера.
 * arg is a borrowed worker index; each worker restores its original hardware control.
 * arg — заимствованный индекс; поток восстанавливает исходное управление FP. */
static void*
worker(void *arg)
{
	int i, n;
	ulong saved, rounding;
	char expected[64], actual[64], number[64];

	i = *(int*)arg;
	saved = getfcr();
	rounding = (i%4)*(FPRMASK/3);
	setfcr((saved&~FPRMASK)|rounding);
	for(n = 0; n < 100; n++){
		snprint(expected, sizeof(expected), "worker %d iteration %d", i, n);
		werrstr("%s", expected);
		pthread_barrier_wait(&rendezvous);
		rerrstr(actual, sizeof(actual));
		if(strcmp(actual, expected) != 0 || (getfcr()&FPRMASK) != rounding)
			failures[i]++;
		snprint(number, sizeof(number), "%llux", 0xfedcba9876543210ULL);
		if(strcmp(number, "fedcba9876543210") != 0)
			failures[i]++;
		pthread_barrier_wait(&rendezvous);
	}
	setfcr(saved);
	return nil;
}

/* EN: Start and join native workers; nonzero exits report thread or isolation failures.
 * RU: Запустить и дождаться native-потоков; ненулевой код означает ошибку изоляции/потока. */
int
main(void)
{
	pthread_t threads[8];
	int indices[8], i;
	ulong saved;

	saved = getfcr();
	werrstr("main thread");
	if(pthread_barrier_init(&rendezvous, nil, 8) != 0)
		return 2;
	for(i = 0; i < 8; i++){
		indices[i] = i;
		if(pthread_create(&threads[i], nil, worker, &indices[i]) != 0)
			exit(2);
	}
	for(i = 0; i < 8; i++){
		if(pthread_join(threads[i], nil) != 0 || failures[i] != 0)
			return 1;
	}
	pthread_barrier_destroy(&rendezvous);
	if(getfcr() != saved)
		return 1;
	{
		char error[64];
		rerrstr(error, sizeof(error));
		if(strcmp(error, "main thread") != 0)
			return 1;
	}
	print("native lib9 thread isolation: passed\n");
	return 0;
}
