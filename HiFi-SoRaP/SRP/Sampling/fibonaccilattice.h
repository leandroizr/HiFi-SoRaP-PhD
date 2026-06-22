#ifndef FIBONACCILATTICE_H
#define FIBONACCILATTICE_H

#include "SRP/advancedsrp.h"

class FibonacciLattice : public AdvancedSRP
{
public:
	FibonacciLattice();
	void computeSRP(Grid *results) override;
	virtual void computeStepSRP(const vector3& XS, vector3& force, const vector3& V1 = DEFAULT_VEC3,
			const vector3& V2 = DEFAULT_VEC3) override = 0 ;
};

#endif // FIBONACCILATTICE_H
