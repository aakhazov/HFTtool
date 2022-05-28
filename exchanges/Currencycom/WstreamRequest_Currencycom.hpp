#ifndef _WSTREAM_REQUEST_CURRENCYCOM_HPP_
#define _WSTREAM_REQUEST_CURRENCYCOM_HPP_

#include "exchanges/base/WstreamRequest.hpp"

// -----------------------------------------------------------------------------

class WstreamRequest_Currencycom : public WstreamRequest {
public:
	explicit WstreamRequest_Currencycom(uint32_t id, uint8_t max_instruments)
		: WstreamRequest(id, max_instruments)
	{
	}

	// Subscription request example:
	// {"destination":"trades.subscribe",
	// "correlationId":"1","payload":{"symbols":["AAVE/ETH","GALA/USDT","IOTX/ETH"]}}
	std::string &make_json() override;
};

#endif // _WSTREAM_REQUEST_CURRENCYCOM_HPP_