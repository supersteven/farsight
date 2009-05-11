#include "Trace.h"
#include "SegInit.h"

#include "myDebug.h"
#include "ColorConsole.h"

Trace::Trace() {
	numNodes = 0;
	TraceID = -1;
	L = 0;

	NodeAID = -1;
	NodeBID = -1;

	dirA[0] = 1;	dirB[0] = 1;
	dirA[1] = 0;	dirB[1] = 0;
	dirA[2] = 0;	dirB[2] = 0;

	fitter = new SegFit();
	

}

Trace::Trace(TVessel* seg, unsigned long tID) {
	TraceID = tID;
	numNodes = 1;
	L = seg->L;
	NodeAID = seg->ID;
	NodeBID = seg->ID;
	for (int i=0; i<3; i++)	{
		dirA[i] = seg->R3[i];
		dirB[i] = -1*seg->R3[i];
	}

	fitter = new SegFit();
	
}


Trace::~Trace() {
}

void Trace::PrintSelf()	{
	std::cout << std::endl << red <<"TraceID: " << this->TraceID << " (" << this->numNodes << " elements)" <<std::endl;
	std::cout << "Terminal Node IDs: [" << this->NodeAID << " , " << this->NodeBID << "]" <<std::endl;
	std::cout << "dirA:[" << this->dirA[0] << ", " << this->dirA[1] << ", " <<this->dirA[2] << "] " \
	          << "dirB:[" << this->dirB[0] << ", " << this->dirB[1] << ", " <<this->dirB[2] << "]" <<std::endl;
	std::cout << "Likelihood: " << this->L/this->numNodes <<std::endl;
	std::cout << white << std::endl;
}



void Trace::UpdateTrace(TVessel* &seg, TVessel* &seg1, const int node)	{

	this->numNodes++;
	//seg->numNbrs = 2;
	//seg->NbrID[1] = seg1->ID;

	//seg1->numNbrs = 1;
	//seg1->NbrID[0] = seg->ID;

	seg->numNbrs++;
	seg->NbrID.push_back(seg1->ID);

	seg1->numNbrs++;
	seg1->NbrID.push_back(seg->ID);


	if (node==1)	{
		this->NodeAID = seg1->ID;
	}
	else {
		this->NodeBID = seg1->ID;
	}
	this->L += seg1->L;
}



TVessel* Trace::Step(TVessel *seg, ImageType3D::Pointer im, unsigned long segID, char direction)	{

	// Donot update trace structure here, because seg1 is still a candidate


	TVessel* seg1 = seg->CopyToNewSegment();
	seg1->ID = segID;


	double stepsize = 0.5*vnl_math_max(seg->a1, seg->a2);

	//S("Checking step size - Start: [" <<seg1->mu[0]<<","<<seg1->mu[1]<<","<<seg1->mu[2]<<"]")
	//S("Checking step size - End: [" <<seg1->mu[0]<<","<<seg1->mu[1]<<","<<seg1->mu[2]<<"]")

	double iterations = 25;
	double AS_RATIO = 1.35;

	//find which of the 2 endnodes to step by comparing seg->ID with NodeAID and NodeBID
	//and advance
	if((seg->ID == this->NodeAID)&&(direction==1))	{
		seg1->mu[0] += this->dirA[0]*stepsize;
		seg1->mu[1] += this->dirA[1]*stepsize;
		seg1->mu[2] += this->dirA[2]*stepsize;
	}
	else if ((seg->ID == this->NodeBID)&&(direction==-1))	{
		seg1->mu[0] += this->dirB[0]*stepsize;
		seg1->mu[1] += this->dirB[1]*stepsize;
		seg1->mu[2] += this->dirB[2]*stepsize;
	}
	else {
		std::cout << "Segment " <<segID << " is not EndNode of Trace " << this->TraceID <<std::endl;
		return NULL;
	}


	bool ret = fitter->fitSE(im, *seg1, iterations, AS_RATIO);
	if (ret==0) {
		std::cout << "Bad fit "<< ret << std::endl;
		seg1->PrintSelf();
		S("fit done")
		seg1 = NULL;
	}
	
	return (seg1);
}


void Trace::Reverse(TVessel * seg)
{
	fitter->reverse(*seg);
	this->dirB[0] = seg->R3[0];
	this->dirB[1] = seg->R3[1];
	this->dirB[2] = seg->R3[2];
}