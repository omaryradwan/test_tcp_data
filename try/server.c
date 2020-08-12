#include "packet.h"

int main(int argc, char** argv){
  if(argc != 2) fatal_err("USAGE: ./webservaddr <desired port #>", -1);
  signal(SIGINT,contrlCHandle);
  int port = atoi(argv[1]);
  if(port <= 1024) fatal_err("Bad port number, please choose port higher than 1024", 1);
  int sockfd = 0;
  struct sockaddr_in cliaddr, servaddr;
  sockfd = socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP );
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);
  memset(servaddr.sin_zero, '\0', sizeof(servaddr.sin_zero));
  if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)    fatal_err("Bind failed",1);
  int clilen = sizeof(cliaddr);
  srand(time(NULL));
  int seq_num = rand() % SEQ_MAX;
  int ack_num = 23;
  int packet_size = sizeof(packet);
  packet* outgoing_packet = NULL;
  packet* incoming_packet = malloc(sizeof(packet));
  if(incoming_packet == NULL) fatal_err("NULL PTR", 2);
  //memset(incoming_packet, 0, packet_size);
  //AFTER INITIATIONS, HAVING WHOLE MAIN LOOP FOR MULTIPLE CONNECTIONS
  int file_num = 1;
  int next_seq = 0;

  do {
    char num[5 + strlen(".file")];
    sprintf(num, "%i", file_num);
    FILE *file = fopen(strcat(num, ".file"), "w");
    do{
    if(recvfrom(sockfd, incoming_packet, packet_size, 0, (struct sockaddr*) &cliaddr, &clilen) == -1){
      continue;
    }
    if(check_1st_handshake(incoming_packet) == 1) {
      //val_report("BAD PACKET");
      break;
    }
    }while(1);
    print_packet_header(RECV, incoming_packet->header);
    ack_num = incoming_packet->header.seq_num + 1;
    outgoing_packet = init_2nd_packet(outgoing_packet, seq_num, ack_num);
    print_packet_header(SEND, outgoing_packet->header);
    if(sendto(sockfd, outgoing_packet, sizeof(packet), 0, (struct sockaddr*)&cliaddr, clilen) == -1){
      fatal_err("Bad Second handshake", 1);
    }

    outgoing_packet = init_ack_packet(outgoing_packet, seq_num, ack_num);

    do{         //////////////////////GEtting 3rd handshake + payloads
      memset(incoming_packet, 0, packet_size);

      if(recvfrom(sockfd, incoming_packet, packet_size, 0, (struct sockaddr*) &cliaddr, &clilen) == -1){
        fatal_err("Bad incoming payload", 1);
      }
      if(incoming_packet->header.seq_num < next_seq){
        outgoing_packet->header.ack_num = incoming_packet->header.seq_num + 1;
        outgoing_packet->header.seq_num = outgoing_packet->header.seq_num + 1;
        print_packet_header(RECV, incoming_packet->header);
      }
      else if(incoming_packet->header.ack == 0){
        if(incoming_packet->header.fin == 1){
          print_packet_header(RECV, incoming_packet->header);
          break;
        }
      }
      else {
        fwrite(incoming_packet->data, sizeof(char), incoming_packet->header.payload, file);
        outgoing_packet->header.ack_num = incoming_packet->header.seq_num + 1;
        next_seq++;
        print_packet_header(RECV, incoming_packet->header);
        //        outgoing_packet->header.seq_num = outgoing_packet->header.seq_num + 1;
        outgoing_packet->header.ack_num += DATA_SIZE - 1;
      }
      outgoing_packet->header.seq_num += 1;
      if(sendto(sockfd, outgoing_packet, sizeof(packet), 0, (struct sockaddr*)&cliaddr, clilen) == -1){
        fatal_err("Bad ack packet", 1);
                                                                                                       }
      print_packet_header(SEND, outgoing_packet->header);
    }while(1);
    /////////////////////////////////////FIN
    outgoing_packet = init_ack_packet(outgoing_packet, incoming_packet->header.seq_num, outgoing_packet->header.ack_num);

    if(sendto(sockfd, outgoing_packet, sizeof(packet), 0, (struct sockaddr*)&cliaddr, clilen) == -1){
      fatal_err("BAD ACK PACKET", 1);
    }
    print_packet_header(SEND, outgoing_packet->header);
    outgoing_packet = init_fin_packet(outgoing_packet, incoming_packet->header.seq_num);
    if(sendto(sockfd, outgoing_packet, sizeof(packet), 0, (struct sockaddr*)&cliaddr, clilen) == -1){
      fatal_err("BAD FIN PACKET", 1);
    }
    print_packet_header(SEND, outgoing_packet->header);
    fclose(file);
    file_num++;
  }while(1);
  close(sockfd);
}
