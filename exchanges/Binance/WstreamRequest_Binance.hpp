#ifndef _WSTREAM_REQUEST_BINANCE_HPP_
#define _WSTREAM_REQUEST_BINANCE_HPP_

#include "exchanges/base/WstreamRequest.hpp"

class WstreamRequest_Binance : public WstreamRequest {
public:
	explicit WstreamRequest_Binance(uint32_t id, uint8_t max_instruments)
		: WstreamRequest(id, max_instruments)
	{
	}

	// Subscription request example:
	// {"method":"SUBSCRIBE","params":["aaveeth@trade","galausdt@trade","iotxeth@trade"],"id":1}
	std::string &make_json() override;
};

#endif // _WSTREAM_REQUEST_BINANCE_HPP_