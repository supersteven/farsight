// ############################################################################################################################################################################
#include"ftkVotingGlobal.h"

double nftkVotingGlobal::diffclock(clock_t clock1,clock_t clock2)
{
	double diffticks=clock1-clock2;
	double diffms=(diffticks)/CLOCKS_PER_SEC;
	return diffms;
};

int  nftkVotingGlobal::round_double( double x ) 
{
 int    ix;
 double dx;

 ix = (int) x;
 dx = (double) x - (double) ix;

 if (dx >= 0.5) return(ix+1);
 else return(ix);
};

int  nftkVotingGlobal::round_double2( double x ) 
{
 int    ix;
 double dx;

 ix = (int) x;
 dx = (double) x - (double) ix;

 if (dx >= 0.5) return(ix+1);
 else return(ix);
};