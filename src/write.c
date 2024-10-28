
#include <mongoc/mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define _GNU_SOURCE
#define PORT 6436
#define SHM_KEY 0x5678
#define MAX_BATCH_SIZE 65536000
#define BUFFER_SIZE 1024
#define SIGNAL_READY 1
#define SIGNAL_IDLE 0

// Global variables for shared memory
pthread_mutex_t shm_lock = PTHREAD_MUTEX_INITIALIZER;
int shmid;
char* shm_addr;
int* signal_mem;

// Global MongoDB client and mutex for thread safety
mongoc_client_t *global_mongo_client;
pthread_mutex_t mongo_mutex = PTHREAD_MUTEX_INITIALIZER;

// DTO (data transfer object) shared memory data
char* dto_data = NULL;
pthread_mutex_t dto_lock = PTHREAD_MUTEX_INITIALIZER;

// Function to bind a thread to a specific core
void set_thread_affinity(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

// MongoDB initialization function (called once)
void init_mongo_client() {
    mongoc_init();
    global_mongo_client = mongoc_client_new("mongodb://localhost:27017");

    if (!global_mongo_client) {
        fprintf(stderr, "Error: Failed to initialize MongoDB client. Check if MongoDB is running and accessible.\n");
        exit(1);  // Exit if MongoDB connection cannot be initialized
    } else {
        printf("MongoDB client successfully initialized.\n");
    }
}

// MongoDB cleanup function (called on shutdown)
void cleanup_mongo_client() {
    if (global_mongo_client) {
        mongoc_client_destroy(global_mongo_client);
        mongoc_cleanup();
        printf("MongoDB client successfully cleaned up.\n");
    }
}

// Thread function to handle data and write to MongoDB
void* datr(void* arg) {
    const char* data = (const char*)arg;
    set_thread_affinity(2);  // Set thread CPU affinity (optional, depends on your system)

    // Safely print the received data
    printf("Received Data: %s\n", data);

    // Check if MongoDB client is initialized
    if (!global_mongo_client) {
        fprintf(stderr, "Error: MongoDB client is not initialized. Exiting thread.\n");
        free((void*)data);  // Free the data before exiting
        return NULL;
    }

    // Prepare MongoDB write inside critical section
    pthread_mutex_lock(&mongo_mutex);  // Lock the mutex for thread-safe MongoDB access

    mongoc_collection_t *collection;
    bson_t *doc;
    bson_error_t error;

    // Get the MongoDB collection (reuse global client)
    collection = mongoc_client_get_collection(global_mongo_client, "hama_db", "hama_collection");
    if (!collection) {
        fprintf(stderr, "Error: Failed to get MongoDB collection. Check if the database and collection exist.\n");
        pthread_mutex_unlock(&mongo_mutex);  // Unlock the mutex before returning
        free((void*)data);
        return NULL;
    }

    // Create a BSON document from the received JSON data
    doc = bson_new_from_json((const uint8_t *)data, -1, &error);
    if (!doc) {
        fprintf(stderr, "Error creating BSON: %s\n", error.message);
        mongoc_collection_destroy(collection);
        pthread_mutex_unlock(&mongo_mutex);  // Unlock the mutex before returning
        free((void*)data);
        return NULL;
    }

    // Insert the document into MongoDB
    if (!mongoc_collection_insert_one(collection, doc, NULL, NULL, &error)) {
        fprintf(stderr, "Error inserting document: %s\n", error.message);
    } else {
        printf("Document successfully inserted into MongoDB.\n");
    }

    // Cleanup BSON document and collection
    bson_destroy(doc);
    mongoc_collection_destroy(collection);

    // Unlock the mutex after MongoDB write is complete
    pthread_mutex_unlock(&mongo_mutex);

    // Free the memory after MongoDB write is done
    free((void*)data);

    return NULL;
}

// Function to handle shared memory updates and signal setting
void* shared_memory_updater(void* arg) {
    const char* data = (const char*)arg;

    set_thread_affinity(1);  // Bind to core 1

    // Lock the shared memory and update it
    pthread_mutex_lock(&shm_lock);

    // Perform a granular update (append new data to shared memory)
    size_t shm_data_len = strlen(shm_addr + sizeof(int));  // Existing shared memory data length
    size_t new_data_len = strlen(data);
    if (shm_data_len + new_data_len < MAX_BATCH_SIZE) {
        strncat(shm_addr + sizeof(int), data, new_data_len);  // Append data to shared memory
    }

    // Update the signal to SIGNAL_READY
    *signal_mem = SIGNAL_READY;

    pthread_mutex_unlock(&shm_lock);

    free((void*)data);  // Free the memory allocated for shared memory data

    return NULL;
}

// Function to run the server (runs in its own thread)
void* run_server(void* arg) {
    set_thread_affinity(0);  // Bind to core 0

    int server_fd, client_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(struct sockaddr_in);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind and listen
    bind(server_fd, (struct sockaddr*)&server, sizeof(server));
    listen(server_fd, 5);
    puts("Server is listening...");

    // Main server loop
    char request[BUFFER_SIZE];
    ssize_t recv_size;
    while ((client_sock = accept(server_fd, (struct sockaddr*)&client, &client_len)) >= 0) {
        recv_size = recv(client_sock, request, BUFFER_SIZE - 1, 0);
        if (recv_size <= 0) {
            close(client_sock);
            continue;
        }
        request[recv_size] = '\0';

        // Handle POST request
        if (strstr(request, "POST /write") != NULL) {
            puts("new _request");
            char* json_start = strstr(request, "\r\n\r\n");
            if (json_start) {
                json_start += 4;
                char* data = strdup(json_start); // Allocate data once


                pthread_t shm_update_thread;
                pthread_create(&shm_update_thread, NULL, shared_memory_updater, (void*)data);

                

            pthread_t datr_thread;
            pthread_create(&datr_thread, NULL, datr, (void*)strdup(data));

                

                // Detach threads for async behavior
                pthread_detach(shm_update_thread);
                 pthread_detach(datr_thread);

                

                const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 19\r\n\r\nWrite successful";
                send(client_sock, response, strlen(response), 0);
                close(client_sock);
            }
        }   
    }

    close(server_fd);
    return NULL;
}

int main() {
        // Initialize MongoDB client (must be done before starting any threads)
    init_mongo_client();

    // Setup shared memory
    shmid = shmget(SHM_KEY, sizeof(int) + MAX_BATCH_SIZE, 0666 | IPC_CREAT);
    shm_addr = shmat(shmid, NULL, 0);
    signal_mem = (int *)shm_addr;  // First part of shared memory is the signal
    *signal_mem = SIGNAL_IDLE;     // Set initial signal to idle

    // Create server thread
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, run_server, NULL);

    // Wait for the server thread to finish
    pthread_join(server_thread, NULL);

    // Clean up shared memory
    shmdt(shm_addr);
    shmctl(shmid, IPC_RMID, NULL);


    // Cleanup MongoDB client when the server is shutting down
    cleanup_mongo_client();

    return 0;
}
