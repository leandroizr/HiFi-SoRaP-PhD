#ifndef RANDOMWALK_H
#define RANDOMWALK_H

#include "SRP/advancedsrp.h"

class RandomWalk : public AdvancedSRP
{
public:
	RandomWalk();
	void computeSRP(Grid *results) override;
};

#endif // RANDOMWALK_H
