#include"define.h"
#include<string>
#define IPADDRESS "127.0.0.1"
#define CLI_PATH "/home/kronos/c++/epollDocServer/"	//�ͻ���Ĭ�ϴ���ļ���·����
using namespace std;


int update(char filename[],int connfd);			//�ͻ����ϴ��ļ� 
int download(char filename[],int connfd);      //�ͻ��˴ӷ����������ļ� 

int update(char filename[],int connfd){			//�ͻ����ϴ��ļ� 
	package pack;
	FILE* fp;

	if((fp=fopen(filename,"rb"))==NULL){
		printf("cannot open file!\n");
		return -1;
	}
	printf("openfile ok!\n");	

  memcpy(pack.filename,filename,sizeof(pack.filename));
	int length;
	char *pBuff=new char[RCV_SIZE];
	while(!feof(fp)){
		length=fread(pack.buf,sizeof(char),BUF_SIZE,fp);
		pack.size=length;
		pack.fd=connfd;
		int iLen=0;
		*(int*)(pBuff+iLen)=(int)htonl(0); //host to net long ���ϴ��ļ�type=0 
		iLen+=sizeof(int);
		*(int*)(pBuff+iLen)=(int)htonl(pack.size); //host to net long 
		iLen+=sizeof(int);
		*(int*)(pBuff+iLen)=(int)htonl(pack.fd); //host to net long 
		iLen+=sizeof(int);	
		memcpy(pBuff+iLen,pack.filename,NAME_SIZE);
		iLen+=sizeof(pack.filename);
		memcpy(pBuff+iLen,pack.buf,BUF_SIZE);
		iLen+=sizeof(pack.buf);
		ssize_t writeLen=MySend(connfd,pBuff,iLen);
		if(writeLen<0){
			printf("write failed\n");
			close(connfd);
			return -1;
		}
		memset(pBuff,0,sizeof(pBuff));
	}
	
	delete[] pBuff;
	fclose(fp);
	return 0;
}


int download(char filename[],int connfd){	 //�ӷ����������ļ� ����0�ɹ�
	char *recvMsg=new char[RCV_SIZE];
	int readLen;
	FILE* fp;
	package pack;
	
	string path=CLI_PATH;
	path+=filename;
	const char *p=path.c_str();
	if((fp=fopen(p,"ab"))==NULL){
		printf("cannot open file");
		return -1;
	}
	
	int count=0;
	while((readLen=MyRecv(connfd,recvMsg,RCV_SIZE))>0){
	//	printf("num:%d readlen:%d\n",++count,readLen);
		int iLen=0;
	
		memcpy(&pack.type,recvMsg+iLen,sizeof(int));				//��ʼ��� 
		pack.type=(int)ntohl(pack.type);
		iLen+=sizeof(int);
		
		memcpy(&pack.size,recvMsg+iLen,sizeof(int));
		pack.size=ntohl(pack.size);
//		printf("pack.type:%d pack.size:%d\n",pack.type,pack.size);
		iLen+=sizeof(int);
		
		memcpy(&pack.fd,recvMsg+iLen,sizeof(int));
		pack.fd=ntohl(pack.fd);
		iLen+=sizeof(int);
	
		memcpy(&pack.filename,recvMsg+iLen,sizeof(pack.filename));
		iLen+=sizeof(pack.filename);
		
		memcpy(&pack.buf,recvMsg+iLen,sizeof(pack.buf));
		iLen+=sizeof(pack.buf);

		if(pack.size>0)
{
			fwrite(pack.buf,sizeof(char),pack.size,fp);
			fflush(fp);
		}
		if(pack.size<BUF_SIZE)
			break;
	}
	
	fclose(fp);
	return 0;
}

int main(int argc,char* argv[]){
	int connfd;
	struct sockaddr_in servaddr;
	connfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(SERV_PORT);
	inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);
	if(connect(connfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
		perror("connect error:");
		return -1;
	}else{
		printf("connect successfully\n");
	}

//struct timeval timeout={3,0};
//setsockopt(connfd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(struct timeval));

// fcntl(connfd,F_SETFL,O_NONBLOCK);		//��socket����Ϊ������
 char opera[OPR_SIZE],com_file[NAME_SIZE];	//�ֱ��Ų�����������·�������ļ�����
	printf("input your opeartion:");
	scanf("%s %s",opera,com_file);
  
	if(strcmp(opera,"update")==0){ 	//update Ҫ�ϴ��ļ���������
		int type=0;
		char *st=new char[4];
		*(int*)st=htonl(type);
		MySend(connfd,st,sizeof(int));			//���Ͳ������͸������� 
		if(update(com_file,connfd)==0)
			printf("sended all packages!\n");
		else
			printf("return -1!\n");	
	}else if(strcmp(opera,"download")==0){	//download Ҫ�����ļ�
		int type=1;	
 		char *st=new char[4];
 		 *(int*)st=htonl(type);
		MySend(connfd,st,sizeof(int));		//���Ͳ������͸�������
		int iLen=0;
		char *pBuff=new char[RCV_SIZE];		 	
	//	printf("sizeof(pBuff):%d",sizeof(pBuff));
		memset(pBuff,0,sizeof(pBuff));		
		*(int*)(pBuff+iLen)=(int)htonl(1); //���ﻹҪ�ٷ�����ٷ���һ�������߷�������Ҫ�����ĸ��ļ� 
		iLen+=sizeof(int);
		*(int*)(pBuff+iLen)=(int)htonl(0); //host to net long 
		iLen+=sizeof(int);
		*(int*)(pBuff+iLen)=(int)htonl(connfd); //host to net long 
		iLen+=sizeof(int);	
		memcpy(pBuff+iLen,com_file,NAME_SIZE);
		iLen+=sizeof(com_file);
	//	printf("connfd:%d  iLen:%d sizeof(pBuff):%d\n",connfd,iLen,RCV_SIZE);
		if((iLen=MySend(connfd,pBuff,RCV_SIZE))>0)
			printf("send ok!  len:%d\n",iLen);
		else
			printf("send failed!\n");
		if(download(com_file,connfd)==0){		//�ӷ����������ļ� 
				printf("download successfully\n");
			} 
	}else{
			printf("illegal operation!\n");
	}

	close(connfd);
	return 0; 
}
