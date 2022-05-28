#include "threadFuncs_Binance.hpp"

#include <boost/asio/spawn.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include "RestSession_Binance.hpp"
#include "Wsession_Binance.hpp"
#include "common/root_certificates.hpp"

void rest_thread_func_binance(std::atomic<bool> *alive,
							  AllInstrumentsInfo &all_instruments,
							  AllRequests &all_requests,
							  AllResponses &all_responses)
{
	int timer = 0;

	do {
		if (--timer <= 0) {
			timer = 30;

			spdlog::trace("!--------- Begin BINANCE GET request ----------!");

			// List of instruments: curl -X GET "https://api.binance.com/api/v3/exchangeInfo"
			std::string const host = "api.binance.com";
			std::string const port = "443";
			std::string const target = "/api/v3/exchangeInfo";
			int version = 11;

			// The io_context is required for all I/O
			net::io_context ioc;

			// The SSL context is required, and holds certificates
			ssl::context ctx{ssl::context::tlsv12_client};

			// This holds the root certificate used for verification
			load_root_certificates(ctx);

			// Verify the remote server's certificate
			ctx.set_verify_mode(ssl::verify_peer);

			// Launch the asynchronous operation
			boost::asio::spawn(ioc, std::bind(&RestSession_Binance::do_session,
											  host,
											  port,
											  target,
											  all_instruments.at("Binance"),
											  version,
											  std::ref(ioc),
											  std::ref(ctx),
											  std::placeholders::_1));

			// Run the I/O service. The call will return when
			// the get operation is complete.
			ioc.run();

			spdlog::trace("!--------- End BINANCE GET request ----------!");
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while (*alive);

	spdlog::debug("BINANCE RestAPI thread exit");
}

// -----------------------------------------------------------------------------

void ws_thread_func_binance(std::atomic<bool> *alive,
							AllRequests &all_requests,
							AllResponses &all_responses,
							AllTicks &all_prices)
{
	do {
		spdlog::trace("!--------- Start BINANCE stream ----------!");

		// The io_context is required for all I/O
		net::io_context io_context_binance;
		// The SSL context is required, and holds certificates
		ssl::context ssl_context_binance{ssl::context::tlsv12_client};
		// This holds the root certificate used for verification
		load_root_certificates(ssl_context_binance);

		// Launch the asynchronous operation
		// https://github.com/binance/binance-spot-api-docs/blob/master/web-socket-streams.md
		// {\"method\": \"SUBSCRIBE\", \"params\": [\"btcusdt@trade\"], \"id\": 1}
		std::make_shared<Wsession_Binance>(io_context_binance,
										   ssl_context_binance,
										   alive,
										   all_requests.at("Binance"),
										   all_responses.at("Binance"),
										   all_prices.at("Binance"))
			->start("stream.binance.com",
					"9443",
					"/stream?streams=btcusdt@trade",
					"");

		io_context_binance.run();

		spdlog::trace("!--------- Close BINANCE stream ----------!");
	} while (*alive);

	spdlog::debug("BINANCE websocket thread exit");
}