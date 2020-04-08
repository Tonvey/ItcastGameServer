#include "WorldManager.h"
WorldManager WorldManager::smManager;
WorldManager::WorldManager()
{
    mVecWorld.resize(3);
    mVecWorld[1] = new AOI_World(85, 410, 75, 400, 10, 20);
    mVecWorld[1]->worldId = 1;
    mVecWorld[2] = new AOI_World(0, 140, 0, 140, 1, 1);
    mVecWorld[2]->worldId = 2;
}
AOI_World *WorldManager::GetWorld(int id)
{
    if(id<0)
    {
        return nullptr;
    }
    if(id>=(int)mVecWorld.size())
    {
        return nullptr;
    }
    return mVecWorld[id];
}
WorldManager::~WorldManager()
{
    for(auto p : mVecWorld)
    {
        delete p;
    }
}
WorldManager &WorldManager::GetInstance()
{
    return smManager;
}
