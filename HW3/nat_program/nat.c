#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <netinet/in.h> // required by "netfilter.h"
#include <arpa/inet.h> // required by ntoh[s|l]()
#include <signal.h> // required by SIGINT
#include <string.h> // required by strerror()
#include <sys/time.h> // required by gettimeofday()
#include <time.h> // required by nanosleep()
#include <errno.h> // required by errno
#include <pthread.h>
#include <netinet/ip.h>        // required by "struct iph"
#include <netinet/tcp.h>    // required by "struct tcph"
#include <netinet/udp.h>    // required by "struct udph"
#include <netinet/ip_icmp.h>    // required by "struct icmphdr"

extern "C" {
#include <linux/netfilter.h> // required by NF_ACCEPT, NF_DROP, etc...
#include <libnetfilter_queue/libnetfilter_queue.h>
}

#include "checksum.h"

#define BUF_SIZE 2076

struct nat_entry {
  u_int32_t internal_ip;
  u_int16_t internal_port;
  u_int16_t translated_port;
  long timestamp_ms;
  int expiry;
};

u_int32_t IP;
u_int32_t LAN;
unsigned int LOCAL_MASK;
int BUCKET_SIZE;
int FILL_RATE;

const int table_size = 2001;
struct nat_entry nat_table[table_size];

long get_timestamp(){
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return round(spec.tv_nsec / 1.0e6) + spec.tv_sec * 1000;
}

void nat_print_table(){
  printf("Updated NAT table:\n");
  for(int i = 0 ; i < table_size ; i++){
    struct nat_entry tmp = nat_table[i];
    if(tmp.expiry == 0){
      struct in_addr ip_addr;
      ip_addr.s_addr = tmp.internal_ip;
      printf("%s:%u -> %s:%u\n", inet_ntoa(ip_addr),ntohs(tmp.internal_port), "10.0.3.1",ntohs(tmp.translated_port));
    }
  }
}

// int nat_map_outbound(){
//   // modify the source ip and port 
//   return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);;
// }

// int nat_map_inbound(){
//   // modify the dest ip and port
//   return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);;
// }

int nat_create_entry(u_int32_t ip, u_int16_t port, struct nat_entry* entry_ptr){
  for(int i = 0 ; i < table_size ; i++){
    struct nat_entry tmp = nat_table[i];
    if(tmp.expiry == 1){
      // replace the nat record if it is expired
      // assume the input is in network bytes order
      // store the records in network bytes order
      tmp.internal_ip = ip;
      tmp.internal_port = port;
      // the translated port is actually the index of the array + 10000, which is a constant
      tmp.timestamp_ms = get_timestamp();
      tmp.expiry = 0;
      nat_table[i] = tmp;
      memcpy(entry_ptr, &nat_table[i], sizeof(nat_entry));
      nat_print_table();
      return 1;
    }
  }
  return 0;
}

int nat_search_entry(int mode, u_int32_t ip, u_int16_t port, struct nat_entry* entry_ptr){
  int result = 0;
  int print_flag = 0;
  if(mode == 1){
    // it is outbound
    for(int i = 0 ; i < table_size ; i++){
      struct nat_entry tmp = nat_table[i];
      if((tmp.internal_ip == ip && tmp.internal_port == port) && tmp.expiry == 0){
        // do thing
        result = 1;
        // update timestamp
        nat_table[i].timestamp_ms = get_timestamp();
        // memcpy the entry
        memcpy(entry_ptr, &nat_table[i], sizeof(struct nat_entry));
      }
      else{
        if(tmp.expiry == 0){
          // only deal with the packet that is not marked expiry
          long now_ms = get_timestamp();
          if(now_ms - tmp.timestamp_ms > 10000){
            // mark the expiry entry
            print_flag = 1;
            nat_table[i].expiry = 1;
          }
        }
      }
    }
  }
  else{
    // it is inbound
    for(int i = 0 ; i < table_size ; i++){
      struct nat_entry tmp = nat_table[i];
      if(tmp.translated_port == port && tmp.expiry == 0){
        // do thing
        result = 1;
        nat_table[i].timestamp_ms = get_timestamp();
        memcpy(entry_ptr, &nat_table[i], sizeof(struct nat_entry));
      }
      else{
        if(tmp.expiry == 0){
          // only deal with the packet that is not marked expiry
          long now_ms = get_timestamp();
          if(now_ms - tmp.timestamp_ms > 10000){
            // mark the expiry entry
            print_flag = 1;
            nat_table[i].expiry = 1;
          }
        }
      }
    }
  }
  if(print_flag == 1){
    nat_print_table();
  }
  return result;
}

void print_ip(unsigned int ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);        
}

static int Callback(struct nfq_q_handle *myQueue, struct nfgenmsg *msg,
    struct nfq_data *pkt, void *cbData) {
  // Get the id in the queue
  unsigned int id = 0;
  struct nfqnl_msg_packet_hdr *header;
  if (header = nfq_get_msg_packet_hdr(pkt)){
    id = ntohl(header->packet_id);
  } 

  // Access IP Packet
  unsigned char *pktData;
  int ip_pkt_len = nfq_get_payload(pkt, &pktData);
  struct iphdr *ipHeader = (struct iphdr *)pktData;

  if (ipHeader->protocol != IPPROTO_UDP) {
    return nfq_set_verdict(myQueue, id, NF_DROP, ip_pkt_len, pktData);
  }

  // Access UDP Packet
  struct udphdr *udph = (struct udphdr *) (((char*)ipHeader) + ipHeader->ihl*4);;

  // Access App Data
  unsigned char* appData = pktData + ipHeader->ihl * 4 + 8;

  // ipHeader->ihl is in unit of 4 bytes
  // prompt: udp_payload_len + udp_header_len + ip_header_len = ip_pkt_len
  int udp_payload_len = ip_pkt_len - ipHeader->ihl * 4 - 8;

  struct nat_entry target;
  if ((ipHeader->saddr & LOCAL_MASK) == LAN) {
    // printf("It is outbound\n");
    if(nat_search_entry(1, ipHeader->saddr, udph->source, &target) == 1){
      // old entry is found
      ipHeader->saddr = IP;
      udph->source = target.translated_port;
      udph->check = udp_checksum(pktData);
      ipHeader->check = ip_checksum(pktData);
      return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);
    }
    else if(nat_create_entry(ipHeader->saddr, udph->source, &target) == 1){
      // new entry create success
      ipHeader->saddr = IP;
      udph->source = target.translated_port;
      udph->check = udp_checksum(pktData);
      ipHeader->check = ip_checksum(pktData);
      return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);
    }
    else{
      return nfq_set_verdict(myQueue, id, NF_DROP, ip_pkt_len, pktData);
    }
  }
  else{
    printf("It is inbound\n");
    if(nat_search_entry(0, IP, udph->dest, &target) == 1){
      // record is found
      ipHeader->daddr = target.internal_ip;
      udph->dest = target.internal_port;
      udph->check = udp_checksum(pktData);
      ipHeader->check = ip_checksum(pktData);
      return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);
    }
    else{
      return nfq_set_verdict(myQueue, id, NF_DROP, ip_pkt_len, pktData);
    }
  }
}


int main(int argc, char** argv) {

  struct in_addr arg_ip_addr;

  inet_aton(argv[1], &arg_ip_addr);
  IP = arg_ip_addr.s_addr;

  inet_aton(argv[2], &arg_ip_addr);
  LAN = arg_ip_addr.s_addr;

  int mask_int = atoi(argv[3]);
  unsigned int tmp_mask = 0xffffffff << (32 - mask_int);
  LOCAL_MASK = htonl(tmp_mask);
  BUCKET_SIZE = atoi(argv[4]);
  FILL_RATE = atoi(argv[5]);

  // Get a queue connection handle from the module
  struct nfq_handle *nfqHandle;
  if (!(nfqHandle = nfq_open())) {
    fprintf(stderr, "Error in nfq_open()\n");
    exit(-1);
  }

  // Unbind the handler from processing any IP packets
  if (nfq_unbind_pf(nfqHandle, AF_INET) < 0) {
    fprintf(stderr, "Error in nfq_unbind_pf()\n");
    exit(1);
  }

  // Install a callback on queue 0
  struct nfq_q_handle *nfQueue;
  if (!(nfQueue = nfq_create_queue(nfqHandle,  0, &Callback, NULL))) {
    fprintf(stderr, "Error in nfq_create_queue()\n");
    exit(1);
  }
  // nfq_set_mode: I want the entire packet 
  if(nfq_set_mode(nfQueue, NFQNL_COPY_PACKET, BUF_SIZE) < 0) {
    fprintf(stderr, "Error in nfq_set_mode()\n");
    exit(1);
  }

  struct nfnl_handle *netlinkHandle;
  netlinkHandle = nfq_nfnlh(nfqHandle);

  int fd;
  fd = nfnl_fd(netlinkHandle);

  int res;
  char buf[BUF_SIZE];

  for(int i = 0 ; i < table_size ; i++){
    nat_table[i].translated_port = htons(i + 10000);
    nat_table[i].expiry = 1;
  }

  // for(int i = 0 ; i < 10 ; i++){
  //   struct in_addr ip_addr;
  //   inet_aton("10.0.3.2", &ip_addr);
  //   nat_create_entry(ip_addr.s_addr, htons(100+i), &nat_table[i]);
  // }

  printf("Start to handle packets\n");
  while ((res = recv(fd, buf, sizeof(buf), 0)) && res >= 0) {
    // printf("received!\n");
    nfq_handle_packet(nfqHandle, buf, res);
  }

  nfq_destroy_queue(nfQueue);
  nfq_close(nfqHandle);

}
