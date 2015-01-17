#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "struct.h"

void Die(char *mess) { perror(mess); exit(1); }

#define N_MSG 1024
cc_msg_t msgPool[N_MSG];
int next = 0;
int count = 0;

cc_msg_ptr allocMsg(int force) {
  if (!force && count == N_MSG) {
    return NULL;
  }

  cc_msg_ptr entry = &msgPool[next];
  next = (next + 1 == N_MSG) ? 0 : next + 1;
  count++;
  return entry;
}

int main(int argc, char**argv) {
  int sockfd,n;
  struct sockaddr_in servaddr,cliaddr;
  socklen_t len;
  cc_msg_ptr next_msg;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(39501);
  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    Die("bind() Call Failed.\n");
  printf("== CS395T Controller Start ==\n");

  while (1) {
    cc_msg_t reply;
    next_msg = allocMsg(1);
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, next_msg, sizeof(cc_msg_t), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    printf("Received the following: %d\n", n);
    msg_decoder(next_msg, &reply);
    sendto(sockfd, &reply, sizeof(cc_msg_t), 0, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
  }
}
