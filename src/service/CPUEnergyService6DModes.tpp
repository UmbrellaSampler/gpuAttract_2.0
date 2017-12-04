/*
 * CPUEnergyService6DModes.tpp
 *
 *  Created on: Nov 4, 2017
 *      Author: glenn
 */

#ifndef SRC_CPUENERGYSERVICE6DMODES_TPP_
#define SRC_CPUENERGYSERVICE6DMODES_TPP_

#include <cassert>
#include "WorkerBuffer.h"
#include "DataManager.h"
#include "DataItem.h"
#include "WorkItem.h"
#include "Protein.h"
#include "GridUnion.h"
#include "ParamTable.h"
#include "SimParam.h"


#include "transform.h"
#include "interpolation.h"
#include "neighborlist.h"
#include "reduction_modes.h"
#include "matrixFunctions.h"
#include "RotMat.h"

// ToDo: remove
#include <iostream>

#include "CPUEnergyService6DModes.h"

namespace as {

template<typename REAL>
CPUEnergyService6DModes<REAL>::CPUEnergyService6DModes(std::shared_ptr<DataManager> dataMng) :
	CPUEnergyService<Types_6D_Modes<REAL>>(dataMng)
{}

template<typename REAL>
class CPUEnergyService6DModes<REAL>::Buffer {
public:

	/**
	 * Allocate new Buffers with size. Old buffers are automatically deallocated;
	 */
	void allocateBuffer(size_t size) {
		h_trafoLig = std::move(WorkerBuffer<REAL>(3,size));
		h_potLig = std::move(WorkerBuffer<REAL>(4,size));
	}

	size_t bufferSize() {
		return h_trafoLig.bufferSize();
	}

	WorkerBuffer<REAL> h_trafoLig;
	WorkerBuffer<REAL> h_potLig;
};


template<typename REAL>
auto CPUEnergyService6DModes<REAL>::createItemProcessor() -> itemProcessor_t {

	std::shared_ptr<Buffer> buffers = std::make_shared<Buffer>();

	itemProcessor_t fncObj = [this, buffers] (workItem_t* item) -> bool {
		assert(item->size() > 0);
		const auto itemSize = item->size();

		/* item pointers */
		const auto dofs = item->inputBuffer();
		const auto common = item->common();
		auto results = item->resultBuffer();

		/* get DataItem pointers */
		const auto grid = std::dynamic_pointer_cast<GridUnion<REAL>>(this->_dataMng->get(common->gridIdRec)).get(); // _dataMng->get() returns shared_ptr<DataItem>
		assert(grid != nullptr);

		const auto lig = std::dynamic_pointer_cast<Protein<REAL>>(this->_dataMng->get(common->ligId)).get();
		assert(lig != nullptr);

		const auto rec = std::dynamic_pointer_cast<Protein<REAL>>(this->_dataMng->get(common->recId)).get();
		assert(rec != nullptr);

		const auto table = std::dynamic_pointer_cast<ParamTable<REAL>>(this->_dataMng->get(common->tableId)).get();
		assert(table != nullptr);

		const auto simParams = std::dynamic_pointer_cast<SimParam<REAL>>(this->_dataMng->get(common->paramsId)).get();
		assert(simParams != nullptr);


		if (lig->numAtoms() > buffers->bufferSize()) {
			buffers->allocateBuffer(lig->numAtoms());
		}

//		lig->print(lig->numAtoms());
//		exit(1);

		for (unsigned i = 0; i < itemSize; ++i) {
			const auto& dof = dofs[i];
			auto& enGrad = results[i];


			rotate_translate(
				lig->xPos(),
				lig->yPos(),
				lig->zPos(),
				dof._6D.pos,
				dof._6D.ang,
				lig->numAtoms(),
				lig->numModes(),
				dof.modesLig,
				lig->xModes(),
				lig->yModes(),
				lig->zModes(),
				buffers->h_trafoLig.getX(),//output
				buffers->h_trafoLig.getY(),
				buffers->h_trafoLig.getZ()
			); // OK

			// Debug
//			for(size_t i = 0; i < lig->numAtoms(); ++i) {
////			for(size_t i = 0; i < 20; ++i) {
//				std::cout << buffers->h_trafoLig.getX()[i] << " " << buffers->h_trafoLig.getY()[i] << " " << buffers->h_trafoLig.getZ()[i] << std::endl;
//			}
//			exit(EXIT_SUCCESS);

			potForce(
					grid->inner.get(),
					grid->outer.get(),
					lig,
					buffers->h_trafoLig.getX(),
					buffers->h_trafoLig.getY(),
					buffers->h_trafoLig.getZ(),
					buffers->h_potLig.getX(), // output
					buffers->h_potLig.getY(),
					buffers->h_potLig.getZ(),
					buffers->h_potLig.getW()
			); // OK

//			exit(EXIT_SUCCESS);
//			for(size_t i = 0; i < lig->numAtoms(); ++i) {
//			for(size_t i = 0; i < 20; ++i) {
				//std::cout << buffers->h_potLig.getX()[i] << " " << buffers->h_potLig.getY()[i] << " " << buffers->h_potLig.getZ()[i] << " " << buffers->h_potLig.getW()[i] << std::endl;
//			}
//			exit(EXIT_SUCCESS);

			NLPotForce(
					grid->NL.get(),
					rec,
					lig,
					simParams,
					table,
					buffers->h_trafoLig.getX(),
					buffers->h_trafoLig.getY(),
					buffers->h_trafoLig.getZ(),
					buffers->h_potLig.getX(), // output
					buffers->h_potLig.getY(),
					buffers->h_potLig.getZ(),
					buffers->h_potLig.getW()
			); // OK

////			// Debug
//			for(size_t i = 0; i < lig->numAtoms(); ++i) {
////			for(size_t i = 0; i < 20; ++i) {
//				std::cout << buffers->h_potLig.getX()[i] << " " << buffers->h_potLig.getY()[i] << " " << buffers->h_potLig.getZ()[i] << " " << buffers->h_potLig.getW()[i] << std::endl;
//			}
////			exit(EXIT_SUCCESS);

			PotForce_Modes<REAL> redPotForce = reducePotForce<REAL,PotForce_Modes<REAL>>(
					buffers->h_potLig.getX(),
					buffers->h_potLig.getY(),
					buffers->h_potLig.getZ(),
					buffers->h_potLig.getW(),
					lig->numAtoms()
			); // OK

//			// Debug
//			REAL x = redPotForce.pos.x;
//			REAL y = redPotForce.pos.y;
//			REAL z = redPotForce.pos.z;
//			REAL E = redPotForce.E;
//			std::cout << x << " " << y << " " << z << " " << E << std::endl;


			reduceModeForce(
					dof._6D.ang,
					buffers->h_potLig.getX(),
					buffers->h_potLig.getY(),
					buffers->h_potLig.getZ(),
					lig->xModes(),
					lig->yModes(),
					lig->zModes(),
					lig->numAtoms(),
					lig->numModes(),
					redPotForce.modes
					);

			for( int mode = 0; mode < lig->numModes(); mode++) {
				enGrad.modesLig[mode]=redPotForce.modes[mode];
			}
			enGrad._6D.E = redPotForce.E;
			enGrad._6D.pos = redPotForce.pos;

			enGrad._6D.ang = reduceTorque(
					lig->xPos(),
					lig->yPos(),
					lig->zPos(),
					buffers->h_potLig.getX(),
					buffers->h_potLig.getY(),
					buffers->h_potLig.getZ(),
					lig->numAtoms(),
					dof._6D.ang
			); // OK

		}

		item->setProcessed();

		return false;

	};

	return fncObj;
}

}  // namespace as




#endif /* CPUENERGYSERVICE6DMODES_TPP_ */