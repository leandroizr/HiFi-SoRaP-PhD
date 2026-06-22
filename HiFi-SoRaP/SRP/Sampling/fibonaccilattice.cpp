#include "fibonaccilattice.h"

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

FibonacciLattice::FibonacciLattice()
{
}

void FibonacciLattice::computeSRP(Grid *results)
{
	int NEL = 180/step_AZ+1;
	int NAZ = 360/step_AZ+1;

	int N = NEL*NAZ;
	std::ofstream outFile("fibonacci_lattice_"+std::to_string(N)+".txt");

	// Check if the file is open
	if (!outFile) {
		std::cerr << "Error opening the file!" << std::endl;
		return;
	}
	outFile << std::fixed << std::setprecision(16);

	const double golden_ratio = (1.0 + std::sqrt(5.0)) / 2.0;

	for (int i = 0; i < N; ++i)
	{
		// Latitud (phi)
		double phi = std::acos(1.0 - 2.0 * (i + 0.5) / N);  // latitud entre -pi/2 y pi/2

		// Longitud (theta)
		double theta = 2.0 * M_PI * golden_ratio * i;  // longitud
		//vector3 RS, V1, V2, XS;
		// Coordenadas cartesianas
		vector3 RS(sin(phi)*cos(theta),  sin(phi)*sin(theta),   cos(phi));
		auto XS = -RS;
		vector3 V1(cos(phi)*cos(theta),  cos(phi) * sin(theta), -sin(phi));
		vector3 V2(-sin(theta),  cos(theta), 0.0);
		//RS[0] =  cos(phi)*cos(theta); RS[1] = cos(phi)*sin(theta);  RS[2] = sin(phi);
		//V1[0] = -sin(phi)*cos(theta); V1[1] = -sin(phi)*sin(theta); V1[2] = cos(phi);
		//V2[0] =  sin(theta); 	   	  V2[1] = -cos(theta); 	 	 	V2[2] = 0;
		//XS[0] = -RS[0]; 	 	 	  XS[1] = -RS[1];   			XS[2] = -RS[2];

		vector3 force;

		computeStepSRP(XS,force,V1,V2);

		outFile << XS.x << ", " << XS.y << ", " << XS.z << ", "
				  << force.x << ", " << force.y << ", " << force.z << ", " << std::endl;

		if(progressBar && progressBar->value()<N) progressBar->setValue(progressBar->value()+1);
		QCoreApplication::processEvents();
		if(stopExecution) return;
	}
	outFile.close();
}
