#ifndef ETH_H
#define ETH_H

#include <sys/socket.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>

#ifndef __USE_MISC

struct ifmap
  {
    unsigned long int mem_start;
    unsigned long int mem_end;
    unsigned short int base_addr;
    unsigned char irq;
    unsigned char dma;
    unsigned char port;
    /* 3 bytes spare */
  };

struct ifreq
  {
# define IFHWADDRLEN	6
# define IFNAMSIZ	16
    union
      {
	char ifrn_name[IFNAMSIZ];	/* Interface name, e.g. "en0".  */
      } ifr_ifrn;

    union
      {
	struct sockaddr ifru_addr;
	struct sockaddr ifru_dstaddr;
	struct sockaddr ifru_broadaddr;
	struct sockaddr ifru_netmask;
	struct sockaddr ifru_hwaddr;
	short int ifru_flags;
	int ifru_ivalue;
	int ifru_mtu;
	struct ifmap ifru_map;
	char ifru_slave[IFNAMSIZ];	/* Just fits the size */
	char ifru_newname[IFNAMSIZ];
	__caddr_t ifru_data;
      } ifr_ifru;
  };

#endif

typedef struct {
    char name[IFNAMSIZ];
    unsigned char mac[ETH_ALEN];
} netdev;

#define MAX_DEV 5

static char convert_hex(char c)
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

static int getall_netdev(netdev **devs)
{
    struct ifaddrs *ifaddr, *ifa;
    int i;

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(1);
    }

    for(i = 0, ifa = ifaddr; ifa != NULL && i < MAX_DEV; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr->sa_family == AF_PACKET)
        {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
            devs[i] = (netdev*)malloc(sizeof(netdev));
            strncpy(devs[i]->name, ifa->ifa_name, IFNAMSIZ);
            memcpy(devs[i]->mac, s->sll_addr, ETH_ALEN);
            printf("%d: %s %02x:%02x:%02x:%02x:%02x:%02x\n", i, devs[i]->name, 
                devs[i]->mac[0],  devs[i]->mac[1],  devs[i]->mac[2], 
                 devs[i]->mac[3],  devs[i]->mac[4],  devs[i]->mac[5]);
            i++;
        }
    }

    freeifaddrs(ifaddr);
    return i;
}

static netdev *choose_netdev(netdev **devs, int i)
{
    unsigned int devid;
    printf("choose an interface: ");
    fflush(stdout);
    scanf("%u", &devid);
    getchar();

    if(devid >= i)
    {
        printf("wrong interface!\n");
        exit(1);
    }

    return devs[devid];
}

static int get_ethsock()
{
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sockfd == -1)
    {
        perror("socket raw");
        exit(1);
    }
    return sockfd;
}

static int get_ifindex(int sockfd, netdev *dev)
{
    struct ifreq ifindex;
    memset(&ifindex, 0, sizeof(struct ifreq));
    strcpy(ifindex.ifr_ifrn.ifrn_name, dev->name);
    if(ioctl(sockfd, SIOCGIFINDEX, &ifindex) < 0)
    {
        perror("ifindex");
        exit(1);
    }
    return ifindex.ifr_ifru.ifru_ivalue;
}

static void set_devaddr(struct sockaddr_ll *devaddr, int ifindex, netdev *dev)
{
    memset(devaddr, 0, sizeof(struct sockaddr_ll));
    devaddr->sll_ifindex = ifindex;
    devaddr->sll_halen = ETH_ALEN;
    memcpy(devaddr->sll_addr, dev->mac, ETH_ALEN);
    devaddr->sll_family = AF_PACKET;
    devaddr->sll_protocol = htons(ETH_P_ALL);
}

static void input_macaddr(const char *msg, unsigned char *dest)
{
    char destaddr[13];
    printf("%s", msg);
    fflush(stdout);
    scanf("%12s", destaddr);

    for(int j = 0, k = 0; j < 12; j+=2, k++)
    {
        char a = convert_hex(destaddr[j]);
        char b = convert_hex(destaddr[j + 1]);
        if(a == -1 || b == -1)
        {
            printf("wrong mac address!\n");
            exit(1);
        }
        dest[k] = (a << 4) + b;
    }
}

static unsigned short input_proto(const char *msg)
{
    char proto[5];
    printf("%s", msg);
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
            exit(1);
        }
        proto_s += ((a << 4) + b) << 8 * (k ^ 1); 
    }
    return proto_s;
}

static unsigned char input_ip_proto(const char *msg)
{
    printf("%s", msg);
    fflush(stdout);
    unsigned char protocol;
    scanf("%hhu", &protocol);
    getchar();
    return protocol;
}

static void input_ip(const char *msg, unsigned char *dest)
{
    char ip[50];
    printf("%s", msg);
    fflush(stdout);
    scanf("%49s", ip);

    int ret = inet_pton(AF_INET, ip, dest);
    if(ret != 1)
    {
        printf("wrong ip.\n");
        exit(1);
    }
}

unsigned short input_port(const char *msg)
{
    printf("%s", msg);
    fflush(stdout);
    unsigned short port;
    scanf("%hu", &port);
    getchar();
    return htons(port);
}

static void convert_ip(char *buf, unsigned char *ip)
{
    if(inet_ntop(AF_INET, ip, buf, INET_ADDRSTRLEN) == NULL)
    {
        perror("ip");
        exit(1);
    }
}

// 计算 IP 头部检验和的函数
static uint16_t calculate_checksum(void *target, int length) 
{
    uint32_t sum = 0;
    uint16_t *data = (uint16_t *)target;

    // 计算所有 16 位字节的和
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }

    // 如果剩下一个字节（奇数长度），添加到和中
    if (length == 1) {
        sum += *(uint8_t *)data;
    }

    // 将结果的高16位和低16位相加（如果有溢出）
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    // 返回取反值作为最终的检验和
    return ~sum;
}

#endif