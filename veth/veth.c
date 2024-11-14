#include<linux/module.h>
#include<linux/init.h>
#include<linux/netdevice.h>
#include<linux/slab.h>
#include<linux/etherdevice.h>
#include<linux/ip.h>
#include<linux/tcp.h>
#include<linux/udp.h>
#include<linux/icmp.h>
#include<linux/inet.h>

struct net_device *veth;

void print_skb(struct sk_buff *skb)
{
    struct ethhdr *ethhdr;
    struct iphdr *iphdr;
    
    ethhdr = eth_hdr(skb);
    pr_alert("src mac: %pM dest mac: %pM ether type: 0x%04x\n",
        ethhdr->h_source, ethhdr->h_dest, ntohs(ethhdr->h_proto));

    iphdr = ip_hdr(skb);
    pr_alert("src ip: %pI4 dest ip: %pI4 protocol: %d\n",
        &iphdr->saddr, &iphdr->daddr, iphdr->protocol);

    if(iphdr->protocol == IPPROTO_UDP)
    {
        struct udphdr *udphdr;
        int size;
        char *payload;

        udphdr = udp_hdr(skb);
        size = ntohs(udphdr->len) - 8;

        payload = kmalloc(size + 1, GFP_KERNEL);
        memcpy(payload, (char *)udphdr + 8, size);
        payload[size] = '\0';

        pr_alert("UDP src port: %u dest port: %u payload size: %d payload: %s\n",
            ntohs(udphdr->source), ntohs(udphdr->dest), size, payload);
        kfree(payload);
    }

    if(iphdr->protocol == IPPROTO_ICMP)
    {
        struct icmphdr *icmp = icmp_hdr(skb);
        pr_alert("ICMP type: %d code: %d\n", icmp->type, icmp->code);
    }

}

/*
    反转数据包：veth -> remote => remote -> veth
*/
struct sk_buff *reverse_skb(struct sk_buff *skb)
{
    struct sk_buff *new_skb;
    struct ethhdr *eth, *new_eth;
    struct iphdr *ip, *new_ip;

    new_skb = dev_alloc_skb(skb->len + 2);
    if(!new_skb)
    {
        pr_alert("dev_alloc_skb failed.\n");
        return NULL;
    }

    skb_reserve(new_skb, 2); //align IP on 16B boundary
    skb_put(new_skb, skb->len);
    memcpy(new_skb->data, skb->data, skb->len);

    //反转mac地址
    eth = eth_hdr(skb);
    new_eth = (struct ethhdr*)(new_skb->data);
    memcpy(new_eth->h_source, eth->h_dest, ETH_ALEN);
    memcpy(new_eth->h_dest, eth->h_source, ETH_ALEN);

    //反转IP
    ip = ip_hdr(skb);
    new_ip = (struct iphdr*)((char *)new_eth + ETH_HLEN);
    memcpy(&new_ip->saddr, &ip->daddr, 4);
    memcpy(&new_ip->daddr, &ip->saddr, 4);
    //rebuild checksum (ip needs it.)
    new_ip->check = 0;
    new_ip->check = ip_fast_csum(new_ip, new_ip->ihl); 

    return new_skb;
} 

/*
    skb 是一个完整的以太网帧
*/
netdev_tx_t	veth_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr* ethhdr = eth_hdr(skb);
    struct sk_buff *new_skb;

    if(ethhdr->h_dest[0] == 0xff 
    || (ethhdr->h_dest[0] == 0x33 && ethhdr->h_dest[1] == 0x33)
    || ntohs(ethhdr->h_proto) != 0x0800)// 非IPV4协议
    {
        dev_kfree_skb(skb);
        return NETDEV_TX_OK;
    }

    dev->stats.tx_packets += 1;
    dev->stats.tx_bytes += skb->len;

    pr_alert("transmit a packet. len: %d task: %s\n", skb->len, current->comm);
    print_skb(skb);

    new_skb = reverse_skb(skb);
    dev_kfree_skb(skb);

    if(!new_skb)
    {
        pr_alert("reverse_skb failed.\n");
        return NETDEV_TX_BUSY;
    }

    dev->stats.rx_packets += 1;
    dev->stats.rx_bytes += new_skb->len;

    new_skb->protocol = eth_type_trans(new_skb, dev); //去掉以太网头
    new_skb->ip_summed = CHECKSUM_UNNECESSARY;

    if(netif_rx(new_skb) != NET_RX_SUCCESS) //提交给IP层
        pr_alert("netif_rx failed.\n");

    return NETDEV_TX_OK;
}

/*
    构建ether协议帧头
*/
int veth_header(struct sk_buff *skb, struct net_device *dev,
	       unsigned short type,
	       const void *daddr, const void *saddr, unsigned int len)
{
	struct ethhdr *eth = skb_push(skb, ETH_HLEN);

	eth->h_proto = htons(type);

	if (!saddr)
		saddr = dev->dev_addr;
	memcpy(eth->h_source, saddr, ETH_ALEN);

	if (daddr) 
    {
        if(((char *)daddr)[0] == 0x01)
            daddr = "\x06\x05\x04\x03\x02\x01";

		memcpy(eth->h_dest, daddr, ETH_ALEN);
		return ETH_HLEN;
	}

	return -ETH_HLEN;
}

int veth_open(struct net_device *dev)
{
    struct header_ops *temp_ops;
    
    temp_ops = kmalloc(sizeof(struct header_ops), GFP_KERNEL);
    memcpy(temp_ops, dev->header_ops, sizeof(struct header_ops));
    temp_ops->create = veth_header;
    dev->header_ops = temp_ops;

    memcpy(dev->dev_addr, "\x01\x02\x03\x04\x05\x06", 6);
    
    pr_alert("vdev up.\n");
    return 0;
}

int veth_stop(struct net_device *dev)
{
    kfree(dev->header_ops);
    pr_alert("vdev stop.\n");
    return 0;
}

const struct net_device_ops veth_ops = {
    .ndo_open = veth_open,
    .ndo_stop = veth_stop,
    .ndo_start_xmit = veth_xmit
};


void veth_setup(struct net_device *dev)
{
    ether_setup(dev);
    dev->flags		= IFF_NOARP;
	dev->priv_flags	= 0;
    dev->netdev_ops = &veth_ops;
}

void veth_exit(void)
{
    if(veth)
    {
        unregister_netdev(veth);
        free_netdev(veth);
    }

    pr_alert("veth exit.\n");
}

int veth_init(void)
{
    int ret;

    pr_alert("start to init veth.\n");
    veth = alloc_netdev(0, "veth", NET_NAME_PREDICTABLE, veth_setup);
    if(!veth)
    {
        pr_alert("alloc_netdev failed.\n");
        veth_exit();
        return -EFAULT;
    }

    pr_alert("veth is ready to register.\n");
    ret = register_netdev(veth);
    if(ret)
    {
        pr_alert("register_netdev failed.\n");
        veth_exit();
        return -EFAULT;
    }

    pr_alert("veth init successful.\n");
    return 0;
}

module_init(veth_init);
module_exit(veth_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pan");