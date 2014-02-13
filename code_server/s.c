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
#include <netdb.h>
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
#define MAXBUF 256
#define FILENAMELEN 50
#define TRUE 1
#define FALSE 0

  int ssock;
  int addr_count=1;
  int sockReuse=TRUE;
  int nodelay=FALSE;
  char recv_filename[FILENAMELEN];
  socklen_t from_len;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  struct sctp_event_subscribe events;//define event
  pthread_mutex_t count_mutex=PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t condition_var=PTHREAD_COND_INITIALIZER;
  pthread_t tid1,tid2,linkid;

  char* usage_msg="usage: sctp_echo ip_addr,try again!\n";

  void usage()
  {
    fprintf(stderr,"%s/n",usage_msg);
    exit(1);
  }

void *checklink()
{
    int zz;
    char recvSig[MAXBUF];
     char changeIPSig[]="changeIP";
    while(1){
	zz=sctp_recvmsg(ssock,recvSig,sizeof(recvSig),(struct sockaddr*)&client_addr,0,NULL,NULL);
   	if(zz==-1)
   	 {
     	   perror("sctp_recvmsg Error!\n");
     	   printf("sctp_recvmsg error!\n");
        exit(1);
         }
	else
	printf("Receive the signal:%s\n",recvSig);

	if(strcmp(recvSig,changeIPSig)==0)
	{
	pthread_cond_signal(&condition_var);
	pthread_exit(NULL);
	}

     }//end of while(1)
 
}

void *changePrimary()
{
  pthread_mutex_lock(&count_mutex);
  //wait for the wired signal
  pthread_cond_wait(&condition_var,&count_mutex);

 //change client address information
  bzero(&client_addr,sizeof(client_addr));
  client_addr.sin_family=AF_INET;
  client_addr.sin_addr.s_addr=inet_addr("192.168.1.100");
  client_addr.sin_port=htons(C_PORT);

  printf("The client address has changed!\n");
  printf("**********************************************\n");
  printf("The client address is 192.168.1.100!\n");
  printf("**********************************************\n");
  pthread_mutex_lock(&count_mutex);
  pthread_exit(NULL);

}

void *File_Transfer()
{
   //read file into the buffer
    FILE *fd1;
    int size;
    char buf[MAXBUF];
   
    fd1=fopen(recv_filename,"rt+");//on the file on the current directory
    if(fd1==NULL)
      { printf("Open file %s is fail!\n",recv_filename);}
    else
      { printf("Open file %s is successed!\n",recv_filename);}

     //read file and send file
    while(fgets(buf,MAXBUF,fd1)!=NULL)
    {
      size=sizeof(buf);
      //printf("!!!Get %d bytes:\n",size);
      //printf("???The read buf is %s\n",buf);             
      int z;
      from_len =sizeof(struct sockaddr_in);
      z=sctp_sendmsg(ssock,buf,size,(struct sockaddr*)&client_addr,from_len,1,0,1,0,0);

      if(z==-1){
	printf("Msg sending error!\n");
	perror("sctp_sendvmsg Error!\n");
	exit(1);}
      else {
       //printf("Msg sending Success!!\n");
	//printf("Sending msg is :%s\n",buf);
       printf("");
        }
      memset(buf,0,MAXBUF);
      
    }//end of while(fgets)

      printf("End of sending message!!\n");

      //send the empty message
      char emptyBuf[]="";
      int z2;
      z2=sctp_sendmsg(ssock,emptyBuf,size,(struct sockaddr*)&client_addr,from_len,1,0,1,0,0);

      if(z2==-1){
	printf("Msg sending error!\n");
	perror("sctp_sendvmsg Error!\n");
	exit(1);}
      else {printf("Msg sending Success!!\n");}

    //close file
     fclose(fd1);
     pthread_exit(NULL);
}


int main(int argc,char *argv[])
{
  //creat server socket
  ssock=socket(AF_INET,SOCK_SEQPACKET,IPPROTO_SCTP);
  if(ssock<0){
    perror("Server socket error!\n");
    printf("socket() failed.\n");
    exit(1);
  }
  else
   printf("Server : got a connection\n");

  //address information
  bzero(&server_addr,sizeof(server_addr));
  server_addr.sin_family=AF_INET;
  server_addr.sin_addr.s_addr=inet_addr("192.168.1.105");
  server_addr.sin_port=htons(PORT);
  //inet_pton(AF_INET,argv[1],&server_addr.sin_addr);

  printf("Server get connetion from %s\n",inet_ntoa(server_addr.sin_addr));

//set the socket option,allow to reuse the address 
      int setso1;
      setso1=setsockopt(ssock,SOL_SOCKET,SO_REUSEADDR,&sockReuse,sizeof(sockReuse));
      if(setso1<0){
	perror("setso error!\n");
	printf("error while setting sctp options.\n");
	exit(1);
      }

//set the socket option,allow to reuse the address 
      int setso2;
      setso2=setsockopt(ssock,IPPROTO_SCTP,SCTP_NODELAY,&nodelay,sizeof(nodelay));
      if(setso2<0){
	perror("setso error!\n");
	printf("error while setting sctp options.\n");
	exit(1);
      }

 //set the socket option,enable come with recvmsg
      bzero(&events,sizeof(events));
      events.sctp_data_io_event=1;

      int setso3;
      setso3=setsockopt(ssock,IPPROTO_SCTP,SCTP_EVENTS,&events,sizeof(events));
      if(setso3<0){
	perror("setso error!\n");
	printf("error while setting sctp options.\n");
	exit(1);
      }

  //sctp_bindx
  if((bind(ssock,(struct sockaddr*)&server_addr,sizeof(server_addr)))<0){
  perror("Bind error!\n");
  printf("server address bind() failed.\n");
  exit(1);
  }

      //listen
  if(listen(ssock,5)<0){
    perror("Listen error!\n");
    printf("listen() failed.\n");
    exit(1);
  }
  
  while(1){      
 //blocking and waiting for receiving
    memset(recv_filename,0,FILENAMELEN);
    struct msghdr msg;
    struct iovec iov;
    int rv_size;
    bzero(&msg,sizeof(msg));
    msg.msg_name=&client_addr;
    msg.msg_namelen=sizeof(struct sockaddr_in);
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;
    iov.iov_base=recv_filename;
    iov.iov_len=sizeof(recv_filename);
    msg.msg_control=0;
    msg.msg_controllen=0;
    msg.msg_flags=0;
     
    rv_size=recvmsg(ssock,&msg,0);
    //printf("The received msg length is: %d\n",rv_size);
    if(rv_size<0)
    {
      printf("Msg receiving error!!\n");      
      exit(1);
    }
    else
      printf("Request file name is %s\n",recv_filename);
      printf("**********************************************\n");
      printf("The client address now is:192.168.1.111!\n");
      printf("**********************************************\n");

//start of sending Assoc ID
    int cbuf_size=CMSG_SPACE(sizeof(struct sctp_sndrcvinfo));
    char cbuf[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
    struct cmsghdr *cmsg;
    struct sctp_sndrcvinfo *sri;
 
    memset(&msg,0,sizeof(msg));
    msg.msg_name=&client_addr;
    msg.msg_namelen=sizeof(struct sockaddr_in);
    iov.iov_base="SUCCESS";
    iov.iov_len=7;
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;
    msg.msg_control=cbuf;
    msg.msg_controllen=cbuf_size;
    msg.msg_flags=0;
 
    memset(cbuf,0,cbuf_size);
    cmsg=(struct cmsghdr *)cbuf;
    sri=(struct sctp_sndrcvinfo*)CMSG_DATA(cmsg);
  // cmsg=CMSG_FIRSTHDR(&msg);  
    cmsg->cmsg_level=IPPROTO_SCTP;
    cmsg->cmsg_type=SCTP_SNDRCV;
    cmsg->cmsg_len=CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
    msg.msg_controllen=cmsg->cmsg_len;

    sri->sinfo_stream=1;
 
  int kk;
  kk=sendmsg(ssock,&msg,0);
  if(kk<0)
  {
    perror("sendmsg error!\n");
    printf("send NO:%d\n",kk);
    exit(1);
  }
  else 
    printf("sending assoc id is success!\n");
//end of sending assoc id

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


 pthread_join(linkid,NULL); 
  printf("Thread checklink is over!\n");

 //wait for the thread exit
  pthread_join(tid2,NULL); 
  printf("Thread changePrimary is over!\n");
  
  //wait for the thread exit
  pthread_join(tid1,NULL); 
  printf("Thread File_Transfer is over!\n");

  }//close of while(1) 
     close(ssock);
     printf("Server shut down!\n");
     exit(1);   
 
}//close of main





