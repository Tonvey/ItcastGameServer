#pragma once
#include "zinx.h"
#include "GameMsg.h"
#include "AOI_World.h"
//#include "ZinxTimer.h"
class GameChannel;
class GameProtocol;
class GameRole :
	public Irole,
	public AOI_Player
	//public TimerOutProc
{
public:

	//作为定时器的回调
    //virtual void Proc();
    //virtual int GetTimerSec();
	//int mTalkInterval =0 ;//聊天间隔的标记,每秒钟自减
	//AOI player的实现
	virtual int GetX() { return x; }
	virtual int GetY() { return z; }  //AOI世界的Y对应3d游戏世界的z轴

	//记录玩家在游戏里边的三维状态
	float x;
	float y;
	float z;
	//玩家面向东西南北哪个方向
	float v;
    int hp;


	GameRole();
	virtual ~GameRole();

	/*初始化函数，开发者可以重写该方法实现对象相关的初始化，该函数会在role对象添加到zinxkernel时调用*/
	virtual bool Init();

	/*处理信息函数，重写该方法可以对业务数据进行处理，若还需要后续处理流程，则应返回相关数据信息（堆对象）*/
	//协议层处理完的数据会传递到这里
	virtual UserData *ProcMsg(UserData &_poUserData);

	/*去初始化函数，类似初始化函数，该函数会在role对象摘除出zinxkernel时调用*/
	virtual void Fini();

	GameChannel *mChannel;
	GameProtocol *mProtocol;

	//处理玩家移动的消息
	void ProcNewPosition(float _x , float _y , float _z, float _v);
	void ProcTalkContent(std::string content);
	void ProcChangeWorld(int srcId , int targetWorldId);
    void ProcSkillTrigger(pb::SkillTrigger *trigger);
    void ProcSkillContact(pb::SkillContact *contact);
	//处理视野消失和视野出现的函数
	void ViewDisappear(std::list<AOI_Player*> &oldList, std::list<AOI_Player*> &newList);
	void ViewAppear(std::list<AOI_Player*> &oldList, std::list<AOI_Player*> &newList);

    /*创建上线时的id和姓名消息*/
    GameMsg *MakeLogonSyncPid();
    /*创建广播聊天消息*/
    GameMsg *MakeTalkBroadcast(std::string _talkContent);
    /*创建广播出生位置消息*/
    GameMsg *MakeInitPosBroadcast();
    /*创建广播移动后新位置消息*/
    GameMsg *MakeNewPosBroadcast();
    /*创建下线时的id和姓名消息*/
    GameMsg *MakeLogoffSyncPid();
    /*创建周围玩家位置消息*/
    GameMsg *MakeSurPlays();
    /*进入场景确认的消息*/
    GameMsg *MakeChangeWorldResponse(int srcId,int targetId);
    /*技能触发消息*/
    GameMsg *MakeSkillTrigger(pb::SkillTrigger *trigger);
    /*技能碰撞消息*/
    GameMsg *MakeSkillContact(pb::SkillContact *contact);


    AOI_World *mCurrentWorld;

	int mPlayerId;
	std::string mPlayerName;
    int mLastTalkTimeStamp;


};

