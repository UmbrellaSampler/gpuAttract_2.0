/*
 * EnergyService.h
 *
 *  Created on: Aug 12, 2017
 *      Author: uwe
 */

#ifndef SRC_SERVICE_ENERGYSERVICE_H_
#define SRC_SERVICE_ENERGYSERVICE_H_

#include "Service.h"

namespace as {

class DataManager;

template<typename GenericTypes>
class EnergyService : public Service<GenericTypes> {
public:
	using typename Service<GenericTypes>::service_t;

	using typename service_t::itemProcessor_t;
	using typename service_t::distributor_t;

public:


	explicit EnergyService(std::shared_ptr<DataManager> dataMng) {
		_dataMng = dataMng;
	}

	virtual ~EnergyService() {};

	virtual itemProcessor_t createItemProcessor() = 0;

	distributor_t createDistributor() = 0;

	void initAllocators() = 0;

protected:
	std::shared_ptr<DataManager> _dataMng;

};

} // namespace



#endif /* SRC_SERVICE_ENERGYSERVICE_H_ */
