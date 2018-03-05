/* client.c

   Sample code of
   Assignment L1: Simple multi-threaded key-value server
   for the course MYY601 Operating Systems, University of Ioannina

   (c) S. Anastasiadis, G. Kappes 2016

*/


#include "utils.h"

#define SERVER_PORT     6767
#define BUF_SIZE        2048
#define MAXHOSTNAMELEN  1024
#define MAX_STATION_ID   128
#define ITER_COUNT          1
#define GET_MODE           1
#define PUT_MODE           2
#define USER_MODE          3
#define THREADNO           2 // set the number of threads

// struct with information previously stored in main function
typedef struct mystruct{
  int count;
  char snd_buffer[BUF_SIZE];
  int mode;
  struct sockaddr_in server_addr;
} Mystruct;

// Declare and initialize mutexes & thread wait condition
pthread_mutex_t mutex_done = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_talk = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_put = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_get = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_done = PTHREAD_COND_INITIALIZER;

// ID of each thread created is stored here
pthread_t client[THREADNO];


/**
 * @name print_usage - Prints usage information.
 * @return
 */
void print_usage() {
  fprintf(stderr, "Usage: client [OPTION]...\n\n");
  fprintf(stderr, "Available Options:\n");
  fprintf(stderr, "-h:             Print this help message.\n");
  fprintf(stderr, "-a <address>:   Specify the server address or hostname.\n");
  fprintf(stderr, "-o <operation>: Send a single operation to the server.\n");
  fprintf(stderr, "                <operation>:\n");
  fprintf(stderr, "                PUT:key:value\n");
  fprintf(stderr, "                GET:key\n");
  fprintf(stderr, "-i <count>:     Specify the number of iterations.\n");
  fprintf(stderr, "-g:             Repeatedly send GET operations.\n");
  fprintf(stderr, "-p:             Repeatedly send PUT operations.\n");
}



/**
 * @name talk - Sends a message to the server and prints the response.
 * @server_addr: The server address.
 * @buffer: A buffer that contains a message for the server.
 *
 * @return
 */
void talk(const struct sockaddr_in server_addr, char *buffer) {
  char rcv_buffer[BUF_SIZE];
  int socket_fd, numbytes;

  // create socket
  pthread_mutex_lock(&mutex_talk);
  if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    ERROR("socket()");
  }

  // connect to the server.
  if (connect(socket_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
    ERROR("connect()");
  }

  // send message.
  write_str_to_socket(socket_fd, buffer, strlen(buffer));
  pthread_mutex_unlock(&mutex_talk);

  // receive results.
  printf("Result: ");
  do {
    memset(rcv_buffer, 0, BUF_SIZE);
    numbytes = read_str_from_socket(socket_fd, rcv_buffer, BUF_SIZE);
    if (numbytes != 0)
      printf("%s", rcv_buffer); // print to stdout
  } while (numbytes > 0);
  printf("\n");

  // close the connection to the server.
  close(socket_fd);
}
/*
 * @name client_thread - Threads run this function
 * @param Mystruct - Holds the request parameters
*/
void *client_thread(void *arg){
  int j;
  Mystruct *arg1 = (Mystruct *) arg;
  int value;
  int count = arg1->count;
  int mode = arg1->mode;
  int station;
  int laststation = 0;
  struct sockaddr_in server_addr = arg1->server_addr;
  char snd_buffer[BUF_SIZE];


  for (j = 0; j < BUF_SIZE; j++){
    snd_buffer[j] = arg1->snd_buffer[j];
  }
  while(--count >= 0) {
    for (station = (0 + laststation); station <= (MAX_STATION_ID + laststation); station++) {
      memset(snd_buffer, 0, BUF_SIZE);
      if (mode == GET_MODE) {
        // Repeatedly GET.
        pthread_mutex_lock(&mutex_get);
        sprintf(snd_buffer, "GET:station.%d", station);
        pthread_mutex_unlock(&mutex_get);
      } else if (mode == PUT_MODE) {
        // Repeatedly PUT.
        // create a random value.
        // value = rand() % 65 + (-20);
        pthread_mutex_lock(&mutex_put);
        value = station;
        sprintf(snd_buffer, "PUT:station.%d:%d", station, value);
        pthread_mutex_unlock(&mutex_put);
      }
      printf("Operation: %s\n", snd_buffer);
      talk(server_addr, snd_buffer);
    }
    laststation = station;
  }
  pthread_cond_signal(&cond_done);
  return 0;
}

/**
 * @name main - The main routine.
 */
int main(int argc, char **argv) {
  int i, j;    // helper variables for for loops
  int res;
  char *host = NULL;
  char *request = NULL;
  int mode = 0;
  int option = 0;
  int count = ITER_COUNT;
  char snd_buffer[BUF_SIZE];
  struct sockaddr_in server_addr;
  struct hostent *host_info;

  // Parse user parameters.
  while ((option = getopt(argc, argv,"i:hgpo:a:")) != -1) {
    switch (option) {
      case 'h':
        print_usage();
        exit(0);
      case 'a':
        host = optarg;
        break;
      case 'i':
        count = atoi(optarg);
	break;
      case 'g':
        if (mode) {
          fprintf(stderr, "You can only specify one of the following: -g, -p, -o\n");
          exit(EXIT_FAILURE);
        }
        mode = GET_MODE;
        break;
      case 'p':
        if (mode) {
          fprintf(stderr, "You can only specify one of the following: -g, -p, -o\n");
          exit(EXIT_FAILURE);
        }
        mode = PUT_MODE;
        break;
      case 'o':
        if (mode) {
          fprintf(stderr, "You can only specify one of the following: -r, -w, -o\n");
          exit(EXIT_FAILURE);
        }
        mode = USER_MODE;
        request = optarg;
        break;
      default:
        print_usage();
        exit(EXIT_FAILURE);
    }
  }

  // Check parameters.
  if (!mode) {
    fprintf(stderr, "Error: One of -g, -p, -o is required.\n\n");
    print_usage();
    exit(0);
  }
  if (!host) {
    fprintf(stderr, "Error: -a <address> is required.\n\n");
    print_usage();
    exit(0);
  }

  // get the host (server) info
  if ((host_info = gethostbyname(host)) == NULL) {
    ERROR("gethostbyname()");
  }

  // create socket adress of server (type, IP-adress and port number)
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr = *((struct in_addr*)host_info->h_addr);
  server_addr.sin_port = htons(SERVER_PORT);

  if (mode == USER_MODE) {
    memset(snd_buffer, 0, BUF_SIZE);
    strncpy(snd_buffer, request, strlen(request));
    printf("Operation: %s\n", snd_buffer);
    talk(server_addr, snd_buffer);
  }
  else {
    Mystruct arg1[THREADNO];
    for (j = 0; j < THREADNO; j++){     // each thread is given the same connection
      arg1[j].count = count;
      arg1[j].mode = mode;
      arg1[j].server_addr = server_addr;
      for (i = 0; i < BUF_SIZE; i++){
        arg1[j].snd_buffer[i] = snd_buffer[i];
      }
    }
    for (i = 0; i < THREADNO; i++){     // Create the threads!
      if (res = pthread_create(&client[i], NULL, client_thread, (void *) &arg1[i])){
        ERROR("pthread_create()");
        fprintf(stderr, "(Error) Thread: Can't create client thread.\n");
      }
    }
  }
  pthread_mutex_lock(&mutex_done);
  pthread_cond_wait(&cond_done, &mutex_done);
  pthread_mutex_unlock(&mutex_done);
  return 0;
}
