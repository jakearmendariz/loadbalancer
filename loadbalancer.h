#ifndef load_h
#define load_h

#include<err.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#define __USE_GNU 
#include <pthread.h>     
#include <time.h>
#include <getopt.h>
#include <err.h>
#include <time.h>
#include <sys/time.h>

#define BUFFER_SIZE 4096


struct ServerInfo {
    int port;
    int fd;
    bool alive;
    int total_requests;
    int error_requests;
};

struct Servers {
    int num_servers;
    struct ServerInfo *servers;
    pthread_mutex_t mut;
    int num_threads;
    int request_count;
    int R;
    int N;
};

struct fd__set {
    int lb;
    int server;
};


//Debug function
void print(char *s);
void pint(int i);
void prints(char *s, char *a);
void printint(char *s, int a);

//socket functions 
int client_connect(uint16_t connectport);
int server_listen(int port);
int init_recursive_mutex(pthread_mutex_t *mutex);


void bridge_loop(void *data);

int bridge_connections(int fromfd, int tofd);

void *process_data(void *data);

void connect_and_serve(struct Servers *servers);

int content_length(char *buff);     //with sscanf
int get_content_length(char *buff); //with memmove

int read_and_write(int content_length, int read_from, int write_to);


//uses getopt to parse the inputs
int parse_and_create(int argc, char **argv, struct Servers *servers);

int choose_server(struct Servers *servers);

void *healthchecks(void *data);

void send500(int fd);
int double_carrage_index(char* request);


#endif
