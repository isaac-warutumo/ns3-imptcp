#include "cluster.h"
#include <ctime>
#include <cstdlib>

int Cluster::GenerateClusterNodes(int x){
    srand(time(0)+x);
    nNodes= 1+(rand() % 10);
    if(nNodes<3){
        nNodes=3;
    }
    return nNodes;
}

int Cluster::GenerateActiveRelays(size_t cNodes, int x) {
    srand(time(0)+x);
    size_t activeRelay= 1 + (rand() % (cNodes));
    
    if (activeRelay>=4)
    {
        activeRelay = 4;
    }
    if(activeRelay<2){
        activeRelay=2;
    }
    return activeRelay;
}