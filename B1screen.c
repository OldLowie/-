#include <func.h>

struct msgbuf{//使用消息队列，必须重新声明定义此结构体
    long mtype;
    char mtext[128];
};

//全局定义msgid变量，便于信号处理函数使用
int msgid;

//接收到退出信号，删除消息队列后退出
void safunc(int signum,siginfo_t *p,void *p1)
{
    int ret=msgctl(msgid,IPC_RMID,NULL);//删除消息队列
    if(-1==ret)
    {
        perror("msgctl");
    }
    printf("screen is about to close\n");
    exit(0);
}

//屏幕进程只用于接收和打印信息
int main()
{   
    //创建消息队列
    msgid=msgget(2000,IPC_CREAT|0600);
     if(-1==msgid)
     {
         perror("msgget");
         return -1;
     }
    struct msgbuf buf;//定义接收信息的结构体
    memset(&buf,0,sizeof(buf));
    pid_t B1pid;
    B1pid=getpid();
    buf.mtype=(long)B1pid;
    int ret=msgsnd(msgid,&buf,sizeof(buf),0);
    if(-1==ret)
    {
        perror("msgsnd");
        return -1;
    }//向B发送pid
    

    //使用sigprocmask全程阻塞其他信号
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

    while(1)
    {
        memset(&buf,0,sizeof(buf));//清空结构体
        int ret=msgrcv(msgid,&buf,sizeof(buf),-2,0);//接收信息
        if(-1==ret)
        {
            perror("msgrcv");
            return -1;
        }
        if(1==buf.mtype)//B输入的显示在右边
        {
            printf("%*s%s\n",15,"",buf.mtext);
        }else{//A消息显示在左边
            printf("%s\n",buf.mtext);
        }
    }
    return 0;
}

