# High Throughput API Built in C

This is an API I have been developing using C. The API is designed to achieve **high performance** with a **non-blocking architecture**, **real-time synchronization**, and **shared memory** for efficient data handling. It is particularly focused on achieving low latency and high throughput, ensuring that data is consistently up-to-date for read requests as it's written to shared memory and MongoDB.

## Features
- **High Throughput**: Handles thousands of requests per second.
- **Low Latency**: Optimized for fast responses under heavy loads.
- **Non-Blocking Architecture**: Ensures smooth operation without delays.
- **Shared Memory for Realtime Synchronization**: Guarantees the latest data for read requests.
- **Core Binding**: Optimizes performance by binding processes to specific CPU cores.
- **MongoDB Integration**: Ensures reliable data persistence.

---

## Technologies Used:
- **Language**: C
- **Database**: MongoDB
- **Libraries**:
    - **POSIX Threads**: Multi-threading
    - **POSIX Shared Memory**: Efficient memory sharing
    - **MongoDB C Driver**: Database interaction

---

## Installation and Setup

Follow these steps to set up the project:

### 1. Install MongoDB
- Install MongoDB by following the [MongoDB installation guide](https://docs.mongodb.com/manual/installation/).

### 2. Install MongoDB C Driver
- Follow [MongoDB C Driver Installation Instructions](https://mongoc.org/libmongoc/current/installing.html).

### 3. Expand Shared Memory Limits
Check your shared memory limits using the following command:
```bash

df -h /dev/shm

```
### 4. Compilation instruuctions
For the read service:
```bash
gcc -o read read.c -Ofast -lpthread -lrt

```

For the write service:
```bash
gcc -o write write.c -Ofast -lpthread -lrt

```

### 5. Run the Services
To run the services:
```bash
./read
./write




