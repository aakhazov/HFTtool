#include "WstreamRequest_Huobi.hpp"

// Subscription request example:
// {"sub":"market.aaveeth.trade.detail","id":"1"}
std::string &WstreamRequest_Huobi::make_json()
{
	if (attempts_ != 0)
		attempts_--;

	if (!json_.empty())
		return json_;

	reverse_conversion_table_.clear();

	for (const auto &instrument : instruments_) {
		std::string conventional_instrument = instrument;
		std::string local_instrument = instrument;

		local_instrument.erase(remove(local_instrument.begin(),
									  local_instrument.end(),
									  '/'),
							   local_instrument.end()); // remove '/' from string

		auto index = local_instrument.find("*1");
		if (index != std::string::npos)
			local_instrument.replace(index,
									 2,
									 "1l"); // replace "*1" to "1l"

		index = local_instrument.find("*2");
		if (index != std::string::npos)
			local_instrument.replace(index,
									 2,
									 "2l"); // replace "*2" to "2l"

		index = local_instrument.find("*3");
		if (index != std::string::npos)
			local_instrument.replace(index,
									 2,
									 "3l"); // replace "*3" to "3l"

		index = local_instrument.find("*(-1)");
		if (index != std::string::npos)
			local_instrument.replace(index,
									 5,
									 "1s"); // replace "*(-1)" to "1s"

		index = local_instrument.find("*(-2)");
		if (index != std::string::npos)
			local_instrument.replace(index,
									 5,
									 "2s"); // replace "*(-2)" to "2s"

		index = local_instrument.find("*(-3)");
		if (index != std::string::npos)
			local_instrument.replace(index,
									 5,
									 "3s"); // replace "*(-3)" to "3s"

		index = local_instrument.find("(C2X)");
		if (index != std::string::npos)
			local_instrument.replace(index,
									 5,
									 "c2x"); // replace "(C2X)" to "c2x"

		boost::algorithm::to_lower(local_instrument); // to lower case
		local_instrument = "market." + local_instrument + ".trade.detail";

		// Create reverse conversion table (market.btcusdt.trade.detail -> BTC/USDT)
		reverse_conversion_table_.insert_or_assign(local_instrument, conventional_instrument);
	}

	// Continue building the subscribe request JSON tree
	boost::property_tree::ptree ptree_subscribe_request;

	for (const auto &subscribe : reverse_conversion_table_) {
		if (method_ == "subscribe")
			ptree_subscribe_request.put("sub", subscribe.first);

		// Subscribe request JSON tree
		ptree_subscribe_request.put("id", id_);
	}

	// Convert JSON tree to plain string
	std::stringstream stringstream;
	boost::property_tree::json_parser::write_json(stringstream, ptree_subscribe_request);
	json_ = stringstream.str();

	remove_blank_symbols();

	request_status_ = RequestStatus::Processing;

	return json_;
}