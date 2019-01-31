#include <func.h>

typedef struct message{
    char message[128];
    int type;
}msg,*pmsg;

int shmid;
int semarrid;

//接收到退出信号，删除shm删除sem再退出
void safunc(int signum,siginfo_t *p,void *p1)
{
    int ret=shmctl(shmid,IPC_RMID,0);//删除共享内存
    if(-1==ret)
    {
        perror("shmctl");
    }
    ret=semctl(semarrid,0,IPC_RMID);//删除信号量
    if(-1==ret)
    {
        perror("semctl");
    }
    printf("screen is about to close\n");
    exit(0);
}

//屏幕进程用于接收和打印信息
int main()
{
    //创建共享内存
    shmid=shmget(1000,4096,IPC_CREAT|0600);
    if(-1==shmid)
    {
        perror("shmget");
        return -1;
    }
    pmsg pshm;
    pshm=(pmsg)shmat(shmid,NULL,0);
    memset(pshm,0,4096);
    
    //创建信号量
    semarrid=semget(1000,1,IPC_CREAT|0600);
    if(-1==semarrid)
    {
        perror("semget");
        return -1;
    }
    semctl(semarrid,0,SETVAL,0);//初始化信号量
    struct sembuf sopp,sopv;
    sopp.sem_num=0;
    sopp.sem_op=-1;
    sopp.sem_flg=SEM_UNDO;
    sopv.sem_num=0;
    sopv.sem_op=1;
    sopv.sem_flg=SEM_UNDO;//定义操作


    pid_t A1pid;
    A1pid=getpid();
    strcpy(pshm->message,"start");
    pshm->type=A1pid;//向A发送pid

    //全局阻塞
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    sigprocmask(SIG_BLOCK,&set,NULL);

    //捕捉退出信号
    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_sigaction=safunc;
    act.sa_flags=SA_SIGINFO;
    sigaction(SIGQUIT,&act,NULL);
    
    msg buf;//定义接收信息结构体

    while(1)
    {
        memset(&buf,0,sizeof(buf));//清空结构体
        semop(semarrid,&sopp,1);//p操作（-1）
        strcpy(buf.message,pshm->message);
        buf.type=pshm->type;//接收消息
        if(1==buf.type)//A的输入在右边
        {
            printf("%*s%s\n",15,"",buf.message);
        }else{//B的消息在左边
            printf("%s\n",buf.message);
        }
    }
    return 0;
}

