#include "zinx.h"
#include "ZinxTCP.h"
#include "ZinxTimer.h"
#include "GameChannel.h"
#include <iostream>
#include "RandomName.h"
#include <unistd.h>
#include <fcntl.h>
#include "msg.pb.h"
#include <sys/wait.h>
using namespace std;

class ProtobufGarbo
{
public:
    ~ProtobufGarbo()
    {
        //清理protobuf的内存泄露
        ::google::protobuf::ShutdownProtobufLibrary();
    }
}gPbGarbo;

class MyShutdown : public TimerOutProc
{
public:
	virtual void Proc()
	{
        //检测当前服务器有没有玩家，没有就关闭服务器
        //对应之后的需求，玩家创建房间，结束之后应该自行关闭房间
        auto roleList = ZinxKernel::Zinx_GetAllRole();
        if(roleList.empty())
        {
            ZinxKernel::Zinx_Exit();
        }
	}
	virtual int GetTimerSec()
	{
		//每30秒执行一次
		return 30;
	}
};


void usage(string argv0)
{
    cerr<<"usage : "<< argv0 << " <debug|daemon>"<<endl;
}


//守护进程+进程监控的实现
void daemon_init()
{
    //1 fork 创建子进程 :目的是作为守护进程
    pid_t pid = fork();
    if(pid>0)
    {
        //父进程退出
        exit(0);
    }
    else if(pid<0)
    {
        //出错就退出
        perror("fork");
        exit(-1);
    }
    //子进程重新创建会话 setsid  转到第二步
    setsid();
    //创建日志文件，重定向到标准输入和标准输出
    int fdLog = open("./game.log",O_CREAT|O_WRONLY,0664);
    if(fdLog<0)
    {
        perror("open");
        exit(-1);
    }
    dup2(fdLog,STDOUT_FILENO);
    dup2(fdLog,STDERR_FILENO);
    int fdNull = open("/dev/null",O_RDONLY,0664);
    if(fdNull<0)
    {
        perror("open");
        exit(-1);
    }
    dup2(fdNull,STDIN_FILENO);

    close(fdLog);
    close(fdNull);

    while(true)
    {
        //2 fork 创建子进程 目的： 实现进程监控
        pid = fork();
        if(pid>0)
        {
            //    父进程：一直监控子进程的退出状态
            //    wait
            int status=-1;
            wait(&status);
            if(status!=0)
            {
                //    如果获取到子进程的退出状态不为0，此时就应该继续重新走步骤2
                continue;
            }
            else
            {
                //    如果获取到的子进程的退出状态是0，那么父进程也正常退出
                exit(0);
            }
        }
        else if(pid<0)
        {
            //出错
            perror("fork");
            exit(-1);
        }

        //子进程：
        //正常跑游戏逻辑 ，让当前函数正常返回即可
        return ;
    }

}

int main(int argc , char **argv)
{
    if(argc!=2)
    {
        usage(argv[0]);
        exit(-1);
    }

    string cmd = argv[1];
    if(cmd=="daemon")
    {
        //守护进程+进程监控
        daemon_init();
    }
    else if(cmd!="debug")
    {
        usage(argv[0]);
        exit(-1);
    }


	//先初始化姓名池子
	RandomName::getInstance().init();

	ZinxKernel::ZinxKernelInit();
	auto timerChannel = new ZinxTimer();

    MyShutdown shudown;
	//注册任务
	//ZinxTimerDeliver::GetInstance().RegisterProcObject(shudown);


	auto listen = new ZinxTCPListen(8899, new GameChannelFact);

	ZinxKernel::Zinx_Add_Channel(*timerChannel);
	ZinxKernel::Zinx_Add_Channel(*listen);
	ZinxKernel::Zinx_Run();
	ZinxKernel::ZinxKernelFini();

	return 0;
}

