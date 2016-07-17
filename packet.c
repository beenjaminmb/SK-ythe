/*
  @author: Ben Mixon-Baca
  @email: bmixonb1@cs.unm.edu
 */
#include "packet.h"
#include "util.h"
unsigned short csum(unsigned short *ptr, int nbytes)
{
  register long sum;
  unsigned short oddbyte;
  register short answer;
 
  sum=0;
  while(nbytes>1) {
    sum+=*ptr++;
    nbytes-=2;
  }
  if(nbytes==1) {
    oddbyte=0;
    *((u_char*)&oddbyte)=*(u_char*)ptr;
    sum+=oddbyte;
  }
  sum = (sum>>16)+(sum & 0xffff);
  sum = sum + (sum>>16);
  answer=(short)~sum;
  return(answer);
}

pseudo_header *make_pseudo_header(char *source_ip,
				  struct sockaddr_in *sin,
				  int total_length) 
{
  pseudo_header *psh = malloc(sizeof(pseudo_header));
  psh->source_address = inet_addr( source_ip );
  psh->dest_address = sin->sin_addr.s_addr;
  psh->placeholder = 0;
  psh->protocol = IPPROTO_UDP;
  /*
    UDP: total_length = htons(sizeof(udphdr) + datalen )
    TCP: total_length = htons(sizeof(tcphdr) + datalen )
   */
  psh->total_length = total_length;
  return psh;
}

udphdr *make_udpheader(unsigned char *buffer, int datalen)
{
  udphdr *udph = (udphdr *)buffer;
  udph->source = htons (6666);
  udph->dest = htons (8622);
  udph->len = htons(8 + datalen);
  udph->check = 0;
  return udph;
}

tcphdr *make_tcpheader(unsigned char *buffer)
{
  struct tcphdr *tcph = (tcphdr *)(buffer + sizeof(struct ip));
  tcph->source = htons(1234);
  tcph->dest = htons(80);
  tcph->seq = 0;
  tcph->ack_seq = 0;
  tcph->doff = 5;
  tcph->fin=0;
  tcph->syn=1;
  tcph->rst=0;
  tcph->psh=0;
  tcph->ack=0;
  tcph->urg=0;
  tcph->window = htons (5840);
  tcph->check = 0;
  tcph->urg_ptr = 0;
  return tcph;
}

iphdr *make_ipheader(char *buffer, struct sockaddr_in *sin, 
		     char *source_ip, int datalen) 
{
  iphdr *iph = (iphdr *)buffer;
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = sizeof(iphdr) + sizeof(tcphdr) + datalen;
  iph->id = htonl(1);
  iph->frag_off = 0;
  iph->ttl = START_TTL;
  iph->protocol = IPPROTO_TCP; // fix this later
  iph->check = 0;
  iph->saddr = inet_addr( source_ip );
  iph->daddr = sin->sin_addr.s_addr;
  iph->check = csum((unsigned short *)buffer, iph->tot_len);
  return iph;
}

/*
While (1)
    Create a random IPv4 packet
    For length, checksum, etc. make it correct 90% of the time,
    incorrect in a random way 10%

    For protocol, TCP 50% of the time, UDP 25%, random 25%
    Dest address random, source address our own (not bound to an
    interface)

    10% of the time have a randomly generated options field
    
    if (TCP) fill in TCP header as below
    
    if (UDP or other) fill in rest of IP packet with random junk
       Send it in a TTL-limited fashion

    for TCP packets:
    50% chance of random dest port, 50% chance of common (22, 80, 
    etc.)
  
    seq and ack numbers random
    Flags random but biased towards usually only 1 or 2 bits set
    with 10% chance add randomly generated options
    Reserved 0 50% of the time and random 50% of the time
    Window random
    Checksum correct 90% of the time, random 10%
    Urgent pointer not there 90% of the time, random 10% of the time

Then we just send out this kind of traffic at 1 Gbps or so and take a
pcap of all outgoing and incoming packets for that source IP, and 
store that on the NFS mount for later analysis
*/
int make_packet(unsigned char *packet_buffer, 
		scanner_worker_t *worker,
		int datalen)
{
  int make_tcp = 0;
  int result = 0;
  long prand = range_random(100, worker->random_data, &result);
  pseudo_header *psh = malloc(sizeof(pseudo_header));
  char *pseudogram, source_ip[32];
  strcpy(source_ip , TEST_IP);

  memset(packet_buffer, 0, PACKET_LEN);
  iphdr *ip = make_ipheader(packet_buffer,
			    worker->sin,
			    TEST_IP,
			    datalen);

  if ( DO_TCP(prand) ) { /*Make a TCP packet*/
    make_tcpheader(packet_buffer);
    tcphdr *tcph = (tcphdr *)(packet_buffer + sizeof(struct ip));
    psh->source_address = inet_addr( source_ip );
    psh->dest_address = worker->sin->sin_addr.s_addr;
    psh->placeholder = 0;
    psh->protocol = IPPROTO_TCP;
    psh->total_length = htons(sizeof(tcphdr) + datalen);
    
    int psize = sizeof( pseudo_header) + 
      sizeof(tcphdr) + datalen;

    pseudogram = malloc(psize);
    memcpy(pseudogram , (char*) psh , sizeof(pseudo_header));
    memcpy(pseudogram + sizeof(pseudo_header) , tcph, 
	   sizeof(tcphdr) + datalen);
    tcph->check = csum( (unsigned short*) pseudogram ,psize);
    return 0;
  }
  else { /* Make a UDP */
    
  }
  return 0;
}
