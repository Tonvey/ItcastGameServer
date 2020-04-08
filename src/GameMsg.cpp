#include "GameMsg.h"
#include "msg.pb.h"


GameMsg::GameMsg()
{
}


GameMsg::~GameMsg()
{
    //释放当前list里边的所有对象
    for (auto &i : mMsgList)
    {
        delete i;
    }
}

std::string GameSingleTLV::serialize()
{
    std::string out;
    if (mPbMsg != nullptr)
    {
        out = mPbMsg->SerializeAsString();
    }
    return out;
}

GameSingleTLV::~GameSingleTLV()
{
    //做到对对象的释放
    if (mPbMsg != nullptr)
    {
        delete mPbMsg;
    }
}

GameSingleTLV::GameSingleTLV(GameMsgType type, std::string content)
    :m_MsgType(type)
{

    //根据不同的消息id,然后反序列化为不同的protobuf对象
    switch (m_MsgType)
    {
    case GAME_MSG_LOGON_SYNCPID:
        {
            mPbMsg = new pb::SyncPid;
            break;
        }
    case GAME_MSG_TALK_CONTENT:
        {
            mPbMsg = new pb::Talk;
            break;
        }
    case GAME_MSG_NEW_POSTION:
        {
            mPbMsg = new pb::Position;
            break;
        }
    case GAME_MSG_SKILL_TRIGGER:
        {
            mPbMsg = new pb::SkillTrigger();
            break;
        }
    case GAME_MSG_SKILL_CONTACT:
        {
            mPbMsg = new pb::SkillContact();
            break;
        }
    case GAME_MSG_CHANGE_WORLD:
        {
            mPbMsg = new pb::ChangeWorldRequest();
            break;
        }
    case GAME_MSG_BROADCAST:
        {
            mPbMsg = new pb::BroadCast;
            break;
        }
    case GAME_MSG_LOGOFF_SYNCPID:
        {
            mPbMsg = new pb::SyncPid;
            break;
        }
    case GAME_MSG_SUR_PLAYER:
        {
            mPbMsg = new pb::SyncPlayers;
            break;
        }
    case GAME_MSG_SKILL_BROAD:
        {
            mPbMsg = new pb::SkillTrigger;
            break;
        }
    case GAME_MSG_SKILL_CONTACT_BROAD:
        {
            mPbMsg = new pb::SkillContact;
            break;
        }
    case GAME_MSG_CHANGE_WORLD_RESPONSE:
        {
            mPbMsg = new pb::ChangeWorldResponse;
            break;
        }
    default:
		break;
	}
	//反序列化
    if(mPbMsg!=nullptr)
    {
        mPbMsg->ParseFromString(content);
    }
}
