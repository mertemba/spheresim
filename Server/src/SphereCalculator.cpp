/** \file
 * \author Max Mertens <max.mail@dameweb.de>
 * \section LICENSE
 * Copyright (c) 2013, Max Mertens.
 * All rights reserved.
 * This file is licensed under the "BSD 3-Clause License".
 * Full license text is under the file "LICENSE" provided with this code. */

#include <SphereCalculator.hpp>
#include <SimulationWorker.hpp>
#include <WorkQueue.hpp>
#include <Console.hpp>
#include <SphereTransmit.hpp>

#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <cmath>
#include <omp.h>

using namespace SphereSim;

SphereCalculator::SphereCalculator():cellCount(3), cellCount3((quint32)cellCount*cellCount*cellCount),
	maxSpheresPerCell(10), maxCellsPerSphere(300),
	sphereIndicesInCells(maxSpheresPerCell, cellCount3), cellIndicesOfSpheres(maxCellsPerSphere)
{
	qDebug()<<"SphereCalculator: constructor called";
	qDebug()<<"SphereCalculator: number of OpenMP threads:"<<omp_get_num_threads();
	updateData();
	boxSize = Vector3(1,1,1);
	timeStep = 0.002;
	updateIntegratorMethod(IntegratorMethods::RungeKuttaFehlberg54);
	calculationCounter = 0;
	stepCounter = 0;
	updateSphereE(5000);
	updateSpherePoissonRatio(0.5);
	updateWallE(5000);
	updateWallPoissonRatio(0.5);
	updateEarthGravity(Vector3(0, -9.81, 0));
	collisionDetectionFlag = true;
	
	updateSphereBox();
	
	workQueueMutex = new QMutex();
	workQueue = new WorkQueue(workQueueMutex);
	simulationWorker = new SimulationWorker(this, workQueue);
	simulationThread = new QThread();
	simulationWorker->moveToThread(simulationThread);
	QObject::connect(simulationThread, SIGNAL(started()), simulationWorker, SLOT(work()));
	QObject::connect(simulationWorker, SIGNAL(finished()), simulationThread, SLOT(quit()));
	QObject::connect(simulationWorker, SIGNAL(finished()), simulationWorker, SLOT(deleteLater()));
	QObject::connect(simulationThread, SIGNAL(finished()), simulationThread, SLOT(deleteLater()));
	QObject::connect(this, SIGNAL(requestingSimulationStop()), workQueue, SLOT(stopSimulation()));
	QObject::connect(this, SIGNAL(requestingWorkerStop()), simulationWorker, SLOT(stop()));
	QObject::connect(this, SIGNAL(requestingWorkerStop()), workQueue, SLOT(stop()));
	simulationThread->start();
}

SphereCalculator::~SphereCalculator()
{
	stopWorker();
	while(!simulationWorker->getHasFinished());
}

QVector<Sphere>& SphereCalculator::getSpheres()
{
	return spheres;
}

template <bool detectCollisions>
Vector3 SphereCalculator::sphereAcceleration(quint16 sphereIndex, Sphere sphere, Scalar timeDiff)
{
	Scalar d, force;
	Vector3 _force, acc;

	_force = simulatedSystem.earthGravity*sphere.mass;
	for(quint8 dim = 0; dim<3; dim++)
	{
		if((d = (sphere.radius - sphere.pos(dim))) > 0)
		{
			force = 4.0f/3.0f*simulatedSystem.E_sphere_wall*std::sqrt(sphere.radius*d*d*d);
			_force(dim) += force;
		}
		if((d = (sphere.radius + sphere.pos(dim) - boxSize(dim))) > 0)
		{
			force = 4.0f/3.0f*simulatedSystem.E_sphere_wall*std::sqrt(sphere.radius*d*d*d);
			_force(dim) -= force;
		}
	}
	if(detectCollisions)
	{
		quint32 cellIndex;
		QList<quint16>::const_iterator sphereIterator;
		quint16 sphereIndex2;
		Sphere sphere2;
		bool collided = false;
		for(int i = cellIndicesOfSpheres.getCount(sphereIndex)-1; i>=0; i--)
		{
			cellIndex = cellIndicesOfSpheres[sphereIndex][i];
			for(int j = sphereIndicesInCells.getCount(cellIndex)-1; j>=0; j--)
			{
				sphereIndex2 = sphereIndicesInCells[cellIndex][j];
				if(sphereIndex2 != sphereIndex)
				{
					sphere2 = sphArr[sphereIndex2];
					Vector3 dVec = sphere2.pos - sphere.pos;
					Scalar d = dVec.norm();
					if(d < (sphere2.radius + sphere.radius))
					{
						if(!collided)
						{
							collided = true;
						}
					}
				}
			}
		}
	}
	acc = _force/sphere.mass;
	calculationCounter++;
	return acc;
}

void SphereCalculator::updateData()
{
	sphArr = spheres.data();
	sphCount = spheres.size();
}

void SphereCalculator::integrateRungeKuttaStep()
{
	if(collisionDetectionFlag)
		integrateRungeKuttaStep_internal<true>();
	else
		integrateRungeKuttaStep_internal<false>();
}

template <bool detectCollisions>
void SphereCalculator::integrateRungeKuttaStep_internal()
{
	if(detectCollisions)
	{
		updateSphereBox();
		updateSphereCellLists();
	}
	//#pragma omp parallel for
	for(quint16 sphereIndex = 0; sphereIndex<sphCount; ++sphereIndex)
	{
		integrateRungeKuttaStep_internal<detectCollisions>(sphereIndex, timeStep, 0.0);
	}
	stepCounter++;
}

template <bool detectCollisions>
quint32 SphereCalculator::integrateRungeKuttaStep_internal(quint16 sphereIndex, Scalar stepLength, Scalar timeDiff)
{
	Sphere sphere = sphArr[sphereIndex];
	Sphere origSphere = sphere;
	const quint8 integratorOrder = butcherTableau.order;
	Vector3 k_acc[integratorOrder];
	Vector3 k_speed[integratorOrder];
	
	k_acc[0] = sphereAcceleration<detectCollisions>(sphereIndex, sphere, timeDiff);
	k_speed[0] = sphere.speed;
	for(quint8 n = 1; n<integratorOrder; n++)
	{
		sphere.pos = origSphere.pos;
		for(quint8 j = 0; j<n; j++)
		{
			sphere.pos += stepLength*butcherTableau.a[n][j]*k_speed[j];
		}
		k_acc[n] = sphereAcceleration<detectCollisions>(sphereIndex, sphere, timeDiff);
		
		k_speed[n] = origSphere.speed;
		for(quint8 j = 0; j<n; j++)
		{
			k_speed[n] += stepLength*butcherTableau.a[n][j]*k_acc[j];
		}
	}
	
	Vector3 pos = origSphere.pos;
	Vector3 pos_ = pos;
	Vector3 speed = origSphere.speed;
	Vector3 speed_ = speed;
	for(quint8 j = 0; j<integratorOrder; j++)
	{
		pos += stepLength*butcherTableau.b[j]*k_speed[j];
		pos_ += stepLength*butcherTableau.b_[j]*k_speed[j];
		speed += stepLength*butcherTableau.b[j]*k_acc[j];
		speed_ += stepLength*butcherTableau.b_[j]*k_acc[j];
	}
	
	Scalar error_pos_ = (pos-pos_).norm();
	Scalar error_speed_ = (speed-speed_).norm();
	if(error_pos_>1.0e-06 || error_speed_>1.0e-06)
	{
		quint32 stepCount = 0;
		stepCount += integrateRungeKuttaStep_internal<detectCollisions>(sphereIndex, stepLength/2, timeDiff);
		stepCount += integrateRungeKuttaStep_internal<detectCollisions>(sphereIndex, stepLength/2, timeDiff+(stepLength/2));
		return stepCount;
	}else{
		sphArr[sphereIndex].pos = pos;
		sphArr[sphereIndex].speed = speed;
		sphArr[sphereIndex].acc = (speed-origSphere.speed)/stepLength;
		return 1;
	}
}

quint16 SphereCalculator::removeSphere(quint16 i)
{
	if(getSphereCount()>i)
	{
		spheres.remove(i);
		cellIndicesOfSpheres.changeSize(spheres.count());
		updateData();
	}
	return getSphereCount();
}

void SphereCalculator::stopWorker()
{
	emit requestingWorkerStop();
}

void SphereCalculator::updateSphereSphereE()
{
	simulatedSystem.E_sphere_sphere = 1/(((1-simulatedSystem.poisson_sphere*simulatedSystem.poisson_sphere)/simulatedSystem.E_sphere)
		+((1-simulatedSystem.poisson_sphere*simulatedSystem.poisson_sphere)/simulatedSystem.E_sphere));
}

void SphereCalculator::updateSphereWallE()
{
	simulatedSystem.E_sphere_wall = 1/(((1-simulatedSystem.poisson_sphere*simulatedSystem.poisson_sphere)/simulatedSystem.E_sphere)
		+((1-simulatedSystem.poisson_wall*simulatedSystem.poisson_wall)/simulatedSystem.E_wall));
}

void SphereCalculator::prepareFrameData()
{
	QByteArray frameData;
	QDataStream dataStream(&frameData, QIODevice::WriteOnly);
	//updateData();
	dataStream<<sphCount;
	for(quint16 i = 0; i<sphCount; i++)
	{
		dataStream<<i;
		writeBasicSphereData(dataStream, sphArr[i]);
	}
	emit frameToSend(frameData);
}

void SphereCalculator::updateSphereBox()
{
	if(spheres.count()>0)
	{
		Vector3 pos = spheres[0].pos;
		Scalar radius;
		Vector3 max = pos, min = pos;
		for(quint16 i = 1; i<spheres.count(); i++)
		{
			pos = spheres[i].pos;
			radius = spheres[i].radius;
			if(pos(0)+radius>max(0))
				max(0) = pos(0)+radius;
			if(pos(1)+radius>max(1))
				max(1) = pos(1)+radius;
			if(pos(2)+radius>max(2))
				max(2) = pos(2)+radius;
			if(pos(0)-radius<min(0))
				min(0) = pos(0)-radius;
			if(pos(1)-radius<min(1))
				min(1) = pos(1)-radius;
			if(pos(2)-radius<min(2))
				min(2) = pos(2)-radius;
		}
		sphereBoxSize = max-min;
		sphereBoxPosition = min;
	}else{
		sphereBoxSize = Vector3(0, 0, 0);
		sphereBoxPosition = Vector3(0, 0, 0);
	}
}

void SphereCalculator::updateSphereCellLists()
{
	for(quint32 i = 0; i<cellCount3; i++)
	{
		sphereIndicesInCells.resetCounter(i);
	}
	quint16 indexMinX, indexMinY, indexMinZ;
	quint16 indexMaxX, indexMaxY, indexMaxZ;
	quint32 indexAll;
	Vector3 pos;
	Scalar radius, value;
	for(quint16 i = 0; i<sphCount; i++)
	{
		cellIndicesOfSpheres.resetCounter(i);
		pos = sphArr[i].pos;
		radius = sphArr[i].radius;
		value = (pos(0)-sphereBoxPosition(0)-radius)/sphereBoxSize(0);
		indexMinX = (quint16)(value*cellCount);
		indexMinX = (indexMinX<cellCount?indexMinX:cellCount-1);
		value = (pos(1)-sphereBoxPosition(1)-radius)/sphereBoxSize(1);
		indexMinY = (quint16)(value*cellCount);
		indexMinY = (indexMinY<cellCount?indexMinY:cellCount-1);
		value = (pos(2)-sphereBoxPosition(2)-radius)/sphereBoxSize(2);
		indexMinZ = (quint16)(value*cellCount);
		indexMinZ = (indexMinZ<cellCount?indexMinZ:cellCount-1);
		value = (pos(0)-sphereBoxPosition(0)+radius)/sphereBoxSize(0);
		indexMaxX = (quint16)(value*cellCount);
		indexMaxX = (indexMaxX<cellCount?indexMaxX:cellCount-1);
		indexMaxX = (indexMaxX<indexMinX?indexMinX:indexMaxX);
		value = (pos(1)-sphereBoxPosition(1)+radius)/sphereBoxSize(1);
		indexMaxY = (quint16)(value*cellCount);
		indexMaxY = (indexMaxY<cellCount?indexMaxY:cellCount-1);
		indexMaxY = (indexMaxY<indexMinY?indexMinY:indexMaxY);
		value = (pos(2)-sphereBoxPosition(2)+radius)/sphereBoxSize(2);
		indexMaxZ = (quint16)(value*cellCount);
		indexMaxZ = (indexMaxZ<cellCount?indexMaxZ:cellCount-1);
		indexMaxZ = (indexMaxZ<indexMinZ?indexMinZ:indexMaxZ);
		
		for(quint16 z = indexMinZ; z<=indexMaxZ; z++)
		{
			for(quint16 y = indexMinY; y<=indexMaxY; y++)
			{
				for(quint16 x = indexMinX; x<=indexMaxX; x++)
				{
					indexAll = z*cellCount*cellCount + y*cellCount + x;
					sphereIndicesInCells.addElement(indexAll, i);
					cellIndicesOfSpheres.addElement(i, indexAll);
				}
			}
		}
	}
}

quint16 SphereCalculator::addSphere()
{
	spheres.append(Sphere());
	cellIndicesOfSpheres.changeSize(spheres.count());
	updateData();
	workQueue->sendFrameData();
	return getSphereCount();
}

quint16 SphereCalculator::removeLastSphere()
{
	if(getSphereCount()>0)
	{
		return removeSphere(getSphereCount()-1);
	}else{
		return getSphereCount();
	}
}

quint16 SphereCalculator::updateSphere(quint16 i, Sphere s)
{
	if(getSphereCount()>i)
	{
		spheres[i] = s;
		updateData();
		workQueue->sendFrameData();
	}
	return getSphereCount();
}

quint16 SphereCalculator::getSphereCount()
{
	return spheres.size();
}

Sphere SphereCalculator::getAllSphereData(quint16 i)
{
	if(getSphereCount()>i)
	{
		return spheres[i];
	}else{
		return Sphere();
	}
}

void SphereCalculator::calculateStep()
{
	calculateSomeSteps(1);
}

void SphereCalculator::updateTimeStep(Scalar timeSt)
{
	timeStep = timeSt;
}

Scalar SphereCalculator::getTimeStep()
{
	return timeStep;
}

void SphereCalculator::updateIntegratorMethod(quint8 integrMethod)
{
	integratorMethod = integrMethod;
	
	if(integrMethod == IntegratorMethods::HeunEuler21)
	{
		const Scalar a[4] = 	{0.0,	0.0,
								1.0,	0.0};
		const Scalar b[2] = 	{1/2.0,	1/2.0};
		const Scalar b_[2] = 	{1.0,	0.0};
		const Scalar c[2] = 	{0.0,	1.0};
		butcherTableau = ButcherTableau(2, a, b, b_, c);
	}else if(integrMethod == IntegratorMethods::BogackiShampine32)
	{
		const Scalar a[16] = 	{0.0,	0.0,	0.0,	0.0,
								1/2.0,	0.0,	0.0,	0.0,
								0.0,	3/4.0,	0.0,	0.0,
								2/9.0,	1/3.0,	4/9.0,	0.0};
		const Scalar b[4] = 	{2/9.0,	1/3.0,	4/9.0,	0.0};
		const Scalar b_[4] = 	{7/24.0,	1/4.0,	1/3.0,	1/8.0};
		const Scalar c[4] = 	{0.0,	1/2.0,	3/4.0,	1.0};
		butcherTableau = ButcherTableau(4, a, b, b_, c);
	}else if(integrMethod == IntegratorMethods::CashKarp54)
	{
		const Scalar a[36] = 	{0.0,			0.0,		0.0,			0.0,			0.0,			0.0,
								1/5.0,			0.0,		0.0,			0.0,			0.0,			0.0,
								3/40.0,			9/40.0,		0.0,			0.0,			0.0,			0.0,
								3/10.0,			-9/10.0,	6/5.0,			0.0,			0.0,			0.0,
								-11/54.0,		5/2.0,		-70/27.0,		35/27.0,		0.0,			0.0,
								1631/55296.0,	175/512.0,	575/13824.0,	44275/110592.0,	253/4096.0,		0.0};
		const Scalar b[6] = 	{37/378.0,		0.0,		250/621.0,		125/594.0,		0.0,			512/1771.0};
		const Scalar b_[6] = 	{2825/27648.0,	0.0,		18575/48384.0,	13525/55296.0,	277/14336.0,	1/4.0};
		const Scalar c[6] = 	{0.0,			1/5.0,		3/10.0,			3/5.0,			1.0,			7/8.0};
		butcherTableau = ButcherTableau(6, a, b, b_, c);
	}else if(integrMethod == IntegratorMethods::DormandPrince54)
	{
		const Scalar a[49] = 	{0.0,			0.0,			0.0,			0.0,		0.0,			0.0,		0.0,
								1/5.0,			0.0,			0.0,			0.0,		0.0,			0.0,		0.0,
								3/40.0,			9/40.0,			0.0,			0.0,		0.0,			0.0,		0.0,
								44/45.0,		-56/15.0,		32/9.0,			0.0,		0.0,			0.0,		0.0,
								19372/6561.0,	-25360/2187.0,	64448/6561.0,	-212/729.0,	0.0,			0.0,		0.0,
								9017/3168.0,	-355/33.0,		46732/5247.0,	49/176.0,	-5103/18656.0,	0.0,		0.0,
								35/384.0,		0.0,			500/1113.0,		125/192.0,	-2187/6784.0,	11/84.0,	0.0};
		const Scalar b[7] = 	{35/384.0,		0.0,			500/1113.0,		125/192.0,	-2187/6784.0,	11/84.0,	0.0};
		const Scalar b_[7] = 	{5179/57600.0,	0.0,			7571/16695.0,	393/640.0,	-92097/339200.0,187/2100.0,	1/40.0};
		const Scalar c[7] = 	{0.0,			1/5.0,			3/10.0,			4/5.0,		8/9.0,			1.0,		1.0};
		butcherTableau = ButcherTableau(7, a, b, b_, c);
	}else{
		integratorMethod = IntegratorMethods::RungeKuttaFehlberg54;
		const Scalar a[36] = 	{0.0,			0.0,			0.0,			0.0,			0.0,		0.0,
								1/4.0,			0.0,			0.0,			0.0,			0.0,		0.0,
								3/32.0,			9/32.0,			0.0,			0.0,			0.0,		0.0,
								1932/2197.0,	-7200/2197.0,	7296/2197.0,	0.0,			0.0,		0.0,
								439/216.0,		-8.0,			3680/513.0,		-845/4104.0,	0.0,		0.0,
								-8/27.0,		2.0,			-3544/2565.0,	1859/4104.0,	-11/40.0,	0.0};
		const Scalar b[6] = 	{16/135.0,		0.0,			6656/12825.0,	28561/56430.0,	-9/50.0,	2/55.0};
		const Scalar b_[6] = 	{25/216.0,		0.0,			1408/2565.0,	2197/4104.0,	-1/5.0,		0.0};
		const Scalar c[6] = 	{0.0,			1/4.0,			3/8.0,			12/13.0,		1.0,		1/2.0};
		butcherTableau = ButcherTableau(6, a, b, b_, c);
	}
}

quint8 SphereCalculator::getIntegratorMethod()
{
	return integratorMethod;
}

quint32 SphereCalculator::popCalculationCounter()
{
	if(sphCount>0)
	{
		quint32 counter = calculationCounter/sphCount;
		calculationCounter = 0;
		return counter;
	}else{
		return 0;
	}
}

void SphereCalculator::calculateSomeSteps(quint32 steps)
{
	workQueue->pushSimulationSteps(steps);
}

void SphereCalculator::startSimulation()
{
	calculateSomeSteps(0);
}

void SphereCalculator::stopSimulation()
{
	emit requestingSimulationStop();
}

bool SphereCalculator::getIsSimulating()
{
	return workQueue->getIsSimulating();
}

quint32 SphereCalculator::popStepCounter()
{
	quint32 counter = stepCounter;
	stepCounter = 0;
	return counter;
}

void SphereCalculator::updateFrameSending(bool sendFramesRegularly)
{
	workQueue->updateFrameSending(sendFramesRegularly);
}

void SphereCalculator::updateCollisionDetection(bool detectCollisions)
{
	collisionDetectionFlag = detectCollisions;
}

Scalar SphereCalculator::getTotalEnergy()
{
	Scalar totalEnergy = 0.0, sphereEnergy, d;
	Sphere sphere;
	for(quint16 sphereIndex = 0; sphereIndex<sphCount; ++sphereIndex)
	{
		sphere = sphArr[sphereIndex];
		sphereEnergy = -sphere.mass*simulatedSystem.earthGravity.dot(sphere.pos);
		sphereEnergy += 0.5*sphere.mass*sphere.speed.squaredNorm();
		
		for(quint8 dim = 0; dim<3; dim++)
		{
			if((d = (sphere.radius - sphere.pos(dim))) > 0)
			{
				sphereEnergy += 8.0/15.0*simulatedSystem.E_sphere_wall*sqrt(sphere.radius)*pow(d, 2.5);
			}
			if((d = (sphere.radius + sphere.pos(dim) - boxSize(dim))) > 0)
			{
				sphereEnergy += 8.0/15.0*simulatedSystem.E_sphere_wall*sqrt(sphere.radius)*pow(d, 2.5);
			}
		}
		totalEnergy += sphereEnergy;
	}
	return totalEnergy;
}

void SphereCalculator::updateSphereE(Scalar E_sphere)
{
	simulatedSystem.E_sphere = E_sphere;
	updateSphereSphereE();
	updateSphereWallE();
}

void SphereCalculator::updateSpherePoissonRatio(Scalar poisson_sphere)
{
	simulatedSystem.poisson_sphere = poisson_sphere;
	updateSphereSphereE();
	updateSphereWallE();
}

void SphereCalculator::updateWallE(Scalar E_wall)
{
	simulatedSystem.E_wall = E_wall;
	updateSphereWallE();
}

void SphereCalculator::updateWallPoissonRatio(Scalar poisson_wall)
{
	simulatedSystem.poisson_wall = poisson_wall;
	updateSphereWallE();
}

void SphereCalculator::updateEarthGravity(Vector3 earthGravity)
{
	simulatedSystem.earthGravity = earthGravity;
}
