#pragma once
#include "ZinxTCP.h"


class GameProtocol;
class GameRole;
class GameChannel :
	public ZinxTcpData
{
public:
	GameChannel(int _fd);
	virtual ~GameChannel();
	virtual AZinxHandler * GetInputNextStage(BytesMsg & _oInput);
	//绑定的协议层和业务层对象
	GameProtocol *mProtocol;
	GameRole *mRole;
};

class GameChannelFact :
	public IZinxTcpConnFact
{
public:
	virtual ZinxTcpData *CreateTcpDataChannel(int _fd);
};
