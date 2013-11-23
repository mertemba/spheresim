/** \file
 * \author Max Mertens <max.mail@dameweb.de>
 * \section LICENSE
 * Copyright (c) 2013, Max Mertens.
 * All rights reserved.
 * This file is licensed under the "BSD 3-Clause License".
 * Full license text is under the file "LICENSE" provided with this code. */

#ifndef _SPHERECALCULATOR_HPP_
#define _SPHERECALCULATOR_HPP_

#include <Sphere.hpp>
#include <Integrators.hpp>
#include <ButcherTableau.hpp>
#include <SimulatedSystem.hpp>
#include <TwoDimArray.hpp>

#include <QMutex>
#include <QObject>

class QTimer;
class QElapsedTimer;

namespace SphereSim
{
	
	class SimulationWorker;
	class WorkQueue;
	class ActionReceiver;
	
	/** \brief Calculator of sphere movements.
	 */
	class SphereCalculator : public QObject
	{
		Q_OBJECT
		
	private:
		/** \brief Spheres managed by the server. */
		Array<Sphere> spheres;
		
		/** \brief New calculated positions of the spheres. */
		Array<Vector3> newSpherePos;
		
		/** \brief Step length (time in s). */
		Scalar timeStep;
		
		/** \brief Calculate the current sphere acceleration.
		 * \param sphereIndex Index of the sphere to be calculated.
		 * \param sphere Sphere to be calculated.
		 * \param timeDiff Time difference (in s) used for the movements of other spheres.
		 * \return Calculated current acceleration of the sphere. */
		template <bool detectCollisions, bool gravity, bool lennardJonesPotential, bool periodicBoundaries>
		Vector3 sphereAcceleration(quint16 sphereIndex, Sphere sphere, Scalar timeDiff);
		
		/** \brief Method to approximately solve differential equations. */
		quint8 integratorMethod;
		
		/** \brief Integrate one step using the Runge Kutta method defined by the Butcher tableau. */
		void integrateRungeKuttaStep();
		
		/** \copydoc integrateRungeKuttaStep */
		template <bool detectCollisions, bool gravity, bool lennardJonesPotential, bool periodicBoundaries>
		void integrateRungeKuttaStep_internal();
		
		/** \brief Integrates one step of one sphere.
		 * \param sphereIndex Index of the sphere to be integrated.
		 * \param stepLength Current step length (time in s).
		 * \param timeDiff Time difference (in s) used for the movements of other spheres.
		 * \return Number of steps used to integrate. */
		template <bool detectCollisions, bool gravity, bool lennardJonesPotential, bool periodicBoundaries>
		quint32 integrateRungeKuttaStep_internal(quint16 sphereIndex, Scalar stepLength, Scalar timeDiff, quint16 stepDivisionCounter);
		
		/** \brief Butcher tableau used in the integrator. */
		ButcherTableau butcherTableau;
		
		/** \brief Number of calculation steps needed for simulation. */
		quint32 calculationCounter;
		
		/** \brief Number of calculated steps. */
		quint32 stepCounter;
		
		/** \brief Storage for physical constants. */
		SimulatedSystem simulatedSystem;
		
		/** \brief Update the E modulus (E*) used for sphere-sphere collisions. */
		void updateSphereSphereE();
		
		/** \brief Update the E modulus (E*) used for sphere-wall collisions. */
		void updateSphereWallE();
		
		/** \brief Thread used for simulation. */
		QThread* simulationThread;
		
		/** \brief Worker used for simulation. */
		SimulationWorker* simulationWorker;
		
		/** \brief Queue storing outstanding simulation work. */
		WorkQueue* workQueue;
		
		/** \brief Mutex locking the queue status. */
		QMutex* workQueueMutex;
		
		/** \brief Stop the worker. */
		void stopWorker();
		
		/** \brief Remove a specific sphere.
		 * \param i Index of the sphere to remove.
		 * \return Current sphere count. */
		quint16 removeSphere(quint16 i);
		
		/** \brief Prepare sphere data for a frame. */
		void prepareFrameData();
		
		Vector3 sphereBoxSize;
		
		Vector3 sphereBoxPosition;
		
		void updateSphereBox();
		
		const quint16 cellCount;
		
		const quint32 cellCount3;
		
		const quint16 maxSpheresPerCell;
		
		const quint16 maxCellsPerSphere;
		
		TwoDimArray<quint16, true> sphereIndicesInCells;
		
		TwoDimArray<quint32, true> cellIndicesOfSpheres;
		
		void updateSphereCellLists();
		
		bool collisionDetectionFlag;
		
		const quint16 maxCollidingSpheresPerSphere;
		
		TwoDimArray<quint16, true> collidingSpheresPerSphere;
		
		template <bool detectCollisions, bool gravity, bool lennardJonesPotential, bool periodicBoundaries>
		Scalar getTotalEnergy_internal();
		
		bool gravityCalculationFlag;
		
		const quint16 gravityCellCount;
		
		const quint32 gravityCellCount3;
		
		const quint32 gravityAllCellCount;
		
		const quint16 maxSpheresPerGravityCell;
		
		TwoDimArray<quint16, true> sphereIndicesInGravityCells;
		
		Vector3 *massVectorSumPerCell;
		
		Scalar *massSumPerCell;
		
		Vector3 *massCenterPerCell;
		
		Vector3 *gravityCellSizes;
		
		Scalar *gravityCellHalfDiagonalLength;
		
		Vector3 *gravityCellPositions;
		
		const quint16 maxApproximatingCellsPerGravityCell;
		
		TwoDimArray<quint32, true> approximatingCellsPerGravityCell;
		
		TwoDimArray<Vector3, true> approximatingCellsOffsetPerGravityCell;
		
		const quint16 maxPairwiseCellsPerGravityCell;
		
		TwoDimArray<quint32, true> pairwiseCellsPerGravityCell;
		
		void buildGravityCells();
		
		void rebuildGravityCellPairs();
		
		void rebuildGravityCellPairs(quint32 currentCellIndex, quint32 testCellIndex);
		
		quint32 *gravityCellIndexOfSpheres;
		
		void updateGravityCellIndexOfSpheresArray();
		
		void updateSphereGravityCellLists();
		
		void updateGravityCellData();
		
		bool lennardJonesPotentialFlag;
		
		quint16 *sphereCountPerGravityCell;
		
		quint16 maxStepDivision;
		
		Scalar maxStepError;
		
		quint32 lastStepCalculationTime;
		
		QElapsedTimer* elapsedTimer;
		
	public:
		SphereCalculator(ActionReceiver* actRcv);
		~SphereCalculator();
		
		WorkQueue* getWorkQueue();
		
		friend class SimulationWorker;
		
	signals:
		/** \brief Stop the running simulation. */
		void requestingSimulationStop();
		
		/** \brief Stop and delete the worker. */
		void requestingWorkerStop();
		
		/** \brief Send frame to client. */
		void frameToSend(QByteArray frameData);
		
	public slots:
		/** \copydoc SpheresUpdatingActions::addSphere
		 * \return New sphere count. */
		quint16 addSphere();
		
		/** \copydoc SpheresUpdatingActions::removeLastSphere
		 * \return Current sphere count. */
		quint16 removeLastSphere();
		
		/** \copydoc SpheresUpdatingActions::updateSphere
		 * \param i Index of the sphere to update.
		 * \param s Sphere data to update.
		 * \return Current sphere count.
		 */
		quint16 updateSphere(quint16 i, Sphere s);
		
		/** \copydoc SpheresUpdatingActions::getSphereCount
		 * \return Current sphere count.
		 */
		quint16 getSphereCount();
		
		/** \copydoc SpheresUpdatingActions::getAllSphereData
		 * \param i Index of the sphere to get.
		 * \return Copy of the requested sphere. */
		Sphere getAllSphereData(quint16 i);
		
		/** \copydoc SpheresUpdatingActions::addSomeSpheres
		 * \copydetails getSphereCount */
		quint16 addSomeSpheres(quint16 sphCount);
		
		/** \copydoc SpheresUpdatingActions::removeSomeLastSpheres
		 * \copydetails getSphereCount */
		quint16 removeSomeLastSpheres(quint16 sphCount);
		
		/** \copydoc SpheresUpdatingActions::updateSpherePositionsInBox */
		void updateSpherePositionsInBox(Scalar randomDisplacement, Scalar randomSpeed);
		
		/** \copydoc SpheresUpdatingActions::updateAllSpheres
		 * \param s Sphere data to update.
		 * \return Current sphere count.
		 */
		quint16 updateAllSpheres(Sphere s);
		
		/** \copydoc CalculationActions::calculateStep */
		void calculateStep();
		
		/** \copydoc CalculationActions::updateTimeStep
		 * \param timeSt Requested time step in seconds. */
		void updateTimeStep(Scalar timeSt);
		
		/** \copydoc CalculationActions::getTimeStep
		 * \return Requested time step in seconds. */
		Scalar getTimeStep();
		
		/** \copydoc CalculationActions::updateIntegratorMethod
		 * \param integrMethod Requested integrator method. */
		void updateIntegratorMethod(quint8 integrMethod);
		
		/** \copydoc CalculationActions::getIntegratorMethod
		 * \return Requested integrator method. */
		quint8 getIntegratorMethod();
		
		/** \copydoc CalculationActions::popCalculationCounter
		 * \return Requested number of force calculations per sphere. */
		quint32 popCalculationCounter();
		
		/** \copydoc CalculationActions::calculateSomeSteps
		 * \param steps Number of steps to calculate (0 = unlimited). */
		void calculateSomeSteps(quint32 steps);
		
		/** \copydoc CalculationActions::startSimulation */
		void startSimulation();
		
		/** \copydoc CalculationActions::stopSimulation */
		void stopSimulation();
		
		/** \copydoc CalculationActions::getIsSimulating
		 * \return Flag if simulation is running or not. */
		bool getIsSimulating();
		
		/** \copydoc CalculationActions::popStepCounter
		 * \return Requested number of calculated steps. */
		quint32 popStepCounter();
		
		/** \copydoc CalculationActions::updateFrameSending */
		void updateFrameSending(bool sendFramesRegularly);
		
		/** \copydoc CalculationActions::updateCollisionDetection */
		void updateCollisionDetection(bool detectCollisions);
		
		/** \copydoc CalculationActions::updateGravityCalculation */
		void updateGravityCalculation(bool calculateGravity);
		
		/** \copydoc CalculationActions::updateLennardJonesPotentialCalculation */
		void updateLennardJonesPotentialCalculation(bool calculateLennardJonesPotential);
		
		/** \copydoc CalculationActions::updateMaximumStepDivision */
		void updateMaximumStepDivision(quint16 maxStepDivision);
		
		/** \copydoc CalculationActions::updateMaximumStepError */
		void updateMaximumStepError(Scalar maxStepError);
		
		/** \copydoc CalculationActions::getLastStepCalculationTime */
		quint32 getLastStepCalculationTime();
		
		/** \copydoc InformationActions::getTotalEnergy
		 * \return Requested total energy. */
		Scalar getTotalEnergy();
		
		/** \copydoc InformationActions::getKineticEnergy
		 * \return Requested kinetic energy. */
		Scalar getKineticEnergy();
		
		/** \copydoc SimulatedSystemActions::updateSphereE
		 * \param E_sphere Requested sphere E modulus. */
		void updateSphereE(Scalar E_sphere);
		
		/** \copydoc SimulatedSystemActions::updateSpherePoissonRatio
		 * \param poisson_sphere Requested sphere poisson number. */
		void updateSpherePoissonRatio(Scalar poisson_sphere);
		
		/** \copydoc SimulatedSystemActions::updateWallE
		 * \param E_wall Requested wall E modulus. */
		void updateWallE(Scalar E_wall);
		
		/** \copydoc SimulatedSystemActions::updateWallPoissonRatio
		 * \param poisson_wall Requested wall poisson number. */
		void updateWallPoissonRatio(Scalar poisson_wall);
		
		/** \copydoc SimulatedSystemActions::updateEarthGravity
		 * \param earthGravity Requested earth gravity. */
		void updateEarthGravity(Vector3 earthGravity);
		
		/** \copydoc SimulatedSystemActions::updateGravitationalConstant
		 * \param G Gravitational constant G. */
		void updateGravitationalConstant(Scalar G);
		
		/** \copydoc SimulatedSystemActions::updateBoxSize
		 * \param boxSize Requested box size. */
		void updateBoxSize(Vector3 boxSize);
		
		/** \copydoc SimulatedSystemActions::updateKineticEnergy
		 * \param factor Kinetic energy scale factor. */
		void updateKineticEnergy(Scalar factor);
		
		/** \copydoc SimulatedSystemActions::updateTargetTemperature */
		void updateTargetTemperature(Scalar targetTemperature);
		
		/** \copydoc SimulatedSystemActions::updatePeriodicBoundaryConditions */
		void updatePeriodicBoundaryConditions(bool periodicBoundaryConditions);
		
	};
	
}

#endif
