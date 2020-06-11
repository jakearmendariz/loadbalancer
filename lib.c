#include "loadbalancer.h"

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

int parse_and_create(int argc, char **argv, struct Servers *servers){
    int opt;
    servers->R = 5;
    servers->N = 4;
    while((opt = getopt(argc, argv, "N:R:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'R':  
                servers->R = atoi(optarg);
                break;
            case 'N':  
                servers->N = atoi(optarg);
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
    servers->num_servers = argc - optind;
    servers->servers = malloc(servers->num_servers * sizeof(struct ServerInfo));
    for(int idx = optind; idx < argc; idx++){       
        servers->servers[idx-optind].port = atoi(argv[idx]);
        servers->servers[idx-optind].alive = true;
        //if(idx-optind > 0)
        //    servers->servers[idx-optind].fd = client_connect(atoi(argv[idx]));
    } 
    if(optind == argc){
        perror("Need to provide load balancer port address");
        return -1;
    }
    printf("N:%i R:%i client_addr:%i num_servers:%i\n", servers->N, servers->R, servers->servers[0].port, servers->num_servers);
    return 0;
}

int content_length(char *buff){
    char *token;
    char *saveptr;
    token = strtok_r(buff, "\r\n", &saveptr);
    //classifies GET, HEAD or PUT and gets the filename and verifies HTTP/1.1

    //each part of the http request is a key: value pair
    char key[64];
    char value[64];

    int content_length = 0;
    token = strtok_r(NULL, "\r\n", &saveptr);
    while( token != NULL ) {
        if(sscanf(token, "%[^:]: %s", key, value) != 2){
            //warn("400 caught in load balancer");
            return content_length;
        }
        if(strstr(key, "Content-Length")!= NULL){
            content_length = atoi(value);
        }
        token =  strtok_r(NULL, "\r\n", &saveptr);
    }
    return content_length;
}

int get_content_length(char *buff){
    int n = strcspn(buff, "Content-Length:");
    if(n == -1)
        return -1;
    char end[1028];
    memmove(end, buff, n);
    return atoi(end);
}



//read_write
int read_and_write(int content_length, int read_from, int write_to){
    int amount;
    char buff[BUFFER_SIZE];
    int zerocount = 0;
    while(content_length > 0){
        amount = read(read_from, buff, BUFFER_SIZE);
        if(amount < 0){
            warn("invalid read");
            return -1;
        }else if (amount == 0){
            zerocount++;
            if(zerocount > 2){
                return 0;
            }   
        }else{
            zerocount = 0;
            content_length -= amount;
            if(write(write_to, buff, amount) < 0){
                warn("writing to file error");
                return -1;
            }
        }
        
    }
    return 0;
}

int bridge_connections(int from, int to){
    char buff[BUFFER_SIZE];
    int n, size;
    //Recieve and send the header
    if((n = recv(from, buff, BUFFER_SIZE, 0)) < 0){ warn("recv error in bridge connection"); return -1; }
    if(send(to, buff, n, 0) < 0) { warn("Error sending buff to server"); return -1; }

    //Based on the content length, recieve and send the rest
    size = content_length(buff);
    if(read_and_write(size, from, to) < 0) return -1;
    return 0;

}


void send500(int fd){
    printf("SENDING 500 RESPONSE");
    char response[100];
    strcpy(response, "HTTP/1.1 500 INTERNAL SERVER ERROR\r\nContent-Length: 0\r\n\r\n");
    send(fd, response, strlen(response), 0);
}


int double_carrage_index(char* request){
    for(int i = 0; request[i] != '\0'; i++){
        if(request[i] == '\r' && request[i+1] == '\n' && request[i+2] == '\r' && request[i+3] == '\n'){
            return i+4;
        }
    }
    return -1;
}
