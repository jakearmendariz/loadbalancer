#include "server.h"

//DEBUG PRINT FUNCTIONS
int print_all = 1;

void print(char *s){
    if (print_all != 0) {
        printf("%s\n",s);
    }
}
void printint(char *s, int a){
    if (print_all != 0) {
        printf("%s%i\n",s,a);
    }
}
void prints(char *s, char *a){
    if (print_all != 0) {
        printf("%s%s\n",s,a);
    }
}
void pint(int i){
    if (print_all != 0) {
        printf("%d\n",i);
    }
}

//Extracts features from the first line of HTTP request
void type_and_name(struct httpObject *message, char* req){
    char* type = malloc(5);
    char* filename = malloc(100);
    char* httptype = malloc(10);

    if(sscanf(req, "%s %s %s", type, filename, httptype) != 3){
        message->valid = false;
    }

    //classify method
    if(strcmp(type, "GET") == 0){
        message->method = GET;
    }else if(strcmp(type, "HEAD") == 0){
        message->method = HEAD;
    }else if(strcmp(type, "PUT") == 0){
        message->method = PUT;
    }else{
        message->valid = false;
    }
    strcpy(message->method_str, type);

    //filename
    if(filename[0] != '/')
        message->valid = false;
    
    filename += 1;
    if(acceptable_file_name(filename) != 0)
        message->valid = false;

    strcpy(message->filename, filename);
   
    filename -= 1;

    //Check http version
    strcpy(message->httptype, httptype);
    if(strcmp(httptype, "HTTP/1.1") != 0){
        message->valid = false;
    }
    free(type);
    free(filename);
    free(httptype);
}


//Checks string, returns 0 if good. 
int acceptable_file_name(char *file_name){
    if(strlen(file_name) > 27){
        return 1;
    }
    for(int i = 0; i < (int)strlen(file_name); i++){
        char letter = file_name[i];
        if((letter >= 'A' && letter <= 'Z') || (letter >= 'a' && letter <= 'z') || (letter >= '0' && letter <= '9') || letter == '_' || letter == '-'){
            continue;
        }else{
            return letter;
        }
    }
    return 0;
}

//Finds \r\n\r\n in the input. Allows to find where the header ends
int double_carrage_index(char* request){
    for(int i = 0; request[i] != '\0'; i++){
        if(request[i] == '\r' && request[i+1] == '\n' && request[i+2] == '\r' && request[i+3] == '\n'){
            return i+4;
        }
    }
    return -1;
}


//https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm for tokenizing strings
//https://stackoverflow.com/questions/7887003/c-sscanf-not-working for [^:] (only that)
void valid_request(struct httpObject *message){
    char *token;
    char *saveptr;
    token = strtok_r(message->buff, "\r\n", &saveptr);
    //classifies GET, HEAD or PUT and gets the filename and verifies HTTP/1.1
    strcpy(message->firstline, token);
    type_and_name(message, token);

    //each part of the http request is a key: value pair
    char key[64];
    char value[64];

    if(message->method == -1 || strlen(message->filename) == 0 || !message->valid){
        message->valid = false;
        return;
    }
    message->content_length = -1;
    token = strtok_r(NULL, "\r\n", &saveptr);
    while( token != NULL ) {
        if(sscanf(token, "%[^:]: %s", key, value) != 2){
            fprintf(stderr, "Token:%s\n", token);
            warn("Request not match format of: string:string");
            message->valid = false;
            return;
        }
        if(strstr(key, "Content-Length")!= NULL){
            message->content_length = atoi(value);
        }
        token =  strtok_r(NULL, "\r\n", &saveptr);
    }
    return;
}

//read_write
void read_and_write(struct httpObject* message, int read_from, int write_to){
    int amount;
    while(message->content_length > 0){
        amount = read(read_from, message->buff, BUFFER_SIZE);
        if(amount < 0){
            warn("invalid read");
            create_error_response(message, errno);
            break;
        }else{
            message->content_length -= amount;
            if(write(write_to, message->buff, amount) < 0){
                warn("writing to file error");
                create_error_response(message, errno);
                break;
            }
        }
    }
}

/**
 * Multi threading enviorment
 */
void initialSetup(struct enviorment *env){
    env->N = 4;
    env->port = 8080;
    env->haslog = false;
    return;
}

void parseCommands(struct enviorment *env, int argc, char **argv){
    int opt;
    //https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    //https://www.geeksforgeeks.org/getopt-function-in-c-to-parse-command-line-arguments/
    while((opt = getopt(argc, argv, "N:l:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'l':  
                strcpy(env->logfile, optarg);
                env->haslog = true;
                break;
            case 'N':  
                env->N = atoi(optarg);
                break;  
            case ':':  
                printf("option needs a value\n");  
                break;  
            case '?':  
                printf("unknown option: %c\n", optopt); 
                break;  
        }  
    }  
    // Finds the port number
    for(; optind < argc; optind++){       
        env->port = atoi(argv[optind]);
    } 

}

/**
 * LOG FUNCTIONS
 */
//Reads from the filename provided, writes to log file
void file_to_log(struct httpObject* message, char* read_from){
    int amount, file = open(read_from, O_RDONLY);
    if(file < 0){ warn("Couldn't open file while writing to log");prints("file-to-log error:filename:%s\n", read_from); }
    long total = 0;
    while(message->content_length > 0){
        amount = read(file, message->buff, BUFFER_SIZE);
        if(log_amount(message, amount, total)){
            warn("invalid read in log");
            printMessage(message);
            break;
        }
        total += amount;
    }
    print("Finished logwrite");
}

//writes the message->buff with amount bytes to the log file in the specified format
int log_amount(struct httpObject *message, int amount, long total){
    if(amount < 0){
        return -1;
    }else{
        message->content_length -= amount;
        char byte[10];
        for(long i = total; i < total + amount; i++){
            if(i%20 == 0){
                sprintf(byte, "\n%08lu", i);
                pwrite(message->logfd, byte, 9, message->log_index);
                message->log_index += 9;
            }
            if(i % 100000 == 0){
                printf("%lu bytes into logwrite\n", i);
            }
            int index = (int)(i - total);
            unsigned char a = (unsigned char)message->buff[index];
            sprintf(byte, " %02x", a);
            pwrite(message->logfd, byte, 3, message->log_index);
            message->log_index += 3;
        }
    }
    return 0;
}


void printMessage(struct httpObject *message){ 
     char *header = "";
    if(print_all == 1){
        sprintf(header, "%s /%s length %zu\n", message->method_str, message->filename, message->content_length);
        prints("Message Object:", header);
    } 
}
