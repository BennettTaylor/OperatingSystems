# Operating Systems
This repository holds a few projects to showcase operating systems knowledge including a file system, multithreading capabilities (scheduler, mutexes, barriers), copy on write (CoW) for thread local storage, and a shell which can parse and execute complex BASH commands. The README for each project is as follows:

## Shell
This project is meant to emulate the behavior of the linux shell by executing any BASH commands. It includes a custom parser for commands of variable length, including features like piping, input/output redirection, and creating background processes. The parser first breaks the command sting into the necessary components and then the shell will proceed to execute the command.

## FS
The purpose of this project was to implement a simple file system abstration ontop of a simulated "disk". The file system included a disk superblock, inode table, directory table and a file descriptor table. The functions that were implemented allows for a file system to be mounted and unmounted along with creating, reading, writing, opening, closing, truncating, and deleting files. Additional functions allowed for file pointer setting, listing files, and getting filesize. The most unique element of this implementation is the use of a bitmap to track disk usage, which is stored on the super block. There aren't enough bytes in a block for the super block to allocate a byte to track every disk block, so a single bit is used to track disk usage.

## Multithreading
The basic idea of this project is to implement a user-level threading system. 
This is accomplished by implementing a number of functions including pthread_create(), pthread_exit(), pthread_self() and pthread_join(). There were also a few internal functions developed behind the API, including a scheduling function which implemented round robin scheduling with context switches between the treads every 50ms.

## CoW
For this project I implemented a TLS which could be created, destroyed, read, written, and cloned. Each thread has its own TLS structure which stored metadata about the storage and had a pointer to the thread's page table. When cloned, not every page was copied into the new thread, instead they pointed to the same page table as the older thread and only changed to a unique page if the page was written to.
