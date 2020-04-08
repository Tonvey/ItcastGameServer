#pragma once
#include "zinx.h"
#include <list>
#include "msg.pb.h"

//这个类是用来存储一个逻辑报文,解决粘包之后拆出来的单独一个业务报文
class GameSingleTLV
{
public:
	//定义逻辑消息的类型
	enum GameMsgType {
		GAME_MSG_LOGON_SYNCPID = 1,
		GAME_MSG_TALK_CONTENT = 2,
		GAME_MSG_NEW_POSTION = 3,
        GAME_MSG_SKILL_TRIGGER = 4,
        GAME_MSG_SKILL_CONTACT = 5,
        GAME_MSG_CHANGE_WORLD = 6,

        GAME_MSG_BROADCAST = 200,
		GAME_MSG_LOGOFF_SYNCPID = 201,
		GAME_MSG_SUR_PLAYER = 202,
        GAME_MSG_SKILL_BROAD = 204,
        GAME_MSG_SKILL_CONTACT_BROAD = 205,
        GAME_MSG_CHANGE_WORLD_RESPONSE = 206,
	} m_MsgType;

	//定义一个父类对象的指针,用来存储不同类型的子类
	::google::protobuf::Message *mPbMsg;

	std::string serialize();

	~GameSingleTLV();
	GameSingleTLV(GameMsgType type, std::string content);
	GameSingleTLV(GameMsgType type, ::google::protobuf::Message *pbmsg) :m_MsgType(type), mPbMsg(pbmsg) {}

};

class GameMsg :
	public UserData
{
public:
	GameMsg();
	virtual ~GameMsg();

	std::list<GameSingleTLV*> mMsgList;
};

