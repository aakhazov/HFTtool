#ifndef _WSESSION_HUOBI_HPP_
#define _WSESSION_HUOBI_HPP_

#include "exchanges/base/Wsession.hpp"

#include "WstreamRequest_Huobi.hpp"
#include "WstreamResponse_Huobi.hpp"

class Wsession_Huobi : public Wsession {
private:
	const uint8_t max_instruments_in_request_ = 1;
	std::chrono::duration<int64_t> timer_period_;
	int request_timer_ = 0;

	std::map<uint32_t, std::shared_ptr<WstreamRequest_Huobi>> requests_;

public:
	explicit Wsession_Huobi(net::io_context &ioc,
							ssl::context &ctx,
							std::atomic<bool> *alive,
							std::shared_ptr<InstrumentsRequestsMap> &instruments_requests_map,
							std::shared_ptr<InstrumentsResponsesMap> &instruments_responses_map,
							std::shared_ptr<TicksQueue> &ticks_queue)
		: Wsession(ioc, ctx, alive, instruments_requests_map, instruments_responses_map, ticks_queue),
		  timer_period_(std::chrono::seconds(1))
	{
	}

	void on_timer(const boost::system::error_code &ec) override;

	void output(beast::flat_buffer &buffer) override;

	std::vector<char> unzip(const std::vector<char> compressed);

	std::uint64_t pong(std::string const &json);
};

#endif // _WSESSION_HUOBI_HPP_