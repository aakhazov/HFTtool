#ifndef _WSTREAM_REQUEST_GATEIO_HPP_
#define _WSTREAM_REQUEST_GATEIO_HPP_

#include "exchanges/base/WstreamRequest.hpp"

// -----------------------------------------------------------------------------

class WstreamRequest_Gateio : public WstreamRequest {
public:
	explicit WstreamRequest_Gateio(uint32_t id, uint8_t max_instruments)
		: WstreamRequest(id, max_instruments)
	{
	}

	// Subscription request example:
	// {"method":"trades.subscribe","params":["AAVE_ETH","GALA_USDT","IOTX_ETH"],"id":1}
	std::string &make_json() override;
};

#endif // _WSTREAM_REQUEST_GATEIO_HPP_