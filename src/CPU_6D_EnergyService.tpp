/*
 * CPUEnergyService.tpp
 *
 *  Created on: Aug 11, 2016
 *      Author: uwe
 */

#ifndef SRC_CPU6DENERGYSERVICE_TPP_
#define SRC_CPU6DENERGYSERVICE_TPP_

#include "CPU_6D_EnergyService.h"

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
#include "reduction.h"
#include "MatrixFunctions.h"
#include "RotMat.h"

// todo: remove
#include <iostream>

namespace as {

template<typename REAL>
CPU_6D_EnergyService<REAL>::CPU_6D_EnergyService() {
	_d = new Private();
}

template<typename REAL>
CPU_6D_EnergyService<REAL>::~CPU_6D_EnergyService() {
	delete _d;
}


template<typename REAL>
class CPU_6D_EnergyService<REAL>::Private {
public:

	/**
	 * Allocate new Buffers with size. Old buffers are automatically deallocated;
	 */
	void allocateBuffer(size_t size) {
		h_trafoLig = std::move(WorkerBuffer<REAL>(3,size));
		h_potLig = std::move(WorkerBuffer<REAL>(4,size));
	}

	WorkerBuffer<REAL> h_trafoLig;
	WorkerBuffer<REAL> h_potLig;
};


template<typename REAL>
auto CPU_6D_EnergyService<REAL>::createItemProcessor() -> itemProcessor_t {


	itemProcessor_t fncObj = [this] (workItem_t* item) -> bool {
		assert(item->size() > 0);
		const auto itemSize = item->size();

		/* item pointers */
		const auto dofs = item->inputBuffer();
		const auto common = item->common();
		auto results = item->resultBuffer();

		/* get DataItem pointers */
		const auto grid = std::dynamic_pointer_cast<GridUnion<REAL>>(_dataMng->get(common->gridId)).get(); // _dataMng->get() returns shared_ptr<DataItem>
		assert(grid != nullptr);

		const auto lig = std::dynamic_pointer_cast<Protein<REAL>>(_dataMng->get(common->ligId)).get();
		assert(lig != nullptr);

		const auto rec = std::dynamic_pointer_cast<Protein<REAL>>(_dataMng->get(common->recId)).get();
		assert(rec != nullptr);

		const auto table = std::dynamic_pointer_cast<ParamTable<REAL>>(_dataMng->get(common->tableId)).get();
		assert(table != nullptr);

		const auto simParams = std::dynamic_pointer_cast<SimParam<REAL>>(_dataMng->get(common->paramsId)).get();
		assert(simParams != nullptr);


		if (itemSize*lig->numAtoms() > _d->h_trafoLig.bufferSize()) {
			_d->allocateBuffer(lig->numAtoms());
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
					dof.pos,
					dof.ang,
					lig->numAtoms(),
					_d->h_trafoLig.getX(), // output
					_d->h_trafoLig.getY(),
					_d->h_trafoLig.getZ()
			); // OK

			potForce(
					grid->inner.get(),
					grid->outer.get(),
					lig,
					_d->h_trafoLig.getX(),
					_d->h_trafoLig.getY(),
					_d->h_trafoLig.getZ(),
					_d->h_potLig.getX(), // output
					_d->h_potLig.getY(),
					_d->h_potLig.getZ(),
					_d->h_potLig.getV()
			); // OK

			NLPotForce(
					grid->NL.get(),
					rec,
					lig,
					simParams,
					table,
					_d->h_trafoLig.getX(),
					_d->h_trafoLig.getY(),
					_d->h_trafoLig.getZ(),
					_d->h_potLig.getX(), // output
					_d->h_potLig.getY(),
					_d->h_potLig.getZ(),
					_d->h_potLig.getV()
			); // OK

			PotForce<REAL> redPotForce = reducePotForce(
					_d->h_potLig.getX(),
					_d->h_potLig.getY(),
					_d->h_potLig.getZ(),
					_d->h_potLig.getV(),
					lig->numAtoms()
			); // OK


			enGrad.E = redPotForce.E;
			enGrad.pos = redPotForce.pos;

			enGrad.ang = reduceTorque(
					lig->xPos(),
					lig->yPos(),
					lig->zPos(),
					_d->h_potLig.getX(),
					_d->h_potLig.getY(),
					_d->h_potLig.getZ(),
					lig->numAtoms(),
					dof.ang
			); // OK

		}

		item->setProcessed();

		return false;

	};

	return fncObj;
}

}  // namespace as


#endif /* SRC_CPU6DENERGYSERVICE_TPP_ */

// for debugging
//
//			size_t numDispl = lig->numAtoms();
//			for (size_t i = 0; i < numDispl; ++i) {
//				std::cout <<  i << " " << _d->h_potLig.getX()[i] << " " << _d->h_potLig.getY()[i] << " " << _d->h_potLig.getZ()[i] << " " << _d->h_potLig.getV()[i] << std::endl;
//			}
//			exit(1);
//
//			std::cout << enGrad << std::endl;
//			exit(1);
