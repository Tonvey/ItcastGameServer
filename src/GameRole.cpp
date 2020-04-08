#include "GameRole.h"
#include "GameMsg.h"
#include "msg.pb.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <ctime>
#include "RandomName.h"
#include <ctime>
#include "WorldManager.h"
using namespace std;

static std::default_random_engine  g_random_creator(time(NULL));

static int gPlayerCount = 0;
GameRole::GameRole()
{
    mLastTalkTimeStamp = 0;
	mPlayerId = ++gPlayerCount;
	//玩家名字 player_1 player_2 ....
	//mPlayerName = string("Player_") + std::to_string(gPlayerCount);
	//每次从姓名池子获取名字
	mPlayerName = RandomName::getInstance().getName();

    mCurrentWorld = WorldManager::GetInstance().GetWorld(1);

	//出生地的指定,比如是 在 100,100
	x = g_random_creator()%20+100;
	y = 0;
	z = g_random_creator()%20+100;
	v = 0;


    hp = 1000;
}


GameRole::~GameRole()
{
	//从实践轮子上注销
	RandomName::getInstance().releaseName(this->mPlayerName);
}

bool GameRole::Init()
{
	//当玩家上线的时候,此时就会触发role init,在这里实现一些初始化的业务

	//1 发送玩家的id号和姓名给玩家
	auto msg = MakeLogonSyncPid();
	//将消息扔到protocol协议层
	ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);

	//2 告诉玩家出生地
	msg = MakeInitPosBroadcast();
	ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);


	//将当前玩家添加到AOI世界
	mCurrentWorld->AddPlayer(this);

	
	//获取周边玩家信息组成一个单独的protobuf消息,发送给自己
	msg = MakeSurPlays();
	ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);

	//3 当前玩家上线
	//从AOI世界获取周边玩家
	auto plist = mCurrentWorld->GetSurPlayers(this);
	for (auto r : plist)
	{
		if (r == this)
			continue;
		auto role = dynamic_cast<GameRole*>(r);
		//3.1 告诉已经在世界的玩家,我上线了
		msg = MakeInitPosBroadcast();//产生当前玩家的出生信息
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)role->mProtocol);

		//3.2 告诉这个上线玩家,本来世界上有xxx玩家
		//msg = role->MakeInitPosBroadcast();//产生当前玩家的出生信息
		//ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);
	}


	return true;
}

UserData * GameRole::ProcMsg(UserData & _poUserData)
{
	//现将userdata 转回到 gamemsg
	auto msg = dynamic_cast<GameMsg*>(&_poUserData);
	//实现对客户端传输过来的protobuf数据进行业务处理
	for (auto &single : msg->mMsgList)
	{
		switch(single->m_MsgType)
		{
		//玩家移动的消息
		case GameSingleTLV::GAME_MSG_NEW_POSTION:
		{
			auto pbmsg = dynamic_cast<pb::Position*>(single->mPbMsg);
			//cout << pbmsg->x() << " " << pbmsg->y() << " " << pbmsg->z() << endl;
			this->ProcNewPosition(
				pbmsg->x(),
				pbmsg->y(),
				pbmsg->z(),
				pbmsg->v()
			);
			break;
		}
		case GameSingleTLV::GAME_MSG_TALK_CONTENT:
		{
			pb::Talk* talkMsg = dynamic_cast<pb::Talk*>(single->mPbMsg);
			string content = talkMsg->content();
			this->ProcTalkContent(content);
			break;
		}
		case GameSingleTLV::GAME_MSG_CHANGE_WORLD:
        {
			auto req = dynamic_cast<pb::ChangeWorldRequest*>(single->mPbMsg);
            ProcChangeWorld(req->srcid(),req->targetid());
			break;
        }
		case GameSingleTLV::GAME_MSG_SKILL_TRIGGER:
            {
                auto trigger = dynamic_cast<pb::SkillTrigger*>(single->mPbMsg);
                ProcSkillTrigger(trigger);
                break;
            }
		case GameSingleTLV::GAME_MSG_SKILL_CONTACT:
            {
                auto contact = dynamic_cast<pb::SkillContact*>(single->mPbMsg);
                ProcSkillContact(contact);
                break;
            }
        default:
            break;
		}
	}
	return nullptr;
}

void GameRole::Fini()
{
	//当玩家下线的时候就会在这里调用
	//告诉这个世界里边所有玩家,我下线了
	auto plist = mCurrentWorld->GetSurPlayers(this);
	for (auto r : plist)
	{
		if (r == this)
			continue;
		auto role = dynamic_cast<GameRole*>(r);
		auto msg = MakeLogoffSyncPid();//产生当前玩家的下线消息
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)role->mProtocol);

	}

	//玩家下线的时候也要从AOI世界清除
	mCurrentWorld->DelPlayer(this);

}

void GameRole::ProcNewPosition(float _x, float _y, float _z, float _v)
{
	//每次玩家移动的时候触发这个函数,在这个函数里边实现AOI变化

	//先判断玩家这次移动有没有改变格子
	if (mCurrentWorld->GridChanged(this, _x, _z))
	{
		//如果格子变动了,此时就在这里实现AOI的变换
		mCurrentWorld->DelPlayer(this);//将当前玩家从旧的格子清理掉
		//1 先获取旧的周围玩家的列表
		auto oldList = mCurrentWorld->GetSurPlayers(this);

		//赋值给自身的成员变量新的坐标
		this->x = _x;
		this->y = _y;
		this->z = _z;
		this->v = _v;

		//2 再获取新的周围玩家的列表
		auto newList = mCurrentWorld->GetSurPlayers(this);

		//处理视野消失
		this->ViewDisappear(oldList, newList);
		//处理视野出现
		this->ViewAppear(oldList, newList);
		
		mCurrentWorld->AddPlayer(this);//将当前玩家添加到新的格子
	}

	//先赋值给自身的成员变量
	this->x = _x;
	this->y = _y;
	this->z = _z;
	this->v = _v;


	//同步当前的位置给AOI世界上周边玩家
	auto plist = mCurrentWorld->GetSurPlayers(this);
	for (auto r : plist)
	{
		if (r == this)
			continue;
		auto role = dynamic_cast<GameRole*>(r);
		auto msg = MakeNewPosBroadcast();
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)role->mProtocol);

	}
}

void GameRole::ProcTalkContent(std::string content)
{
    int currTimeStamp = time(nullptr);
	if (currTimeStamp-mLastTalkTimeStamp<10)
	{
		//不允许发送聊天广播,发送单播给当前客户端
		auto msg = MakeTalkBroadcast("(系统消息:两次聊天消息间隔不能低于10秒)");
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);
	}
	else
	{
        mLastTalkTimeStamp = currTimeStamp;
		//获取世界所有玩家,发送广播消息
		auto roleList = ZinxKernel::Zinx_GetAllRole();
		for (auto r : roleList)
		{
			auto role = dynamic_cast<GameRole*>(r);
			auto msg = MakeTalkBroadcast(content);
			ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)role->mProtocol);
		}
	}
}

void GameRole::ProcChangeWorld(int srcId , int targetId)
{
    if(mCurrentWorld->worldId!=srcId||mCurrentWorld->worldId==targetId)
        return;
    mCurrentWorld->DelPlayer(this);
    auto surList = mCurrentWorld->GetSurPlayers(this);
    //发送下线通知
    for(auto p : surList)
    {
        auto role = dynamic_cast<GameRole*>(p);
        auto msg = MakeLogoffSyncPid();
        ZinxKernel::Zinx_SendOut(*msg,*(Iprotocol*)role->mProtocol);
    }

    //重新生成一个随机出生点
    if(targetId==1)
    {
        x = g_random_creator()%20+100;
        y = 0;
        z = g_random_creator()%20+100;
        v = 0;
    }
    else if(targetId==2)
    {
        x = g_random_creator()%120+10;
        y = 0;
        z = g_random_creator()%120+10;
        v = 0;
    }
    else
    {
        //出错
        exit(-1);
    }
    mCurrentWorld = WorldManager::GetInstance().GetWorld(targetId);
    mCurrentWorld->AddPlayer(this);
    surList = mCurrentWorld->GetSurPlayers(this);
    //发送上线通知
    for(auto p : surList)
    {
        auto role = dynamic_cast<GameRole*>(p);
        auto msg = MakeInitPosBroadcast();
        ZinxKernel::Zinx_SendOut(*msg,*(Iprotocol*)role->mProtocol);
    }
    //发送进入场景确认消息
    {
        auto msg = this->MakeChangeWorldResponse(srcId,targetId);
        ZinxKernel::Zinx_SendOut(*msg,*(Iprotocol*)this->mProtocol);

        msg = MakeLogonSyncPid();
        ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);

        msg = MakeInitPosBroadcast();
        ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);

        msg = this->MakeSurPlays();
        ZinxKernel::Zinx_SendOut(*msg,*(Iprotocol*)this->mProtocol);
    }
}
void GameRole::ProcSkillTrigger(pb::SkillTrigger *trigger)
{
    auto surList = mCurrentWorld->GetSurPlayers(this);
    for(auto p : surList)
    {
        if(p==this)
            continue;
        auto role = dynamic_cast<GameRole*>(p);
        auto msg = MakeSkillTrigger(trigger);
        ZinxKernel::Zinx_SendOut(*msg,*(Iprotocol*)role->mProtocol);
    }
}
void GameRole::ProcSkillContact(pb::SkillContact *contact)
{
    if(contact->srcpid()!=this->mPlayerId)
    {
        //可以用来做开挂检测
        return;
    }
    auto surList = mCurrentWorld->GetSurPlayers(this);
    //产生随机伤害
    int attackVal = 100 + g_random_creator()%50;
    GameRole *targetRole = nullptr;
    for(auto p : surList)
    {
        auto role = dynamic_cast<GameRole*>(p);
        if(role->mPlayerId==contact->targetpid())
        {
            targetRole=role;
            break;
        }
    }
    if(targetRole==nullptr)
    {
        //报错
        return;
    }
    pb::SkillContact *newContact = new pb::SkillContact(*contact);
    auto pos = newContact->mutable_contactpos();
    targetRole->hp-=attackVal;
    pos->set_bloodvalue(targetRole->hp);
    for(auto p : surList)
    {
        auto role = dynamic_cast<GameRole*>(p);
        auto msg = MakeSkillContact(newContact);
        ZinxKernel::Zinx_SendOut(*msg,*(Iprotocol*)role->mProtocol);
    }
    delete newContact;

    if(targetRole->hp<=0)
    {
        targetRole->hp=1000;
        targetRole->ProcChangeWorld(targetRole->mCurrentWorld->worldId,1);
    }

}

void GameRole::ViewDisappear(std::list<AOI_Player*>& oldList, std::list<AOI_Player*>& newList)
{
	//做集合运算,首先要排序,使用stl的排序算法,首先注意容器必须是可以随机访问
	vector<AOI_Player*> vOld(oldList.begin(), oldList.end());
	vector<AOI_Player*> vNew(newList.begin(), newList.end());
	std::sort(vOld.begin(), vOld.end());
	std::sort(vNew.begin(), vNew.end());
	//差集
	vector<AOI_Player*> diff;
	set_difference(vOld.begin(), vOld.end(), vNew.begin(), vNew.end(),
		std::inserter(diff, diff.begin()));
	for (auto r : diff)
	{
		auto role = dynamic_cast<GameRole*>(r);
		//视野消失,双方都要发送一个下线的消息
		auto msg = role->MakeLogoffSyncPid();
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);

		msg = this->MakeLogoffSyncPid();
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)role->mProtocol);
	}
}

void GameRole::ViewAppear(std::list<AOI_Player*>& oldList, std::list<AOI_Player*>& newList)
{
	//做集合运算,首先要排序,使用stl的排序算法,首先注意容器必须是可以随机访问
	vector<AOI_Player*> vOld(oldList.begin(), oldList.end());
	vector<AOI_Player*> vNew(newList.begin(), newList.end());
	std::sort(vOld.begin(), vOld.end());
	std::sort(vNew.begin(), vNew.end());
	//差集
	vector<AOI_Player*> diff;
	set_difference(vNew.begin(), vNew.end(), vOld.begin(), vOld.end(),
		std::inserter(diff, diff.begin()));
	for (auto r : diff)
	{
		auto role = dynamic_cast<GameRole*>(r);
		//视野出现,双方都要发送一个上线的消息
		auto msg = role->MakeInitPosBroadcast();
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)this->mProtocol);

		msg = this->MakeInitPosBroadcast();
		ZinxKernel::Zinx_SendOut(*msg, *(Iprotocol*)role->mProtocol);
	}
}

GameMsg * GameRole::MakeLogonSyncPid()
{
	//在这里构造一个玩家上线同步 id的消息
	auto msg = new GameMsg;
	auto pbMsg = new pb::SyncPid;
	pbMsg->set_pid(this->mPlayerId);
	pbMsg->set_username(this->mPlayerName);
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_LOGON_SYNCPID, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

GameMsg * GameRole::MakeTalkBroadcast(std::string _talkContent)
{
	//创建初始位置的广播消息
	auto msg = new GameMsg;
	auto pbMsg = new pb::BroadCast;
	pbMsg->set_pid(this->mPlayerId);
	pbMsg->set_username(this->mPlayerName);
	pbMsg->set_tp(1);
	//创建一个子对象,自动添加到pbMsg的protobuf对象里边
	pbMsg->set_content(_talkContent);
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_BROADCAST, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

GameMsg * GameRole::MakeInitPosBroadcast()
{
	//创建初始位置的广播消息
	auto msg = new GameMsg;
	auto pbMsg = new pb::BroadCast;
	pbMsg->set_pid(this->mPlayerId);
	pbMsg->set_username(this->mPlayerName);
	pbMsg->set_tp(2);
	//创建一个子对象,自动添加到pbMsg的protobuf对象里边
	auto pos = pbMsg->mutable_p();
	pos->set_x(x);
	pos->set_y(y);
	pos->set_z(z);
	pos->set_v(v);
    pos->set_bloodvalue(hp);
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_BROADCAST, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

//构造一个新的位置消息
GameMsg * GameRole::MakeNewPosBroadcast()
{
	//创建初始位置的广播消息
	auto msg = new GameMsg;
	auto pbMsg = new pb::BroadCast;
	pbMsg->set_pid(this->mPlayerId);
	pbMsg->set_username(this->mPlayerName);
	pbMsg->set_tp(4);
	//创建一个子对象,自动添加到pbMsg的protobuf对象里边
	auto pos = pbMsg->mutable_p();
	pos->set_x(this->x);
	pos->set_y(this->y);
	pos->set_z(this->z);
	pos->set_v(this->v);
    pos->set_bloodvalue(hp);
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_BROADCAST, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

//这个函数是创建一个下线的消息
GameMsg * GameRole::MakeLogoffSyncPid()
{
	auto msg = new GameMsg;
	auto pbMsg = new pb::SyncPid;
	pbMsg->set_pid(this->mPlayerId);
	pbMsg->set_username(this->mPlayerName);
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_LOGOFF_SYNCPID, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

GameMsg * GameRole::MakeSurPlays()
{
	auto msg = new GameMsg;
	auto pbMsg = new pb::SyncPlayers;
	auto surPlayerList = mCurrentWorld->GetSurPlayers(this);
	for (auto r : surPlayerList)
	{
        if(r==this)
            continue;
		auto role = dynamic_cast<GameRole*>(r);
		//数组里边添加一个player同时返回该对象指针
		auto singPlayer = pbMsg->add_ps();
		singPlayer->set_pid(role->mPlayerId);
		singPlayer->set_username(role->mPlayerName);
		//加入一个position对象,同时返回该指针
		auto pos = singPlayer->mutable_p();
		pos->set_x(role->x);
		pos->set_y(role->y);
		pos->set_z(role->z);
		pos->set_v(role->v);
        pos->set_bloodvalue(role->hp);
	}
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_SUR_PLAYER, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

GameMsg * GameRole::MakeChangeWorldResponse(int srcId , int targetId)
{
	auto msg = new GameMsg;
    auto pbMsg = new pb::ChangeWorldResponse;

    pbMsg->set_pid(this->mPlayerId);
    pbMsg->set_changeres(1);
    pbMsg->set_srcid(srcId);
    pbMsg->set_targetid(targetId);
    auto pos = pbMsg->mutable_p();
    pos->set_x(x);
    pos->set_y(y);
    pos->set_z(z);
    pos->set_v(v);
    pos->set_bloodvalue(hp);

	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_CHANGE_WORLD_RESPONSE, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

GameMsg * GameRole::MakeSkillTrigger(pb::SkillTrigger *trigger)
{
	auto msg = new GameMsg;
    auto pbMsg = new pb::SkillTrigger(*trigger);
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_SKILL_BROAD, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}

GameMsg *GameRole::MakeSkillContact(pb::SkillContact *contact)
{
	auto msg = new GameMsg;
    auto pbMsg = new pb::SkillContact(*contact);
	auto single = new GameSingleTLV(GameSingleTLV::GAME_MSG_SKILL_CONTACT_BROAD, pbMsg);
	msg->mMsgList.push_back(single);
	return msg;
}
