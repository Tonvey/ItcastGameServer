#include "GameProtocol.h"
#include <iostream>
#include "GameMsg.h"
using namespace std;


GameProtocol::GameProtocol()
{
}


GameProtocol::~GameProtocol()
{
}

UserData * GameProtocol::raw2request(std::string _szInput)
{
	//通道层TCP读取的数据会传输到这里来进行协议解析
	//每次都将新读到的数据追加到上一次的数据
	mLastBuf.append(_szInput);
	GameMsg *gameMsg = nullptr;
	//数据长度至少要大于8才做处理
	while (mLastBuf.size() >= 8)
	{
		// 0x01 0x02 0x03 0x04    ->   0x04030201
		int len =
			mLastBuf[0] |
			mLastBuf[1] << 8 |
			mLastBuf[2] << 16 |
			mLastBuf[3] << 24;
		int msgType =
			mLastBuf[4] |
			mLastBuf[5] << 8 |
			mLastBuf[6] << 16 |
			mLastBuf[7] << 24;
		cout << "len:" << len << " msgType:" << msgType << endl;
		//判断消息内容长度够不够  01000000010000000102000000020000000202
		if (mLastBuf.size() - 8 >= len)
		{
			//从字符串取子串就可以   该子串是protobuf的一个消息
			string msgContent = mLastBuf.substr(8, len);
			//buf要清理掉这个报文
			mLastBuf.erase(0, 8 + len);

			//产生一个SingletTLV
			auto msg = new GameSingleTLV((GameSingleTLV::GameMsgType)msgType, msgContent);
            if(msg->mPbMsg==nullptr)
            {
                continue;
            }

			if (gameMsg == nullptr)
			{
				gameMsg = new GameMsg;
			}
			//将singletlv加到GameMsg
			gameMsg->mMsgList.push_back(msg);
		}
		else
		{
			//剩下长度不够,就不要继续循环等待下一次数据到来再出来
			break;
		}
	}
	//返回值包含多个逻辑消息的列表
	return gameMsg;
}

std::string * GameProtocol::response2raw(UserData & _oUserData)
{
	//protocol 协议层要在这里完成 role业务层 传过来的数据的协议封装
	string *buf = new std::string;
	auto msg = dynamic_cast<GameMsg*>(&_oUserData);
	for (auto &single : msg->mMsgList)
	{
		//获取protobuf消息内容
		string content = single->serialize();
		//消息长度
		//以小端的字节序输出
		int32_t len = content.size();
		buf->push_back((char)(len & 0xff));
		buf->push_back((char)(len>>8 & 0xff));
		buf->push_back((char)(len>>16 & 0xff));
		buf->push_back((char)(len>>24& 0xff));


		int32_t type = single->m_MsgType;
		buf->push_back((char)(type & 0xff));
		buf->push_back((char)(type>>8 & 0xff));
		buf->push_back((char)(type>>16 & 0xff));
		buf->push_back((char)(type>>24& 0xff));

		buf->append(content);
	}

	return buf;
}

Irole * GameProtocol::GetMsgProcessor(UserDataMsg & _oUserDataMsg)
{
	return (Irole*)this->mRole;
}

Ichannel * GameProtocol::GetMsgSender(BytesMsg & _oBytes)
{
	return (Ichannel*)mChannel;
}
