#ifndef _WSTREAM_REQUEST_HUOBI_HPP_
#define _WSTREAM_REQUEST_HUOBI_HPP_

#include "exchanges/base/WstreamRequest.hpp"

// -----------------------------------------------------------------------------

class WstreamRequest_Huobi : public WstreamRequest {
public:
	explicit WstreamRequest_Huobi(uint32_t id, uint8_t max_instruments)
		: WstreamRequest(id, max_instruments)
	{
	}

	// Subscription request example:
	// {"sub":"market.aaveeth.trade.detail","id":"1"}
	std::string &make_json() override;
};

#endif // _WSTREAM_REQUEST_HUOBI_HPP_