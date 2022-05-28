#ifndef _RESTSESSION_GATEIO_HPP_
#define _RESTSESSION_GATEIO_HPP_

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

#include "exchanges/base/RestSession.hpp"

// -----------------------------------------------------------------------------

class RestSession_Gateio : public RestSession<RestSession_Gateio> {
public:
	// Parse the response
	static void output(http::response<http::dynamic_body> const &response,
					   std::shared_ptr<InstrumentsInfoMap> &instruments_info_map);
};

// -----------------------------------------------------------------------------

// Parse the response
void RestSession_Gateio::output(http::response<http::dynamic_body> const &response,
								std::shared_ptr<InstrumentsInfoMap> &instruments_info_map)
{
	std::string json_raw = beast::buffers_to_string(response.body().data());

	spdlog::trace("Gateio RestAPI response: '{}'", json_raw);

	try {
		// Read json.
		boost::property_tree::ptree json_tree_root;

		boost::iostreams::array_source as(&json_raw[0], json_raw.size());
		boost::iostreams::stream<boost::iostreams::array_source> is(as);

		boost::property_tree::read_json(is, json_tree_root);

		// iterate over JSON properties
		for (boost::property_tree::ptree::iterator iter = json_tree_root.begin();
			 iter != json_tree_root.end();
			 ++iter) {

			std::string trade_status = iter->second.get<std::string>("trade_status");

			std::string instrument = iter->second.get<std::string>("base") +
									 "/" +
									 iter->second.get<std::string>("quote");

			if (trade_status == "tradable")
				instruments_info_map->update_or_add_entry(instrument,
														  std::make_shared<InstrumentInfo>(true));
			else
				instruments_info_map->update_or_add_entry(instrument,
														  std::make_shared<InstrumentInfo>(false));
		}
	}
	catch (const boost::property_tree::ptree_error &e) {
		spdlog::trace("RestSession_Gateio catch boost::property_tree::ptree_error: '{}'",
					  e.what());
	}
	catch (std::exception const &e) {
		spdlog::warn("RestSession_Gateio catch std::exception: '{}'",
					 e.what());
	}
}

#endif // _RESTSESSION_GATEIO_HPP_