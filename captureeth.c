#include "eth.h"

void user_frame(char *frame, int size);
void arp_frame(char *frame, int size);

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

    unsigned short proto_s = input_proto("target eth proto: ");

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