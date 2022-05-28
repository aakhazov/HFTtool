#include "WstreamRequest_Binance.hpp"

// Subscription request example:
// {"method":"SUBSCRIBE","params":["aaveeth@trade","galausdt@trade","iotxeth@trade"],"id":1}
std::string &WstreamRequest_Binance::make_json()
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

		local_instrument.erase(remove(local_instrument.begin(),
									  local_instrument.end(),
									  '/'),
							   local_instrument.end()); // remove '/' from string

		// Create reverse conversion table (BTCUSDT -> BTC/USDT)
		reverse_conversion_table_.insert_or_assign(local_instrument, conventional_instrument);

		boost::algorithm::to_lower(local_instrument); // to lower case

		// Push request into JSON tree
		if (subscription_channel_ == "trade.detail") {
			boost::property_tree::ptree ptree_subscribe_instrument;

			ptree_subscribe_instrument.put("", local_instrument + "@trade"); // instrument@trade
			ptree_subscribe_symbols.push_back(std::make_pair("", ptree_subscribe_instrument));
		}
	}

	// Continue building the subscribe request JSON tree
	boost::property_tree::ptree ptree_subscribe_request;

	if (method_ == "subscribe")
		ptree_subscribe_request.put("method", "SUBSCRIBE");

	ptree_subscribe_request.add_child("params", ptree_subscribe_symbols);

	// Convert JSON tree to plain string
	std::stringstream stringstream;
	boost::property_tree::json_parser::write_json(stringstream, ptree_subscribe_request);
	json_ = stringstream.str();

	// Trick to put an unsigned integer "id".
	// The answer is '{"result":null,"id":1}' if all ok,
	// and '{"error":{"code":2,"msg":"Invalid request:
	// request ID must be an unsigned integer"}}' if "id" is a string
	json_.pop_back();
	json_.pop_back();
	json_.pop_back();
	json_ += ",\n    \"id\": " + std::to_string(id_) + "\n}";

	remove_blank_symbols();

	request_status_ = RequestStatus::Processing;

	return json_;
}