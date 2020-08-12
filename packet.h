#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define SEQ_MAX 25600
#define MAXLINE 1024
#define LISTENQ 1024
#define DATA_SIZE 512
#define SEQ_MAX 25600
#define WINDOW_SIZE 10
int connfd, listenfd;
#define RECV 0
#define SEND 1
#define RESEND 2


void contrlCHandle(int sig_num)
{
  signal(SIGINT, contrlCHandle);
  printf("terminating, while closing sockets\n");
  fflush(stdout);
  close(connfd);
  close(listenfd);
  exit(1);
}

typedef struct {
  int seq_num;    //4 bytes
  int ack_num;    //4 bytes
  char empty;     //don't use 1 byte
  char ack;       //1 byte
  char syn;       // 1 byte
  char fin;       //1 byte
  int payload;
} packet_header;

typedef struct{
  packet_header header;
  char data[DATA_SIZE];
} packet;

//////////Borrowed function from https://www.strudel.org.uk/itoa/
char* itoa(int val, int base){
  static char buf[32] = {0};
  int i = 30;
  for(; val && i ; --i, val /= base)
    buf[i] = "0123456789abcdef"[val % base];
  return &buf[i+1];
}
void val_report(char *disc, int val){
  fprintf(stderr, "%s: %i\n", disc, val);
}
void fatal_err(char* err_disc, int type){
  //  perror(err_disc);
  fprintf(stderr, "%s\n", err_disc);
  close(connfd);
  close(listenfd);
  exit(type);
}

void print_packet_header(int type, packet_header head){
  if(type == RECV) fprintf(stdout, "RECV ");
  if(type == SEND) fprintf(stdout, "SEND ");
  if(type == RESEND) fprintf(stdout, "RESEND ");
  fprintf(stdout, "%i %i", head.seq_num, head.ack_num);
  if(head.ack == 1) printf(" ACK");
  if(head.syn == 1) printf(" SYN");
  if(head.fin == 1) printf(" FIN");
  printf("\n");
}


packet* init_empty_packet(int seq){
  packet* tmp_packet = malloc(sizeof(packet));
  memset(tmp_packet->data, 0, DATA_SIZE);
  tmp_packet->header.fin = 0;
  tmp_packet->header.syn = 0;
  tmp_packet->header.ack_num = 0;
  tmp_packet->header.ack = 0;
  tmp_packet->header.seq_num = seq;
  return tmp_packet;
}

packet* init_packet(packet* orig, int fin, int syn, int ack_num, int ack, int seq, char* dest, int cpy_size){
  if(orig != NULL) free(orig);
  packet* tmp_packet = malloc(sizeof(packet));
  if(tmp_packet == NULL) fatal_err("MALLOC OF PACKET FAIL", 1);
  memset(tmp_packet, 0, sizeof(packet));
  tmp_packet->header.fin = fin;
  tmp_packet->header.syn = syn;
  tmp_packet->header.ack_num = ack_num;
  tmp_packet->header.ack = ack;
  tmp_packet->header.seq_num = seq;
  memcpy(tmp_packet->data, dest, cpy_size);
  return tmp_packet;
}

packet* init_1st_packet(packet* orig, int seq){
  return init_packet(orig, 0, 1, 0, 0, seq, NULL, 0);
}

packet* init_2nd_packet(packet* orig, int seq, int ack_num){
  return init_packet(orig, 0, 1, ack_num, 1, seq, NULL, 0);
}

packet* init_payload_packet(packet* orig, int seq, int ack_num, char *data_cpy, int cpy_size){
  return init_packet(orig, 0, 0, ack_num, 1, seq, data_cpy, cpy_size);;
}
packet* init_ack_packet(packet* orig, int seq, int ack_num){
  return init_packet(orig, 0, 0, ack_num, 1, seq, NULL, 0);;
}

int check_1st_handshake(packet* packet){
  if(packet->header.ack == 0 && packet->header.fin == 0 && packet->header.syn == 1) return 1;
  else return 0;
}

int check_2nd_handshake(packet* packet){
  if(packet->header.ack == 1 && packet->header.fin == 0 && packet->header.syn == 1) return 1;
  else return 0;
}

int check_3rd_handshake(packet* packet){
  if(packet->header.ack == 1 && packet->header.fin == 0 && packet->header.syn == 0) return 1;
  else return 0;
}

packet* init_fin_packet(packet* orig, int seq){
  return init_packet(orig, 1, 0, 0, 0, seq, NULL, 0);
}

int check_fin_handshake(packet* packet){
  if(packet->header.ack == 0 && packet->header.fin == 1 && packet->header.ack_num == 0) return 1;
  else return 0;
}

char* file_read(char* filename, int* file_size){
  *file_size = 0;
  fprintf(stderr, "Filename: %s\n", filename);
  FILE* filefd;
  char* file = NULL;
  filefd = fopen(filename, "rb");
  if(!filefd) fatal_err("Bad File descripter", 1);
  fseek(filefd, 0, SEEK_END);
  *file_size = ftell(filefd);
  rewind(filefd);
  file = malloc(sizeof(char) * *file_size);
  if(file == NULL) fatal_err("BAD malloc",  1);
  if(fread(file, *file_size, 1, filefd) == 0) fatal_err("Bad fread", 1);
  fclose(filefd);
  return file;
}
