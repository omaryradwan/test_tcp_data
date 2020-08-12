#include "packet.h"

int main(int argc, char **argv){
  srand (time (0));
  int sockfd;
  struct sockaddr_in servaddr;

  if(argc != 4) {
    perror("Usage: client ipaddr portnum Filename");
  }

  int port = atoi(argv[2]);
  if(port <= 1024) perror("Bad port number, please choose port higher than 1024");
  int *file_size = malloc(sizeof(int));
  char* file = file_read(argv[3], file_size);
  //    fprintf(stderr, "##############FILE###################\n%s", file); //this works, proves we open file

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("problem creating socket");
    exit(2);
  }
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);
  servaddr.sin_port = htons(port);
  if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
    perror("Problem in connecting to the server");
    exit(3);
  }
  packet *tmp_2 = malloc(sizeof(packet));
  int seq_num = 12345; //rand() % SEQ_MAX;
  while(1){ //CLEAR SECOND HANDSHAKE
    val_report("BEGGINING 3 way handshake", 1);
    ///BEGIN PACKET CONNECTION
    packet *first_packet = init_1st_packet(seq_num);
    seq_num++; //as in a byte that will be sent
    print_packet_header(SEND, first_packet->header);
    send(sockfd, first_packet, sizeof(packet), 0);

    if(recv(sockfd, tmp_2, sizeof(packet), 0) == 0){
      perror("The server terminated prematurely");
      exit(4);
    }
    print_packet_header(RECV, tmp_2->header);
    if(check_2nd_handshake(tmp_2) == 1) {
      val_report("Successful 2nd handshake check", 1);
      break;
    } else continue;
  }

  ///////////////////////////////////////////////////3rd handshake send + payload


  val_report("File Size", *file_size);
  int packet_num = (*file_size % DATA_SIZE) == 0 ? *file_size / DATA_SIZE : *file_size / DATA_SIZE + 1;
  val_report("Number of Packets", packet_num);
  packet* payload1 = init_payload_packet(tmp_2->header.ack_num, tmp_2->header.seq_num + 1, file, 500);//file_size);
  send(sockfd, payload1, sizeof(packet), 0);
  print_packet_header(SEND, payload1->header);
  //after send, will begin waiting for answers
  /////after 2nd handshake is successful, must send third now, with data
  if(recv(sockfd, tmp_2, sizeof(packet), 0) == 0){
    perror("The server terminated prematurely");
    exit(4);
  }
 print_packet_header(RECV, tmp_2->header);


  ////////////////////////FIN procedure

  seq_num = tmp_2->header.ack_num;
  free(tmp_2);
  tmp_2 = init_fin_packet(seq_num);
  print_packet_header(SEND, tmp_2->header);
  send(sockfd, tmp_2, sizeof(packet), 0);
  free(tmp_2);
  if(recv(sockfd, tmp_2, sizeof(packet), 0) == 0){
    perror("The server terminated prematurely");
    exit(4);
  }

  print_packet_header(RECV, tmp_2->header);
  if(tmp_2->header.ack != 1 || tmp_2->header.fin == 1) {
    fprintf(stderr, "BAD 1st ACK of FIN PROC");
  }

  packet* fin_packet = malloc(sizeof(packet));
  if(recv(sockfd, fin_packet, sizeof(packet), 0) == 0){
    perror("The server terminated prematurely");
    exit(4);
  }
  print_packet_header(RECV, fin_packet->header);
  if(tmp_2->header.ack != 0 || tmp_2->header.fin == 0)
    {
      fprintf(stderr, "BAD 2st ACK of FIN PROC"); //second FIN, start 2 second thing after this
    }
  //int ack_num = fin_packet->header.seq_num;

  time_t clock_begin = clock();
  time_t clock_end = clock();
  //  int final_ack = 0;
  int ack_num = tmp_2->header.seq_num+ 1;
  int seq_final = tmp_2->header.ack_num;
  packet* final_packet = init_ack_packet(seq_final, ack_num);
  send(sockfd, final_packet, sizeof(packet), 0); //send final packet,
  print_packet_header(SEND, final_packet->header);
  while(1){
    clock_end = clock();
    if(clock_end - clock_begin >= 2000000) break;
    //    val_report("Time since", clock_end - clock_begin);
    // if(final_ack == 0){
    // free(tmp_2);
    // if(recv(sockfd, tmp_2, sizeof(packet), 0) == 0){
    //       perror("The server terminated prematurely");
    //   exit(          }
    // }
  }
  fprintf(stderr, "Closing connection\n");
  close(sockfd);



}
