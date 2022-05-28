#ifndef _WSTREAM_RESPONSE_BINANCE_HPP_
#define _WSTREAM_RESPONSE_BINANCE_HPP_

#include "exchanges/base/WstreamResponse.hpp"

class WstreamResponse_Binance : public WstreamResponse<WstreamResponse_Binance> {
public:
	static constexpr const char *exchange_name_ = "Binance";

public:
	static auto response_specific(boost::property_tree::ptree const &json_tree_root)
	{
		uint32_t id = 0;

		// ----- Parse response -----
		// Response example:
		// {"result":null,"id":1}

		std::string result = json_tree_root.get<std::string>("result");

		if (result == "null") {
			id = json_tree_root.get<uint32_t>("id");
		}

		return id;
	}

	// -------------------------------------------------------------------------

	static auto trade_detail_specific(boost::property_tree::ptree const &json_tree_root)
	{
		auto ticks = std::make_unique<std::vector<std::shared_ptr<Tick>>>();

		// ----- Parse price -----
		// trade.detail update example:
		// ../../__dev_data/BINANCE_trade_details.json

		std::string stream = json_tree_root.get<std::string>("stream");
		boost::property_tree::ptree data = json_tree_root.get_child("data");
		std::string event = data.get<std::string>("e");

		if (!stream.empty() && event == "trade") {
			auto tick = std::make_shared<Tick>(data.get<std::string>("s"),
											   data.get<std::uint64_t>("T"),
											   data.get<std::string>("p").c_str(),
											   data.get<std::string>("q").c_str(),
											   data.get<bool>("m"));

			ticks->push_back(tick);
		}

		return ticks;
	}
};

#endif // _WSTREAM_RESPONSE_BINANCE_HPP_