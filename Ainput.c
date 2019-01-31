#include <func.h>

typedef struct message{//定义消息结构体
    char message[128];
    int type;
}msg,*pmsg;

pid_t Bpid;//全局定义Bpid;
pid_t A1pid;

//接收退出信号，发送给A1和B进程，(关闭管道)，实现有序退出
void safunc(int signum,siginfo_t *p,void *p1)
{
    kill(Bpid,SIGQUIT);//向B发送退出信号
    kill(A1pid,SIGQUIT);//向A1发送退出信号
    printf("process is about to exit\n");
    exit(0);
}

int main()
{
    //连接共享内存，由A1创建初始化
    int shmid;
    shmid=shmget(1000,4096,IPC_CREAT|0600);
    if(-1==shmid)
    {
        perror("shmget");
        return -1;
    }
    pmsg pshm;
    pshm=(pmsg)shmat(shmid,NULL,0);

    //获取信号量，由A1创建初始化
    int semarrid=semget(1000,1,IPC_CREAT|0600);
    if(-1==semarrid)
    {
        perror("semget");
        return -1;
    }
    struct sembuf sopp,sopv;
    sopp.sem_num=0;
    sopp.sem_op=-1;
    sopp.sem_flg=SEM_UNDO;
    sopv.sem_num=0;
    sopv.sem_op=1;
    sopv.sem_flg=SEM_UNDO;//定义pv操作

    //共享内存建立后，获取A1 pid
    while(1)
    {
        if(!strcmp(pshm->message,"start"))
        {
            break;
        }
    }
    A1pid=pshm->type;
    memset(pshm,0,4096);//清空重置共享内存
    
    //使用sigprocmask全程阻塞其它信号
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    sigprocmask(SIG_BLOCK,&set,NULL);

    //使用sigaction捕捉SIGQUIT信号
    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_sigaction=safunc;
    act.sa_flags=SA_SIGINFO;
    sigaction(SIGQUIT,&act,NULL);

    //建立管道链接
    int fdr=open("./1pipe",O_RDONLY);//以只读的方式打开一号管道
    if(-1==fdr)
    {
        perror("open");
        return -1;
    }
    int fdw=open("./2pipe",O_WRONLY);//以只写的方式打开二号管道
    if(-1==fdw)
    {
        perror("open2");
        return -1;
    }
    printf("I am chatA\n");
    pid_t Apid;//链接后互传pid，以便退出时发送信号使用
    Apid=getpid();
    write(fdw,&Apid,sizeof(Apid));
    read(fdr,&Bpid,sizeof(Bpid));
    printf("Bpid=%d,Apid=%d,A1pid=%d\n",Bpid,Apid,A1pid);

    char buf[128]={0};
    int ret;
    fd_set readset;//描述符监控的读集合
    while(1)
    {
        FD_ZERO(&readset);//清空重置结构体readset
        FD_SET(0,&readset);
        FD_SET(fdr,&readset);//readset是传入传出参数，每完成一次监控，内容都会改变
        ret=select(fdr+1,&readset,NULL,NULL,NULL);
        if(ret>0)
        {
            if(FD_ISSET(STDIN_FILENO,&readset))//标准输入可读
            {
                memset(buf,0,sizeof(buf));
                ret=read(STDIN_FILENO,buf,sizeof(buf)-1);
                if(0==ret)
                {
                    printf("connection is broken!\n");
                    break;
                }
                write(fdw,buf,strlen(buf)-1);//写入写端管道
                strcpy(pshm->message,buf);
                pshm->type=1;//写入共享内存,A1读完后会清空重置
                semop(semarrid,&sopv,1);//v操作（+1）
            }
            if(FD_ISSET(fdr,&readset))//读管道可读
            {
                memset(buf,0,sizeof(buf));
                ret=read(fdr,buf,sizeof(buf)-1);
                if(0==ret)
                {
                    printf("connection is broken!\n");
                    break;
                }
                strcpy(pshm->message,buf);
                pshm->type=2;//写入共享内存
                semop(semarrid,&sopv,1);//v操作（+1）
            }
        }
    }
    return 0;
}

