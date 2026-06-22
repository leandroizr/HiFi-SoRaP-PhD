#ifndef ADAPTIVESAMPLING_H
#define ADAPTIVESAMPLING_H

#include "SRP/advancedsrp.h"

class AdaptiveSampling : public AdvancedSRP
{
public:
	AdaptiveSampling();
	void computeSRP(Grid *results) override;
};

#endif // ADAPTIVESAMPLING_H
