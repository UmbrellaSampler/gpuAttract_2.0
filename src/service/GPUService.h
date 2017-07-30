/*
 * GPUService.h
 *
 *  Created on: Sep 8, 2016
 *      Author: uwe
 */

#ifndef SRC_GPUSERVICE_H_
#define SRC_GPUSERVICE_H_

#ifdef CUDA

#include "Service.h"

namespace as {

template<typename GenericTypes>
class GPUService : public Service<GenericTypes> {
protected:
	using service_t = Service<GenericTypes>;
public:

	using typename service_t::input_t;
	using typename service_t::common_t;
	using typename service_t::result_t;

	using typename service_t::workItem_t;
	using typename service_t::itemProcessor_t;
	using typename service_t::distributor_t;

	virtual ~GPUService() {};

	virtual itemProcessor_t createItemProcessor() = 0;

	distributor_t createDistributor() = 0;

	void initAllocators() override;

};

}  // namespace as

#include "GPUService.tpp"

#endif


#endif /* SRC_GPUSERVICE_H_ */
