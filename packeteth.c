#include "eth.h"

int user_frame(struct ethhdr *hdr, char *data);
int arp_frame(struct ethhdr *eth, struct arphdr *arp);
int ip_frame(struct ethhdr *eth, struct iphdr *ip);

int main()
{

    netdev **devs = malloc(sizeof(void*) * MAX_DEV);
    int i = getall_netdev(devs);
    netdev *dev = choose_netdev(devs, i);

    int sockfd = get_ethsock();

    struct sockaddr_ll devaddr;
    int ifindex = get_ifindex(sockfd, dev);
    set_devaddr(&devaddr, ifindex, dev);

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

    char frame[ETH_FRAME_LEN];
    struct ethhdr *hdr = (struct ethhdr *)frame;
    char *data = frame + sizeof(struct ethhdr);
    memcpy(hdr->h_source, dev->mac, 6);

    int datasize;
    if(mode == 1){
        datasize = user_frame(hdr, data);
    } else if(mode == 2) {
        datasize = arp_frame(hdr, (struct arphdr *)data);
    } else if(mode == 3) {
        datasize = ip_frame(hdr, (struct iphdr *)data);
    }

    ssize_t size = sendto(sockfd, frame, sizeof(struct ethhdr) + datasize, 0,
        (struct sockaddr *)&devaddr, sizeof(struct sockaddr_ll));
    if(size == -1)
    {
        perror("sendto");
        return 1;
    }

    printf("%ld bytes sent on %s!\n", size, dev->name);

    return 0;
}

int user_frame(struct ethhdr *hdr, char *data)
{
    input_macaddr("dest mac address: ", hdr->h_dest);
    hdr->h_proto = htons(input_proto("eth proto: "));
    printf("payload: ");
    fflush(stdout);
    scanf("%1500s", data);
    return strlen(data);
}

int arp_frame(struct ethhdr *eth, struct arphdr *arp)
{
    printf("choose op: 1.request 2.reply ");
    fflush(stdout);
    int op;
    scanf("%d", &op);
    getchar();
    if(op < 1 || op > 2)
    {
        printf("wrong arp op!\n");
        exit(1);
    }

    eth->h_proto = htons(0x0806);
    arp->ar_hrd = htons(1);
    arp->ar_pro = htons(0x0800);
    arp->ar_hln = 6;
    arp->ar_pln = 4;

    unsigned char *sender_mac = (unsigned char *)arp + sizeof(struct arphdr);
    unsigned char *sender_ip = sender_mac + 6;
    unsigned char *dest_mac = sender_ip + 4;
    unsigned char *dest_ip = dest_mac + 6;

    if(op == 1)
    {
        memcpy(eth->h_dest, "\xff\xff\xff\xff\xff\xff", 6);
        arp->ar_op = htons(1);
        input_macaddr("sender mac address: ", sender_mac);
        input_ip("sender ip: ", sender_ip);
        memcpy(dest_mac, "\x0\x0\x0\x0\x0\x0", 6);
        input_ip("query ip: ", dest_ip);
    } else if(op == 2)
    {
        input_macaddr("dest mac address: ", eth->h_dest);
        arp->ar_op = htons(2);
        input_macaddr("sender mac address: ", sender_mac);
        input_ip("sender ip: ", sender_ip);
        memcpy(dest_mac, eth->h_dest, 6);
        input_ip("dest ip: ", dest_ip);
    }

    return sizeof(struct arphdr) + 20;
}

// 计算 IP 头部检验和的函数
uint16_t calculate_checksum(uint16_t *data, int length) 
{
    uint32_t sum = 0;

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

int ip_frame(struct ethhdr *eth, struct iphdr *ip)
{
    input_macaddr("dest mac address: ", eth->h_dest);
    eth->h_proto = htons(0x0800);

    input_ip("src ip addr: ", (unsigned char *)&ip->saddr);
    input_ip("dest ip addr: ", (unsigned char *)&ip->daddr);
    printf("ip payload protocol: ");
    fflush(stdout);
    unsigned char protocol;
    scanf("%hhu", &protocol);
    getchar();
    ip->protocol = protocol;

    char *data = (unsigned char *)ip + 20;
    printf("ip payload: ");
    fflush(stdout);
    scanf("%1000s", data);

    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(strlen(data) + 20);
    ip->id = htons(0xabcd);
    ip->frag_off = htons(0x4000);
    ip->ttl = 64;
    ip->check = 0;
    ip->check = calculate_checksum((uint16_t *)ip, 20);

    // printf("checksum: %d\n", calculate_checksum((uint16_t *)ip, 20));

    return 20 + strlen(data);
}