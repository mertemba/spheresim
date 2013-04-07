
#ifndef _EIGEN_CALCULATOR_CELL_SORT_COLLIDER_H_
#define _EIGEN_CALCULATOR_CELL_SORT_COLLIDER_H_

#include "EigenCalculator_Force.h"
#include "EigenCalculator_CellForce.h"
#include "EigenCalculator_PairCollider.h"
#include <cstdio>

template <int dims, bool _3D_>
class EigenCalculator_Engine;

template <int dims, bool _3D_>
class EigenCalculator_CellSortCollider : 
	protected EigenCalculator_CellForce<dims,_3D_>,
	public EigenCalculator_PairCollider<dims,_3D_>{
protected:
	
	void sortSpheresByCells();
	void checkCollision(int i, int x, int y, int z, bool sameCell=false);
	
public:
	EigenCalculator_CellSortCollider(EigenCalculator_Engine<dims,_3D_>* c):
		EigenCalculator_Force<dims,_3D_>(c),
		EigenCalculator_CellForce<dims,_3D_>(c),
		EigenCalculator_PairCollider<dims,_3D_>(c){
	}
	
	virtual void calcForces();
};

#endif  /*_EIGEN_CALCULATOR_CELL_SORT_COLLIDER_H_*/
