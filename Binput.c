#include <func.h>

struct msgbuf{
    long mtype;
    char mtext[128];
};

pid_t Apid;
pid_t B1pid;

//接收退出信号，发送给A和B1进程，实现有序退出
void safunc(int signum,siginfo_t *p,void *p1)
{
    kill(Apid,SIGQUIT);
    kill(B1pid,SIGQUIT);
    printf("process is about to exit\n");
    exit(0);
}

int main()
{
    //链接消息队列
    int msgid;
    msgid=msgget(2000,IPC_CREAT|0600);
    if(-1==msgid)
    {
        perror("msgget");
        return -1;
    }
    struct msgbuf msgbuf;
    memset(&msgbuf,0,sizeof(msgbuf));
    //消息队列链接后，获取B1 pid
    int ret;
    ret=msgrcv(msgid,&msgbuf,sizeof(msgbuf),0,0);
    if(-1==ret)
    {
        perror("msgrcv");
        return -1;
    }
    B1pid=(pid_t)msgbuf.mtype;//消息队列接收后不用重置清空

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
    int fdw=open("./1pipe",O_WRONLY);//以只写的方式打开一号管道
    if(-1==fdw)
    {
        perror("open1");
        return -1;
    }
    int fdr=open("./2pipe",O_RDONLY);//以只读的方式打开二号管道
    if(-1==fdr)
    {
        perror("open2");
        return -1;
    }
    printf("I am chatB\n");
    pid_t Bpid;//链接后互传pid，以便退出时发送信号使用
    Bpid=getpid();
    write(fdw,&Bpid,sizeof(Bpid));
    read(fdr,&Apid,sizeof(Apid));
    printf("Bpid=%d,Apid=%d,B1pid=%d\n",Bpid,Apid,B1pid);
    
    char buf[128]={0};
    fd_set readset;
    while(1)
    {
        FD_ZERO(&readset);
        FD_SET(0,&readset);
        FD_SET(fdr,&readset);
        ret=select(fdr+1,&readset,NULL,NULL,NULL);
        if(ret>0)
        {
            if(FD_ISSET(0,&readset))//标准输入可读
            {
                memset(buf,0,sizeof(buf));
                ret=read(STDIN_FILENO,buf,sizeof(buf)-1);
                if(0==ret)
                {
                    printf("connection is broken!\n");
                    break;
                }
                write(fdw,buf,strlen(buf)-1);//写入管道
                msgbuf.mtype=1;
                strcpy(msgbuf.mtext,buf);
                msgsnd(msgid,&msgbuf,sizeof(msgbuf),0);//写入队列
            }
            if(FD_ISSET(fdr,&readset))//管道可读
            {
                memset(buf,0,sizeof(buf));
                ret=read(fdr,buf,sizeof(buf)-1);
                if(0==ret)
                {
                    printf("connection is broken\n");
                    break;
                }
                msgbuf.mtype=2;
                strcpy(msgbuf.mtext,buf);
                msgsnd(msgid,&msgbuf,sizeof(msgbuf),0);//写入队列
            }
        }
    }
    return 0;
}

