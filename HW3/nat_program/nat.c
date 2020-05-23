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
struct Queue 
{ 
	int front, rear, size; 
	int capacity; 
	unsigned char** array; 
}; 

u_int32_t IP;
u_int32_t LAN;
unsigned int LOCAL_MASK;
int BUCKET_SIZE;
int FILL_RATE;
int TOKENS;

pthread_mutex_t bucket_lock;
pthread_mutex_t packet_buf_lock;

const int table_size = 2001;
struct nat_entry nat_table[table_size];
struct Queue* packet_queue;

struct Queue* createQueue(int capacity) 
{ 
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue)); 
	queue->capacity = capacity; 
  queue->size = 0;
	queue->front =  0;
	queue->rear = capacity - 1; // This is important, see the enqueue 
	queue->array = (unsigned char**) malloc(sizeof(unsigned char*) * queue->capacity);
  for(int i = 0; i < queue->capacity ; i++){
    queue->array[i] = (unsigned char *) malloc(BUF_SIZE);
  }
	return queue; 
} 

// Queue is full when size becomes equal to the capacity 
int isFull(struct Queue* queue) 
{ return (queue->size == queue->capacity); } 

// Queue is empty when size is 0 
int isEmpty(struct Queue* queue) 
{ return (queue->size == 0); } 

// Function to add an item to the queue. 
// It changes rear and size 
int enqueue(struct Queue* queue, unsigned char* item) 
{ 
  if(isFull(queue)){
    return 0;
  }
  // item is the address of the pktData in handle
	queue->rear = (queue->rear + 1)%queue->capacity; 
	memcpy(queue->array[queue->rear], item, BUF_SIZE); 
	queue->size = queue->size + 1; 
  return 1;
} 

// Function to remove an item from queue. 
// It changes front and size 
int dequeue(struct Queue* queue, unsigned char* item) 
{ 
  if(isEmpty(queue)){
    return 0;
  }
	memcpy(item, queue->array[queue->front], BUF_SIZE); 
	queue->front = (queue->front + 1)%queue->capacity; 
	queue->size = queue->size - 1; 
  return 1;
} 

int my_sleep(long ns){
  struct timespec tim1, tim2;
  tim1.tv_sec = 0;
  tim1.tv_nsec = ns;
  if(nanosleep(&tim1, &tim2) < 0){
    printf("nanosleep() failed!\n");
    return 0;
  }
  return 1;
}

long get_timestamp(){
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return round(spec.tv_nsec / 1.0e6) + spec.tv_sec * 1000;
}

void nat_print_table(){
  int empty_flag = 1;
  printf("Updated NAT table:\n");
  for(int i = 0 ; i < table_size ; i++){
    struct nat_entry tmp = nat_table[i];
    if(tmp.expiry == 0){
      empty_flag = 0;
      struct in_addr ip_addr;
      ip_addr.s_addr = tmp.internal_ip;
      printf("%s:%u -> %s:%u\n", inet_ntoa(ip_addr),ntohs(tmp.internal_port), "10.0.3.1",ntohs(tmp.translated_port));
    }
  }
  if(empty_flag == 1){
    printf("<Empty>\n");
  }
}

int consume_token(){
  int result = 0;
  pthread_mutex_lock(&bucket_lock);
  // int tmp1, tmp2;
  // tmp1 = TOKENS;
  if(TOKENS > 0){
    // tokens exist
    result = 1;
    TOKENS = TOKENS - 1;
  }
  // tmp2 = TOKENS;
  // if(tmp1 != 0 || tmp2 != 0){
  //   printf("Tokens: %d -> %d\n", tmp1, tmp2);
  // }
  pthread_mutex_unlock(&bucket_lock);
  return result;
}

void transmit_pkt(struct nfq_q_handle *myQueue, unsigned int id, int ip_pkt_len, unsigned char *pktData){
  while (consume_token() == 0){
    my_sleep(500000);
  }
  nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);
}

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
      if(tmp.expiry == 1){
        continue;
      }
      if(tmp.expiry == 0){
        // only deal with the packet that is not marked expiry
        long now_ms = get_timestamp();
        if(now_ms - tmp.timestamp_ms > 10000){
          // mark the expiry entry
          print_flag = 1;
          nat_table[i].expiry = 1;
          continue;
        }
      }
      if((tmp.internal_ip == ip && tmp.internal_port == port) && tmp.expiry == 0){
        // do thing
        result = 1;
        // update timestamp
        nat_table[i].timestamp_ms = get_timestamp();
        // memcpy the entry
        memcpy(entry_ptr, &nat_table[i], sizeof(struct nat_entry));
      }
    }
  }
  else{
    // it is inbound
    for(int i = 0 ; i < table_size ; i++){
      struct nat_entry tmp = nat_table[i];
      if(tmp.expiry == 1){
        continue;
      }
      if(tmp.expiry == 0){
        // only deal with the packet that is not marked expiry
        long now_ms = get_timestamp();
        if(now_ms - tmp.timestamp_ms > 10000){
          // mark the expiry entry
          print_flag = 1;
          nat_table[i].expiry = 1;
          continue;
        }
      }
      if(tmp.translated_port == port && tmp.expiry == 0){
        // do thing
        result = 1;
        nat_table[i].timestamp_ms = get_timestamp();
        memcpy(entry_ptr, &nat_table[i], sizeof(struct nat_entry));
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

  // put the packet into the queue
  int drop_flag = 1;
  pthread_mutex_lock(&packet_buf_lock);
  printf("Packet queue: %d -> ", packet_queue->size);
  if(enqueue(packet_queue, (unsigned char *)pkt) == 1){
    drop_flag = 0;
  }
  printf("%d\n", packet_queue->size);
  if(drop_flag == 1){
    printf("packet dropped\n");
  }
  pthread_mutex_unlock(&packet_buf_lock);
  if(drop_flag == 1){
    return nfq_set_verdict(myQueue, id, NF_DROP, ip_pkt_len, pktData);
  }
  else{
    return 1;
  }
}

void *pthread_prog(void *argv){
  struct nfq_q_handle *myQueue = (struct nfq_q_handle *) argv;
  // this thread program is used for process packet in buffer
  while(1){
    struct nfq_data* pkt = (struct nfq_data*)malloc(BUF_SIZE);

    while(1){
      // loop until dequeue success
      int dequeue_result;
      pthread_mutex_lock(&packet_buf_lock);
      dequeue_result = dequeue(packet_queue, (unsigned char*)pkt);
      pthread_mutex_unlock(&packet_buf_lock);
      if(dequeue_result == 0){
        my_sleep(100000);
      }
      else{
        printf("dequeue!!!\n");
        break;
      }
    }

    // Get the id in the queue
    unsigned int id = 0;
    struct nfqnl_msg_packet_hdr *header;
    if (header = nfq_get_msg_packet_hdr((nfq_data *)pkt)){
      id = ntohl(header->packet_id);
    } 

    // Access IP Packet
    unsigned char *pktData;
    int ip_pkt_len = nfq_get_payload((nfq_data *)pkt, &pktData);
    struct iphdr *ipHeader = (struct iphdr *)pktData;
    
    struct nat_entry target;
    if (ipHeader->protocol == IPPROTO_UDP) {
      // Access UDP Packet
      struct udphdr *udph = (struct udphdr *) (((char*)ipHeader) + ipHeader->ihl*4);;
      if((ipHeader->saddr & LOCAL_MASK) == LAN){
        // outbound traffic
        int search_flag = 0;
        int create_flag = 0;
        search_flag = nat_search_entry(1, ipHeader->saddr, udph->source, &target);
        if(search_flag == 0){
          create_flag = nat_create_entry(ipHeader->saddr, udph->source, &target);
        }
        if(search_flag == 1 || create_flag == 1){
          ipHeader->saddr = IP;
          udph->source = target.translated_port;
          udph->check = udp_checksum(pktData);
          ipHeader->check = ip_checksum(pktData);
          transmit_pkt(myQueue, id, ip_pkt_len, pktData);
        }
        else{
          nfq_set_verdict(myQueue, id, NF_DROP, ip_pkt_len, pktData);
        }
      }
      else{
        // inbound traffic
        if(nat_search_entry(0, IP, udph->dest, &target) == 1){
          // record is found
          ipHeader->daddr = target.internal_ip;
          udph->dest = target.internal_port;
          udph->check = udp_checksum(pktData);
          ipHeader->check = ip_checksum(pktData);
          transmit_pkt(myQueue, id, ip_pkt_len, pktData);
          }
        else{
          nfq_set_verdict(myQueue, id, NF_DROP, ip_pkt_len, pktData);
        }
      }
    }
    free(pkt);
  }
  pthread_exit(NULL);
}

void *bucket_prog(void* argv){
  // this thread program is used for bucket counter;
  TOKENS = BUCKET_SIZE;
  while (1) {
    pthread_mutex_lock(&bucket_lock);
    TOKENS = TOKENS + FILL_RATE;
    if(TOKENS > BUCKET_SIZE){
      TOKENS = BUCKET_SIZE;
    }
    pthread_mutex_unlock(&bucket_lock);
    sleep(1);
  }
  pthread_exit(NULL);
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

  pthread_t worker, bucket;
  pthread_mutex_init(&bucket_lock, 0);
  pthread_create(&bucket, NULL, bucket_prog, NULL);
  pthread_detach(bucket);
  packet_queue = createQueue(10);
  pthread_mutex_init(&packet_buf_lock, 0);
  pthread_create(&worker, NULL, pthread_prog, (void *)nfQueue);
  pthread_detach(worker);

  printf("Start to handle packets\n");
  while ((res = recv(fd, buf, sizeof(buf), 0)) && res >= 0) {
    // printf("received!\n");
    nfq_handle_packet(nfqHandle, buf, res);
  }

  nfq_destroy_queue(nfQueue);
  nfq_close(nfqHandle);

}
