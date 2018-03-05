/* server.c

   Sample code of
   Assignment L1: Simple multi-threaded key-value server
   for the course MYY601 Operating Systems, University of Ioannina

   (c) S. Anastasiadis, G. Kappes 2016

*/

#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>s
#include <sys/stat.h>
#include "utils.h"
#include "kissdb.h"


#define MY_PORT                 6767
#define BUF_SIZE                1160
#define KEY_SIZE                 128
#define HASH_SIZE               1024
#define VALUE_SIZE              1024
#define MAX_PENDING_CONNECTIONS   10
#define STACK_SIZE                 2 // set the request stack size
#define THREADNO                   4 // set the number of threads

// Definition of the operation type.
typedef enum operation {
  PUT,
  GET
} Operation;

// Definition of the request.
typedef struct request {
  Operation operation;
  char key[KEY_SIZE];
  char value[VALUE_SIZE];
} Request;

// Definition of new connection parameters
typedef struct stack {
  int connection_fd;
  struct timeval curtime;
} Stack;

// Declare and initialize global counters
long  total_waiting_time = 0;
long  total_service_time = 0;
long completed_requests = 0;

// Declare and initialize mutexes
pthread_mutex_t put = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t top_change = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stack_full = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t global_times = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stack_empty = PTHREAD_MUTEX_INITIALIZER;

// Declare and initialize thread wait conditions
pthread_cond_t cond_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_full = PTHREAD_COND_INITIALIZER;

// Declare consumer thread(s)
pthread_t consumerTid[THREADNO];

// Declare stack
Stack connectionStack[STACK_SIZE];

// Top of stack declared and initialized to empty stack
int top = -1;

// Definition of the database.
KISSDB *db = NULL;


/* @name signal_func - called by handler for SIGSTOP to print data
 * @param signal number
 *
 * print average waiting and service time
*/
void signal_func(int sig){
  int j;  // helper variable for loops

  // Print total number of requests served
  printf("\nTotal No of requests served: %ld\n", completed_requests);

  // Calculate average waiting & service time
  long double avg_waiting_time = (total_waiting_time / (double)completed_requests);
  long double avg_service_time = (total_service_time / (double)completed_requests);

  // print average waiting & service time
  printf("Average waiting time: %Lf miliseconds\n", avg_waiting_time);
  printf("Average service time: %Lf miliseconds\n", avg_service_time);

  // Get rid of thread zombies
  for (j = 0; j < THREADNO; j++){
    pthread_detach(consumerTid[j]);
  }

  // Exit the program
  exit(1);
}

/*
 * @name parse_request - Parses a received message and generates a new request.
 * @param buffer: A pointer to the received message.
 *
 * @return Initialized request on Success. NULL on Error.
 */
Request *parse_request(char *buffer) {
  char *token = NULL;
  Request *req = NULL;

  // Check arguments.
  if (!buffer)
    return NULL;

  // Prepare the request.
  req = (Request *) malloc(sizeof(Request));
  memset(req->key, 0, KEY_SIZE);
  memset(req->value, 0, VALUE_SIZE);

  // Extract the operation type.
  token = strtok(buffer, ":");
  if (!strcmp(token, "PUT")) {
    req->operation = PUT;
  } else if (!strcmp(token, "GET")) {
    req->operation = GET;
  } else {
    free(req);
    return NULL;
  }

  // Extract the key.
  token = strtok(NULL, ":");
  if (token) {
    strncpy(req->key, token, KEY_SIZE);
  } else {
    free(req);
    return NULL;
  }

  // Extract the value.
  token = strtok(NULL, ":");
  if (token) {
    strncpy(req->value, token, VALUE_SIZE);
  } else if (req->operation == PUT) {
    free(req);
    return NULL;
  }
  return req;
}

/*
 * @name process_request - Process a client request.
 * @param socket_fd: The accept descriptor.
 *
 * @return
 */
void *process_request(int temp_fd) {

  // initialize new socket
  const int socket_fd = temp_fd;

  char response_str[BUF_SIZE], request_str[BUF_SIZE];
    int numbytes = 0;
    Request *request = NULL;

    // Clean buffers.
    memset(response_str, 0, BUF_SIZE);
    memset(request_str, 0, BUF_SIZE);

    // receive message.
    numbytes = read_str_from_socket(socket_fd, request_str, BUF_SIZE);
    // parse the request.
    if (numbytes) {
      request = parse_request(request_str);
      if (request) {
        switch (request->operation) {
          case GET:
            // Read the given key from the database.
            if (KISSDB_get(db, request->key, request->value))
              sprintf(response_str, "GET ERROR\n");
            else
              sprintf(response_str, "GET OK: %s\n", request->value);
            break;
          case PUT:
            // Write the given key/value pair to the database.
            pthread_mutex_lock(&put);
            if (KISSDB_put(db, request->key, request->value))
              sprintf(response_str, "PUT ERROR\n");
            else
              sprintf(response_str, "PUT OK\n");
            pthread_mutex_unlock(&put);
            break;
          default:
            // Unsupported operation.
            sprintf(response_str, "UNKOWN OPERATION\n");
        }
        // Reply to the client.
        write_str_to_socket(socket_fd, response_str, strlen(response_str));
        if (request)
          free(request);
        request = NULL;
        return 0;
      }
    }
    // Send an Error reply to the client.
    sprintf(response_str, "FORMAT ERROR\n");
    write_str_to_socket(socket_fd, response_str, strlen(response_str));
    return 0;
}

/*
 * @name thread_func - Consumer threads run this function
 * @param int i - the thread number
 *
 * @return
*/
void *thread_func(void *arg){
  int i = *(int *) arg;
  int temp_fd;

  // variable to hold time difference (in miliseconds)
  long long unsigned int timedif;

  while(1){

    // consumer thread waits for signal if stack is empty
    pthread_mutex_lock(&stack_empty);
    while (top < 0){
      pthread_cond_wait(&cond_empty, &stack_empty);
    }
    // pthread_mutex_unlock(&stack_empty);

    // assign stack items to variables
    temp_fd = connectionStack[top].connection_fd;
    struct timeval accept_time;
    accept_time = connectionStack[top].curtime;

    // change pointer to top of stack
    top--;

    pthread_mutex_unlock(&stack_empty);
    // if stack is not empty, keep processing requests

    // start counting service time
    struct timeval startservicetime;
    gettimeofday(&startservicetime, NULL);

    // calculate waiting time
    timedif = startservicetime.tv_usec - accept_time.tv_usec;
    total_waiting_time += timedif / 1000;

    // call process_request();
    process_request(temp_fd);

    // calculate service time and add to total
    struct timeval stopservicetime;
    gettimeofday(&stopservicetime, NULL);
    timedif = stopservicetime.tv_usec - startservicetime.tv_usec;

    // close connection file
    close(temp_fd);

    // request served, change position to new top of stack
    pthread_mutex_lock(&global_times);
    completed_requests++;
    total_service_time += timedif / 1000;
    printf("Thread #%d processed request %ld\n", i, completed_requests);
    pthread_mutex_unlock(&global_times);

    // signal server
    if (top < STACK_SIZE){
      pthread_cond_signal(&cond_full);
    }
  }
}

/*
 * @name main - The main routine.
 *
 * @return 0 on success, 1 on error.
 */

int main() {
  int i;
  int res;                          // check this variable to determine if thread creation ok
  int socket_fd,                    // listen on this socket for new connections
      new_fd;                       // use this socket to service a new connection
  socklen_t clen;
  struct sockaddr_in server_addr,   // my address information
                     client_addr;   // connector's address information

  // Create socket
  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    ERROR("socket()");

  // Ignore the SIGPIPE signal in order to not crash when a
  // client closes the connection unexpectedly.
  signal(SIGPIPE, SIG_IGN);

  // Handle CTRL+Z (SIGTSTP) and print data
  if (signal(SIGTSTP, signal_func) == SIG_ERR)
        printf("Can't catch SIGTSTP\n");

  // create socket adress of server (type, IP-adress and port number)
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // any local interface
  server_addr.sin_port = htons(MY_PORT);

  // bind socket to address
  if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    ERROR("bind()");

  // start listening to socket for incomming connections
  listen(socket_fd, MAX_PENDING_CONNECTIONS);
  //start counting time for new request


  fprintf(stderr, "(Info) main: Listening for new connections on port %d ...\n", MY_PORT);
  clen = sizeof(client_addr);

  // Allocate memory for the database.
  if (!(db = (KISSDB *)malloc(sizeof(KISSDB)))) {
    fprintf(stderr, "(Error) main: Cannot allocate memory for the database.\n");
    return 1;
  }

  // Open the database.
  if (KISSDB_open(db, "mydb.db", KISSDB_OPEN_MODE_RWCREAT, HASH_SIZE, KEY_SIZE, VALUE_SIZE)) {
    fprintf(stderr, "(Error) main: Cannot open the database.\n");
    return 1;
  }

  // create threads
  for (i=0; i<THREADNO; i++){
    if (res = (pthread_create(&consumerTid[i], NULL, thread_func, (void *) &i))){
      ERROR("pthread_create()");
      fprintf(stderr, "(Error) Thread: Can't create thread #%d.\n", i);
    }
    else{
      printf("Thread #%d created.\n", i);
    }
  }

  // main loop: wait for new connection/requests
  while (1) {
    // check if stack is full
    pthread_mutex_lock(&stack_full);
    while (top >= STACK_SIZE){
      pthread_cond_signal(&cond_empty);
      pthread_cond_wait(&cond_full, &stack_full);
    }
    pthread_mutex_unlock(&stack_full);

    // lock global variable change
    pthread_mutex_lock(&top_change);
    // wait for incomming connection
    if ((new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &clen)) == -1) {
      ERROR("accept()");
    }

    // get time of accept
    struct timeval acceptTime;
    gettimeofday(&acceptTime, NULL);

    // pointer to top of stack is raised
    top++;

    // save parameters to struct
    connectionStack[top].connection_fd = new_fd;
    connectionStack[top].curtime = acceptTime;

    // got connection, serve request
    fprintf(stderr, "(Info) main: Got connection from '%s'\n", inet_ntoa(client_addr.sin_addr));

    pthread_mutex_unlock(&top_change);

    // send signal to consumer threads
    if (top > -1){
      pthread_cond_signal(&cond_empty);
    }


  }
  // close socket
  close(new_fd);

  // Destroy the database.
  // Close the database.
  KISSDB_close(db);

  // Free memory.
  if (db)
    free(db);
  db = NULL;

  return 0;
}
