#include<stdio.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<list>
#include<pthread.h>
#pragma pack(1)  //���ֽڶ���

#define OPR_SIZE 20
#define MAX_EVENTS 1024
#define BUF_SIZE 4096
#define SERV_PORT 6666
#define LISNUM 100
#define NAME_SIZE 128
#define POOL_NUM 5
#define RCV_SIZE BUF_SIZE+NAME_SIZE+24
using namespace std;

struct my_event{		
	int fd;												//�ļ������� 
	int events;											//��Ӧ�ļ����¼� 
	void *arg;											//���Ͳ��� ������ָ��ṹ���Լ� 
	void (*call_back)(int fd,int events,void *arg);		//�ص�����
	int status;											//�Ƿ��ڼ�����1��ʾ�ں�����ϣ��ڼ����� 0��ʾ�������ϣ����ڼ�����
	long last_active;									//��һ�λ�Ծ��ʱ��								
};


struct package{
	int type;			//0��ʾ�ͻ����ϴ����ݣ�������Ҫ�����ݣ���1��ʾ�ͻ����������ݣ�������Ҫд���ݣ� 
	int size;  			//���δ�����ļ����ݵĴ�С
	int fd;				//�ļ������� 
	char filename[NAME_SIZE];	//�ļ��� 
	char buf[BUF_SIZE];  	//�ļ����� 
}; 

struct node{		//�����б��еĽ�� 
	char buf[RCV_SIZE];
};

int g_efd;
struct my_event g_events[MAX_EVENTS+1];
//queue<package> packtask;
list<node> packtask;			//�����б� 
pthread_mutex_t t_mutex;		//�����б�Ļ����� 

int MySend(int iSock,char* pchBuf,size_t tLen){	//��pchBuf��tLen�������ݷ��ͳ�ȥ��ֱ��������Ϊֹ 
	int iThisSend; //һ�η����˶������� 
	unsigned int iSended=0;		//�Ѿ������˶��� 
	if(tLen==0)
		return 0;
	while(iSended<tLen){
		do{
			iThisSend=send(iSock,pchBuf,tLen-iSended,0);
		}while((iThisSend<0)&&(errno==EINTR));		//���͵�ʱ���������ж� 
		if(iThisSend<0){
		//	perror("Mysend error:");
			return iSended;
		}
		iSended+=iThisSend;
		pchBuf+=iThisSend;
	}
	return tLen;
} 

int MyRecv(int iSock,char* pchBuf,size_t tLen){
	int iThisRead;
	unsigned int iReaded=0;
	if(tLen==0)
		return 0;
	while(iReaded<tLen){
		do{
			iThisRead=recv(iSock,pchBuf,tLen-iReaded,0);
		}while((iThisRead<0)&&(errno==EINTR));
		if(iThisRead<0){
		//	perror("MyRecv error");
			return iThisRead;
		}
		else if(iThisRead==0)
			return iReaded;
		iReaded+=iThisRead;
		pchBuf+=iThisRead;
	}
}

void getOper(char a[],char c[]){	//�������Ҫ�ϴ����ļ�·���ָ���ļ���������c�����У����� e://aaa.txt  c=aaa.txt 
	int i=0;
	for(;i<strlen(a)-1;i++){
		if((a[i]=='/'&&a[i+1]!='/')||(a[i]=='\\'&&a[i+1]!='\\'))
			break;
		else 
			c[i]=a[i];
//		cout<<i<<endl;
	}
	if(i==strlen(a)-1){
  //����ļ�����û��\ ����/
		c[i]=a[i];
		return;
	}	
	int t=++i;
	for(;i<strlen(a);i++){
		c[i-t]=a[i];
		c[i+1]='\0';
	}
	c[i]='\0';
//	cout<<"c:"<<c<<endl;
}
 
