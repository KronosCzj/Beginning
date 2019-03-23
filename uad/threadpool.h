#include"condition.h"

typedef struct task{		//�̳߳��еĶ�����Ҫִ�е����� 
	void (*run)(void *args);	//����ָ�룬��Ҫִ�еĺ���
	void *arg;				//����
	struct task *next;		//��������е���һ������ 
}task_t;


typedef struct threadpool{		//�̳߳ؽṹ�� 
	condition_t ready;			//״̬�� ������߳�����һ������Ҫ��ֻ֤��һ���̶߳�ã�������idle,counter���в���ʱ��Ҫ���� 
	task_t *first;				//��������е�һ������ 
	task_t *last;				//������������һ������ 
	int counter;				//�̳߳��������߳���Ŀ �����ڹ������߳����� 
	int idle;					//�̳߳��п����߳��� 
	int max_threads;			//�̳߳�����߳��� 
	int quit;					//�Ƿ�ر��̳߳أ�0���رգ�1�ر� 
}threadpool_t; 

void threadpool_init(threadpool_t *pool,int threads);

void threadpool_add_task(threadpool_t *pool,void (*run)(void *arg),void *arg);

void threadpool_destroy(threadpool_t *pool);



 
