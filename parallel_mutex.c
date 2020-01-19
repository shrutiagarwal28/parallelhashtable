/* Operating Systems Homework 4:


Part1: 
Loss can occur only during insertion, therefore locking mechanism is used only for inserting values 
parallelly into the hashtable. Since losses don't happen during the retrieval of keys, there is no 
need for additional code in retrieve function to handle parallel execution.

Part2: 
No losses occur after implementation of mutex locks and spinlocks, as the actual insertion of keys 
is set as the critical section of the insert function which prevents any race condition from 
happening. As we know that, mutex and spinlock can be used interchangeably but spinlock is faster 
than mutex.
Upon execution we observe that code which uses spinlock runs a bit slower than the one with mutex 
lock. This happens because in spinlock the thread enters a busy-waiting state in which it keeps 
checking the lock until its released and wastes CPU cycles whereas in mutex, the thread busy-waiting 
thread is blocked or put to sleep and the CPU executes other threads from the ready queue.
	Mutex
		- 1 Thread 6.305449 sec
		- 5 Threads 2.584941 sec
		- 10 Threads 2.656845 sec
		- 20 Threads 2.745133 sec

	Spinlock
		- 1 Thread 6.534183 sec
		- 5 Threads 2.620264 sec
		- 10 Threads 2.805377 sec
		- 20 Threads 3.645035 sec


Part 3: 
Insert failed in the parallel_hastable.c due to occurrence of race condition which was fixed by 
using locks. However, retrieve doesn't need locks in order to get keys from the hashtable parallelly,
because the retrieval of keys is performed in a single step, therefore race conditions don't happen.
Hence, locks are only implemented for the insert function and not the retrieve function.


Part 4: 
The file was modified to make insert function run parallelly using multiple threads with the 
help of locks.
If i has the same value as key % NUM_BUCKETS, the locks are set. When a race condition happens,
if the same bucket is accessed the thread acquires the lock and it enters into waiting state.
If a different bucket is accessed, the lock is released and can be acquired by another thread.
In the code given, the hashtable is considered as a shared resource.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

#define NUM_BUCKETS 5     // Buckets in hash table
#define NUM_KEYS 100000   // Number of keys inserted per thread
int num_threads = 1;      // Number of threads (configurable)
int keys[NUM_KEYS];
pthread_mutex_t lock[NUM_BUCKETS]; // declare a lock

typedef struct _bucket_entry {
	int key;
	int val;
	struct _bucket_entry* next;
} bucket_entry;

bucket_entry* table[NUM_BUCKETS];

void panic(char* msg) {
	printf("%s\n", msg);
	exit(1);
}

double now() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Inserts a key-value pair into the table
void insert(int key, int val) {
	int i = key % NUM_BUCKETS;
	bucket_entry* e = (bucket_entry*)malloc(sizeof(bucket_entry));
	if (!e) panic("No memory to allocate bucket!");
	pthread_mutex_lock(&lock[i]); //acquire lock
	e->next = table[i];
	e->key = key;
	e->val = val;
	table[i] = e;
	pthread_mutex_unlock(&lock[i]); // release lock
}

// Retrieves an entry from the hash table by key
// Returns NULL if the key isn't found in the table
bucket_entry* retrieve(int key) {
	bucket_entry* b;
	for (b = table[key % NUM_BUCKETS]; b != NULL; b = b->next) {
		if (b->key == key)
			return b;
	}
	return NULL;
	
}

void* put_phase(void* arg) {
	long tid = (long)arg;
	int key = 0;

	// If there are k threads, thread i inserts
	//      (i, i), (i+k, i), (i+k*2)
	for (key = tid; key < NUM_KEYS; key += num_threads) {
		insert(key, tid);
	}

	pthread_exit(NULL);
}

void* get_phase(void* arg) {
	long tid = (long)arg;
	int key = 0;
	long lost = 0;

	for (key = tid; key < NUM_KEYS; key += num_threads) {
		if (retrieve(key) == NULL) lost++;
	}
	printf("[thread %ld] %ld keys lost!\n", tid, lost);

	pthread_exit((void*)lost);
}

int main(int argc, char** argv) {
	long i;
	pthread_t* threads;
	double start, end;

	if (argc != 2) {
		panic("usage: ./parallel_hashtable <num_threads>");
	}
	if ((num_threads = atoi(argv[1])) <= 0) {
		panic("must enter a valid number of threads to run");
	}

	srandom(time(NULL));
	for (i = 0; i < NUM_KEYS; i++)
		keys[i] = random();

	//init the lock	
	for(i=0; i < NUM_BUCKETS; i++)
		pthread_mutex_init(&lock[i], NULL);
																  
	threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
	if (!threads) {
		panic("out of memory allocating thread handles");
	}
	
	// Insert keys in parallel
	start = now();
	for (i = 0; i < num_threads; i++) {
		pthread_create(&threads[i], NULL, put_phase, (void*)i);
	}

	// Barrier
	for (i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}
	end = now();

	printf("[main] Inserted %d keys in %f seconds\n", NUM_KEYS, end - start);

	// Reset the thread array
	memset(threads, 0, sizeof(pthread_t) * num_threads);

	// Retrieve keys in parallel
	start = now();
	for (i = 0; i < num_threads; i++) {
		pthread_create(&threads[i], NULL, get_phase, (void*)i);
	}

	// Collect count of lost keys
	long total_lost = 0;
	long* lost_keys = (long*)malloc(sizeof(long) * num_threads);
	for (i = 0; i < num_threads; i++) {
		pthread_join(threads[i], (void**)&lost_keys[i]);
		total_lost += lost_keys[i];
	}
	end = now();

	printf("[main] Retrieved %ld/%d keys in %f seconds\n", NUM_KEYS - total_lost, NUM_KEYS, end - start);

	return 0;
}


