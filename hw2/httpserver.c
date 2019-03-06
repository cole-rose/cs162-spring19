#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>

#include "libhttp.h"
#include "wq.h"

#define BUFFER_SIZE 8192
/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
wq_t work_queue;
int num_threads;
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;
pthread_cond_t cond;

typedef struct fd_pair {
    int from;
    int to;
} fd_pair;

void not_found_response(int fd){
  http_start_response(fd, 404);
  http_send_header(fd, "Content-Type:", "text/html");
}

void http_file_response(int fd, char *file_name){

  char file_size[64];

    FILE *file = fopen(file_name, "r");
    if (file != NULL){
      fseek(file, 0, SEEK_END);
      int len = ftell(file);
      sprintf(file_size, "%d", len);

      //put file pointer back to the beginning
      fseek(file, 0, SEEK_SET);
      char *buffer = malloc(len+1);

      size_t newLen = fread(buffer, sizeof(char), len, file);
      fprintf(stdout, "newLen: %d\n", newLen);


//      fprintf(stdout, "Can open file.\n");
      http_start_response(fd, 200);
      http_send_header(fd, "Content-Type", http_get_mime_type(file_name));
      http_send_header(fd, "Content-Length", file_size);
      fprintf(stdout, "Content-Length: %d\n", len);
      http_end_headers(fd);



      if (ferror(file) != 0 ) {
        fputs("Error reading file", stderr);
      } else {
        buffer[newLen++] = '\0'; /* Just to be safe. */
      }
//      http_send_string(fd,buffer);
//      http_send_data(fd, buffer, fileStat->st_size);
      http_send_data(fd, buffer, newLen);
      fclose(file);

//      fprintf(stdout, "buffer: %s\n", buffer);
      free(buffer);
    }else{
      not_found_response(fd);
    }


}

void http_directory_response(int fd, char *dir_path){
  DIR *dir = opendir(dir_path);
  struct dirent *dirent;
  struct stat fileStat;

  http_start_response(fd, 200);
  http_send_header(fd, "Content-type", "text/html");
  http_end_headers(fd);

  fprintf(stdout, "dir_path: %s\n", dir_path);

  if (dir != NULL) {
    char *path = malloc(BUFFER_SIZE + 1);
    char *link = malloc(BUFFER_SIZE + 1);

    while ((dirent = readdir(dir)) != NULL) {
      strcpy(path, dir_path);
        fprintf(stdout, "path in strcpy(full_path, dir_path): %s\n", path);
      if (path[strlen(path) - 1] != '/') {
        strcat(path, "/");
      }
      strcat(path, dirent->d_name);
      fprintf(stdout, "path in strcat(full_path, dir_entry->d_name): %s\n", path);
      int file_status = stat(path, &fileStat);
      if (file_status < 0){
        // send a 404 Not found response
        fprintf(stdout, "stat(http_path, &fileStat) < 0\n");
        not_found_response(fd);
      } else{
        if (S_ISDIR(fileStat.st_mode)) {
          fprintf(stdout, "This is directory.\n");
          fprintf(stdout, "dir_entry->d_name: %s\n", dirent->d_name);
          snprintf(link, BUFFER_SIZE, "<a href='%s/'>%s/</a>\n", dirent->d_name, dirent->d_name);
        } else {
          fprintf(stdout, "This is NOT directory.\n");
          fprintf(stdout, "dir_entry->d_name: %s\n", dirent->d_name);
          snprintf(link, BUFFER_SIZE, "<a href='./%s'>%s/</a>\n", dirent->d_name, dirent->d_name);
        }
        http_send_string(fd, link);
      }

    }
    closedir(dir);
    free(link);
    free(path);

  }
}

//int has_index_file(char* path, char *file_path, struct stat *fileStat) {
int has_index_file(char* path) {
//  fprintf(stdout, "Inside has_index_file\n");
  int status;
  char* file_name = malloc(BUFFER_SIZE + 1);
//  fprintf(stdout, "After char file_name\n");
  strcpy(file_name, path);
//  fprintf(stdout, "After strcpy\n");
  strcat(file_name, "index.html");
//  fprintf(stdout, "After strcat\n");

  status = open(file_name, O_RDONLY);
  if (status == -1) {
    fprintf(stdout, "Oh there's NO index.html.\n");
  } else{
    fprintf(stdout, "Oh there's index.html.\n");
    strcpy(path, file_name);
  }
  free(file_name);

  return status;
}
/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void handle_files_request(int fd) {

  /*
   * TODO: Your solution for Task 1 goes here! Feel free to delete/modify *
   * any existing code.
   */

  struct http_request *request = http_request_parse(fd);

  char *http_path = malloc(BUFFER_SIZE+1);
  strcpy(http_path, server_files_directory);
  strcat(http_path, request->path);

  fprintf(stdout, "http_path: %s\n", http_path);
  struct stat fileStat;

  if (request == NULL) {
    not_found_response(fd);
  }

  int file_status = stat(http_path, &fileStat);
//  fprintf(stdout, "file_status: %d\n", file_status);

  if (file_status < 0) {
    // send a 404 Not found response
    fprintf(stdout, "stat(http_path, &fileStat) < 0\n");
    not_found_response(fd);
  } else {
    // if HTTP request's path corresponds to a file
    if (S_ISREG(fileStat.st_mode)) {
      fprintf(stdout, "This is file.\n");
      // response a file
      http_file_response(fd, http_path);
    }
      // HTTP request's path is a directory
    else if (S_ISDIR(fileStat.st_mode)) {
      fprintf(stdout, "This is directory.\n");

      // check if index.html exist
      int index_file_check = has_index_file(http_path);
      fprintf(stdout, "index_file_check: %d\n", index_file_check);
      // return -1 if index.html file does not exist
      if (index_file_check == -1){
        // send a list of file in directory
        fprintf(stdout, "index_file_check: %d\n", index_file_check);
//        ;
         http_directory_response(fd, http_path);
      } else{
        //send index.html file
        fprintf(stdout, "http_path with directory: %s\n", http_path);
        http_file_response(fd, http_path);
      }

    }
  }
  free(http_path);
}



/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd) {

  /*
  * The code below does a DNS lookup of server_proxy_hostname and 
  * opens a connection to it. Please do not modify.
  */

  struct sockaddr_in target_address;
  memset(&target_address, 0, sizeof(target_address));
  target_address.sin_family = AF_INET;
  target_address.sin_port = htons(server_proxy_port);

  struct hostent *target_dns_entry = gethostbyname2(server_proxy_hostname, AF_INET);

  int client_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (client_socket_fd == -1) {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  if (target_dns_entry == NULL) {
    fprintf(stderr, "Cannot find host: %s\n", server_proxy_hostname);
    exit(ENXIO);
  }

  char *dns_address = target_dns_entry->h_addr_list[0];

  memcpy(&target_address.sin_addr, dns_address, sizeof(target_address.sin_addr));
  int connection_status = connect(client_socket_fd, (struct sockaddr*) &target_address,
      sizeof(target_address));

  if (connection_status < 0) {
    /* Dummy request parsing, just to be compliant. */
    http_request_parse(fd);

    http_start_response(fd, 502);
    http_send_header(fd, "Content-Type", "text/html");
    http_end_headers(fd);
    http_send_string(fd, "<center><h1>502 Bad Gateway</h1><hr></center>");
    return;

  }

  /* 
  * TODO: Your solution for task 3 belongs here! 
  */

//
//  pthread_t server;
//  pthread_create(&server, NULL, proxy_helper, &fd);
//
//  pthread_t client;
//  pthread_create(&client, NULL, proxy_helper, &client_socket_fd);
//
//  /* Wait for child thread to finish */
//  pthread_join(client, NULL);
//  pthread_join(server, NULL);

  size_t BUF_SIZE = 8192;
  char read_buf[BUF_SIZE];
  read(fd, read_buf, BUF_SIZE);
  send(client_socket_fd, read_buf, BUF_SIZE, MSG_EOR);

  recv(client_socket_fd, read_buf, BUF_SIZE, MSG_PEEK);

  send(fd, read_buf, BUF_SIZE, MSG_EOR);
//  http_send_data(fd, read_buf, BUF_SIZE);



  printf("Finish handling proxy\n");

}


void *helper(void *args){
    while (1) {
        void (*request_handler)(int) = args;
        int fd = wq_pop(&work_queue);
        request_handler(fd);
        close(fd);
    }
//  return NULL;

}

void init_thread_pool(int num_threads, void (*request_handler)(int)) {
  /*
   * TODO: Part of your solution for Task 2 goes here!
   */
  wq_init(&work_queue);

  fprintf(stdout, "num_threads: %d\n", num_threads);
  fprintf(stdout, "work_queue size: %d\n", work_queue.size);

//  pthread_t threads[num_threads];
  fprintf(stdout, "After threads[num_threads]\n");

  /* start threads */
  for (int i = 0; i < num_threads; i++){
    pthread_t thread;
    pthread_create(&thread, NULL, &helper, request_handler);
//    pthread_create(&threads[i], NULL, &helper, request_handler);
  }

  fprintf(stdout, "Number of threads created: %d\n", num_threads);

}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  init_thread_pool(num_threads, request_handler);

  while (1) {
    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    // TODO: Change me?
    if (num_threads == 0){
      fprintf(stdout, "Num threads is 0\n");
//      request_handler(client_socket_number);
//      close(client_socket_number);
    } else{
      fprintf(stdout, "Num threads is not 0\n");
      wq_push(&work_queue, client_socket_number);
    }


    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);
  }

  shutdown(*socket_number, SHUT_RDWR);
  close(*socket_number);
}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000 [--num-threads 5]\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000 [--num-threads 5]\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--num-threads", argv[i]) == 0) {
      char *num_threads_str = argv[++i];
      if (!num_threads_str || (num_threads = atoi(num_threads_str)) < 1) {
        fprintf(stderr, "Expected positive integer after --num-threads\n");
        exit_with_usage();
      }
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
