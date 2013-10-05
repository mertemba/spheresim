/**
 * \file
 * \author Max Mertens <mail@sheepstyle.comeze.com>
 * \section LICENSE
 * Copyright (c) 2013, Max Mertens.
 * All rights reserved.
 * This file is licensed under the "BSD 3-Clause License".
 * Full license text is under the file "LICENSE" provided with this code.
 */

#ifndef _SPHERECALCULATOR_HPP_
#define _SPHERECALCULATOR_HPP_

#include <Sphere.hpp>
#include <Integrators.hpp>
#include <ButcherTableau.hpp>
#include <PhysicalConstants.hpp>

#include <QVector>
#include <QMutex>
#include <QObject>

class QTimer;

namespace SphereSim{
	
	class SimulationWorker;
	class WorkQueue;
	
	/**
	 * \brief Calculates sphere physics.
	 */
	class SphereCalculator : QObject{
		Q_OBJECT
		
	private:
		/** \brief Holds the spheres managed by the server. */
		QVector<Sphere> spheres;
		/** \brief Holds the spheres in an array. */
		Sphere* sphArr;
		/** \brief Holds the current spheres count. */
		quint16 sphCount;
		/** \brief Holds the current box size. */
		Vector3 boxSize;
		/** \brief Holds the current step length (time in s). */
		Scalar timeStep;
		
		/**
		 * \brief Gets the spheres managed by the server.
		 * \return Spheres managed by the server.
		 */
		QVector<Sphere>& getSpheres();
		
		/**
		 * \brief Calculates the current sphere acceleration.
		 * \param sphereIndex Index of the sphere to be calculated.
		 * \param sphere Sphere to be calculated.
		 * \param timeDiff Time difference (in s) used for the movements of other spheres.
		 * \return Calculated current acceleration of the sphere.
		 */
		Vector3 sphereAcceleration(quint16 sphereIndex, Sphere sphere, Scalar timeDiff);
		
		/** \brief Updates local data about spheres. */
		void updateData();
		
		/** \brief Stores which method is used to solve the differential equations. */
		quint8 integratorMethod;
		
		/** \brief Integrates one step of the differential equations using the Runge Kutta method defined by the Butcher tableau. */
		void integrateRungeKuttaStep();
		
		/** \brief Integrates one step of one sphere. */
		/**
		 * \brief Integrates one step of one sphere.
		 * \param sphereIndex Index of the sphere to be integrated.
		 * \param stepLength Current step length (time in s).
		 * \param timeDiff Time difference (in s) used for the movements of other spheres.
		 * \return Number of steps used to integrate.
		 */
		quint32 integrateRungeKuttaStep(quint16 sphereIndex, Scalar stepLength, Scalar timeDiff);
		
		/** \brief Stores the Butcher tableau used in the integrator. */
		ButcherTableau butcherTableau;
		
		/** \brief Stores the calculation steps needed for simulation. */
		quint32 calculationCounter;
		
		/** \brief Stores the calculated steps. */
		quint32 stepCounter;
		
		/** \brief Stores the used physical constants. */
		PhysicalConstants physicalConstants;
		
		/** \brief Updates the E modulus (E*) used for sphere-sphere collisions. */
		void updateSphereSphereE();
		
		/** \brief Updates the E modulus (E*) used for sphere-wall collisions. */
		void updateSphereWallE();
		
		/** \brief Thread used for simulation. */
		QThread* simulationThread;
		
		/** \brief Worker used for simulation. */
		SimulationWorker* simulationWorker;
		
		/** \brief Queue storing outstanding simulation work. */
		WorkQueue* workQueue;
		
		/** \brief Mutex locking the queue status. */
		QMutex* workQueueMutex;
		
		/**
		 * \brief Stops the worker.
		 */
		void stopWorker();
		
	public:
		SphereCalculator();
		~SphereCalculator();
		
		/**
		 * \brief Adds a specific sphere.
		 * \param s Sphere to add.
		 * \return Current sphere count.
		 */
		quint16 addSphere(Sphere s);
		/**
		 * \brief Adds a new sphere.
		 * \return Current sphere count.
		 */
		quint16 addSphere();
		
		/**
		 * \brief Removes the last sphere.
		 * \return Current sphere count.
		 */
		quint16 removeLastSphere();
		/**
		 * \brief Removes a specific sphere.
		 * \param i Index of the sphere to remove.
		 * \return Current sphere count.
		 */
		quint16 removeSphere(quint16 i);
		
		/**
		 * \brief Returns the current sphere count.
		 * \return Current sphere count.
		 */
		quint16 getCount();
		
		/**
		 * \brief Updates a specific sphere.
		 * \param i Index of the sphere to update.
		 * \param s Sphere data to update.
		 * \return Current sphere count.
		 */
		quint16 updateSphere(quint16 i, Sphere s);
		
		/**
		 * \brief Gets a specific sphere.
		 * \param i Index of the sphere to get.
		 * \return Copy of the requested sphere.
		 */
		Sphere getSphere(quint16 i);
		
		/**
		 * \brief Calculates the sphere movements for one step.
		 */
		void doOneStep();
		
		/**
		 * \brief Sets the time step.
		 * \param timeSt Requested time step in seconds.
		 */
		void setTimeStep(Scalar timeSt);
		
		/**
		 * \brief Gets the time step.
		 * \return Requested time step in seconds.
		 */
		Scalar getTimeStep();
		
		/**
		 * \brief Sets the integrator method.
		 * \param integrMethod Requested integrator method.
		 */
		void setIntegratorMethod(quint8 integrMethod);
		
		/**
		 * \brief Gets the integrator method.
		 * \return Requested integrator method.
		 */
		quint8 getIntegratorMethod();
		
		/**
		 * \brief Gets and resets the used calculation steps.
		 * \return Requested used calculation steps.
		 */
		quint32 popCalculationCounter();
		
		/**
		 * \brief Gets and resets the number of calculated steps.
		 * \return Requested number of calculated steps.
		 */
		quint32 popStepCounter();
		
		/**
		 * \brief Calculates the sphere movements for some steps.
		 * \param steps Number of steps to calculate (0 = unlimited).
		 */
		void doSomeSteps(quint32 steps);
		
		/**
		 * \brief Gets the total energy.
		 * \return Requested total energy.
		 */
		Scalar getTotalEnergy();
		
		/**
		 * \brief Sets the sphere E modulus.
		 * \param E_sphere Requested sphere E modulus.
		 */
		void setSphereE(Scalar E_sphere);
		
		/**
		 * \brief Sets the sphere poisson number.
		 * \param poisson_sphere Requested sphere poisson number.
		 */
		void setSpherePoisson(Scalar poisson_sphere);
		
		/**
		 * \brief Sets the wall E modulus.
		 * \param E_wall Requested wall E modulus.
		 */
		void setWallE(Scalar E_wall);
		
		/**
		 * \brief Sets the wall poisson number.
		 * \param poisson_wall Requested wall poisson number.
		 */
		void setWallPoisson(Scalar poisson_wall);
		
		/**
		 * \brief Sets the earth gravity.
		 * \param earthGravity Requested earth gravity.
		 */
		void setEarthGravity(Vector3 earthGravity);
		
		/**
		 * \brief Starts the simulation.
		 */
		void startSimulation();
		
		/**
		 * \brief Stops the simulation.
		 */
		void stopSimulation();
		
		/**
		 * \brief Returns the simulation status.
		 * \return Shows if simulation is running or not.
		 */
		bool getIsSimulating();
		
		friend class SimulationWorker;
		
	signals:
		/** \brief Stops the running simulation. */
		void requestingSimulationStop();
		
		/** \brief Stops and deletes the worker. */
		void requestingWorkerStop();
		
	};
	
}

#endif
