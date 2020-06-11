#include "server.h"

/**
 * reads request
 * 
 * Handles all bad request possibilities, calling on the other functions in this file to do so
 */
void read_request(int client_fd, struct httpObject *message){
    message->valid = true;
    ssize_t bytes = recv(client_fd, message->buff, BUFFER_SIZE, 0);
    if(bytes < 0){ warn("read request | recv()"); message->valid = false; return; printMessage(message);}
    //null terminate
    message->buff[bytes] = '\0';

    return;
}


void prepend(char* s, const char* t)
{
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}

/**
 * validates the request, if theres a problem, its a 400 error
 * 
 * uses stat to check file for get, put and head. Creates the response header
 * Does not reading or writing to files
 */
void process_request(struct httpObject *message){
    char* cpy = malloc(4096); strcpy(cpy, message->buff);
    //separates the header in body (by view of C string functios)
    int n = double_carrage_index(cpy);
    if(n == -1){warn("no double carraige"); printf("buffer:%s\n", message->buff); message->valid = false;} 
    else{ 
        cpy += n;
        message->buff[n-1] = '\0';
        valid_request(message);
        strcpy(message->buff, cpy);
        cpy -= n;
    }
    free(cpy);
    
    if(message->valid == false){
        create_http_response(message, "400 BAD REQUEST", 0);
        message->status_code = 400;
        return; }
    
    if(strcmp(message->filename, "healthcheck") == 0 && message->method != GET){
        //Head or post automatically go to 404
        warn("403 only get requests to healthcheck\n");
        create_error_response(message, 13);
        return;
    }

    struct stat st;         //to get the info on a file

    //REMOVE THIS LINE FOR TESTING
    //
    //
    //
    // REMOVE THIS LINE FOR TESTING
    //prepend(message->filename, "files/");
    // REMOVE THIS LINE FOR TESTING
    // I added in files/ to all healthcheck instances
    //
    //
    //REMOVE THIS LINE FOR TESTING
    //printf("\n\n\nFILENAME: %s\n\n\n", message->filename);

    //switch for GET, PUT, HEAD (constant values)
    switch(message->method)
    {
        case HEAD:
        case GET:
            if(strcmp(message->filename, "healthcheck") == 0){
                print("HEALTHCHECK");
                message->status_code = 200;
                break;
            }
            //prints("filename:", message->filename);
            //https://www.includehelp.com/c-programs/find-size-of-file.aspx to find the size of a file
            if(stat(message->filename,&st)==0) {message->content_length = (st.st_size);}
            else {/* warn("stat caught error in GET or HEAD");*/ create_error_response(message, errno); break;}

            //send valid header
            create_http_response(message, "200 OK", message->content_length);
            message->status_code = 200;
            break;
        case PUT:
            //will be overwritten in the case of errors
            if(message->content_length == -1){
                message->valid = false;
                warn("Content length was not found in put request");
                create_http_response(message, "400 BAD REQUEST", 0);
            }
            create_http_response(message, "201 CREATED", 0);
            message->status_code = 201;

            if(stat(message->filename,&st) != 0){
                if(errno == 13){
                    create_error_response(message, errno);
                }
            }
            break;
    }
}

/**
 * send_response to client_fd
 * 
 * if get, it will read and send the file contenets as well
 * 
 * if head it will send message
 * 
 * if put, it will write to a file, then send the message
 */
void send_response(int client_fd, struct httpObject *message){
    if(!message->valid || strcmp(message->filename, "healthcheck") == 0){
        send(client_fd, message->response, strlen(message->response), 0);
        return;
    }
    int file;               //File descriptor
    ssize_t filesize = message->content_length;
    //switch for GET, PUT, HEAD (constant values)
    switch(message->method)
    {
        case HEAD:
            send(client_fd, message->response, strlen(message->response), 0);
            break;
        case GET:
            file = open(message->filename, O_RDONLY);
            if(file < 0){ create_error_response(message, errno); break; }

            send(client_fd, message->response, strlen(message->response), 0);
            
            //while any space is left in file continue to read
            read_and_write(message, file, client_fd);
            close(file);
            break;
        case PUT:
            //to test concurrency. Program should continue to run
            file = open(message->filename,  O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if(file < 0) { warn("put open error");create_error_response(message, errno); break; }
            if(strlen(message->buff) > 0){
                printf("bufferlen:%i\nbuffer:%s\n", (int)strlen(message->buff), message->buff);
                if(write(file, message->buff, strlen(message->buff)) < 0){
                    warn("error writing response tail");
                    create_error_response(message, errno);
                }
                message->content_length -= strlen(message->buff);
            }
            //Writes data to server
            read_and_write(message, client_fd, file);
            send(client_fd, message->response, strlen(message->response), 0);
            close(file);
            break;
    }
    message->content_length = filesize;
    close(client_fd);
}

//Creates the http response!
void create_http_response(struct httpObject *message, char* response_type, int filesize){
    sprintf(message->response, "HTTP/1.1 %s\r\nContent-Length: %d\r\n\r\n", response_type, filesize);
}


//Hands in the client file descriptor and error type
void create_error_response(struct httpObject *message, int errnum){
    message->valid = false;
    if(errnum == 2){
        create_http_response(message, "404 NOT FOUND", 0);
        message->status_code = 404;
    }else if(errnum == 13){
        create_http_response(message, "403 FORBIDDEN", 0);
        message->status_code = 403;
    }else{
        create_http_response(message, "500 INTERNAL SERVER ERROR", 0);
        message->status_code = 402;
    }
    return;
}
