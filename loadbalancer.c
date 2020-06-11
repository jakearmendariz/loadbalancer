#include "loadbalancer.h"

pthread_mutex_t hc_mutex;
pthread_cond_t hc_cond = PTHREAD_COND_INITIALIZER;
pthread_t hc_thread;

pthread_mutex_t max_mutex;
pthread_cond_t max_cond = PTHREAD_COND_INITIALIZER;

int inprogress = 0;
int N = 0;

//Chooses port with least total requests, more errors is a good sign.
int choose_server(struct Servers *servers){
    long min = 4294967295;
    int idx = -1;
    for(int i = 1; i < servers->num_servers; i++)
    {
        if(servers->servers[i].alive){
            if(servers->servers[i].total_requests < min){
                min = servers->servers[i].total_requests;
                idx = i;
            }else if(servers->servers[i].total_requests == min){
                if(servers->servers[i].error_requests < servers->servers[idx].error_requests){
                    idx = i;
                }
            }
        }
    }
    servers->servers[idx].total_requests++;
    return idx;
}


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


void* healthchecks(void *data){
    struct Servers *servers = (struct Servers *)data;
    struct timespec ts;
    struct timeval now;

    while(true){
        //printf("HEALTHCHECK\n");
        char * healthcheck = "GET /healthcheck HTTP/1.1\r\nContent-Length: 0\r\n\r\n\0";
        int serverfd, err = -1, total = -1;
        for(int i = 1; i < servers->num_servers; i++){
            if((serverfd = client_connect(servers->servers[i].port)) < 0){ servers->servers[i].alive = false; continue; }

            if(send(serverfd, healthcheck, strlen(healthcheck), 0) < 0){ servers->servers[i].alive = false; close(serverfd); continue; }

            char buff[255];
            int n = recv(serverfd, buff, 255, 0);
            if(n == -1){ servers->servers[i].alive = false; close(serverfd); continue; }
            buff[n] = '\0';

            if(strstr(buff, "200 OK") == NULL){ servers->servers[i].alive = false; close(serverfd); continue; }
            int idx = double_carrage_index(buff);
            if(idx == -1){ servers->servers[i].alive = false; continue; }
            if(n != idx){//If the body is included in healthcheck
                char *cpy = malloc(255);
                strcpy(cpy, buff);
                cpy += idx;
                if(sscanf(cpy, "%i\n%i", &err, &total) != 2){ servers->servers[i].alive = false; }
                cpy -= idx;
                free(cpy);
            }else{ //Body is sent seperatly
                n = recv(serverfd, buff, 255, 0);
                buff[n] = '\0';
                if(sscanf(buff, "%i\n%i", &err, &total) != 2)
                    servers->servers[i].alive = false;
            }

            servers->servers[i].total_requests = total;
            servers->servers[i].error_requests = err;
            servers->servers[i].alive = true;
            if(serverfd == -1)
                close(serverfd);
        }
        pthread_mutex_lock(&hc_mutex);
        memset(&ts, 0, sizeof(ts));

        gettimeofday(&now, NULL);
        ts.tv_sec = now.tv_sec + 3;
        pthread_cond_timedwait(&hc_cond, &hc_mutex, &ts);

        pthread_mutex_unlock(&hc_mutex);
    }
}


void connect_and_serve(struct Servers *servers)
{
    int lbfd = servers->servers[0].fd;
    printf("Accepted a request\n");
    //Connect to an httpserver
    int server = choose_server(servers);
    printint("Chose server:", servers->servers[server].port);
    
    if(server == -1){ send500(lbfd); return; }
    int serverfd = client_connect(servers->servers[server].port);
    
    struct fd__set *set = malloc(sizeof(struct fd__set));
    set->lb = lbfd;
    set->server = serverfd;
    
    pthread_t process;

    pthread_mutex_lock(&max_mutex);
    inprogress ++;
    if(inprogress > servers->N) {
        printf("MAX CONNECTIONS");
        pthread_cond_wait(&max_cond, &max_mutex);
    }
    pthread_mutex_unlock(&max_mutex);
    pthread_create(&process, NULL, process_data, set);    
    
}

/**
    This area is a non-critical zone
    Thus, multiple threads (up to N) operate here.
    This is where the loadbalancer will wait for servers
**/
void *process_data(void *data){
    struct fd__set *set = (struct fd__set *)data;
    int lbfd = set->lb;
    int serverfd = set->server;

    print("Begin process...");
    if(bridge_connections(lbfd, serverfd) < 0) { send500(lbfd); return NULL;}
    print("sent request");
    if(bridge_connections(serverfd, lbfd) < 0) { send500(lbfd); return NULL;}
    printf("Completed Request\n");
    close(serverfd);
    close(lbfd);
    pthread_mutex_lock(&max_mutex);
    inprogress --;
    if(inprogress == N-1)
        pthread_cond_signal(&max_cond);
    pthread_mutex_unlock(&max_mutex);
    free(set);
    pthread_exit(NULL);
    return NULL;
}

/**
* Main
* parses the command line
* initialiezes the struct Servers with proper port numbers
* Waits for requests
*/
int main(int argc,char **argv) {
    int listenfd;
    struct Servers *servers = malloc(sizeof(struct Servers));
    int result = init_recursive_mutex(&hc_mutex);
    result = init_recursive_mutex(&max_mutex);

    if (argc < 3) {
        printf("missing arguments: usage %s port_to_connect port_to_listen", argv[0]);
        return 1;
    }

    if(parse_and_create(argc, argv, servers) < 0){ return -1; }
    N = servers->N;
    
    struct sockaddr lb_addr;
    socklen_t lb_addrlen = sizeof(lb_addr);

    if ((listenfd = server_listen(servers->servers[0].port)) < 0){
        warn("failed listening"); return -1;}

    int counter = 0;
    pthread_create(&hc_thread, NULL, healthchecks, servers);
    pthread_cond_signal(&hc_cond);
    
    while(true){
        printf("[+] server is waiting\n");
        if ((servers->servers[0].fd = accept(listenfd, &lb_addr, &lb_addrlen)) < 0){ warn("failed accepting"); return -1;}
        //Connects each client, sends the info
        connect_and_serve(servers);
        //Healthcheck every R requests

        //Should be sent to 0 after a healtcheck
        if(++counter%servers->R == 0) {
            pthread_cond_signal(&hc_cond);
        }
    }
} 
