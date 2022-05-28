#ifndef _WSTREAM_RESPONSE_CURRENCYCOM_HPP_
#define _WSTREAM_RESPONSE_CURRENCYCOM_HPP_

#include "exchanges/base/WstreamResponse.hpp"

class WstreamResponse_Currencycom : public WstreamResponse<WstreamResponse_Currencycom> {
public:
	static constexpr const char *exchange_name_ = "Currencycom";

public:
	static auto response_specific(boost::property_tree::ptree const &json_tree_root)
	{
		uint32_t id = 0;

		// ----- Parse response -----
		// Response example:
		// {"status":"OK","destination":"trades.subscribe","correlationId":"1","payload":
		// {"subscriptions":{"BAL/USDT":"PROCESSED","LRC/BTC":"PROCESSED",
		// "UNI/USDT":"PROCESSED","HOT/USDT":"PROCESSED","UMA/USDT":"PROCESSED",
		// "CRV/BTC":"PROCESSED","ETH/BTC":"PROCESSED","LTC/BTC":"PROCESSED",
		// "LRC/USDT":"PROCESSED","XRP/USDT":"PROCESSED"}}}

		std::string status = json_tree_root.get<std::string>("status");

		if (status == "OK") {
			id = json_tree_root.get<uint32_t>("correlationId");
		}

		return id;
	}

	// -------------------------------------------------------------------------

	static auto trade_detail_specific(boost::property_tree::ptree const &json_tree_root)
	{
		auto ticks = std::make_unique<std::vector<std::shared_ptr<Tick>>>();

		// ----- Parse price -----
		// trade.detail update example:
		// ../../__dev_data/CURRENCYCOM_trade_details.json

		std::string status = json_tree_root.get<std::string>("status");
		std::string destination = json_tree_root.get<std::string>("destination");

		if (status == "OK" && destination == "internal.trade") {
			boost::property_tree::ptree payload = json_tree_root.get_child("payload");

			auto tick = std::make_shared<Tick>(payload.get<std::string>("symbol"),
											   payload.get<std::uint64_t>("ts"),
											   payload.get<std::string>("price").c_str(),
											   payload.get<std::string>("size").c_str(),
											   payload.get<bool>("buyer"));

			ticks->push_back(tick);
		}

		return ticks;
	}
};

#endif // _WSTREAM_RESPONSE_CURRENCYCOM_HPP_
