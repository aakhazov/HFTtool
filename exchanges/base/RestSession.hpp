#ifndef _RESTSESSION_HPP_
#define _RESTSESSION_HPP_

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include "common/communication.hpp"

namespace beast = boost::beast;	  // from <boost/beast.hpp>
namespace http = beast::http;	  // from <boost/beast/http.hpp>
namespace net = boost::asio;	  // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

template <class T>
class RestSession {
public:
	// Report a failure
	static void fail(beast::error_code ec, std::string const &host, std::string const &what)
	{
		spdlog::warn("!--------- RestSession Warning ----------! {} '{}' '{}' ({})", host, what, ec.message(), ec.value());
	}

	// -----------------------------------------------------------------------------

	// Parse the response
	static void output(http::response<http::dynamic_body> const &response,
					   std::shared_ptr<InstrumentsInfoMap> &instruments_info_map)
	{
	}

	// -----------------------------------------------------------------------------

	// Performs an HTTP GET
	static void do_session(std::string const &host,
						   std::string const &port,
						   std::string const &target,
						   std::shared_ptr<InstrumentsInfoMap> &instruments_info_map,
						   int version,
						   net::io_context &ioc,
						   ssl::context &ctx,
						   net::yield_context yield)
	{
		beast::error_code ec;

		// These objects perform our I/O
		tcp::resolver resolver(ioc);
		beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

		// Set SNI Hostname (many hosts need this to handshake successfully)
		if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
			ec.assign(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
			std::cerr << ec.message() << "\n";
			return;
		}

		// Look up the domain name
		auto const results = resolver.async_resolve(host, port, yield[ec]);
		if (ec)
			return T::fail(ec, host, "resolve");

		// Set the timeout.
		beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

		// Make the connection on the IP address we get from a lookup
		get_lowest_layer(stream).async_connect(results, yield[ec]);
		if (ec)
			return T::fail(ec, host, "connect");

		// Set the timeout.
		beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

		// Perform the SSL handshake
		stream.async_handshake(ssl::stream_base::client, yield[ec]);
		if (ec)
			return T::fail(ec, host, "handshake");

		// Set up an HTTP GET request message
		http::request<http::string_body> req{http::verb::get, target, version};
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		// Set the timeout.
		beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

		// Send the HTTP request to the remote host
		http::async_write(stream, req, yield[ec]);
		if (ec)
			return T::fail(ec, host, "write");

		// This buffer is used for reading and must be persisted
		beast::flat_buffer b;

		// Declare a container to hold the response
		http::response<http::dynamic_body> response;

		// Receive the HTTP response
		http::async_read(stream, b, response, yield[ec]);
		if (ec)
			return T::fail(ec, host, "read");

		// Write the message to standard out
		T::output(response, instruments_info_map);

		// Set the timeout.
		beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

		// Gracefully close the stream
		stream.async_shutdown(yield[ec]);
		if (ec == net::error::eof) {
			// Rationale:
			// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
			ec = {};
		}
		if (ec != net::ssl::error::stream_truncated &&
			ec != boost::system::errc::success) {

			return T::fail(ec, host, "shutdown");
		}

		// If we get here then the connection is closed gracefully
	}
};

#endif // _RESTSESSION_HPP_