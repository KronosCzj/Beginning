#include"define.h"
#include"threadpool.h"
#include<string>
using namespace std;
#define PATH "/home/kronos/c++/getFile/"
threadpool_t pool;			//����̳߳ظ���1.���ܿͻ����ϴ����ļ������ӵ������б� 	2.���ͻ���Ҫ���ص��ļ��������ݰ������������б� 
threadpool_t handler_pool;	//����̳߳ظ��𣺽����������İ������հ����type��fd���з��ͣ��ͻ��������ļ�������д�������ļ����ͻ����ϴ��ļ��� 

void recvdata(int fd,int events,void *arg);		//�������տͻ����ȷ���һ��type���ж��ͻ���Ҫ���еĲ��� 
//void senddata(int fd,int events,void *arg);
void initlistensocket(int efd,short port);		//��ʼ��listenfd 
void eventset(struct my_event* ev,int fd,void (*call_back)(int ,int ,void *),void *arg);	//����event 
void eventadd(int efd,int events,struct my_event* ev);	//��ev->fd�ӵ�g_efd������� ����g_efd����ev->fd 
void eventdel(int efd,struct my_event *ev);		//��ev->fd��g_efd�������ɾ�� ,��g_efd������ev->fd 
void acceptconn(int lfd,int events,void *arg); 	//�������� 
void recvfile(void *arg);			// �ͻ����ϴ��ļ�ʱ�������յ����ݰ����������б� ���̳߳�pool���������� 
void readfilefromlocal(void *arg);		//�ͻ���Ҫ�����ļ�ʱ�������ӷ������˶��ļ�����װ�����ݰ�����������б� ���̳߳�pool���������� 
void handle(void *arg);		//�����������İ������հ����type��fd���з��ͣ��ͻ��������ļ�������д�������ļ����ͻ����ϴ��ļ��� ���̳߳�handler_pool����������  

void recvdata(int fd,int events,void *arg){
	char *type=new char[4];
	int readLen;
	struct my_event *ev=(struct my_event*)arg;

 	printf("recvdate fd:%d\n",fd);
	readLen=MyRecv(ev->fd,type,sizeof(int));		//�ȴӿͻ��˽���type���жϿͻ���Ҫ���еĲ��� 
	if(readLen<0){
		printf("��ȡ����ʧ�ܣ�\n");
		return;
	}
	int itype;
	memcpy(&itype,type,sizeof(int));
	itype=(int)ntohl(itype);
	if(itype==0){			// �ͻ���Ҫ�����ϴ����� 
		eventdel(g_efd,ev);		//ɾ��ev->fd������ͻ���ÿ��һ�����ݰ��ͻᴥ��һ��������� 
		threadpool_add_task(&pool, recvfile, arg);		//�̳߳�pool��������������recvfile 
	}
	else if(itype==1){		//�ͻ���Ҫ�������ز��� 
		eventdel(g_efd,ev);
		threadpool_add_task(&pool, readfilefromlocal, arg);	//�̳߳�pool��������������readfilefromlocal
	}

	delete[] type;
}

void readfilefromlocal(void *arg){
//	printf("excuting readfilefromlocal\n");
	char *recvMsg=new char[RCV_SIZE];
	struct my_event *ev=(struct my_event*)arg;
	int readlen;
	package pack;
	if((readlen=MyRecv(ev->fd,recvMsg,RCV_SIZE))<0)		//�Ƚ���һ���ͻ��˵�һ��������֪���ͻ���Ҫ�����ĸ��ļ� 
	{
		perror("errno:");
		printf("fd:%d readlen:%d\n",ev->fd,readlen);
		return;
	}
//	printf("readlen:%d\n",readlen);
	int iLen=0;
	memcpy(&pack.type,recvMsg+iLen,sizeof(int));			//���������ݶ�һ��һ������ 
	pack.type=ntohl(pack.type);
//	printf("pack.tpye:%d\n",pack.type);
	if(pack.type!=1)
		return;
	iLen+=sizeof(int);
		
//	memcpy(&pack.size,recvMsg+iLen,sizeof(int));
//	pack.size=ntohl(pack.size);
	iLen+=sizeof(int);
	
	
	pack.fd=ntohl(ev->fd);
	iLen+=sizeof(int);

	memcpy(&pack.filename,recvMsg+iLen,sizeof(pack.filename));
	iLen+=sizeof(pack.filename);
//	printf("pack.name:%s\n",pack.filename);
	delete[] recvMsg;
	
	string path=PATH;									
	path+=pack.filename;
	//	printf("path: %s\n",path.c_str());
	const char *p=path.c_str();				//�õ�PATH+filename������·�� 
	char *pBuff=new char[RCV_SIZE];
	struct node n;
	FILE *fp;
	int length;
	if((fp=fopen(p,"rb"))==NULL){		 
		printf("cannot open file!\n");
		return;
	}
	while(!feof(fp)){						//��ʼ���ͻ���Ҫ���صİ���װ�ɰ�������������� 
		memset(pack.buf,0,BUF_SIZE);
		length=fread(pack.buf,sizeof(char),BUF_SIZE,fp);
		pack.size=length;
	//	printf("file length:%d\n",length);
		int iLen=0;
		*(int*)(pBuff+iLen)=(int)htonl(pack.type); //host to net long 
		iLen+=sizeof(int);
		*(int*)(pBuff+iLen)=(int)htonl(pack.size); //host to net long 
		iLen+=sizeof(int);
		*(int*)(pBuff+iLen)=(int)htonl(pack.fd); //host to net long 
		iLen+=sizeof(int);	
		memcpy(pBuff+iLen,pack.filename,NAME_SIZE);
		iLen+=sizeof(pack.filename);
		memcpy(pBuff+iLen,pack.buf,BUF_SIZE);
		iLen+=sizeof(pack.buf);
		
		memcpy(&n.buf,pBuff,RCV_SIZE);
		pthread_mutex_lock(&t_mutex);			//�����̣߳��������б�packtask���в���Ҫ�ӻ����� 
		packtask.push_back(n);
		pthread_mutex_unlock(&t_mutex);
//		usleep(100000);			//��λ��΢���������0.1��������֤���߳�
	}
	delete[] pBuff;
	fclose(fp);
	threadpool_add_task(&handler_pool, handle, arg);		//�������ļ�����󣬵���handle�����������б� 
}

void recvfile(void *arg){					//���ͻ����ϴ������ݰ����������б� 
//	printf("excuting recvfile\n");
	char *recvMsg=new char[RCV_SIZE];
	struct node n;
	int readlen;
	struct my_event *ev=(struct my_event*)arg;
	while((readlen=MyRecv(ev->fd,recvMsg,sizeof(package)))>0){
		memcpy(&n.buf,recvMsg,RCV_SIZE);
		pthread_mutex_lock(&t_mutex);
		packtask.push_back(n); 
		pthread_mutex_unlock(&t_mutex);
//		sleep(1);  ��������û���֤���߳� 
	} 
	threadpool_add_task(&handler_pool, handle, arg);
}

void handle(void *arg){			//�������б���в��� 
	int count=0;
	struct my_event *ev=(struct my_event*)arg;
	while(!packtask.empty()){
	//	printf("excuting handle!\n");
		pthread_mutex_lock(&t_mutex);
		int type;
		struct node n;
		n=packtask.front();
		packtask.pop_front();
		pthread_mutex_unlock(&t_mutex);
		memcpy(&type,n.buf,sizeof(int));
		type=(int)ntohl(type);
	
  	//	printf("type:%d\n",type);
		if(type==0){			//�����Ҫд�������������ļ��� ���ͻ����ϴ��ļ��� 
			struct package pack;
			memset(pack.filename,0,sizeof(pack.filename));
			memset(pack.buf,0,sizeof(pack.buf));
			 int iLen=0;
			memcpy(&pack.type,n.buf+iLen,sizeof(int));
			pack.type=ntohl(pack.type);
			iLen+=sizeof(int);
			
			memcpy(&pack.size,n.buf+iLen,sizeof(int));
			pack.size=ntohl(pack.size);
			iLen+=sizeof(int);
	//		printf("pack.size:%d    ",pack.size);		
	
			memcpy(&pack.fd,n.buf+iLen,sizeof(int));
			pack.fd=ntohl(pack.fd);
			iLen+=sizeof(int);
	
			memcpy(&pack.filename,n.buf+iLen,sizeof(pack.filename));
			iLen+=sizeof(pack.filename);
	//		printf("pack.filename:%s\n",pack.filename);
			
			memcpy(&pack.buf,n.buf+iLen,sizeof(pack.buf));
			iLen+=sizeof(pack.buf);
			
			char part_file[NAME_SIZE];
			memset(part_file,0,sizeof(part_file));
			getOper(pack.filename,part_file);  //�ָ���ļ���������e:\\aaa.txt��part_file=aaa.txt 
			string path=PATH;
			path+=part_file;
	//		printf("path: %s\n",path.c_str());
			const char *p=path.c_str();
			FILE* fp;
			if((fp=fopen(p,"a"))==NULL){
				printf("cannot open file!\n");
				return;
			}
				if(pack.size>0)
			{
//				printf("writing!\n");
				fwrite(pack.buf,sizeof(char),pack.size,fp);
			}
			fflush(fp);
			
		}
		else if(type==1){		//���ͻ��������ļ��� 
//		printf("type=1\n");
			++count;
			int fd;
			int iLen=2*sizeof(int);
			memcpy(&fd,n.buf+iLen,sizeof(int));
  
			if(MySend(fd,n.buf,RCV_SIZE)>0){
	//			usleep(100000);
	//			printf("fd:%d     %d send successfully!\n",fd,count);
			}
		}
	}
	
}

void eventset(struct my_event* ev,int fd,void (*call_back)(int,int,void *),void *arg){
	ev->fd=fd;
	ev->events=0;
	ev->call_back=call_back;
	ev->arg=arg;
	ev->status=0;
	ev->last_active=time(NULL); 
	return;
}

void eventadd(int efd,int events,struct my_event* ev){
	struct epoll_event epv={0,{0}};
	int op;
	epv.data.ptr=ev;
	epv.events=ev->events=events;
	
	if(ev->status==1){
		op=EPOLL_CTL_MOD;
		
	}else{
		op=EPOLL_CTL_ADD;
		ev->status=1;
	}
	
	if(epoll_ctl(g_efd,op,ev->fd,&epv)<0){
		printf("event add failed! fd:%d  events:%d",ev->fd,events);
	}else{
		printf("event add successfully!efd:%d  fd:%d  events:%d\n",efd,ev->fd,events);
	}
	
	return;
	
}

void eventdel(int efd,struct my_event *ev){
	struct epoll_event epv={0,{0}};
	if(ev->status!=1)
		return;
	epv.data.ptr=ev;
	ev->status=0;
	epoll_ctl(efd,EPOLL_CTL_DEL,ev->fd,&epv);
	
	return; 
}

void acceptconn(int lfd,int events,void *arg){
	struct sockaddr_in cliaddr;
	socklen_t len=sizeof(cliaddr);
	int acceptfd,i;
	
	if((acceptfd=accept(lfd,(struct sockaddr*)&cliaddr,&len))==-1){
		perror("accept error:");
		return;
	}else{
		printf("accept a new client:%s,%d\n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
	}
	
	do{
		for(i=0;i<MAX_EVENTS;i++){		//�ҵ�ĳ���������ϵ�g_events[i]
			if(g_events[i].status==0)
				break;
			}
			if(i==MAX_EVENTS){
				printf("cannot accept more client!\n");
				break;
			} 
			
			int flag=0;
			if((flag=fcntl(acceptfd,F_SETFL,O_NONBLOCK))<0){
				printf("fcntl nonblock failed:%s\n ",strerror(errno));
				break;
			}
	//	void eventset(struct my_event* ev,int fd,void (*call_back)(int ,int ,void *),void *arg)		
			eventset(&g_events[i],acceptfd,recvdata,&g_events[i]);   //���¼�ע�ᵽ��������ӵ�cfd�ϵ�
	//  void eventadd(int efd,int events,struct my_event* ev)
			eventadd(g_efd,EPOLLIN,&g_events[i]);				//��g_events[i].fd��Ҳ����cfd����ӵ�g_efd��,��g_efd������ g_events[i].fd
			
	}while(0);
	
}

void initlistensocket(int efd,short port){
	int lfd=socket(AF_INET,SOCK_STREAM,0);
	printf("lfd:%d\n",lfd);
	fcntl(lfd,F_SETFL,O_NONBLOCK);		//��socket����Ϊ������
	
	int reuse=1;
	setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(const void*)&reuse,sizeof(reuse));		//���ö˿���TIME_WAIT״̬�¿�������
	

	
	struct sockaddr_in seraddr;
	memset(&seraddr,0,sizeof(seraddr));
	
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(port);
	seraddr.sin_addr.s_addr=htonl(INADDR_ANY);
	
	if(	bind(lfd,(struct sockaddr*)&seraddr,sizeof(seraddr))<0)
		perror("bind error:");
	else
		printf("bind ok!\n");
	if(listen(lfd,LISNUM)<0)
		perror("listen error:");
	else
		printf("listen ok!\n");

 
	//	void eventset(struct my_event* ev,int fd,void (*call_back)(int ,int ,void *),void *arg)
			eventset(&g_events[MAX_EVENTS],lfd,acceptconn,&g_events[MAX_EVENTS]);		//�Ѽ�����lfd��ֻ��һ����������g_events�����һ��Ԫ���� g_events[MAX_EVENTS]
	//		//  void eventadd(int efd,int events,struct my_event* ev)
				eventadd(efd,EPOLLIN,&g_events[MAX_EVENTS]);			//�� g_events[MAX_EVENTS] ���뵽efd���¼�ΪEPOLLIN 
	
	return;
	
}



int main(int argc,char *argv[]){
	
	unsigned short port=SERV_PORT;
	
	if(argc == 2)
		port=atoi(argv[1]);  //��������������˶˿ڣ��Ͱ�����Ķ˿���
	
	g_efd=epoll_create(MAX_EVENTS+1);
	if(g_efd<0)
		printf("create efd error:%s",strerror(errno));
	else{
		printf("g_efd:%d",g_efd);
	}
	
	initlistensocket(g_efd,port);		//��ʼ������socket
	
	
	threadpool_init(&pool, POOL_NUM);		//��ʼ���̳߳� 
	threadpool_init(&handler_pool, POOL_NUM-1);
	struct epoll_event events[MAX_EVENTS+1];
	
	int checkpos=0,i;
	while(1){		//����ģ�� 
		
		long now=time(NULL);
	/*	for(i=0;i<100;i++,checkpos++){
			if(checkpos==MAX_EVENTS)
				checkpos=0;
			if(g_events[checkpos].status!=1)
				continue;
			
			long duration=now-g_events[checkpos].last_active;		//�ͻ��˲���Ծ��ʱ��
			
			if(duration>=60){						//�������һ���Ӿ͹رտͻ������ӣ�������ռ��Դ 
				close(g_events[checkpos].fd);
				printf("fd=%d timeout",g_events[checkpos].fd);
				eventdel(g_efd,&g_events[checkpos]);
			} 
		}
*/
		
		int nfd=epoll_wait(g_efd,events,MAX_EVENTS+1,-1);		//nfd��ʾ���¼���fd����Ŀ��events�������ں��еõ����¼���epoll_event�ļ��� 
		if(nfd<0){
			printf("epoll_wait error!\n");
			break;
		}
		
		for(i=0;i<nfd;i++){
			struct my_event* ev=(struct my_event *)events[i].data.ptr;
			
			if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
				ev->call_back(ev->fd,events[i].events,ev->arg);
		//	if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
		//		ev->call_back(ev->fd,events[i].events,ev->arg);	
			
		}
		
		
	} 
	threadpool_destroy(&pool);
	threadpool_destroy(&handler_pool);
	return 0;
}


