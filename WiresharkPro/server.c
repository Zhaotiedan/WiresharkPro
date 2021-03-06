#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h> 
#include <time.h>
#include <pcap.h>

#define SERVPORT 3333
#define BACKLOG 10
char* DeviceList()
{
  pcap_if_t *alldevs;
  pcap_if_t *d;
  char errbuf[PCAP_ERRBUF_SIZE];
  /* 获取本地机器设备列表 */
  if (pcap_findalldevs( &alldevs, errbuf  ) == -1)
  {
    fprintf(stderr,"Error in pcap_findalldevs_ex: %s\n", errbuf);
    exit(1);
  }
  /* 打印列表 */
  for(d= alldevs; d != NULL; d= d->next)
  {
    if (strcmp(d->name,"ens33")==0)
    {
      return d->name;
    } 
  }
}


void processPacket(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
  int *count = (int *)arg;
  printf("Packet Count: %d\n", ++(*count));
  printf("Received Packet Size: %d\n", pkthdr->len);
  printf("Payload:\n");
  int i=0;
  for(i=0; i < pkthdr->len; ++i)
  {
    printf("%02x ", packet[i]);
    if ((i + 1) % 16 == 0)
    {
      printf("\n");
    }

  }
  printf("\n\n");
  return;
}
//发送的数据
void SendBuf()
{
  char* send;
  char errBuf[PCAP_ERRBUF_SIZE], * devStr;
  devStr=DeviceList();//接收设备
  printf("success device:%s\n",devStr);
  /* open a device, wait until a packet arrives */
  pcap_t * device = pcap_open_live(devStr, 65535, 1, 0, errBuf);
  if (!device)
  {
    printf("error: pcap_open_live(): %s\n", errBuf);
    exit()
  }
  /* construct a filter */
  struct bpf_program filter;
  pcap_compile(device, &filter, "tcp", 1, 0);
  pcap_setfilter(device, &filter);
  int count = 0;
  /*Loop forever & call processPacket() for every received packet.*/
  pcap_loop(device, 2, processPacket, (u_char *)&count);//发送2个包
  pcap_close(device);
}

int main()
{ 
    int sockfd,client_fd;
    int recvbytes;
    int sin_size;
    char buf[50];
    struct sockaddr_in my_addr;
    struct sockaddr_in remote_addr; 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);//建立socket --
    my_addr.sin_family=AF_INET;//AF_INET地址族
    my_addr.sin_port=htons(SERVPORT);//设定端口号(host -> networks)
    my_addr.sin_addr.s_addr = INADDR_ANY;//32位IPv4地址
    bzero(&(my_addr.sin_zero),8); //置前8个字节为0
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) 
    {
      perror("bind 出错！");    
      exit(1);                 
    }
    if (listen(sockfd, BACKLOG) == -1) //监听socket连接，设置队列中最多拥有连接个数为10
    {  
      perror("listen 出错！");  
      exit(1);     
    }
    while(1)
    {
      sin_size = sizeof(struct sockaddr_in);//记录sockaddr_in结构体所占字节数
      if ((client_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &sin_size)) == -1) //accept()缺省是阻塞函数，阻塞到有连接请求为止 --
      { 
        perror("accept error");   
        continue;   
      }
      printf("收到一个连接来自： %s\n", inet_ntoa(remote_addr.sin_addr));
      if (!fork()) 
      { 
        if (send(client_fd, "连接上了 \n", 26, 0) == -1) //--   
          perror("send 出错！");    
        //receive pieces of message from host
        
        close(client_fd); 
        exit(0);
      }
      close(client_fd);
    }
}
