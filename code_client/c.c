#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netinet/sctp.h>
#include <time.h>
#include <getopt.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <pthread.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <net/if.h>
#include <sys/utsname.h>
#include <limits.h>
#include <linux/sockios.h>

#define PORT 8000
#define C_PORT 8001
#define C_PORT2 8001
#define MAXBUF 256
#define FILENAMELEN 50
#define TRUE 1
#define FALSE 0
#define ETHTOOL_GLINK       0x0000000a /* Get link status (ethtool_value) */

  pthread_mutex_t count_mutex=PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t condition_var=PTHREAD_COND_INITIALIZER;
  pthread_t tid1,tid2,linkid;

  struct sockaddr_in server_addr;
  struct sockaddr_in lo_addr,client_addr,add_addr;
  struct sockaddr_storage *sar,*sal;
  struct sctp_status status;
  struct sctp_event_subscribe events;
  struct cmsghdr *cmsg;
  struct sctp_sndrcvinfo *sri2;

  int sockfd,nbytes,sed;
  int num_rem,num_loc;
  socklen_t from_len;
  int nodelay=FALSE;
  char recv_ACK[FILENAMELEN];
  char recv_buf[MAXBUF];
  char get_filename[FILENAMELEN];

  int cbuf_size=CMSG_SPACE(sizeof(struct sctp_sndrcvinfo));
  char cbuf[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];

  struct ethtool_value
  {
    unsigned int cmd;
    unsigned int data; 
  };

char* usage_msg="usage: echo_c ip_addr";

void usage()
{
  fprintf(stderr,"%s/n",usage_msg);
  exit(1);
}

int get_netlink_status(const char *if_name)
{
    int skfd;
    struct ifreq ifr;
    struct ethtool_value edata;

    edata.cmd = ETHTOOL_GLINK;
    edata.data = 0;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *)&edata;

    if((skfd=socket(AF_INET,SOCK_DGRAM,0))==0)
       return -1;
    if (ioctl(skfd,SIOCETHTOOL,&ifr)==-1)
    {
      close(skfd);
      return -1;
    }
    close(skfd);

    return edata.data;
}

void *checklink()
{
  char net[]="eth0";
  if( get_netlink_status(net) == 1) 
  {
    printf("Wired net link status is up.\n");


    pthread_cond_signal(&condition_var);

//send the notice to server to change the primary IP
    char ChangeIPSig[]="changeIP";
    int size;
    size=sizeof(ChangeIPSig);
      //printf("!!!Get %d bytes:\n",size);
      //printf("???The read buf is %s\n",buf);             
      int z;
      from_len =sizeof(struct sockaddr_in);
      z=sctp_sendmsg(sockfd,ChangeIPSig,size,(struct sockaddr*)&server_addr,from_len,1,0,1,0,0);

      if(z==-1){
	printf("Msg sending error!\n");
	perror("sctp_sendvmsg Error!\n");
	exit(1);}
   
    //print current time
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo=localtime(&rawtime);
    printf("The current time is:%s\n",asctime(timeinfo)); 

    pthread_exit(NULL);
  }
  else
    // printf("Net link status is down.\n");
    printf("");
    checklink();
 
}

void *File_Transfer()
{
  //creat the file
  FILE * fd1;
  char filename1[]="copy_one";

  fd1=fopen("copy_one","wt+");
  if(fd1==NULL)
  {
    printf("Creat file %s is fail!\n",filename1);
    exit(1);
  }
  else
    { printf("Creat file %s is successed!\n",filename1);}

  //set for the loop condition
  nbytes=1;
  while(nbytes)
  {
    nbytes=sctp_recvmsg(sockfd,recv_buf,sizeof(recv_buf),(struct sockaddr*)&server_addr,0,NULL,NULL);
    if(nbytes==-1)
    {
      perror("sctp_recvmsg Error!\n");
      printf("sctp_recvmsg error!\n");
      exit(1);
    }
    else if(strlen(recv_buf)==0)
    {
      printf("all the message is received!\n");
      break;
    }
    else
      //printf("The recied msg is:%s\n",recv_buf);
      printf("");

      int bytes=fwrite(recv_buf,strlen(recv_buf),1,fd1);
      fseek(fd1,0,SEEK_END);
              
      if(bytes!=1)
      {
	perror("Error!\n");
	printf("Write error!\n");
	break;
      }
      else
	memset(recv_buf,0,MAXBUF);
	  
  }//end of while(nbytes)

  printf("write success!!\n");
  //close file
  fclose(fd1);
  pthread_exit(NULL);
}

int ifconfig(const char *ifname,const char *ipaddr) 
{ 
  struct   sockaddr_in   sin; 
  struct   ifreq   ifr; 
  int   fd; 

  int   ret; 
  char   *ptr; 
  short   found_colon   =   0; 

  bzero(&ifr,   sizeof(struct   ifreq)); 
  if(ifname   ==   NULL) 
    return   (-1); 
  if(ipaddr   ==   NULL) 
  return   (-1); 
  
  fd   =   socket(AF_INET,   SOCK_DGRAM,   0); 
  if   (fd   ==   -1) 
    { 
      perror( "Not   create   network   socket   connection\n "); 
      return   (-1); 
      } 

  strncpy(ifr.ifr_name,ifname,IFNAMSIZ); 
  ifr.ifr_name[IFNAMSIZ-1]   =   0; 
  memset(&sin,0,sizeof(sin)); 
  sin.sin_family=AF_INET; 
  sin.sin_addr.s_addr=inet_addr(ipaddr); 
  memcpy(&ifr.ifr_addr,&sin,sizeof(sin)); 

  if   (ioctl(fd,SIOCSIFADDR,&ifr)<0) 
    { 
      perror( "Not   setup   interface\n "); 
      return   (-1); 
    } 

  ifr.ifr_flags|=IFF_UP|IFF_RUNNING; 

  if   (ioctl(fd,SIOCSIFFLAGS,&ifr)   <   0)   
    { 
      perror( "SIOCSIFFLAGS "); 
      return(-1); 
    } 
  return   (0);   
} 

void *changePrimary()
{
  pthread_mutex_lock(&count_mutex);
  //wait for the wired signal
  pthread_cond_wait(&condition_var,&count_mutex);

      //set up the new IP address
    ifconfig("eth0","192.168.1.100");

//set the option for the second client IP address
 //adds new address information
  bzero(&add_addr,sizeof(add_addr));
  add_addr.sin_family=AF_INET;
  add_addr.sin_addr.s_addr=inet_addr("192.168.1.100");
  add_addr.sin_port=htons(C_PORT2);

 //!!!start binding new IP Address
  if((sctp_bindx(sockfd,(struct sockaddr*)&add_addr,1,SCTP_BINDX_ADD_ADDR))<0)
    {
      perror("Bind wired new IP error!\n");
      printf("new bind() failed.\n");
      exit(1);
    }
  else
    printf("Now a new IP is added success!\n");
 
 //change the primary IP
  struct sctp_setpeerprim primary;
  primary.sspp_assoc_id=sri2->sinfo_assoc_id;
  memcpy(&primary.sspp_addr,&add_addr,sizeof(add_addr));

  int setso3;
  setso3=setsockopt(sockfd,IPPROTO_SCTP,SCTP_SET_PEER_PRIMARY_ADDR,&primary,sizeof(struct sctp_setpeerprim));
  if(setso3<0)
    {
	perror("setso3 error!\n");
	printf("setsockopt() failed.\n");
	//exit(1);
     }
  else 
    printf("Change primary IP address success!\n");
    printf("********************************************\n");
    printf("The client IP address is:192.168.1.100!\n");
    printf("********************************************\n");

//delete previous IP addresses
  if((sctp_bindx(sockfd,(struct sockaddr*)&client_addr,1,SCTP_BINDX_REM_ADDR))<0)
   {
     perror("Delete error!\n");
     printf("bind() failed.\n");
     exit(1);
   }
  printf("The previous IP address is deleted!\n");

/* After delete, Print Local Addresses */
  num_loc = sctp_getladdrs(sockfd,0,(struct sockaddr **)&sal);
  if(num_loc > 0)
    {
      printf("There are %d local addresses: \n", num_loc);
      sctp_freeladdrs((struct sockaddr *)sal);
    }
   else 
    printf("No local addresses.\n");

  pthread_mutex_lock(&count_mutex);

  pthread_exit(NULL);

}

int main(int argc,char *argv[])
{
  //creat server socket
  sockfd=socket(AF_INET,SOCK_SEQPACKET,IPPROTO_SCTP);
  if(sockfd<0){
    perror("socket error!\n");
    printf("socket() failed.\n");
    exit(1);
  }
  else
     printf("Socket create success!\n");

 //set the socket option of the event
  bzero(&events, sizeof(events));
  events.sctp_data_io_event = 1 ;
  int setso;
  setso=setsockopt(sockfd,IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof (events)) ;
  if(setso<0){
	perror("setso error!\n");
	printf("setsockopt() failed.\n");
	exit(1);
      }

//set the socket option,allow to reuse the address 
    int setso2;
    setso2=setsockopt(sockfd,IPPROTO_SCTP,SCTP_NODELAY,&nodelay,sizeof(nodelay));
    if(setso2<0)
    {
      perror("setso error!\n");
      printf("error while setting sctp options.\n");
      exit(1);
    }
    
   
 //local address information
  bzero(&lo_addr,sizeof(lo_addr));
  lo_addr.sin_family=AF_INET;
  lo_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
  lo_addr.sin_port=htons(C_PORT);
  
  //server address information
  bzero(&server_addr,sizeof(server_addr));
  server_addr.sin_family=AF_INET;
  server_addr.sin_addr.s_addr=inet_addr("192.168.1.105");
  server_addr.sin_port=htons(PORT);
 
 //client address information
  bzero(&client_addr,sizeof(client_addr));
  client_addr.sin_family=AF_INET;
  client_addr.sin_addr.s_addr=inet_addr("192.168.1.111");
  client_addr.sin_port=htons(C_PORT);
   
 //bind the first address
  if((sctp_bindx(sockfd,(struct sockaddr*)&lo_addr,1,SCTP_BINDX_ADD_ADDR))<0)
    {
      perror("Bind error!\n");
      printf("bind() failed.\n");
      exit(1);
      }

//!!!start binding the first wireless IP Address 192.168.1.111
  if((sctp_bindx(sockfd,(struct sockaddr*)&client_addr,1,SCTP_BINDX_ADD_ADDR))<0)
    {
      perror("Bind new IP error!\n");
      printf("client address bind() failed.\n");
      exit(1);
    }
  else
    printf("********************************************\n");
    printf("Now the client IP is 192.168.1.111!\n");
    printf("********************************************\n");

//delete the local IP 127.0.0.1
  if((sctp_bindx(sockfd,(struct sockaddr*)&lo_addr,1,SCTP_BINDX_REM_ADDR))<0)
   {
     perror("Delete error!\n");
     printf("bind() failed.\n");
     exit(1);
   }
   printf("The previous IP address is deleted!\n");

  if(connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
  {
    perror("connect error!!");
    exit(1);
  }
  printf("The client is connected to server now!\n");
  
 //request for file name 
  memset(get_filename,0,FILENAMELEN);
  printf("Please input the filename that you request for move:");
  fscanf(stdin,"%s",get_filename);
  printf("The filename that you request for move is %s:\n",get_filename);

//send the filename request to server
  struct msghdr msg;
  struct iovec iov;
  ssize_t z;

  msg.msg_name=&server_addr;
  msg.msg_namelen=sizeof(struct sockaddr_in);
  msg.msg_iov=&iov;
  msg.msg_iovlen=1;
  iov.iov_base=get_filename;
  iov.iov_len=strlen(get_filename);
  msg.msg_control=0;
  msg.msg_controllen=0;
  msg.msg_flags=0;
 
  z=sendmsg(sockfd,&msg,0); 
  if(z<0)
    {printf("Msg sending error!\n");}
  else 
    {
      printf("Requested file name is %s\n",get_filename);
    }
 
//Receiving
  memset(&msg,0,sizeof(msg));
  memset(cbuf,0,cbuf_size);
  msg.msg_name=&server_addr;
  msg.msg_namelen=sizeof(struct sockaddr_in);
  iov.iov_base=recv_ACK;
  iov.iov_len=sizeof(recv_ACK);
  msg.msg_iov=&iov;
  msg.msg_iovlen=1;
  msg.msg_control=cbuf;
  msg.msg_controllen=cbuf_size;
  msg.msg_flags=0;

  if(recvmsg(sockfd,&msg,0)<0)
  {
    perror("recvmsg error!\n");
    exit(1);
  }
  printf("receive ACK is %s:\n",recv_ACK);
  cmsg=CMSG_FIRSTHDR(&msg);
  while(cmsg!=NULL)
  {
    if( cmsg->cmsg_level==IPPROTO_SCTP && cmsg->cmsg_type==SCTP_SNDRCV)
    {
      sri2=(struct sctp_sndrcvinfo*)CMSG_DATA(cmsg);
      printf("The assoc_id is %d:\n",sri2->sinfo_assoc_id);
      cmsg=CMSG_NXTHDR(&msg,cmsg);
    }
    else
      printf("receive cmsg wrong!\n");
//end of receive Assoc ID

//creat thread changePrimary
  if((pthread_create(&tid2,NULL,changePrimary,NULL))!=0)
  {
    printf("The thread changePrimary creat failed!\n");
    exit(1);
  }
  else 
    {printf("The thread changePrimary creat success!\n");}

//creat thread checklink
  if((pthread_create(&linkid,NULL,checklink,NULL))!=0)
  {
    printf("The thread checklink creat failed!\n");
    exit(1);
  }
  else 
    {printf("The thread checklink creat success!\n");}

 //creat thread File_Transfer
  if((pthread_create(&tid1,NULL,File_Transfer,NULL))!=0)
  {
    printf("The thread File_Transfer creat failed!\n");
    exit(1);
  }
  else 
    {printf("The thread File_Transfer creat success!\n");}



  //wait for the thread exit
  pthread_join(tid2,NULL); 
  printf("Thread changePrimary is over!\n");

 //wait for the thread exit
  pthread_join(linkid,NULL); 
  printf("Thread checklink is over!\n");

  //wait for the thread exit
  pthread_join(tid1,NULL); 
  printf("Thread File_Transfer is over!\n");
  
/* Print Peer Addresses */ 
  num_rem = sctp_getpaddrs(sockfd,0,(struct sockaddr **)&sar);
  if(num_rem > 0)
    {
      printf("There are %d remote addresses: \n", num_rem);
      sctp_freepaddrs((struct sockaddr *)sar);
    }
  else 
    printf("No remote addresses. \n");

/* Print Local Addresses */
  num_loc = sctp_getladdrs(sockfd,0,(struct sockaddr **)&sal);
  if(num_loc > 0)
    {
      printf("There are %d local addresses: \n", num_loc);
      sctp_freeladdrs((struct sockaddr *)sal);
    }
   else 
    printf("No local addresses.\n");

  //close socket
  if(close(sockfd)<0)
  {
    printf("close() failed.\n");
    exit(1);
  }
  else
    printf("End of receiving message.\n");
  
  exit(1);
 
  }
}




