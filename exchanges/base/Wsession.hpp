#ifndef _WSESSION_HPP_
#define _WSESSION_HPP_

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>

#include "common/communication.hpp"

namespace beast = boost::beast;			// from <boost/beast.hpp>
namespace http = beast::http;			// from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;			// from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

// Sends a WebSocket message and prints the response
class Wsession : public std::enable_shared_from_this<Wsession> {
private:
	Wsession(const Wsession &other);

protected:
	tcp::resolver resolver_;
	websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;

	std::atomic<bool> *alive_;

	beast::flat_buffer buffer_;
	std::string host_;
	std::string port_;
	std::string endpoint_;
	std::string text_;

	std::shared_ptr<InstrumentsRequestsMap> &instruments_requests_map_;
	std::shared_ptr<InstrumentsResponsesMap> &instruments_responses_map_;
	std::shared_ptr<TicksQueue> &ticks_queue_;

	net::steady_timer timer_;

	uint32_t counter_ = 0;

	uint32_t xmit_id_ = 0;
	uint32_t last_request_id_ = 0;
	std::map<std::string, std::string> reverse_conversion_table_;

public:
	// Resolver and socket require an io_context
	explicit Wsession(net::io_context &ioc,
					  ssl::context &ctx,
					  std::atomic<bool> *alive,
					  std::shared_ptr<InstrumentsRequestsMap> &instruments_requests_map,
					  std::shared_ptr<InstrumentsResponsesMap> &instruments_responses_map,
					  std::shared_ptr<TicksQueue> &ticks_queue)
		: resolver_(net::make_strand(ioc)),
		  ws_(net::make_strand(ioc), ctx),
		  alive_(alive),
		  instruments_requests_map_(instruments_requests_map),
		  instruments_responses_map_(instruments_responses_map),
		  ticks_queue_(ticks_queue),
		  timer_(ioc)
	{
		instruments_responses_map_->clear();
	}

	// -------------------------------------------------------------------------

	virtual ~Wsession()
	{
	}

	// -------------------------------------------------------------------------

	virtual void on_timer(const boost::system::error_code &ec)
	{
	}

	// -------------------------------------------------------------------------

	// Parse stream data
	virtual void output(beast::flat_buffer &buffer) = 0;

	void fail(beast::error_code ec, std::string const &what);

	void set_control_callback(void);

	// Start the asynchronous operation
	void start(std::string const &host,
			   std::string const &port,
			   std::string const &endpoint,
			   std::string const &text);

	void on_resolve(beast::error_code ec,
					tcp::resolver::results_type results);

	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

	void on_ssl_handshake(beast::error_code ec);

	void on_handshake(beast::error_code ec);

	void on_write(beast::error_code ec,
				  std::size_t bytes_transferred);

	void on_read(beast::error_code ec,
				 std::size_t bytes_transferred);

	void on_close(beast::error_code ec);
};

#endif // _WSESSION_HPP_