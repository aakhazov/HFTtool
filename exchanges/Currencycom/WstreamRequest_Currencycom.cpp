#include "WstreamRequest_Currencycom.hpp"

// Subscription request example:
// {"destination":"trades.subscribe",
// "correlationId":"1","payload":{"symbols":["AAVE/ETH","GALA/USDT","IOTX/ETH"]}}
std::string &WstreamRequest_Currencycom::make_json()
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

		// Create reverse conversion table (BTC/USDT -> BTC/USDT)
		reverse_conversion_table_.insert_or_assign(local_instrument, conventional_instrument);

		// Push request into JSON tree
		if (subscription_channel_ == "trade.detail") {
			boost::property_tree::ptree ptree_subscribe_instrument;

			ptree_subscribe_instrument.put("", local_instrument); // BTC/USDT
			ptree_subscribe_symbols.push_back(std::make_pair("", ptree_subscribe_instrument));
		}
	}

	// Continue building the subscribe request JSON tree
	boost::property_tree::ptree ptree_subscribe_request;

	if (method_ == "subscribe")
		ptree_subscribe_request.put("destination", "trades.subscribe");

	ptree_subscribe_request.put("correlationId", id_);

	boost::property_tree::ptree ptree_subscribe_payload;
	ptree_subscribe_payload.push_back(std::make_pair("symbols", ptree_subscribe_symbols));

	ptree_subscribe_request.add_child("payload", ptree_subscribe_payload);

	// Convert JSON tree to plain string
	std::stringstream stringstream;
	boost::property_tree::json_parser::write_json(stringstream, ptree_subscribe_request);
	json_ = stringstream.str();

	remove_blank_symbols();

	request_status_ = RequestStatus::Processing;

	return json_;
}