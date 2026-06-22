#include "randomwalk.h"

#include "Lib/common.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <cmath>
#include <iomanip>

namespace
{

void computeOrthonormalBasis(const vector3& n, vector3& t1, vector3& t2)
{
	vector3 aux;
	if (std::fabs(n.x) < 0.9)
	{
		aux = vector3(1.0, 0.0, 0.0);
	} else
	{
		aux = vector3(0.0, 1.0, 0.0);
	}

	t1 = Common::normalize(Common::cross(n, aux));
	t2 = Common::normalize(Common::cross(n, t1));
}

}

RandomWalk::RandomWalk()
{
}

void RandomWalk::computeSRP(Grid *results)
{
	int NEL = 180/step_AZ+1;
	int NAZ = 360/step_AZ+1;

	int N = NEL*NAZ;
	std::ofstream outFile("random_walk_"+std::to_string(N)+".txt");

	// Check if the file is open
	if (!outFile) {
		std::cerr << "Error opening the file!" << std::endl;
		return;
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(-1.0, 1.0);

	vector3 XS(dis(gen), dis(gen), dis(gen));
	XS = Common::normalize(XS);
	outFile << std::fixed << std::setprecision(16);

	for (int i = 1; i < N; ++i)
	{
		vector3 V1, V2;
		computeOrthonormalBasis(XS, V1, V2);
		vector3 force;

		computeStepSRP(XS,force,V1,V2);

		outFile << XS.x << ", " << XS.y << ", " << XS.z << ", "
				  << force.x << ", " << force.y << ", " << force.z << ", " << std::endl;

		vector3 step(dis(gen), dis(gen), dis(gen));

		XS += step;
		XS = Common::normalize(XS);

		if(progressBar && progressBar->value()<N) progressBar->setValue(progressBar->value()+1);
		QCoreApplication::processEvents();
		if(stopExecution) return;
	}
	outFile.close();
}
