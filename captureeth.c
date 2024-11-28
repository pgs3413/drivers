#define _GNU_SOURCE

#include <ifaddrs.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>

typedef struct {
    char name[IFNAMSIZ];
    unsigned char mac[ETH_ALEN];
} netdev;

#define MAX_DEV 5

char convert_hex(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

void convert_mac(char *buf, unsigned char *mac)
{
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int main()
{

    netdev **devs = malloc(sizeof(void*) * MAX_DEV);

    struct ifaddrs *ifaddr, *ifa;
    int i;

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return 1;
    }

    for(i = 0, ifa = ifaddr; ifa != NULL && i < MAX_DEV; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr->sa_family == AF_PACKET)
        {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
            devs[i] = malloc(sizeof(netdev));
            strncpy(devs[i]->name, ifa->ifa_name, IFNAMSIZ);
            memcpy(devs[i]->mac, s->sll_addr, ETH_ALEN);
            printf("%d: %s %02x:%02x:%02x:%02x:%02x:%02x\n", i, devs[i]->name, 
                devs[i]->mac[0],  devs[i]->mac[1],  devs[i]->mac[2], 
                 devs[i]->mac[3],  devs[i]->mac[4],  devs[i]->mac[5]);
            i++;
        }
    }

    freeifaddrs(ifaddr);

    unsigned int devid;
    printf("choose an interface: ");
    fflush(stdout);
    scanf("%u", &devid);
    getchar();

    if(devid >= i)
    {
        printf("wrong interface!\n");
        return 1;
    }

    netdev *dev = devs[devid];

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sockfd == -1)
    {
        perror("socket raw");
        return 1;
    }

    struct ifreq ifindex;
    memset(&ifindex, 0, sizeof(struct ifreq));
    strcpy(ifindex.ifr_name, dev->name);
    if(ioctl(sockfd, SIOCGIFINDEX, &ifindex) < 0)
    {
        perror("ifindex");
        return 1;
    }

    struct sockaddr_ll devaddr;
    memset(&devaddr, 0, sizeof(struct sockaddr_ll));
    devaddr.sll_ifindex = ifindex.ifr_ifindex;
    devaddr.sll_family = AF_PACKET;
    devaddr.sll_protocol = htons(ETH_P_ALL);
    
    if(bind(sockfd, (struct sockaddr *)&devaddr, sizeof(struct sockaddr_ll)) < 0)
    {
        perror("bind");
        return 1;
    }

    char proto[5];
    printf("eth proto: ");
    fflush(stdout);
    scanf("%4s", proto);

    unsigned short proto_s = 0;

    for(int j = 0, k = 0; j < 4; j+=2, k++)
    {
        char a = convert_hex(proto[j]);
        char b = convert_hex(proto[j + 1]);
        if(a == -1 || b == -1)
        {
            printf("wrong eth proto!\n");
            return 1;
        }
        proto_s += ((a << 4) + b) << 8 * (k ^ 1); 
    }

    char frame[ETH_FRAME_LEN];
    struct ethhdr *hdr = (struct ethhdr *)frame;
    char *data = frame + sizeof(struct ethhdr);

    int size;
    char macbuf[20];

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

        printf("size: %d ", size);

        convert_mac(macbuf, hdr->h_source);
        printf("src: %s ", macbuf);

        convert_mac(macbuf, hdr->h_dest);
        printf("dest: %s ", macbuf);

        printf("type: 0x%4x ", ntohs(hdr->h_proto));

        frame[size] = '\0';
        printf("data: %s ", data);

        printf("\n");
    }

    return 0;
}