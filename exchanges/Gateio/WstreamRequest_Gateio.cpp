#include "WstreamRequest_Gateio.hpp"

// Subscription request example:
// {"method":"trades.subscribe","params":["AAVE_ETH","GALA_USDT","IOTX_ETH"],"id":1}
std::string &WstreamRequest_Gateio::make_json()
{
	if (attempts_ != 0)
		attempts_--;

	if (!json_.empty())
		return json_;

	// Start building a subscription request JSON tree
	boost::property_tree::ptree ptree_subscribe_symbols;

	reverse_conversion_table_.clear();

	for (const auto &instrument : instruments_) {
		std::string conventional_instrument = instrument;
		std::string local_instrument = instrument;

		std::replace(local_instrument.begin(),
					 local_instrument.end(),
					 '/',
					 '_'); // replace '/' to '_'

		// Create reverse conversion table (BTC_USDT -> BTC/USDT)
		reverse_conversion_table_.insert_or_assign(local_instrument, conventional_instrument);

		// Push request into JSON tree
		if (subscription_channel_ == "trade.detail") {
			boost::property_tree::ptree ptree_subscribe_instrument;

			ptree_subscribe_instrument.put("", local_instrument); // BTC_USDT
			ptree_subscribe_symbols.push_back(std::make_pair("", ptree_subscribe_instrument));
		}
	}

	// Continue building the subscribe request JSON tree
	boost::property_tree::ptree ptree_subscribe_request;

	if (method_ == "subscribe")
		ptree_subscribe_request.put("method", "trades.subscribe");

	ptree_subscribe_request.add_child("params", ptree_subscribe_symbols);

	// Convert JSON tree to plain string
	std::stringstream stringstream;
	boost::property_tree::json_parser::write_json(stringstream, ptree_subscribe_request);
	json_ = stringstream.str();

	// Trick to put an unsigned integer "id".
	// The answer is '{"error": null, "result": {
	// "status": "success"}, "id": 12312}' if all ok,
	// and '{"error": {"code": 1, "message": "invalid argument"},
	// "result": null, "id": 0}' if "id" is a string
	json_.pop_back();
	json_.pop_back();
	json_.pop_back();
	json_ += ",\n    \"id\": " + std::to_string(id_) + "\n}";

	remove_blank_symbols();

	request_status_ = RequestStatus::Processing;

	return json_;
}