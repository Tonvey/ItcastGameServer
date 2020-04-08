#pragma once
#include <string>
#include <deque>
class RandomName
{
public:
	~RandomName();
	
	//获取一个随机的姓名
	std::string getName();
	//归还一个姓名
	void releaseName(std::string name);

	static RandomName &getInstance() { return smInstance; }
	void init();
private:
	RandomName();
	static RandomName smInstance;
	//姓名池子
	std::deque<std::string> mNamePool;
};

