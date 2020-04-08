#include "GameChannel.h"
#include "GameProtocol.h"
#include "GameRole.h"


GameChannel::GameChannel(int _fd)
	:ZinxTcpData(_fd)
{
}

GameChannel::~GameChannel()
{



	//通道层被释放的释放就要协助释放协议层和业务层的对象
	if (mProtocol)
	{
		ZinxKernel::Zinx_Del_Proto(*mProtocol);
		delete mProtocol;
	}
	if (mRole)
	{
		ZinxKernel::Zinx_Del_Role(*mRole);
		delete mRole;
	}
}

AZinxHandler * GameChannel::GetInputNextStage(BytesMsg & _oInput)
{
	//通道层的下一个处理环节是协议层
	return mProtocol;
}

ZinxTcpData * GameChannelFact::CreateTcpDataChannel(int _fd)
{
	//当有新的tcp连接进来的时候除了创建通道层对象也要同时创建
	//协议层和业务层的对象
	auto ch = new GameChannel(_fd);
	auto protocol = new GameProtocol;
	auto role = new GameRole;

	ch->mProtocol = protocol;
	ch->mRole = role;

	protocol->mChannel = ch;
	protocol->mRole = role;

	role->mChannel = ch;
	role->mProtocol = protocol;

	//要调用ZinxKernel 的add role 和 add proto 才会执行 role 和 protocol的init
	ZinxKernel::Zinx_Add_Role(*role);
	ZinxKernel::Zinx_Add_Proto(*protocol);

	return ch ;
}
