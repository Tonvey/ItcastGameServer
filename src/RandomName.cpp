#include "RandomName.h"
#include <fstream>
#include <algorithm>
using namespace std;
#define FIRST_NAME_FILE "./random_first.txt"
#define LAST_NAME_FILE "./random_last.txt"



RandomName RandomName::smInstance;
std::string RandomName::getName()
{
	//出队,从前面出
	string name = mNamePool.front();
	mNamePool.pop_front();
	return name;
}

void RandomName::releaseName(string name)
{
	//归还名字,入队
	mNamePool.push_back(name);
}

RandomName::RandomName()
{
}

void RandomName::init()
{

	srand(time(NULL));
	//读取姓氏文件以及名字文件
	ifstream first, last;
	first.open(FIRST_NAME_FILE);
	last.open(LAST_NAME_FILE);

	if (first.is_open() && last.is_open())
	{
		string firstName;
		//每次读取一行姓氏
		while (getline(first, firstName))
		{
			//每次读取一行名字
			string lastName;
			while (getline(last, lastName))
			{
				string name = firstName + " " + lastName;
				this->mNamePool.push_back(name);
			}
			//清理一下last文件的末尾状态,让其从头开始读
			last.clear(ios::goodbit);
            last.seekg(ios::beg);
		}
	}
	first.close();
	last.close();

	//洗牌
	random_shuffle(this->mNamePool.begin(), this->mNamePool.end());
}


RandomName::~RandomName()
{
}
