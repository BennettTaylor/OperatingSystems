#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdbool.h>
#include "tls.h"

/* Constants */
#define SUCCESS 0
#define FAILURE -1
#define MAX_THREADS 128
#define PAGE_SIZE getpagesize()

/* Data Structures */
struct page {
    /* Address of page */
    unsigned long int address;
    /* Number of threads referencing the page */
    int ref_count;
};

struct TLS {
    /* ID of thread with local storage area */
    pthread_t tid;
    /* Size of the LSA in bytes */
    unsigned int size;
    /* Number of pages in LSA */
    int num_pages;
    /* Pointer to thread's LSA pages */
    struct page **pages;
};

/* Static Function Declarations */
static void tls_init(void);
static void tls_page_fault(int, siginfo_t *, void *);
static void tls_protect(struct page *);
static void tls_unprotect(struct page *);
static void tls_protect_read_only(struct page *);

/* Global Variables */
static struct TLS *thread_storage[MAX_THREADS];

/* Protect a memory page */
static void tls_protect(struct page *p)
{
    if (mprotect((void *) p->address, PAGE_SIZE, PROT_NONE)) {
        fprintf(stderr, "tls_protect: could not protect page\n");
        exit(1);
    }
}

/* Unprotect a memory page  */
static void tls_unprotect(struct page *p)
{
    if (mprotect((void *) p->address, PAGE_SIZE, PROT_READ | PROT_WRITE)) {
        fprintf(stderr, "tls_unprotect: could not unprotect page\n");
        exit(1);
    }
}

/* Protect a memory page to read only (for shared pages after CoW) */
static void tls_protect_read_only(struct page *p)
{
    if (mprotect((void *) p->address, PAGE_SIZE, PROT_READ)) {
        fprintf(stderr, "tls_protect_read_only: could not protect page to READ\n");
        exit(1);
    }
}

/* Handle segmentation fault signals */
static void tls_page_fault(int sig, siginfo_t *si, void *context)
{
    unsigned long int fault_addr;
    int i, j;

    /* Find the address of the page that caused the fault */
    fault_addr = ((unsigned long int) si->si_addr) & ~(PAGE_SIZE - 1);

    /* Check if the fault was within a TLS page */
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i]) {
            for (j = 0; j < thread_storage[i]->num_pages; j++) {
                // If the fault occurred on one of our TLS pages, it's a double fault or invalid access.
                if (thread_storage[i]->pages[j]->address == fault_addr) {
                    /* Fault was within a TLS. Terminate the thread. */
                    fprintf(stderr, "TLS Error: Segmentation fault in thread %ld at page %d. Exiting thread.\n",
                        (long) thread_storage[i]->tid, j);
                    pthread_exit(NULL);
                }
            }
        }
    }

    /* If not in a TLS, restore default handler and re-raise signal */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    raise(sig);
}

/* Initialize the signal handler for page faults */
static void tls_init(void)
{
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = tls_page_fault;
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
}

/* Create a new Thread-Local Storage area */
int tls_create(unsigned int size)
{
    static bool is_first_call = true;
    pthread_t tid;
    int i;

    /* One-time initialization of signal handler */
    if (is_first_call) {
        is_first_call = false;
        tls_init();
    }

    /* Validate input size */
    if (size <= 0) {
        return FAILURE;
    }

    /* Check if TLS already exists for the current thread */
    tid = pthread_self();
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i] && thread_storage[i]->tid == tid) {
            return FAILURE;
        }
    }

    /* Find an empty slot and create the new TLS */
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i] == NULL) {
            int j;

            /* Allocate memory for the TLS struct */
            thread_storage[i] = (struct TLS *) malloc(sizeof(struct TLS));

            /* Initialize TLS members */
            thread_storage[i]->tid = tid;
            thread_storage[i]->size = size;
            thread_storage[i]->num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
            thread_storage[i]->pages = (struct page **) calloc(thread_storage[i]->num_pages, sizeof(struct page *));

            /* Allocate and initialize pages */
            for (j = 0; j < thread_storage[i]->num_pages; j++) {
                thread_storage[i]->pages[j] = (struct page *) malloc(sizeof(struct page));
                // Map with PROT_NONE, access will be granted only through tls_read/write
                thread_storage[i]->pages[j]->address = (unsigned long int) mmap(0, PAGE_SIZE, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
                thread_storage[i]->pages[j]->ref_count = 1;
            }
            return SUCCESS;
        }
    }

    /* No empty slot found */
    return FAILURE;
}

/* Destroy the current thread's TLS */
int tls_destroy(void)
{
    pthread_t tid;
    int tid_index = -1;
    int i;

    /* Find the current thread's TLS index */
    tid = pthread_self();
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i] && thread_storage[i]->tid == tid) {
            tid_index = i;
            break;
        }
    }

    if (tid_index == -1) {
        return FAILURE;
    }

    /* Free or decrement reference count for each page */
    for (i = 0; i < thread_storage[tid_index]->num_pages; i++) {
        if (thread_storage[tid_index]->pages[i]->ref_count > 1) {
            thread_storage[tid_index]->pages[i]->ref_count--;          
        } else {
            munmap((void *) thread_storage[tid_index]->pages[i]->address, PAGE_SIZE);
            free(thread_storage[tid_index]->pages[i]);
        }
    }

    /* Free the pages array and the TLS struct itself */
    free(thread_storage[tid_index]->pages);
    free(thread_storage[tid_index]);
    thread_storage[tid_index] = NULL;

    return SUCCESS;
}

/* Read from the current thread's TLS */
int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
    pthread_t tid;
    int tid_index = -1;
    int i;
    unsigned int byte;

    /* Find the current thread's TLS index */
    tid = pthread_self();
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i] && thread_storage[i]->tid == tid) {
            tid_index = i;
            break;
        }
    }

    if (tid_index == -1) {
        return FAILURE;
    }

    /* Check for out-of-bounds access */
    if ((offset + length) > thread_storage[tid_index]->size) {
        return FAILURE;
    }

    /* Read bytes one by one */
    for (byte = 0; byte < length; byte++) {
        int page_num = (offset + byte) / PAGE_SIZE;
        int page_offset = (offset + byte) % PAGE_SIZE;
        struct page *current_page = thread_storage[tid_index]->pages[page_num];

        /* Unprotect, read, and re-protect the page */
        tls_unprotect(current_page);
        buffer[byte] = *((char *) current_page->address + page_offset);
        tls_protect(current_page);
    }

    return SUCCESS;
}

/* Write to the current thread's TLS */
int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
    pthread_t tid;
    int tid_index = -1;
    int i;
    unsigned int byte;

    /* Find the current thread's TLS index */
    tid = pthread_self();
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i] && thread_storage[i]->tid == tid) {
            tid_index = i;
            break;
        }
    }

    if (tid_index == -1) {
        return FAILURE;
    }

    /* Check for out-of-bounds access */
    if ((offset + length) > thread_storage[tid_index]->size) {
        return FAILURE;
    }

    /* Write bytes one by one */
    for (byte = 0; byte < length; byte++) {
        int page_num = (offset + byte) / PAGE_SIZE;
        int page_offset = (offset + byte) % PAGE_SIZE;
        struct page *current_page = thread_storage[tid_index]->pages[page_num];

        if (current_page->ref_count > 1) {
            /* Implement CoW */
            struct page *new_page;
            
            /* Allocate a new page */
            new_page = (struct page *) malloc(sizeof(struct page));
            // Map the new page with full write access
            new_page->address = (unsigned long int) mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
            new_page->ref_count = 1;
            
            /* Copy data from the old page */
            tls_unprotect(current_page);
            memcpy((void *) new_page->address, (void *) current_page->address, PAGE_SIZE);
            
            tls_protect_read_only(current_page);

            /* Decrement old page's ref count and update TLS to point to new page */
            current_page->ref_count--;
            thread_storage[tid_index]->pages[page_num] = new_page;
            current_page = new_page;
            tls_protect(current_page);
        }

        /* Unprotect, write, and reprotect the page */
        tls_unprotect(current_page);
        *((char *) current_page->address + page_offset) = buffer[byte];
        tls_protect(current_page);
    }
    
    return SUCCESS;
}

/* Clone the TLS from a target thread to the current thread */
int tls_clone(pthread_t tid)
{
    pthread_t current_tid;
    int target_tid_index = -1;
    int new_tid_index = -1;
    int i;

    current_tid = pthread_self();

    /* Check if current thread already has a TLS */
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i] && thread_storage[i]->tid == current_tid) {
            return FAILURE;
        }
    }

    /* Find the target thread's TLS and an empty slot for the new one */
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_storage[i]) {
            if (thread_storage[i]->tid == tid) {
                target_tid_index = i;
            }
        } else if (new_tid_index == -1) {
            new_tid_index = i;
        }
    }

    if (target_tid_index == -1 || new_tid_index == -1) {
        return FAILURE;
    }

    /* Allocate and initialize the new TLS struct */
    thread_storage[new_tid_index] = (struct TLS *) malloc(sizeof(struct TLS));
    thread_storage[new_tid_index]->tid = current_tid;
    thread_storage[new_tid_index]->size = thread_storage[target_tid_index]->size;
    thread_storage[new_tid_index]->num_pages = thread_storage[target_tid_index]->num_pages;
    thread_storage[new_tid_index]->pages = (struct page **) malloc(thread_storage[new_tid_index]->num_pages * sizeof(struct page *));

    /* Share pages by copying pointers and incrementing reference counts */
    for (i = 0; i < thread_storage[new_tid_index]->num_pages; i++) {
        struct page *shared_page = thread_storage[target_tid_index]->pages[i];
        shared_page->ref_count++;
        thread_storage[new_tid_index]->pages[i] = shared_page;
    }

    return SUCCESS;
}