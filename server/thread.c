#include "server.h"

pthread_mutex_t request_mutex;
pthread_mutex_t log_mutex;
pthread_cond_t request_cond = PTHREAD_COND_INITIALIZER;

struct requestObject* requests = NULL;     
struct requestObject* last_request = NULL; 
int queue_size = 0;

long log_cursor = 0;
int num_errors = 0;
int total = 0;



int init_recursive_mutex(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    int r;

    r = pthread_mutexattr_init(&attr);
    if (r != 0)
        return r;

    r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    if (r == 0)
        r = pthread_mutex_init(mutex, &attr);

    pthread_mutexattr_destroy(&attr);

    return r;
}

void initMuxes(){
    init_recursive_mutex(&request_mutex);
    init_recursive_mutex(&log_mutex);
}

/**
 * Loop to hold threads
 */
void* handle_requests_loop(void* data){
    int id = *((int*)data);
    struct requestObject *request;
    struct httpObject *message;
    while(true){
        pthread_mutex_lock(&request_mutex);
        if(queue_size > 0){
            request = get_request();    //This locks, if the request is already 0 (another thread intercepts it will return NULL)
            pthread_mutex_unlock(&request_mutex);
            if(request != NULL){
                printf("Thread.%i RECIEVED:%i\n", id, request->id); 
                message = malloc(sizeof(struct httpObject));
                message->logfd = request->logfd;
                read_request(request->clientfd, message);
                //printf("message->buffer:%s\n", message->buff);
                process_request(message);           
                
                if(message->valid && strcmp(message->filename, "healthcheck") == 0){
                    sprintf(message->buff, "%i\n%i", num_errors, total);
                    sprintf(message->response, "HTTP/1.1 200 OK\r\nContent-Length: %i\r\n\r\n%s", (int)strlen(message->buff), message->buff);
                }
                
                send_response(request->clientfd, message);

                write_to_log(message);
                //keeps track of the number of messages
                total += 1;
                if(!message->valid){
                    num_errors += 1;
                }
                
                free(message);
                printf("Thread.%i COMPLETED:%i\n", id, request->id);
                free(request);
            }else{
                pthread_mutex_unlock(&request_mutex);
            }
        }else{
            pthread_mutex_unlock(&request_mutex);
            pthread_cond_wait(&request_cond, &request_mutex);
        }
    }
}

/**
 * Adds message to the queue
 */
void add_request(int client_fd, int log_fd, int id){
    struct requestObject *request = malloc(sizeof(struct requestObject));
    request->next = NULL;
    request->clientfd = client_fd;
    request->logfd = log_fd;
    request->id = id;

    pthread_mutex_lock(&request_mutex);
    queue_size += 1;
    if(requests == NULL){ //empty list
        requests = request;
        last_request = request;
    }else{
        last_request->next = request;
        last_request = request;
    }

    pthread_cond_signal(&request_cond);
    pthread_mutex_unlock(&request_mutex);
}

/**
 * get_request
 * if queue size is > 0, it will remove the first element and return it
 */
struct requestObject* get_request(){
    struct requestObject *request;
    if(queue_size > 0){
        request = requests;
        requests = requests->next;
        
        queue_size-=1;
    }else{
        //get request was called
        request = NULL;
    }
    return request;
}


//Writes the header, and each following response
void write_to_log(struct httpObject* message){
    if(message->logfd == -1){
        return;
    }
    create_log_header(message); //Also sets the correct size
    pwrite(message->logfd, message->header, strlen(message->header),  message->log_index);
    if(!message->valid){
        return;
    }
    
    message->log_index += strlen(message->header);
    if(message->method != HEAD){
        if(strcmp(message->filename, "healthcheck") == 0){
            log_amount(message, strlen(message->buff), 0);
        }else{
            file_to_log(message, message->filename);
        }
    }
    pwrite(message->logfd, "\n========\n", 10, message->log_index);
}

/**
 * Create logfile header and size of the response
 * increases the log_index
 */
void create_log_header(struct httpObject *message){
    if(message->logfd == -1){ return; }

    //Creates the header. If healthcheck is sets the size properly
    if(strcmp(message->filename, "healthcheck") == 0){
            sprintf(message->buff, "%i\n%i", num_errors, total);
            message->content_length = strlen(message->buff);
    }
    if(!message->valid)
        sprintf(message->header, "FAIL: %s --- response %d\n========\n", message->firstline, message->status_code);
    else
        sprintf(message->header, "%s /%s length %zu", message->method_str, message->filename, message->content_length);
    
    //Locks to do the math for size of header
    pthread_mutex_lock(&log_mutex);
    message->log_index = log_cursor;
    log_cursor += strlen(message->header);

    if(message->valid && message->content_length > 0 && message->method != HEAD){
        int numlines; 
        if(message->content_length%20)
            numlines = ((int)(message->content_length/20)+1);
        else
            numlines = ((int)(message->content_length/20));
        log_cursor += /*for each hex byte and a space */ message->content_length*3 + /** padded decimal*/ 9*numlines + 10/*\n=====\n*/; 
    }else if(message ->valid){
        log_cursor += 10; //The \n========\n
    }
    pthread_mutex_unlock(&log_mutex);
}
