#include"threadpool.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<time.h>

//#include "threadpool.h"
#include <unistd.h>
//#include <stdlib.h>
//#include <stdio.h>

//https://www.cnblogs.com/yangang92/p/5485868.html
//https://bbs.csdn.net/topics/390642622

void *thread_routine(void *arg){
	struct timespec abstime;
	int timeout;
	threadpool_t *pool=(threadpool_t *)arg;
	printf("thread %d is starting          poolsize:%d\n",(int)pthread_self(),pool->max_threads);
	
	while(1){
		timeout=0;
		condition_lock(&pool->ready);
		pool->idle++;
		while(pool->first==NULL&&!pool->quit){	//���û������Ҫִ�в����̳߳�û�б�������ȴ� 
			printf("thread %d is waiting      poolsize:%d\n",(int)pthread_self(),pool->max_threads);
			clock_gettime(CLOCK_REALTIME,&abstime);
			abstime.tv_sec+=10;
			int status;
			status=condition_timewait(&pool->ready,&abstime);
			if(status == ETIMEDOUT){
				printf("thread %d wait time out\n",(int)pthread_self());
				timeout=1;
				break;
			}
		}
		
		pool->idle--;
		
		if(pool->first!=NULL){	//���������Ҫִ�еĻ� 
			task_t *t=pool->first;		//����ͷ����ȡ���� 
			pool->first=t->next;		//���ö�ͷָ�� 
			condition_unlock(&pool->ready);	//��ʱ���������ͱ�Ψһһ���߳�ռ���ˣ������ͷ����� 
			t->run(t->arg);		//ִ������ 
			free(t);			//�ǵ��ͷ�ָ�룬��Ȼ���ڴ�й© 
			condition_lock(&pool->ready);	
		}
		
		if(pool->quit && pool->first==NULL){ //�˳��̳߳أ�����û����Ҫִ���� 
			pool->counter--;		//��ǰ�������߳���-1 
			if(pool->counter==0){
				condition_signal(&pool->ready);
			}
			condition_unlock(&pool->ready);
			break;
		}
		if(timeout==1){  //��ʱ�˶���û������Ҫִ�У���ô������ѭ���� ����߳�Ҳ��Ϊ������������������� 
			pool->counter--;
			condition_unlock(&pool->ready);
			break;
		}
		condition_unlock(&pool->ready);
			
	}
	printf("thread %d is exiting\n",(int)pthread_self());
	return NULL;
} 

void threadpool_init(threadpool_t *pool,int threads){
	condition_init(&pool->ready);
	pool->first=NULL;
	pool->last=NULL;
	pool->counter=0;
	pool->idle=0;
	pool->max_threads=threads;
	pool->quit=0;   
}

void threadpool_add_task(threadpool_t *pool,void (*run)(void *arg),void *arg){
	task_t *newtask=(task_t *)malloc(sizeof(task_t));
	newtask->run=run;
	newtask->arg=arg; 
	newtask->next=NULL;	//�¼ӵ�������ڶ�β 
	
	condition_lock(&pool->ready);
	
	if(pool->first==NULL){
		pool->first=newtask;
	}else{
		pool->last->next=newtask;
	}
	pool->last=newtask;
	
	if(pool->idle>0){
		condition_signal(&pool->ready);
	}else if(pool->counter<pool->max_threads){
		pthread_t tid;
		pthread_create(&tid,NULL,thread_routine,pool);
		pool->counter++;
	}
	
	condition_unlock(&pool->ready);
	
} 

void threadpool_destroy(threadpool_t *pool){
	if(pool->quit)
		return;
	
	condition_lock(&pool->ready);
	pool->quit=1;
	if(pool->counter>0){
		if(pool->idle>0)
			condition_broadcast(&pool->ready);
			
		while(pool->counter)
			condition_wait(&pool->ready);
	}
	condition_unlock(&pool->ready);
	condition_destroy(&pool->ready);
}




/*void* mytask(void *arg)
{
	int i=2;
	while(i--){
		printf("thread %d is working on task %d\n", (int)pthread_self(), *(int*)arg);
    	sleep(1);
    	
	} 
    free(arg);
    return NULL;
}

//���Դ���
int main(void)
{
    threadpool_t pool;
    //��ʼ���̳߳أ���������߳�
    threadpool_init(&pool, 3);
    int i;
    //����ʮ������
    for(i=0; i < 10; i++)
    {
        int *arg = malloc(sizeof(int));
        *arg = i;
        threadpool_add_task(&pool, mytask, arg);
        
    }
    threadpool_destroy(&pool);
    return 0;
}
*/





