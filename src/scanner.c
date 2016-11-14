/*
  @author: Ben Mixon-Baca
  @email: bmixonb1@cs.unm.edu
 */
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pcap.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "scanner.h"

#include "sniffer.h"
#include "packet.h"
#include "worker.h"
#include "util.h"
#include "dtable.h"

static struct scanner_t *scanner = NULL;

void *sniffer_function(void * args)
{
  sniffer_t *sniffer = scanner->sniffer;
  int run = 1;
  struct pcap_pkthdr header;/* The header that pcap gives us */
  const u_char *packet;/* The actual packet */

  while ( run ) {
    packet = pcap_next(sniffer->cap_handle, &header);
    pthread_mutex_lock(sniffer->lock);
    if ( !sniffer->sniff ) {
      
    }
    pthread_mutex_unlock(sniffer->lock);
    
  }
  
  
  return NULL;
}

void  start_sniffer() 
{

  /* if (pthread_create(scanner->workers[i].thread, NULL, */
  /* 		       find_responses, */
  /* 		       (void *)&scanner->workers[i]) < 0) { */

  if ((pthread_create(scanner->sniffer->thread, NULL,
		      sniffer_function,
		      NULL)) < 0) {
    printf("%s %d: Scanner couldn't start sniffer thread.\n", __func__, __LINE__);
    exit(-1);
  }
    
  return;
  
}

void stop_sniffer() 
{
  
  sniffer_t * sniffer = scanner->sniffer;
  pthread_mutex_lock(sniffer->lock);
  sniffer->sniff = 0;
  pthread_mutex_unlock(sniffer->lock);
  return;
}


dict_t * split_query_response()
{
  /**
   * 1. R = pcap file sniffer just saved. 
   * 2. for p in pcap_file
   * 4.    if p.src == 64.106.82.6: 
   * 5.       handle_query(p)
   * 6.    else if p.dst == 64.106.82.6:
   * 7.       handle_qresponse(p)
   * 8.    else :
   * 9.       continue
   *
   */

  dict_t *q_r = new_dict_size(QR_DICT_SIZE);
  return q_r;
    
}

void *response_replay()
{
  
  return NULL;
}

void generate_phase2_packets()
{
  /**
   * 1. Split queries and response
   * 2. Generate response replays.  
   *    
   *    For each host that responsded, take its response
   *    and reply at back to set how people respond to THESE
   *    packets. 
   * 
   * 3. 
   * 2. For query in : 
   * 3.   for response in query[response]
   * 4.      
   */

  dict_t *query_response = split_query_response();

  dict_destroy(query_response);
  return;
}



void send_scan_packet(unsigned char *restrict packet_buffer, int sockfd, 
		      scanner_worker_t *restrict worker, int probe_idx,
		      int ttl)
{
  struct sockaddr *dest_addr =
    (struct sockaddr *)worker->probe_list[probe_idx].sin;
  iphdr *iph = (iphdr *)packet_buffer;
  int len = iph->tot_len;
  int result;
  if ( worker->probe_list[probe_idx].good_csum ) {
    iph->check = csum((unsigned short *)packet_buffer,
		      iph->tot_len);
  }

  else {
    iph->check = range_random(65536, worker->random_data,
			      &result);
  }

  sendto(sockfd, packet_buffer, len, 0, dest_addr, 
	 sizeof(struct sockaddr));

  return ;
}

const char* get_proto(iphdr *ip){
 switch(ip->protocol){
 case IPPROTO_TCP:
   return "TCP";
 case IPPROTO_ICMP:
   return "ICMP";
 case IPPROTO_UDP:
   return "UDP";
 }
 return "Other";
}

void
send_phase1_packet(unsigned char *restrict packet_buffer, 
		   scanner_worker_t *restrict worker, int probe_idx,
		   int sockfd)
{
  struct sockaddr *dest_addr =
    (struct sockaddr *)worker->probe_list[probe_idx].sin;
  iphdr *iph = (iphdr *)packet_buffer;
  int len = iph->tot_len;

  int ret = sendto(sockfd, packet_buffer, len, 0, dest_addr, 
		   sizeof(struct sockaddr));

  int localerror = errno;

  if (localerror == EINVAL) {
    printf("FOO: %d %s %d %d %s %d %s\n",
	   __LINE__,__func__, ret, errno,
	   strerror(errno), len, get_proto(iph));
  }
  else if (localerror == EMSGSIZE){
    printf("BAR: %d %s %d %d %s %d %s\n",
	   __LINE__,__func__, ret, errno,
	   strerror(errno), len, get_proto(iph));
  }
  else {
    printf("BAZ: %d %s %d %d %s %d %s\n",
	   __LINE__,__func__, ret, errno, 
	   strerror(errno), len, get_proto(iph));
  }
  return ;
}


void phase1(scanner_worker_t *self)
{
  for (int i = 0; i < ADDRS_PER_WORKER; i++) {
    make_phase1_packet((unsigned char *)
		       &self->probe_list[i].probe_buff,
		       self, i);
  }

  for (int probe_idx = 0;
       probe_idx < ADDRS_PER_WORKER; probe_idx++) {
    send_phase1_packet((unsigned char *)
		       &self->probe_list[probe_idx].probe_buff,
		       self, probe_idx, self->ssocket->sockfd);
    sleep(1);
  }  
  pthread_mutex_lock(self->scanner->phase1_lock);
  self->scanner->phase1 += 1;
  pthread_cond_signal(self->scanner->phase1_cond);
  pthread_mutex_unlock(self->scanner->phase1_lock);

  printf("%d %s %d\n",__LINE__,__func__, ADDRS_PER_WORKER);

  return ;
}

void phase2(scanner_worker_t *self)
{
  pthread_mutex_lock(self->scanner->phase2_lock);
  self->scanner->phase2 += 1;
  pthread_cond_signal(self->scanner->phase2_cond);
  pthread_mutex_unlock(self->scanner->phase2_lock);
  return;
}


 void phase2_wait(scanner_worker_t *self)
{

  pthread_mutex_lock(self->scanner->phase2_wait_lock);
  while( self->scanner->phase2_wait ) {
    pthread_cond_wait(self->scanner->phase2_wait_cond,
		      self->scanner->phase2_wait_lock);
  }
  pthread_mutex_unlock(self->scanner->phase2_wait_lock);

  return;
}

/**
 * @param vself: Generic pointer to a scanner_worker_t.
 * @return: Always return
 *
 * Overview:
 * 
 * 1. Generates packets based on my randomized
 * algorithm for setting fields.
 * 
 * 2. Send packets from (1).
 * 
 * 3. Once finished, find packets that illicited a response.
 * 
 */
void *find_responses(void *vself)
{
  scanner_worker_t *self = vself;
  phase1(self);

  phase2_wait(self);
  phase2(self);

  printf("DONE\n");
  return NULL;
}


/**
 * This is the worker routine that generates packets with varying 
 * fields. Spins up a sniffer thread with with appropriate 
 * pcap filter and sends the pcap off with modulated TTL.
 * 
 * @param: vself. A void pointer to the worker that is actually
 * sending of packets.
 *
 * @return: Always returns NULL.
 */
void *worker_routine(void *vself)
{
  printf("%d %s ",__LINE__, __func__);
  scanner_worker_t *self = vself;
  int scanning = 1;
  // Probably change this so we can make a list of ipaddresses.
  int sockfd = self->ssocket->sockfd;
  double start_time;
  START_TIMER(start_time);
  double end_time;
  while ( scanning ) {
    START_TIMER(end_time);
    if (end_time - start_time > SCAN_DURATION){
      break;
    }
    for (int i = 0; i < ADDRS_PER_WORKER; i++) {
      make_packet ((unsigned char *)&self->probe_list[i].probe_buff,
		   self, i);
    }

    int ttl = START_TTL;
    self->current_ttl = START_TTL;
    int probe_idx = self->probe_idx;
    for (int j = 0; j < TTL_MODULATION_COUNT; j++) {
      ttl = START_TTL;
      while ( self->current_ttl < END_TTL ) {
	if (probe_idx == ADDRS_PER_WORKER) {
	  ttl++;
	  self->current_ttl = ttl;
	  probe_idx = 0;
	}
	send_scan_packet((unsigned char *)
			 &self->probe_list[probe_idx].probe_buff,
			 sockfd, self, probe_idx, ttl);
	probe_idx += 1;
      }
      self->probe_idx = 0;
    }
  }
  printf("Done scanning. Total scan time %f sec\n",
	 (end_time - start_time));
  return NULL;
}

/**
 * Main loop for the scanner code. ''main" calls this function.
 */
int scanner_main_loop()
{
  new_scanner_singleton();
  pthread_mutex_lock(scanner->continue_lock);
  pthread_mutex_lock(scanner->phase1_lock);
  pthread_mutex_lock(scanner->phase2_lock);
  pthread_mutex_lock(scanner->phase2_wait_lock);

  start_sniffer();
  for (int i = 0; i < MAX_WORKERS; i++) {
    if (pthread_create(scanner->workers[i].thread, NULL,
		       find_responses,
		       (void *)&scanner->workers[i]) < 0) {
      printf("Couldn't initialize thread for worker[%d]\n", i);
      exit(-1);
    }
  }
  

  while(scanner->phase1 < MAX_WORKERS) {
    pthread_cond_wait(scanner->phase1_cond, scanner->phase1_lock);
  }
  /** 
   * By this point, all of the workers should have 
   * finished sending all of their phase 1 probes.
   */
  pthread_mutex_unlock(scanner->phase1_lock);
  
  /* 
   * We should wait 1 minute before signalling the 
   * sniffer to pause sniffing.
   */
  sleep(60);

  stop_sniffer();

  generate_phase2_packets();

  
  scanner->phase2_wait = 0;
  pthread_cond_signal(scanner->phase2_wait_cond);
  pthread_mutex_unlock(scanner->phase2_wait_lock);

  while(scanner->phase2 < MAX_WORKERS) {
    pthread_cond_wait(scanner->phase2_cond, scanner->phase2_lock);
  }
  pthread_mutex_unlock(scanner->phase2_lock);

  delete_scanner(scanner);
  free(scanner);
  scanner = NULL;
  return 0;
}

/**
 * Creates a new scanner_worker_t and initializes all of its fields.
 * @param: worker. Pointer to the scanner_worker_t to be initialized
 * @param: id. And int that is the worker identifier.
 * 
 * @return: 0 on succes. -1 on failure with an error message printed
 * to the screen..
 */
int new_worker(scanner_worker_t *worker, int id)
{
  printf("%d %s \n", __LINE__, __func__);
  worker->ssocket = malloc(sizeof(scanner_socket_t));
  if ((long)worker->ssocket == -1) {
    printf("Couldn't allocate scanner_socket_t for worker[%d]\n", id);
    return -1;
  }

  worker->ssocket->sockfd = socket(AF_INET, SOCK_RAW,
				   IPPROTO_RAW);
  if (worker->ssocket->sockfd < 0) {
    printf("Couldn't open socket fd for worker[%d]\n", id);
    return -1;
  };

  if (setsockopt(worker->ssocket->sockfd, SOL_SOCKET, SO_BINDTODEVICE,
		 CAPTURE_INTERFACE, strlen(CAPTURE_INTERFACE)) ) {
    printf("getsockopt() for worker[%d]\n", id);
    return -1;
  }
  
  worker->thread = malloc(sizeof(pthread_t));
  if ((long)worker->thread == -1) {
    printf("Couldn't allocate thread for worker[%d]\n", id);
    return -1;
  }

  worker->random_data = malloc(sizeof(struct random_data));
  if ((long)worker->random_data == -1) {
    printf("Couldn't allocate random_data storage for worker[%d]\n", 
	   id);
    return -1;
  }

  worker->state_size = STATE_SIZE;
  worker->random_state = malloc(STATE_SIZE);
  if ((long)worker->random_state == -1) {
    printf("Couldn't allocate random_state storage for worker[%d]\n",
	   id);
    return -1;
  }  

  if (initstate_r(TEST_SEED, worker->random_state, STATE_SIZE,
		  worker->random_data) < 0) {
    printf("Couldn't initialize random_state for worker[%d]'s.\n",
	   id);
    return -1;
  }
  
  worker->probe_list = malloc(sizeof(probe_t) * ADDRS_PER_WORKER);
  if (worker->probe_list == NULL) {
    printf("Couldn't allocate space for "
	   "address list for worker[%d]\n", id);
    return -1;
  }

  for (int i = 0; i < ADDRS_PER_WORKER; i++) {
    worker->probe_list[i].sin = malloc(sizeof(struct sockaddr_in));
    if (worker->probe_list[i].sin == NULL) {
      printf("Cannot allocate space for probe sockaddr_in for "
	     "worker[%d]\n", id);
      return -1;
    }
  }
  printf("%d %s \n", __LINE__, __func__);
  double time = wall_time();
  srandom_r((long)time, worker->random_data);
  printf("%d %s \n", __LINE__, __func__);
  worker->worker_id = id;
  worker->probe_idx = 0;
  worker->current_ttl = START_TTL;
  printf("%d %s FUCK\n", __LINE__, __func__);
  return id;
}

void init_conds()
{

  scanner->continue_cond = malloc(sizeof(pthread_cond_t));
  if ((long)scanner->continue_cond == -1) {
    exit(-1);
  }
  pthread_cond_init(scanner->continue_cond, NULL);

  scanner->phase1_cond = malloc(sizeof(pthread_cond_t));
  if ((long)scanner->phase1_cond == -1) {
    exit(-1);
  }
  pthread_cond_init(scanner->phase1_cond, NULL);

  scanner->phase2_cond = malloc(sizeof(pthread_cond_t));
  if ((long)scanner->phase2_cond == -1) {
    exit(-1);
  }
  pthread_cond_init(scanner->phase2_cond, NULL);


  scanner->phase2_wait_cond = malloc(sizeof(pthread_cond_t));
  if ((long)scanner->phase2_wait_cond == -1) {
    exit(-1);
  }
  pthread_cond_init(scanner->phase2_wait_cond, NULL);

  return;
}

void init_locks()
{
  scanner->continue_lock = malloc(sizeof(pthread_mutex_t));
  if ((long)scanner->continue_lock == -1) {
    exit(-1);
  }
  pthread_mutex_init(scanner->continue_lock, NULL);

  scanner->phase1_lock = malloc(sizeof(pthread_mutex_t));
  if ((long)scanner->phase1_lock == -1) {
    exit(-1);
  }
  pthread_mutex_init(scanner->phase1_lock, NULL);

  scanner->phase2_lock = malloc(sizeof(pthread_mutex_t));
  if ((long)scanner->phase2_lock == -1) {
    exit(-1);
  }
  pthread_mutex_init(scanner->phase2_lock, NULL);

  scanner->phase2_wait_lock = malloc(sizeof(pthread_mutex_t));
  if ((long)scanner->phase2_wait_lock == -1) {
    exit(-1);
  }
  pthread_mutex_init(scanner->phase2_wait_lock, NULL);

  return;
}


void init_sniffer()
{
  scanner->sniffer = malloc(sizeof(sniffer_t));
  if (scanner->sniffer == NULL) {
    printf("%s %d: scannercouldn't allocate sniffer\n",
	   __func__, __LINE__);
    exit(-1);
  }

  scanner->sniffer->thread = malloc(sizeof(pthread_t));
  if (scanner->sniffer->thread == NULL) {
    printf("%s %d scanner couldn't allocate sniffer thread\n",
	   __func__, __LINE__);
    exit(-1);
  }

  scanner->sniffer->lock = malloc(sizeof(pthread_mutex_t));
  if (scanner->sniffer->lock == NULL) {
    printf("%s %d scanner couldn't allocate sniffer lock\n",
	   __func__, __LINE__);
    exit(-1);
  }
  pthread_mutex_init(scanner->sniffer->lock, NULL);

  scanner->sniffer->cond = malloc(sizeof(pthread_cond_t));
  if (scanner->sniffer->cond == NULL) {
    printf("%s %d scanner couldn't allocate sniffer condition "
	   "variable\n", __func__, __LINE__);
    exit(-1);
  }
  pthread_cond_init(scanner->sniffer->cond, NULL);

  /* scanner->sniffer->cap_handle = malloc(sizeof(pcap_t)); */
  /* if (scanner->sniffer->cap_handle == NULL) { */
  /*   printf("%s %d scanner couldn't allocate sniffer capture handle\n", */
  /* 	   __func__, __LINE__); */
  /*   exit(-1); */
  /* } */
  scanner->sniffer->sniff = 1;
  return;  
}

/** 
 * Either build a scanner singleton or create a completely new one
 *  if we have already built on in the past. This is simply an 
 *  interface
 *  to get at the statically declared one.
 */
scanner_t *new_scanner_singleton()
{
  if ( scanner ) {
    return scanner;
  }
  scanner = malloc(sizeof(scanner_t));
  scanner->keep_scanning = 1;
  scanner->phase1 = 0;
  scanner->phase2 = 0;
  scanner->phase2_wait = 1;
  scanner->workers = malloc(sizeof(scanner_worker_t) * MAX_WORKERS);

  for (int i = 0 ; i < MAX_WORKERS; i++) {
    if (new_worker(&scanner->workers[i], i) != i) {
      exit(-1);
    }
    scanner->workers[i].scanner = scanner;
  }

  init_sniffer();
  
  init_locks();
  
  init_conds();

  return scanner;
}

void delete_conds()
{
  free(scanner->continue_cond);
  free(scanner->phase1_cond);
  free(scanner->phase2_cond);
  free(scanner->phase2_wait_cond);
  return;
}

void  delete_locks()
{ 
  free(scanner->continue_lock);
  free(scanner->phase1_lock);
  free(scanner->phase2_lock);
  free(scanner->phase2_wait_lock);

  return;
}

void delete_sniffer()
{
  free(scanner->sniffer->thread);
  scanner->sniffer->thread = NULL;

  free(scanner->sniffer->cap_handle);
  scanner->sniffer->cap_handle = NULL;

  free(scanner->sniffer->lock);
  scanner->sniffer->lock = NULL;
  
  free(scanner->sniffer->cond);
  scanner->sniffer->cond = NULL;

  free(scanner->sniffer);
  scanner->sniffer = NULL;
  return;
}

void delete_workers(scanner_worker_t *worker)
{

  close(worker->ssocket->sockfd);
  free(worker->ssocket);
  worker->ssocket = NULL;

  free(worker->thread);
  worker->thread = NULL;

  free(worker->random_data);
  worker->random_data = NULL;  

  free(worker->random_state);
  worker->random_state = NULL;

  for (int i = 0; i < ADDRS_PER_WORKER; i++) {
    free(worker->probe_list[i].sin);
  }
  free(worker->probe_list);
  worker->probe_list = NULL;
  return;
}

void delete_scanner()
{
  delete_conds();
  delete_locks();
  for(int i = 0; i < MAX_WORKERS; i++) {
    delete_workers(&(scanner->workers[i]));
  }
  free(scanner->workers);
  scanner->workers = NULL;
  return ;
}