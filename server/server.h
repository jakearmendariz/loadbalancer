/**
 * server.h
 * 
 * Contains object and functio declarations for httpserver
 */
#ifndef server_h
#define server_h

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#define __USE_GNU 
#include <pthread.h>     
#include <time.h>
#include <getopt.h>


//warn
#include <err.h>
#define PORT 8080 
#define BUFFER_SIZE 4096
#define LOG_BUFFER_SIZE 8192
#define BACKLOG 5

#define PUT 2
#define GET 1
#define HEAD 0

//isspace
#include <ctype.h>

//used for reading and processing http request
struct httpObject {
    int method;
    char method_str[20];
    char filename[100];
    ssize_t content_length;
    bool valid;
    char buff[BUFFER_SIZE];
    char response[100];
    char httptype[10];
    char firstline[300];
    int status_code;

    //thread attributes
    int id;
    int logfd;
    long log_index;
    char header[200];
};


struct enviorment {
    int N;
    char logfile[28];
    int port;
    bool haslog;
};

struct requestObject {
    int id;
    int clientfd;
    int logfd;
    struct requestObject *next;
};


//Debug function
void print(char *s);
void pint(int i);
void prints(char *s, char *a);
void printMessage(struct httpObject *message);


//request functions
void read_request(int client_fd, struct httpObject *message);
void process_request(struct httpObject *message);
void send_response(int client_fd, struct httpObject *message);
void create_http_response(struct httpObject *message, char * response_type, int filesize);
void create_error_response(struct httpObject *message, int errnum);


//Library functions
int double_carrage_index(char* request);
void valid_request(struct httpObject *message);
int acceptable_file_name(char *file_name);
void read_and_write(struct httpObject* message, int read_from, int write_to);
void delete(struct httpObject *message);

/*
 * THREADS
 */
void* handle_requests_loop(void* data);
void add_request(int client_fd, int log_fd, int id);
struct requestObject* get_request();
int init_recursive_mutex(pthread_mutex_t *mutex);
void initMuxes();


void initialSetup(struct enviorment *env);
void parseCommands(struct enviorment *env, int argc, char **argv);

//log functions
void write_to_log(struct httpObject* message);
void file_to_log(struct httpObject* message, char* read_from);
int log_amount(struct httpObject *message, int amount, long total);
void create_log_header(struct httpObject *message);


#endif
