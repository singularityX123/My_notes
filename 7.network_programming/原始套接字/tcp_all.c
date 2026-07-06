#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/ethernet.h>
 
#define MTU 1500
 
int main()
{
    /* 定义变量 */
    int sockfd, len;
    uint8_t buf[MTU]={};
    uint16_t ether_type;
 
    struct iphdr *iph;  //IP包头
    struct tcphdr *tcph;//TCP包头
    struct ether_header *eth;
 
    /* 创建一个链路层原始套接字 */
    if( (sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)) ) < 0){
        perror("socket");
        return 0;
    }
    printf("sockfd = %d\n", sockfd);
 
    /* 接收(只接收TCP数据协议)并处理IP数据报 */
    while(1)
    {
        /* 接收包含TCP协议的IP数据报 */
        len = recvfrom(sockfd, buf, sizeof(buf),0,NULL,NULL);
 
        eth = (struct ether_header *)buf;
        ether_type = htons(eth->ether_type);
        switch(ether_type){
        case ETHERTYPE_IP:
            printf("IP协议\n");
            break;
        case ETHERTYPE_ARP:
            printf("ARP协议\n");
            break;
        case ETHERTYPE_LOOPBACK:
            printf("loop back\n");
            break;
        default:
            printf("其他协议 %x\n",eth->ether_type);
        }
        if(ether_type != ETHERTYPE_IP)
            continue;
 
        /* 打印源IP和目的IP */
        iph = (struct iphdr *)(buf+14);
        if(iph->protocol != IPPROTO_TCP)
            continue;
        printf("源IP:%s\n",inet_ntoa(*(struct in_addr *)&iph->saddr) );
        printf("目的IP%s\n",inet_ntoa(*(struct in_addr *)&iph->daddr) );
 
        /* 打印TCP包头的源端口号和目的端口号 */
        tcph = (struct tcphdr *)(buf+14+iph->ihl*4);
        printf("%hu--->", ntohs(tcph->source));
        printf("%hu\n", ntohs(tcph->dest));
 
        /* 打印TCP数据段的长度 */
        printf("TCP首部长度:%d\n", tcph->doff*4);
    }
    //关闭套接字
    close(sockfd);
    return 0;
}