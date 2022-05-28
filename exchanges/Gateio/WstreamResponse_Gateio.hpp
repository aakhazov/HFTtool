#ifndef _WSTREAM_RESPONSE_GATEIO_HPP_
#define _WSTREAM_RESPONSE_GATEIO_HPP_

#include "exchanges/base/WstreamResponse.hpp"

class WstreamResponse_Gateio : public WstreamResponse<WstreamResponse_Gateio> {
public:
	static constexpr const char *exchange_name_ = "Gateio";

public:
	static auto response_specific(boost::property_tree::ptree const &json_tree_root)
	{
		uint32_t id = 0;

		// ----- Parse response -----
		// Response example:
		// {"error": null, "result": {"status": "success"}, "id": 1}
		// {"error": null, "result": "pong", "id": 2}

		std::string error = json_tree_root.get<std::string>("error");

		if (error == "null") {
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
		// ../../__dev_data/GATEIO_trade_details.json

		std::string method = json_tree_root.get<std::string>("method");

		if (method == "trades.update") {
			boost::property_tree::ptree params = json_tree_root.get_child("params");

			for (auto subscription = params.begin();
				 subscription != params.end();
				 ++subscription) {

				std::string instrument = subscription->second.data();

				++subscription;

				for (const auto &trade : subscription->second) {
					auto trade_data = trade.second;

					auto tick = std::make_shared<Tick>(instrument,
													   static_cast<std::uint64_t>(1E03 * trade_data.get<double>("time")),
													   trade_data.get<std::string>("price").c_str(),
													   trade_data.get<std::string>("amount").c_str(),
													   trade_data.get<std::string>("type") == "buy" ? true : false);

					ticks->push_back(tick);
				}
			}
		}

		return ticks;
	}
};

#endif // _WSTREAM_RESPONSE_GATEIO_HPP_