/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef Cluster_h
#define Cluster_h

#include <ctime>
#include <cstdlib>

class Cluster
{
  size_t nNodes;

public:
  int  GenerateClusterNodes (int x);
  int GenerateActiveRelays (size_t cNodes, int x);
};
#endif // Cluster_h