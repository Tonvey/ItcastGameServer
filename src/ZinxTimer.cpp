#include "ZinxTimer.h"
#include <iostream>
using namespace std;

#ifdef __ZINX_EPOLL__
#include <sys/timerfd.h>
#include <unistd.h>
bool ZinxTimer::Init()
{
    //初始化的时候创建timer
    m_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    itimerspec spec = { {1,0},{1,0} };
    int ret = timerfd_settime(m_fd, 0, &spec, nullptr);
    return ret == 0;
}
bool ZinxTimer::ReadFd(std::string & _input)
{
    uint64_t overtimes=0;
    int len = read(m_fd, (char*)&overtimes, sizeof(overtimes));
    if (len == sizeof(overtimes))
    {
        //string可以当做一个容器,存储二进制的数据
        _input.append((char*)&overtimes, sizeof(overtimes));
        return true;
    }
    return false;
}
#elif defined(__ZINX_KQUEUE__)
#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>
bool ZinxTimer::Init()
{
    //初始化的时候创建timer
    m_fd = 1;
    return true;
}
bool ZinxTimer::ReadFd(std::string & _input)
{
    uint64_t overtimes=this->m_Timeouts;
    _input.append((char*)&overtimes, sizeof(overtimes));
    return true;
}
#endif

ZinxTimer::ZinxTimer()
    :m_fd(-1)
{
}

ZinxTimer::~ZinxTimer()
{
}

bool ZinxTimer::WriteFd(std::string & _output)
{
    return false;
}

void ZinxTimer::Fini()
{
    if (m_fd >= 0)
    {
        close(m_fd);
    }
}

int ZinxTimer::GetFd()
{
    return m_fd;
}

std::string ZinxTimer::GetChannelInfo()
{
    return "ZinxTimer";
}

AZinxHandler * ZinxTimer::GetInputNextStage(BytesMsg & _oInput)
{
	//ZinxTimer的下一个处理节点,就是指定dilever
	return &ZinxTimerDeliver::GetInstance();
}

ZinxTimerDeliver ZinxTimerDeliver::sm_Instance;
ZinxTimerDeliver::ZinxTimerDeliver()
	:m_TimerWheel(4) //设定轮子默认长度是10
	,m_curr_index(0)
{
}

IZinxMsg * ZinxTimerDeliver::InternalHandle(IZinxMsg & _oInput)
{
	BytesMsg *msg = dynamic_cast<BytesMsg*>(&_oInput);
	//拿到overtimes
	uint64_t overtimes = *(uint64_t*)msg->szData.data();
	//在这里实现时间轮子的轮转

	//要根据超时的次数控制轮子要遍历多少格
	for (int i = 0; i < overtimes; ++i)
	{
		//反注册列表
		list<TimerOutProc*> unregisterList;

		//遍历当前刻度下的所有任务
		for (auto it = m_TimerWheel[m_curr_index].begin(); it != m_TimerWheel[m_curr_index].end(); ++it)
		{
			it->LastCount--;
			if (it->LastCount < 0)
			{
				//执行任务
				it->pProc->Proc();
				unregisterList.push_back(it->pProc);
			}
		}

		for (auto &i : unregisterList) //C++11 遍历容器
		{
			//反注册
			UnregisterProcObject(*i);
			//重新注册
			RegisterProcObject(*i);
		}

		//刻度是需要自增,如果超过了轮子的长度,就应该循环
		m_curr_index = (m_curr_index + 1) % m_TimerWheel.size();

	}


	return nullptr;
}

AZinxHandler * ZinxTimerDeliver::GetNextHandler(IZinxMsg & _oNextMsg)
{
	return nullptr;
}

bool ZinxTimerDeliver::RegisterProcObject(TimerOutProc & _proc)
{
	//时间间隔应该要大于0
	if (_proc.GetTimerSec() <= 0)
	{
		return false;
	}
	//有任务注册进来
	int spec = _proc.GetTimerSec(); //获取该任务多少秒执行一次
	//计算圈数
	int round = spec / m_TimerWheel.size();
	//计算格子下标
	int idx = (m_curr_index + spec) % m_TimerWheel.size();

	if (idx == m_curr_index)
	{
		round -= 1;
	}

	WheelNode node;
	node.pProc = &_proc;
	node.LastCount = round;


	//将节点放到轮子里边
	m_TimerWheel[idx].push_back(node);
	return true;
}

void ZinxTimerDeliver::UnregisterProcObject(TimerOutProc & _proc)
{
	for (auto listIt = m_TimerWheel.begin(); listIt != m_TimerWheel.end(); ++listIt)
	{
		for (auto it = listIt->begin(); it != listIt->end();)
		{
			if (it->pProc == &_proc)
			{
				it = listIt->erase(it);
			}
			else
			{
				++it;
			}

		}
	}
}
