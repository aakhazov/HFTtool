#ifndef _WSTREAM_RESPONSE_HUOBI_HPP_
#define _WSTREAM_RESPONSE_HUOBI_HPP_

#include "exchanges/base/WstreamResponse.hpp"

class WstreamResponse_Huobi : public WstreamResponse<WstreamResponse_Huobi> {
public:
	static constexpr const char *exchange_name_ = "Huobi";

public:
	static auto response_specific(boost::property_tree::ptree const &json_tree_root)
	{
		uint32_t id = 0;

		// ----- Parse response -----
		// Response example:
		// {"id":"1","status":"ok","subbed":"market.btcusdt.trade.detail","ts":1651495946497}

		std::string status = json_tree_root.get<std::string>("status");
		std::string subbed = json_tree_root.get<std::string>("subbed");

		if (status == "ok" && !subbed.empty()) {
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
		// ../../__dev_data/HUOBI_trade_details.json

		std::string channel = json_tree_root.get<std::string>("ch");

		std::vector<std::string> channel_splitted;
		boost::split(channel_splitted, channel, boost::is_any_of("."));

		if (channel_splitted.size() == 4 &&
			channel_splitted[2] == "trade" &&
			channel_splitted[3] == "detail") {

			boost::property_tree::ptree tick = json_tree_root.get_child("tick");
			boost::property_tree::ptree data = tick.get_child("data");

			for (const auto &trade : data) {
				auto trade_data = trade.second;

				auto tick = std::make_shared<Tick>(channel,
												   trade_data.get<std::uint64_t>("ts"),
												   trade_data.get<std::string>("price").c_str(),
												   trade_data.get<std::string>("amount").c_str(),
												   trade_data.get<std::string>("direction") == "buy" ? true : false);

				ticks->push_back(tick);
			}
		}

		return ticks;
	}
};

#endif // _WSTREAM_RESPONSE_HUOBI_HPP_