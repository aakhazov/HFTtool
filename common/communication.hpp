#ifndef _COMMUNICATION_HPP_
#define _COMMUNICATION_HPP_

#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>

#include <boost/algorithm/string.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "spdlog/spdlog.h"

//------------------------------------------------------------------------------

class InstrumentInfo {
private:
	bool tradable_;
	mutable std::shared_mutex mutex;

public:
	InstrumentInfo(bool tradable) : tradable_(tradable)
	{
	}

	//--------------------------------------------------------------------------

	auto is_tradable() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return tradable_;
	}
};

//------------------------------------------------------------------------------

class InstrumentsInfoMap {
private:
	std::map<std::string, std::shared_ptr<InstrumentInfo>> instruments_info_;
	mutable std::shared_mutex mutex;

public:
	bool gui_selected = false;

public:
	void update_or_add_entry(std::string const &instrument,
							 std::shared_ptr<InstrumentInfo> instrument_info)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		instruments_info_[instrument] = instrument_info;
	}

	//--------------------------------------------------------------------------

	auto find_entry(std::string const &instrument) const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		auto it = instruments_info_.find(instrument);

		return (it == instruments_info_.end()) ? nullptr : it->second;
	}

	//--------------------------------------------------------------------------

	auto get_entry(std::map<std::string,
							std::shared_ptr<InstrumentInfo>>::const_iterator const &it) const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return std::make_pair(it->first, it->second);
	}

	//--------------------------------------------------------------------------

	auto begin() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return instruments_info_.begin();
	}

	//--------------------------------------------------------------------------

	auto end() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return instruments_info_.end();
	}

	//--------------------------------------------------------------------------

	auto size() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return instruments_info_.size();
	}
};

//------------------------------------------------------------------------------

// All trading allowed instruments for all exchanges
// {"exchange": pointer_to_list_of_trading_allowed_instruments}
typedef std::map<std::string, std::shared_ptr<InstrumentsInfoMap>> AllInstrumentsInfo;

//------------------------------------------------------------------------------

class InstrumentRequest {
private:
	std::string status_;
	mutable std::shared_mutex mutex;

public:
	InstrumentRequest(std::string const &status) : status_(status)
	{
	}

	//--------------------------------------------------------------------------

	auto &get_status() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return status_;
	}
};

//------------------------------------------------------------------------------

class InstrumentsRequestsMap {
private:
	std::map<std::string, std::shared_ptr<InstrumentRequest>> instruments_requests_;
	mutable std::shared_mutex mutex;

public:
	void update_or_add_entry(std::string const &instrument,
							 std::shared_ptr<InstrumentRequest> instruments_request)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		instruments_requests_[instrument] = instruments_request;
	}

	//--------------------------------------------------------------------------

	auto find_entry(std::string const &instrument) const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		auto it = instruments_requests_.find(instrument);

		return (it == instruments_requests_.end()) ? nullptr : it->second;
	}

	//--------------------------------------------------------------------------

	auto get_entry(std::map<std::string,
							std::shared_ptr<InstrumentRequest>>::const_iterator const &it) const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return std::make_pair(it->first, it->second);
	}

	//--------------------------------------------------------------------------

	auto begin() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return instruments_requests_.begin();
	}

	//--------------------------------------------------------------------------

	auto end() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return instruments_requests_.end();
	}
};

//------------------------------------------------------------------------------

// Exchange requests for all exchanges
// {"exchange": pointer_to_list_of_exchange_requests}
typedef std::map<std::string, std::shared_ptr<InstrumentsRequestsMap>> AllRequests;

//------------------------------------------------------------------------------

class InstrumentResponse {
private:
	std::string status_;
	mutable std::shared_mutex mutex;

public:
	InstrumentResponse(std::string const &status) : status_(status)
	{
	}

	//--------------------------------------------------------------------------

	auto &get_status() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return status_;
	}
};

//------------------------------------------------------------------------------

class InstrumentsResponsesMap {
private:
	std::map<std::string, std::shared_ptr<InstrumentResponse>> instruments_requests_;
	mutable std::shared_mutex mutex;

public:
	void clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		instruments_requests_.clear();
	}

	//--------------------------------------------------------------------------

	void update_or_add_entry(std::string const &instrument,
							 std::shared_ptr<InstrumentResponse> instruments_request)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		instruments_requests_[instrument] = instruments_request;
	}

	//--------------------------------------------------------------------------

	auto find_entry(std::string const &instrument) const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		auto it = instruments_requests_.find(instrument);

		return (it == instruments_requests_.end()) ? nullptr : it->second;
	}

	//--------------------------------------------------------------------------

	auto get_entry(std::map<std::string,
							std::shared_ptr<InstrumentResponse>>::const_iterator const &it) const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return std::make_pair(it->first, it->second);
	}

	//--------------------------------------------------------------------------

	auto begin() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return instruments_requests_.begin();
	}

	//--------------------------------------------------------------------------

	auto end() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return instruments_requests_.end();
	}
};

//------------------------------------------------------------------------------

// Exchange responses for all exchanges
// {"exchange": pointer_to_list_of_exchange_responses}
typedef std::map<std::string, std::shared_ptr<InstrumentsResponsesMap>> AllResponses;

//------------------------------------------------------------------------------

struct SafeFloat {
	std::uint64_t value;
	std::size_t precision;

	bool operator==(const SafeFloat &other)
	{
		return ((value == other.value) && (precision == other.precision));
	}
};

//------------------------------------------------------------------------------

// Price objects class
class Tick {
private:
	static constexpr const std::uint64_t pow10_[19] = {
		1,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
		10000000,
		100000000,
		1000000000,
		10000000000,
		100000000000,
		1000000000000,
		10000000000000,
		100000000000000,
		1000000000000000,
		10000000000000000,
		100000000000000000,
		1000000000000000000};

	mutable std::string instrument_;
	std::uint64_t timestamp_;
	std::string price_str_;
	std::string size_str_;
	bool buyer_;

	double price_double_;
	double size_double_;

	SafeFloat price_ = {0, 0};
	SafeFloat size_ = {0, 0};

public:
	Tick(std::string const &instrument,
		 std::uint64_t timestamp,
		 std::string const &price,
		 std::string const &size,
		 bool buyer) : instrument_(instrument),
					   timestamp_(timestamp),
					   price_str_(price),
					   size_str_(size),
					   buyer_(buyer)
	{
		// Double values of price and size
		price_double_ = std::stod(price_str_);
		size_double_ = std::stod(size_str_);

		// Building integer components of a floating point price value
		std::vector<std::string> price_components;
		boost::split(price_components, price_str_, boost::is_any_of("."));

		price_.precision = 0;
		price_.value = 0;

		if (price_components.size() == 2) {
			price_.precision = price_components[1].size();

			if (price_.precision < std::size(pow10_))
				price_.value = std::stoll(price_components[0]) *
								   pow10_[price_.precision] +
							   std::stoll(price_components[1]);
			else
				price_.precision = 0;
		}
		else if (price_components.size() == 1)
			price_.value = std::stoll(price_components[0]);

		// Building integer components of a floating point size value
		std::vector<std::string> size_components;
		boost::split(size_components, size_str_, boost::is_any_of("."));

		size_.precision = 0;
		size_.value = 0;

		if (size_components.size() == 2) {
			size_.precision = size_components[1].size();

			if (size_.precision < std::size(pow10_))
				size_.value = std::stoll(size_components[0]) *
								  pow10_[size_.precision] +
							  std::stoll(size_components[1]);
			else
				size_.precision = 0;
		}
		else if (size_components.size() == 1)
			size_.value = std::stoll(size_components[0]);
	}

	//--------------------------------------------------------------------------

	auto &get_instrument() const
	{
		return instrument_;
	}

	void set_instrument(std::string const &instrument) const
	{
		instrument_ = instrument;
	}

	auto get_timestamp() const
	{
		return timestamp_;
	}

	auto &get_price_str() const
	{
		return price_str_;
	}

	auto &get_size_str() const
	{
		return size_str_;
	}

	bool is_buyer() const
	{
		return buyer_;
	}

	auto get_price_double() const
	{
		return price_double_;
	}

	auto get_size_double() const
	{
		return size_double_;
	}

	auto &get_price_struct() const
	{
		return price_;
	}

	auto &get_size_struct() const
	{
		return size_;
	}
};

// Price lockfree spsc_queue
typedef boost::lockfree::spsc_queue<std::shared_ptr<Tick>, boost::lockfree::capacity<1024>> TicksQueue;
// Prices of all exchanges
// {"exchange": pointer_to_price_lockfree_spsc_queue}
typedef std::map<std::string, std::shared_ptr<TicksQueue>> AllTicks;

#endif // _COMMUNICATION_HPP_