#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include "threads.h"


#define MAX_THREADS 128
#define THREAD_STACK_SIZE (1<<15)
#define QUANTUM (50 * 1000)

enum thread_status
{
 TS_EXITED,
 TS_RUNNING,
 TS_READY
};

struct thread_control_block {
	pthread_t ID;
	enum thread_status status;
	jmp_buf reg_buffer;
	unsigned long int *stack_ptr;
	void *retval;
	struct thread_control_block *next;
	struct thread_control_block *prev;
};

struct mutex_info {
	bool is_locked;
	bool is_init;
};

struct barrier_info {
	int current_count;
	int max_count;
	int exited;
};

struct thread_control_block *current_thread = NULL;
int num_running_threads = 0;
int num_thread_total = 0;

static void schedule(int signal);

static void lock() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, NULL);
}

static void unlock() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}

/* Mutex initialization function */
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr) {
	lock();
	struct mutex_info *new_mutex = malloc(sizeof(struct mutex_info));
	new_mutex->is_locked = false;
	new_mutex->is_init = true;
	mutex->__align = (long) new_mutex;
	unlock();
	return 0;
}

/* Mutex destructor function */
int pthread_mutex_destroy(pthread_mutex_t *mutex) {
	lock();
	struct mutex_info *new_mutex = (struct mutex_info *) mutex->__align;
	new_mutex->is_locked = false;
	new_mutex->is_init = false;
	free(new_mutex);
	mutex->__align = (long) new_mutex;
	unlock();
	return 0;
}

/* Function for locking a given mutex */
int pthread_mutex_lock(pthread_mutex_t *mutex) {
	lock();
	struct mutex_info *new_mutex = (struct mutex_info *) mutex->__align;
	while (new_mutex->is_locked) {
		unlock();
		schedule(0);
	}
	new_mutex->is_locked = true;
	mutex->__align = (long) new_mutex;
	unlock();
	return 0;
}

/* Function for unlocking a given mutex */
int pthread_mutex_unlock(pthread_mutex_t *mutex) {
	lock();
	struct mutex_info *new_mutex = (struct mutex_info *) mutex->__align;
	new_mutex->is_locked = false;
	mutex->__align = (long) new_mutex;
	unlock();
	return 0;
}

/* Function for initialization of a barrier */
int pthread_barrier_init(pthread_barrier_t *restrict barrier, const pthread_barrierattr_t *restrict attr, unsigned count) {
	lock();
	if (count == 0) {
		unlock();
		return EINVAL;
	}
	struct barrier_info *new_barrier = malloc(sizeof(struct barrier_info));
	new_barrier->max_count = count;
	new_barrier->current_count = 0;
	new_barrier->exited = 0;
	barrier->__align = (long) new_barrier;
	unlock();
	return 0;
}

/* Barrier destructor function */
int pthread_barrier_destroy(pthread_barrier_t *barrier) {
	lock();
	struct barrier_info *new_barrier = (struct barrier_info *) barrier->__align;
	new_barrier->max_count = 0;
	new_barrier->current_count = 0;
	new_barrier->exited = 1;
	free(new_barrier);
	barrier->__align = (long) NULL;
	unlock();
	return 0;
}

/* Barrier wait function */
int pthread_barrier_wait(pthread_barrier_t *barrier) {
	lock();
	int is_serial = 0;
	struct barrier_info *new_barrier = (struct barrier_info *) barrier->__align;
	if (new_barrier->exited == 1) {
		new_barrier->current_count = 0;
		new_barrier->exited = 0;
	}
	new_barrier->current_count++;
	if (new_barrier->current_count >= new_barrier->max_count) {
		is_serial = PTHREAD_BARRIER_SERIAL_THREAD;
	}
	while (!new_barrier && (new_barrier->current_count < new_barrier->max_count)) {
		unlock();
		schedule(0);
	}
	new_barrier->exited = 1;
	barrier->__align = (long) new_barrier;
	unlock();
	return is_serial;
}

/* Scheduler function */
static void schedule(int signal)
{
	/* Setting current thread to ready if it hasn't just exited */
	if (current_thread->status != TS_EXITED) {
		current_thread->status = TS_READY;
	}

	/* Get ready for jump to next thread */
	if (setjmp(current_thread->reg_buffer) == 0) {
		current_thread = current_thread->next;
		int i = 0;
		/* Finding next ready thread, checking that there aren't too many */
		while (current_thread->status != TS_READY) {
			current_thread = current_thread->next;
			i++;
			if (i > MAX_THREADS) {
				return;
			}
		}
		/* Jump to next thread */
		current_thread->status = TS_RUNNING;
		longjmp(current_thread->reg_buffer, 1);
	}
	else {
		/* Running current thread again, continue... */
	}
}

static void schedule(int signal) __attribute__((unused));

/* Scheduler initialization function */
static void scheduler_init()
{
	/* Set up TCB for first 'main' thread */
	current_thread = (struct thread_control_block *) malloc(sizeof(struct thread_control_block));
	num_running_threads++;
	current_thread->next = current_thread;
	current_thread->prev = current_thread;
	current_thread->ID = num_thread_total;
	current_thread->status =  TS_READY;
	current_thread->stack_ptr = NULL;
	num_thread_total++;

	/* Set up alarm */
	struct sigaction sa;
	sa.sa_handler = &schedule;
	sa.sa_flags = SA_NODEFER;
	sigaction(SIGALRM, &sa, 0);
	ualarm(QUANTUM, QUANTUM);
	return;
}	

/* Thread creation function */
int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	/* Create the timer and handler for the scheduler. Create thread 0. */
	static bool is_first_call = true;
	if (is_first_call) {
		is_first_call = false;
		scheduler_init();
	}

  	/* Check that there aren't too many threads */
	if (num_running_threads < MAX_THREADS) {
		/* Adding thread to circular control block */
		struct thread_control_block *new = (struct thread_control_block *) malloc(sizeof(struct thread_control_block));
		num_running_threads++;
		current_thread->prev->next = new;
		new->prev = current_thread->prev;
		current_thread->prev = new;
		new->next = current_thread;

		/* Setting thread ID */
		new->ID = num_thread_total;
		*thread = new->ID;
		num_thread_total++;

		/* Setting up stack and return to pthread exit */
		new->stack_ptr = (unsigned long int *) malloc(THREAD_STACK_SIZE);
		unsigned long int *temp_stack_ptr = new->stack_ptr;
		temp_stack_ptr = (temp_stack_ptr + (THREAD_STACK_SIZE / sizeof(unsigned long int) -1));
		*temp_stack_ptr = (unsigned long int) &pthread_exit;

		/* Setting up new thread registers */
		setjmp(new->reg_buffer);

		set_reg(&new->reg_buffer, JBL_PC, (unsigned long int) start_thunk);
		set_reg(&new->reg_buffer, JBL_R12, (unsigned long int) start_routine);
		set_reg(&new->reg_buffer, JBL_R13, (unsigned long int) arg);
		set_reg(&new->reg_buffer, JBL_RSP, (unsigned long int) temp_stack_ptr);
		
		new->status = TS_READY;
		/* Indicate that thread was created successfully */
		return 0;
	}


  return -1;
}

/* Thread exit function */
void pthread_exit(void *value_ptr)
{
	/* Set variables to indicate exiting */
	num_running_threads--;
	current_thread->status = TS_EXITED;
	current_thread->retval = value_ptr;

	/* Continue on if there are more threads */
	if (num_running_threads > 0) {
		schedule(0);
	}
	else {
		/* Stop timer, exit */
		ualarm(0,0);
		schedule(0);
	}
	exit(0);
}

pthread_t pthread_self(void)
{
  /* Return current thread's ID */
  return current_thread->ID;
}

/* Thread join function */
int pthread_join(pthread_t thread, void **retval)
{
	/* Find thread */
	struct thread_control_block *temp = current_thread;
	while (temp->ID != thread) {
		temp = temp->next;
	}
	/* Waiting for thread to be finished */
	while (temp->status != TS_EXITED) {
		schedule(SIGALRM);
	}
	/* Set return value */
	if (retval != NULL) {
		*retval = temp->retval;
	}
	/* Free the stack */
	if (temp->stack_ptr) {
		free(temp->stack_ptr);
	}
	return 0;
}
