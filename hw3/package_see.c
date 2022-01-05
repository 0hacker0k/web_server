#include <stdio.h>
#include <pcap/pcap.h>
#include <arpa/inet.h>
#define ETHER_ADDR_LEN	6
#define SIZE_ETHERNET 14
typedef u_int tcp_seq;
#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
struct sniff_ethernet { 
	u_char ether_dhost[ETHER_ADDR_LEN]; /* 目標主機地址 */ 
	u_char ether_shost[ETHER_ADDR_LEN]; /* 源主機地址 */ 
	u_short ether_type; /* IP? ARP？RARP？等 */ 
};
struct sniff_ip {
	u_char ip_vhl;		/* version << 4 | header length >> 2 */
	u_char ip_tos;		/* type of service */
	u_short ip_len;		/* total length */
	u_short ip_id;		/* identification */
	u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* don't fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	u_char ip_ttl;		/* time to live */
	u_char ip_p;		/* protocol */
	u_short ip_sum;		/* checksum */
	struct in_addr ip_src,ip_dst; /* source and dest address */
};
struct sniff_tcp {
	u_short th_sport;	/* source port */
	u_short th_dport;	/* destination port */
	tcp_seq th_seq;		/* sequence number */
	tcp_seq th_ack;		/* acknowledgement number */
	u_char th_offx2;	/* data offset, rsvd */
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) > 4)
	u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
	u_short th_win;		/* window */
	u_short th_sum;		/* checksum */
	u_short th_urp;		/* urgent pointer */
};
/* UDP header */
struct sniff_udp {
 u_short uh_sport;  /* source port */
 u_short uh_dport;  /* destination port */
 u_short uh_ulen;  /* udp length */
 u_short uh_sum;   /* udp checksum */
};
void sec_to_time(int time,int *year,int *month,int *day,int *hour,int *min,int *sec){
	time+=8*3600;
	*day=time/86400+1;
	*year=1970;
	*month=1;
	while((*day>366 || (*day>365 && ((*year%4==0 && *year%100!=0) || *year%400==0)))){
		*day=*day-(365+(*year%4==0 && *year%100!=0 || *year%400==0));
		(*year)++;
	}
	if(*day>31){
		*day-=31;
		*month+=1;
	}
	if(*day>28){
		if(*day==29 && (*year%4==0 && *year%100!=0 || *year%400==0)){
			*day=29;
			*month=2;
		}else{
			*day-=28;
			*month+=1;
		}
	}
	if(*day>31){
		*day-=31;
		*month+=1;
	}
	if(*day>30){
		*day-=30;
		*month+=1;
	}
	if(*day>31){
		*day-=31;
		*month+=1;
	}
	if(*day>30){//6
		*day-=30;
		*month+=1;
	}
	if(*day>31){
		*day-=31;
		*month+=1;
	}
	if(*day>31){
		*day-=31;
		*month+=1;
	}
	if(*day>30){
		*day-=30;
		*month+=1;
	}
	if(*day>31){
		*day-=31;
		*month+=1;
	}
	if(*day>30){
		*day-=30;
		*month+=1;
	}
	if(*day>31){
		*day-=31;
		*month+=1;
	}
	time%=86400;
	*hour=time/3600;
	*min=time%3600/60;
	*sec=time%3600%60;
}
int all_count=0;
void callback(u_char *args, const struct pcap_pkthdr *header,const u_char *packet){
	all_count++;
	int time=header->ts.tv_sec;
	int day;
	int year;
	int month;
	int sec;
	int hour;
	int min;
	sec_to_time(time,&year,&month,&day,&hour,&min,&sec);
	printf("-------------------------------------------\n");
	printf("%d\t┐\n",all_count);
	printf("\t├Time: %d/%d/%d %d:%d:%d\n",year,month,day,hour,min,sec);
	const struct sniff_ethernet *ethernet = (struct sniff_ethernet*)(packet);
	printf("\t├Src mac:");
	for(int i=0;i<=4;i++){
		printf("%02x:",ethernet->ether_shost[i]);
	}printf("%02x\n",ethernet->ether_shost[5]);
	printf("\t├Dst mac:");
	for(int i=0;i<=4;i++){
		printf("%02x:",ethernet->ether_dhost[i]);
	}printf("%02x\n",ethernet->ether_dhost[5]);
	int type=htons(ethernet->ether_type);
	printf("\t└");
	int x=1;
	if(type==2048){
		printf("IPv4");
	}else if(type==2054){
		x=0;
		printf("ARP");
	}else if(type==34525){
		printf("IPv6");
	}else{
		x=0;
		printf("unknown");
	}
	if(x==0){
		printf("\n");
	}else{
		printf("\t┐\n");
	}
	if(type==2048 || type==34525){
		const struct sniff_ip *ip=(struct sniff_ip*)(packet+SIZE_ETHERNET);
		int size_ip = IP_HL(ip)*4;
		char src_ip[100],dst_ip[100];
		long temp=ip->ip_src.s_addr;
		long temp2=ip->ip_dst.s_addr;
		inet_ntop(AF_INET,(void*)&(temp), src_ip, 16);
		inet_ntop(AF_INET,(void*)&(temp2), dst_ip, 16);
		printf("\t\t├ID:%d\n",htons(ip->ip_id));
		printf("\t\t├Src:%s\n\t\t├Dst:%s\n",src_ip,dst_ip);
		printf("\t\t├Total Length:%d\n",htons(ip->ip_len));
		printf("\t\t├Time to Live:%d\n",ip->ip_ttl);
		printf("\t\t└");
		x=1;
		if(ip->ip_p==17){//UDP
			struct sniff_udp* udp = (struct sniff_udp*)(packet + SIZE_ETHERNET + size_ip);
			printf("Type: UDP\t┐\n");
			printf("\t\t\t\t├Src port:%d\n\t\t\t\t└Dst port:%d\n",htons(udp->uh_sport),htons(udp->uh_dport));
		}else if(ip->ip_p==6){//TCP
			struct sniff_tcp* tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
			printf("Type: TCP\t┐\n");
			printf("\t\t\t\t├Src port:%d\n\t\t\t\t└Dst port:%d\n",htons(tcp->th_sport),htons(tcp->th_dport));
		}else if(ip->ip_p==1){
			printf("Type: ICMP\n");
		}else{
			printf("Type: unknown (%d)\n",ip->ip_p);
		}
	}
	printf("-------------------------------------------\n");
	
}
int main(int argc, char *argv[]){
	pcap_t *handle;			/* Session handle */
	char *dev;			/* The device to sniff on */
	char errbuf[PCAP_ERRBUF_SIZE];	/* Error string */
	struct bpf_program fp;		/* The compiled filter */
	char filter_exp[] = "port 23";	/* The filter expression */
	bpf_u_int32 mask;		/* Our netmask */
	bpf_u_int32 net;		/* Our IP */
	struct pcap_pkthdr header;	/* The header that pcap gives us */
	const u_char *packet;		/* The actual packet */
	/* Open the session in promiscuous mode */
	handle = pcap_open_offline(argv[1],errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return(2);
	}
	int test=pcap_loop(handle,2147483647,callback, NULL);
	if(test==-1){
		printf("error\n");
	}
	/* Print its length */
	//printf("Jacked a packet with length of [%d]\n", header.len);
	pcap_close(handle);
	/* And close the session */
	return(0);
}
