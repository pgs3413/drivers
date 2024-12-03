#include "net.h"

void user_frame(char *frame, int size);
void arp_frame(char *frame, int size);
void ip_frame(struct ethhdr *eth, struct iphdr *ip);

int main()
{

    netdev **devs = malloc(sizeof(void*) * MAX_DEV);
    int i = getall_netdev(devs);
    netdev *dev = choose_netdev(devs, i);

    int sockfd = get_ethsock();

    struct sockaddr_ll devaddr;
    set_devaddr(&devaddr, get_ifindex(sockfd, dev), dev);
    
    if(bind(sockfd, (struct sockaddr *)&devaddr, sizeof(struct sockaddr_ll)) < 0)
    {
        perror("bind");
        return 1;
    }

    printf("choose mode: 1.user 2.ARP 3.IP ");
    fflush(stdout);
    int mode;
    scanf("%d", &mode);
    getchar();
    if(mode < 1 || mode > 3)
    {
        printf("wrong mode!\n");
        return 1;
    }

    unsigned short proto_s = 0;

    if(mode == 0)
        proto_s = input_proto("target eth proto: ");
    
    if(mode == 2)
        proto_s = 0x0806;
    
    uint32_t saddr;
    uint32_t daddr;
    unsigned char ip_proto;
    if(mode == 3)
    {
        input_ip("src ip: ", (unsigned char *)&saddr);
        input_ip("dest ip: ", (unsigned char *)&daddr);
        ip_proto = input_ip_proto("ip protocol: ");
        proto_s = 0x0800;
    }

    char frame[ETH_FRAME_LEN];
    struct ethhdr *hdr = (struct ethhdr *)frame;
    int size;
   
    while(1)
    {
        size = recvfrom(sockfd, frame, ETH_FRAME_LEN, 0, NULL, NULL);
        if(size < 0)
        {
            perror("recvfrom");
            return 1;
        }

        if(ntohs(hdr->h_proto) != proto_s)
            continue;

       if(proto_s == 0x0806)
       {
            arp_frame(frame, size);
       } 
       else if(proto_s == 0x0800)
       {
            struct iphdr *ip = (struct iphdr *)(frame + sizeof(struct ethhdr));
            if(ip->saddr != saddr || ip->daddr != daddr || ip->protocol != ip_proto)
                continue;
            ip_frame(hdr, ip);
       }
       else
       {
            user_frame(frame, size);
       }
    }

    return 0;
}

void user_frame(char *frame, int size)
{
    struct ethhdr *hdr = (struct ethhdr *)frame;
    char *data = frame + sizeof(struct ethhdr);

    printf("user defined frame:\n");

    char macbuf[20];

    convert_mac(macbuf, hdr->h_source);
    printf("  src: %s\n", macbuf);

    convert_mac(macbuf, hdr->h_dest);
    printf("  dest: %s\n", macbuf);

    printf("  type: 0x%4x\n", ntohs(hdr->h_proto));

    frame[size] = '\0';
    printf("  data: %s\n", data);
}

void arp_frame(char *frame, int size)
{
    struct ethhdr *eth = (struct ethhdr *)frame;
    struct arphdr *arp =(struct arphdr *)(frame + sizeof(struct ethhdr));
    unsigned char *sender_mac = (unsigned char *)arp + sizeof(struct arphdr);
    unsigned char *sender_ip = sender_mac + 6;
    unsigned char *dest_mac = sender_ip + 4;
    unsigned char *dest_ip = dest_mac + 6;

    printf("arp frame:\n");

    char macbuf[20];
    char ipbuf[INET_ADDRSTRLEN];

    convert_mac(macbuf, eth->h_source);
    printf("  src: %s\n", macbuf);
    convert_mac(macbuf, eth->h_dest);
    printf("  dest: %s\n", macbuf);

    char *op = "none";
    if(ntohs(arp->ar_op) == 1)
        op = "request";
    if(ntohs(arp->ar_op) == 2)
        op = "reply";
    printf("  op: %s\n", op);

    convert_mac(macbuf, sender_mac);
    printf("  sender mac: %s\n", macbuf);
    convert_ip(ipbuf, sender_ip);
    printf("  sender ip: %s\n", ipbuf);
    convert_mac(macbuf, dest_mac);
    printf("  dest mac: %s\n", macbuf);
    convert_ip(ipbuf, dest_ip);
    printf("  dest ip: %s\n", ipbuf);
}

void ip_frame(struct ethhdr *eth, struct iphdr *ip)
{
    printf("ip frame:\n");

    char macbuf[20];
    char ipbuf[INET_ADDRSTRLEN];

    convert_mac(macbuf, eth->h_source);
    printf("  mac src: %s\n", macbuf);
    convert_mac(macbuf, eth->h_dest);
    printf("  mac dest: %s\n", macbuf);

    printf("  ttl: %d\n", ip->ttl);
    printf("  protocol: %d\n", ip->protocol);
    convert_ip(ipbuf, ( unsigned char *)&ip->saddr);
    printf("  src ip: %s\n", ipbuf);
    convert_ip(ipbuf, ( unsigned char *)&ip->daddr);
    printf("  dest ip: %s\n", ipbuf);

}