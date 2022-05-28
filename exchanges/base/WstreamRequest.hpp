#ifndef _WSTREAM_REQUEST_HPP_
#define _WSTREAM_REQUEST_HPP_

#include <map>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

#include <boost/algorithm/string.hpp>

#include "common/communication.hpp"

enum class RequestStatus {
	Empty = 0,
	Fill,
	Ready,
	Processing
};

// -----------------------------------------------------------------------------

enum class RequestError {
	Success = 0,
	WrongFormat,
	MethodMismatch,
	ChannelMismatch,
	SizeLimit,
	Busy
};

// -----------------------------------------------------------------------------

class WstreamRequest {
private:
	WstreamRequest(const WstreamRequest &other);

protected:
	uint32_t id_ = 0;
	std::string method_;
	std::string subscription_channel_;
	uint8_t attempts_ = 1;

	RequestStatus request_status_ = RequestStatus::Empty;

	uint8_t max_instruments_;
	std::vector<std::string> instruments_;
	uint8_t instruments_number_ = 0;

	std::map<std::string, std::string> reverse_conversion_table_;
	std::map<std::string, std::string> responses_;
	std::string json_;

public:
	explicit WstreamRequest(uint32_t id, uint8_t max_instruments)
		: id_(id),
		  max_instruments_(max_instruments)
	{
	}

	// -------------------------------------------------------------------------

	virtual ~WstreamRequest()
	{
	}

	// -------------------------------------------------------------------------

	virtual std::string &make_json() = 0;

	// -------------------------------------------------------------------------

	RequestError try_insert(const std::pair<std::string, std::string> &request);

	// -------------------------------------------------------------------------

	std::map<std::string, std::string> &get_conventional_responses();

	// -------------------------------------------------------------------------

	void finalyze()
	{
		request_status_ = RequestStatus::Ready;
	}

	// -------------------------------------------------------------------------

	void remove_blank_symbols()
	{
		// Remove '\n' from string
		json_.erase(remove(json_.begin(),
						   json_.end(),
						   '\n'),
					json_.end());

		// Remove spaces from string
		json_.erase(remove(json_.begin(),
						   json_.end(),
						   ' '),
					json_.end());
	}

	// -------------------------------------------------------------------------

	std::vector<std::string> &get_instruments()
	{
		return instruments_;
	}

	// -------------------------------------------------------------------------

	auto &get_reverse_conversion_table_() const
	{
		return reverse_conversion_table_;
	}

	// -------------------------------------------------------------------------

	auto get_size() const
	{
		return instruments_number_;
	}

	// -------------------------------------------------------------------------

	auto get_attempts() const
	{
		return attempts_;
	}
};

#endif // _WSTREAM_REQUEST_HPP_