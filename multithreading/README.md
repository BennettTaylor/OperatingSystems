The basic idea of this project is to implement a user-level threading system. 
This is accomplished by implementing a number of functions including pthread_create(), pthread_exit(), pthread_self() and pthread_join(). There were also a few internal functions developed behind the API, including a scheduling function which implemented round robin scheduling with context switches between the treads every 50ms.

