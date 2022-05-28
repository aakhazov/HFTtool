#ifndef _WSTREAM_RESPONSE_HPP_
#define _WSTREAM_RESPONSE_HPP_

#include <map>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

#include <boost/algorithm/string.hpp>

#include "common/communication.hpp"

template <class T>
class WstreamResponse {
public:
	static constexpr const char *exchange_name_ = "";

public:
	static auto response_specific(boost::property_tree::ptree const &json_tree_root)
	{
		uint32_t id = 0;

		return id;
	}

	// -------------------------------------------------------------------------

	static std::unique_ptr<std::vector<Tick>> trade_detail_specific(boost::property_tree::ptree const &json_tree_root)
	{
		return nullptr;
	}

	// -------------------------------------------------------------------------

	static auto response(std::string const &json_raw)
	{
		uint32_t id = 0;

		boost::iostreams::array_source as(&json_raw[0], json_raw.size());
		boost::iostreams::stream<boost::iostreams::array_source> is(as);

		boost::property_tree::ptree json_tree_root;

		try {
			boost::property_tree::read_json(is, json_tree_root);
			id = T::response_specific(json_tree_root);
		}
		catch (const boost::property_tree::ptree_error &e) {
			spdlog::trace("Catch boost::property_tree::ptree_error: {} '{}'",
						  T::exchange_name_,
						  e.what());
		}
		catch (std::exception const &e) {
			spdlog::warn("Catch std::exception: {} '{}'",
						 T::exchange_name_,
						 e.what());
		}

		return id;
	}

	// -------------------------------------------------------------------------

	static auto trade_detail(std::string const &json_raw)
	{
		std::unique_ptr<std::vector<std::shared_ptr<Tick>>> ticks = nullptr;

		boost::iostreams::array_source as(&json_raw[0], json_raw.size());
		boost::iostreams::stream<boost::iostreams::array_source> is(as);

		boost::property_tree::ptree json_tree_root;

		try {
			boost::property_tree::read_json(is, json_tree_root);
			ticks = T::trade_detail_specific(json_tree_root);
		}
		catch (const boost::property_tree::ptree_error &e) {
			spdlog::trace("Catch boost::property_tree::ptree_error: {} '{}'",
						  T::exchange_name_,
						  e.what());
		}
		catch (std::exception const &e) {
			spdlog::warn("Catch std::exception: {} '{}'",
						 T::exchange_name_,
						 e.what());
		}

		return ticks;
	}
};

#endif // _WSTREAM_RESPONSE_HPP_