#ifndef SFSLIB_H
#define SFSLIB_H 

int get(char *, char**);
int get_writer(char *, size_t, size_t, char *);
int isdir(char*);

//GLOBALS
char* get_resp;
char* post_resp;
char* put_resp;
char* delete_resp;
static const char* sfs_server = "http://localhost:8080";

pthread_mutex_t get_lock;
pthread_mutex_t put_lock;
pthread_mutex_t post_lock;
pthread_mutex_t delete_lock;

#endif
