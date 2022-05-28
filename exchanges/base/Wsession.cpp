#include "Wsession.hpp"

// Report a failure
void Wsession::fail(beast::error_code ec, std::string const &what)
{
	timer_.cancel();

	if (what != "resolve") {
		spdlog::warn("!--------- Wsession Warning ----------! {} '{}' '{}' ({})",
					 host_,
					 what,
					 ec.message(),
					 ec.value());
	}
}

// -----------------------------------------------------------------------------

void Wsession::set_control_callback(void)
{
	ws_.control_callback([&](websocket::frame_type kind,
							 boost::beast::string_view payload) {
		if (kind == websocket::frame_type::ping) {
			ws_.pong(websocket::ping_data(payload));

			spdlog::trace("!--------- ping ----------! {} {}", host_, payload.data());
		}
		else if (kind == websocket::frame_type::pong) {
			spdlog::trace("!--------- pong ----------! {} {}", host_, payload.data());
		}
		else if (kind == websocket::frame_type::close) {
			spdlog::trace("!--------- close ----------! {} {}", host_, payload.data());
		}
	});
}

// -----------------------------------------------------------------------------

// Start the asynchronous operation
void Wsession::start(std::string const &host,
					 std::string const &port,
					 std::string const &endpoint,
					 std::string const &text)
{
	// Save these for later
	host_ = host;
	port_ = port;
	endpoint_ = endpoint;
	text_ = text;

	set_control_callback();

	// Look up the domain name
	resolver_.async_resolve(host,
							port,
							beast::bind_front_handler(&Wsession::on_resolve,
													  shared_from_this()));
}

// -----------------------------------------------------------------------------

void Wsession::on_resolve(beast::error_code ec,
						  tcp::resolver::results_type results)
{
	if (ec) {
		return fail(ec, "resolve");
	}

	// Set a timeout on the operation
	beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

	// Make the connection on the IP address we get from a lookup
	beast::get_lowest_layer(ws_).async_connect(results,
											   beast::bind_front_handler(&Wsession::on_connect,
																		 shared_from_this()));
}

// -----------------------------------------------------------------------------

void Wsession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
{
	if (ec) {
		return fail(ec, "connect");
	}

	// Set a timeout on the operation
	beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

	// Set SNI Hostname (many hosts need this to handshake successfully)
	if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(),
								  host_.c_str())) {
		ec = beast::error_code(static_cast<int>(::ERR_get_error()),
							   net::error::get_ssl_category());

		return fail(ec, "connect");
	}

	// Update the host_ string. This will provide the value of the
	// Host HTTP header during the WebSocket handshake.
	// See https://tools.ietf.org/html/rfc7230#section-5.4
	host_ += ':' + std::to_string(ep.port());

	// Perform the SSL handshake
	ws_.next_layer().async_handshake(ssl::stream_base::client,
									 beast::bind_front_handler(&Wsession::on_ssl_handshake,
															   shared_from_this()));
}

// -----------------------------------------------------------------------------

void Wsession::on_ssl_handshake(beast::error_code ec)
{
	if (ec) {
		return fail(ec, "ssl_handshake");
	}

	// Turn off the timeout on the tcp_stream, because
	// the websocket stream has its own timeout system.
	beast::get_lowest_layer(ws_).expires_never();

	// Set suggested timeout settings for the websocket
	ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

	// Set a decorator to change the User-Agent of the handshake
	ws_.set_option(websocket::stream_base::decorator(
		[](websocket::request_type &req) {
			req.set(http::field::user_agent,
					std::string(BOOST_BEAST_VERSION_STRING) + " websockets");
		}));

	// Perform the websocket handshake
	ws_.async_handshake(host_,
						endpoint_,
						beast::bind_front_handler(&Wsession::on_handshake,
												  shared_from_this()));
}

// -----------------------------------------------------------------------------

void Wsession::on_handshake(beast::error_code ec)
{
	if (ec) {
		return fail(ec, "handshake");
	}

	timer_.expires_from_now(std::chrono::milliseconds(1));
	//  Wait on the timer
	timer_.async_wait(beast::bind_front_handler(&Wsession::on_timer,
												shared_from_this()));

	// Set a timeout on the operation
	beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

	if (net::buffer(text_).size() > 0) {
		// Send the message
		ws_.async_write(net::buffer(text_),
						beast::bind_front_handler(&Wsession::on_write,
												  shared_from_this()));
	}
	else {
		// Read a message into our buffer
		ws_.async_read(buffer_,
					   beast::bind_front_handler(&Wsession::on_read,
												 shared_from_this()));
	}
}

// -----------------------------------------------------------------------------

void Wsession::on_write(beast::error_code ec,
						std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec) {
		return fail(ec, "write");
	}

	// Set a timeout on the operation
	beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

	// Read a message into our buffer
	ws_.async_read(buffer_,
				   beast::bind_front_handler(&Wsession::on_read,
											 shared_from_this()));
}

// -----------------------------------------------------------------------------

void Wsession::on_read(beast::error_code ec,
					   std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec) {
		return fail(ec, "read");
	}

	output(buffer_);

	// Set a timeout on the operation
	beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

	// Read a message into our buffer
	ws_.async_read(buffer_,
				   beast::bind_front_handler(&Wsession::on_read,
											 shared_from_this()));
}

// -----------------------------------------------------------------------------

void Wsession::on_close(beast::error_code ec)
{
	if (ec) {
		return fail(ec, "close");
	}

	// If we get here then the connection is closed gracefully
	spdlog::debug("*on_close*: {} {}", host_, beast::buffers_to_string(buffer_.data()));
}