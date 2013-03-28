#ifdef ENGINE_CPP

#ifndef _EIGEN_CALCULATOR_ENGINE_H_
#define _EIGEN_CALCULATOR_ENGINE_H_

#include "EigenCalculator_eVector.h"

#include <QThread>

#include "Spheres.h"
#include "Calculator.h"
#include "EigenCalculator_QObject.h"
#include "EigenCalculator_Collider.h"

struct Pos {
	int posOfSphere, sphereAtPos;
};

template <int dims, bool _3D_>
class EigenCalculator_Engine: public EigenCalculator_QObject {

protected:
		
	eVector** renderBuffer;
	Sphere* spheres;
	eVector *spheresOldPos, *spheresPos, *spheresSpeed, *spheresForce;
	scalar** both_r;
	int gridSteps;
	scalar gridWidth;
	int **gridIndex;
	
	void doStep();
	bool saveFrame();
	
	void save();
	
	bool isFixed(int i);
	
	void loadConfig(const char* file);
	
	Sphere c;
	
	scalar timeInterval;
	
	void sphereCountChanged_subclass(int i);
	void maxSphereCountChanged_subclass(int i);
	
	void initSphere(int i);
	
	scalar _E;
	
	void calcWallResistance();
	void calcBallResistance();
	void sumUpForces();
	
	void collideBalls(int i, int j);
	
	Pos *posX, *posY, *posZ; //positions of spheres sorted by axes
	void sort(Pos* p, int dim); //sort the spheres
	void calcSortedBallResistance();
	
	int numCells;
	int *cellOfSphere;
	Pos *posCell; //spheres sorted by cell ID
	int *firstSphereInCell; //first sphere in each cell, or -1
	void sortSpheresByCells();
	void calcCellSortedBallResistance();
	void checkCollision(int i, int x, int y, int z, bool sameCell=false);
	inline int calcCellID(int x, int y, int z=0);
	
	int* curveIndices;
	int indexCounter;
	void buildCurveIndices_RowColumn();
	void buildCurveIndices_zOrder();
	void buildCurveIndices_Peano();
	void buildCurveIndices_Hilbert();
	void buildPeanoCurve(int x, int y, int z, int step, int direction);
	
	int pow_int(int a, unsigned int b){
		return (b>=2 ? a*a*pow(a, b-2) : (b == 1 ? a : 1));
	}
	
	char* numSpheresInCell;
	int** spheresInCell;
	void countSpheresPerCell();
	void collideSpheresPerCell();
	char* numCollsPerSphere;
	int** collsPerSphere;
	
	EigenCalculator_Collider<dims,_3D_>* collider;

public:
	EigenCalculator_Engine();
	
	void paintGL(bool b);
	
	void fpsChanged(scalar timeInterval);
	
	Sphere* getSphere(int i);
	Sphere* getDirectSphere(int i);
	
	scalar getTemperature();
	
	void boxSizeChanged();
	void gravityChanged();
	void updateG();
	void updateAirResistance();
	void updateWallResistance();
	void updateEModul();
	void updatePoisson();
	void updateElasticity();
	void updateGridSize();
	
	void loadConfig();
};

#endif  /* _EIGEN_CALCULATOR_ENGINE_H_ */

#endif
