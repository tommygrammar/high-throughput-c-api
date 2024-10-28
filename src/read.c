
/*All the necessary headers*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>


#define _GNU_SOURCE  // Define GNU feature test macro
#define PORT 6435 /* Port number for the server*/
#define SHM_KEY 0x5678  /* Key for the shared memory*/
#define MAX_BATCH_SIZE 65536000 /* Max data size in shared memory */
#define BUFFER_SIZE 1024 /* Buffer size for client communication */
#define SIGNAL_READY 1 /*Signal ready for update*/
#define SIGNAL_IDLE 0 /*Signal not ready for update*/

char* dto_data = NULL;        // Data Transfer Object (DTO) for shared memory data
pthread_mutex_t dto_lock = PTHREAD_MUTEX_INITIALIZER;  // Mutex for DTO data

// Function to bind a thread to a specific core
void set_thread_affinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

// Function to read from shared memory (runs in its own thread)
void* shared_memory_reader(void* arg) {
    set_thread_affinity(1);  // Bind this thread to core 1

    // Access shared memory
    int shmid = shmget(SHM_KEY, sizeof(int) + MAX_BATCH_SIZE, 0666);

    char* shm_addr = shmat(shmid, NULL, 0);
    // Update the DTO only once with shared memory data - testing purposes, feel free to constantly update it
    pthread_mutex_lock(&dto_lock); 
    dto_data = strdup(shm_addr + sizeof(int));  // Copy the shared data to DTO
     pthread_mutex_unlock(&dto_lock);
     // Use a mutex for thread safety

    
    int *signal_mem = (int *)shm_addr;

    while (1) {
        if (*signal_mem == SIGNAL_READY) {
            // Lock the mutex to safely update the DTO
            pthread_mutex_lock(&dto_lock);

            // Free the old DTO data if it exists
            if (dto_data != NULL) {
                free(dto_data);
            }

            // Copy the shared memory data into dto_data
            dto_data = strdup(shm_addr + sizeof(int));

            // Unlock the mutex
            pthread_mutex_unlock(&dto_lock);

            // Print update confirmation to the terminal
            puts("Shared memory updated. Data copied to DTO.");

            // Set the signal back to idle
            *signal_mem = SIGNAL_IDLE;

            // Sleep for a short time to avoid rapid loops
            usleep(1000);  // Sleep for 1 millisecond
        }

        // Sleep to avoid busy-waiting if there's no update
        usleep(1000);  // Sleep for 1 millisecond
    }

    
    
    return NULL;
}

// Function to run the server (runs in its own thread)
void* run_server(void* arg) {
    set_thread_affinity(0);  // Bind this thread to core 0

    int server_fd, client_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(struct sockaddr_in);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);


    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind the socket
   bind(server_fd, (struct sockaddr*)&server, sizeof(server));

    // Listen for connections
    listen(server_fd, 5);
    puts("Waiting for incoming connections...");

    // Function to handle communication with a client in persistent mode
    /*handle client */
    char request[BUFFER_SIZE];
    ssize_t recv_size;

    // Main server loop
    while ((client_sock = accept(server_fd, (struct sockaddr*)&client, &client_len)) >= 0) {   
    
    const char* data = dto_data;  // Access the DTO data
    size_t data_len = strlen(data);
            
    while (1) {
        recv_size = recv(client_sock, request, BUFFER_SIZE - 1, 0);
        if (recv_size <= 0) {
            break;
        }
        request[recv_size] = '\0';
        if (strstr(request, "POST /read") != NULL) {          

            // Prepare HTTP headers
            char headers[BUFFER_SIZE];
            snprintf(headers, sizeof(headers),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: %zu\r\n"
                     "\r\n", data_len);

            // Send headers first
            send(client_sock, headers, strlen(headers), 0) ;

            // Send the entire DTO data
            send(client_sock, data, data_len, 0);
        }

    close(client_sock);
}
    }

    close(server_fd);
    return NULL;
}

int main() {
    pthread_t server_thread, shm_thread;

    // Create the shared memory reader thread
    pthread_create(&shm_thread, NULL, shared_memory_reader, NULL);

    // Create the server thread
    pthread_create(&server_thread, NULL, run_server, NULL);

    // Wait for both threads to finish
    pthread_join(shm_thread, NULL);
    pthread_join(server_thread, NULL);

    return 0;
}
