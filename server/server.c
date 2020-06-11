#include "server.h"
//Jake Armendariz
//Main code for the server

int main(int argc, char **argv) {
    //Create the enviorment: number of threads, logfile, port number
    struct enviorment *env = malloc(sizeof(struct enviorment));
    initialSetup(env); 
    parseCommands(env, argc, argv);
    initMuxes();

    //Create and initialize threads
    int thread_id[env->N-1];
    pthread_t p_threads[env->N-1]; 
    int logfd;
    if(env->haslog){
         logfd = open(env->logfile,  O_CREAT | O_RDWR | O_TRUNC, 0666);
    }else{
        logfd = -1;
    }
    
    for(int i = 0; i < (int)env->N; i++){
        thread_id[i] = i+1;
        pthread_create(&p_threads[i], NULL, handle_requests_loop, (void*)&thread_id[i]);
     }

    //Creates sockaddr with informtion
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(env->port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addrlen = sizeof(server_addr);

    //Create server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Need to check if server_fd < 0, meaning an error
    if (server_fd < 0) { warn("socket");}

    //used to configure the server socket
    int enable = 1;

    //binds the address, handles "already in use"
    int ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if(ret != 0){ warn("set sockopt error");}
    
    //Binds the socket to the address
    ret = bind(server_fd, (struct sockaddr *) &server_addr, addrlen);
    if(ret != 0){ warn("bind error"); }
    
    //marks the socket referred to buy socketfd as a passive socket, will be used by accept to get requests
    ret = listen(server_fd, 10); // 10 should be enough, if not use SOMAXCONN
    if (ret != 0) { warn("listen error"); return -1; }
   
    int counter = 0;
    printf("server is ready for requests...\n");
    struct sockaddr client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    while(true){   
        int client_fd = accept(server_fd, &client_addr, &client_addrlen);
        if(client_fd < 0){ warn("accept error");  continue; }
        add_request(client_fd, logfd, counter);
        counter++;   
    }
    close(server_fd);
    free(env);
    
    return 0;
}
