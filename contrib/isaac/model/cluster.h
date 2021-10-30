/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef Cluster_h
#define Cluster_h

#pragma once
// C++ program to find the Number of Subsets that have sum between A and B
#include <vector>
#include <algorithm>
using namespace std;
class Cluster
{
	int m_nNodes, m_upperNodeCapacity;
	int * m_nodeCapacities;//a pointer to array to hold cluster node capacities
public:
	Cluster(int myClusterNodes, int arrayWithNodeCapacities[],  int myUpperNodeCapacity);
	
	int findAndPrintSubsets(int mySet[], int nElements, int lowerLimit, int upperLimit);
	void generateSubsets(int start, int setSize, int myFullSet[], vector< vector<int>>& subsetSumVector2D, vector<int>& subsetSumVector1D);
	void printHalfSetSumArrays(int myHalfSetSize, vector<vector<int>>& myHalfSetArray2D, vector<int>& myHalfSetArray1D);
	//function to sort the 2D vector on basis of a particular column
	//function to sort the 2D vector on basis of a particular column (0 in this case)

};


#endif // Cluster_h