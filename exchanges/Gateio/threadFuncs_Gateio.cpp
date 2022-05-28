#include "threadFuncs_Gateio.hpp"

#include <boost/asio/spawn.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include "RestSession_Gateio.hpp"
#include "Wsession_Gateio.hpp"
#include "common/root_certificates.hpp"

void rest_thread_func_gateio(std::atomic<bool> *alive,
							 AllInstrumentsInfo &all_instruments,
							 AllRequests &all_requests,
							 AllResponses &all_responses)
{
	int timer = 0;

	do {
		if (--timer <= 0) {
			timer = 30;

			spdlog::trace("!--------- Begin GATEIO GET request ----------!");

			// List of instruments: curl -X GET "https://api.gateio.ws/api/v4/spot/currency_pairs"
			std::string const host = "api.gateio.ws";
			std::string const port = "443";
			std::string const target = "/api/v4/spot/currency_pairs";
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
			boost::asio::spawn(ioc, std::bind(&RestSession_Gateio::do_session,
											  host,
											  port,
											  target,
											  all_instruments.at("Gateio"),
											  version,
											  std::ref(ioc),
											  std::ref(ctx),
											  std::placeholders::_1));

			// Run the I/O service. The call will return when
			// the get operation is complete.
			ioc.run();

			spdlog::trace("!--------- End GATEIO GET request ----------!");
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while (*alive);

	spdlog::debug("GATEIO RestAPI thread exit");
}

// -----------------------------------------------------------------------------

void ws_thread_func_gateio(std::atomic<bool> *alive,
						   AllRequests &all_requests,
						   AllResponses &all_responses,
						   AllTicks &all_prices)
{
	do {
		spdlog::trace("!--------- Start GATEIO stream ----------!");

		// The io_context is required for all I/O
		net::io_context io_context_gateio;
		// The SSL context is required, and holds certificates
		ssl::context ssl_context_gateio{ssl::context::tlsv12_client};
		// This holds the root certificate used for verification
		load_root_certificates(ssl_context_gateio);

		// Launch the asynchronous operation
		// https://www.gate.io/docs/websocket/index.html
		std::make_shared<Wsession_Gateio>(io_context_gateio,
										  ssl_context_gateio,
										  alive,
										  all_requests.at("Gateio"),
										  all_responses.at("Gateio"),
										  all_prices.at("Gateio"))
			->start("ws.gate.io",
					"443",
					"/v3",
					"{\"id\": 55555, \"method\": \"trades.subscribe\", \"params\": [\"BTC_USDT\"]}");

		io_context_gateio.run();

		spdlog::trace("!--------- Close GATEIO stream ----------!");
	} while (*alive);

	spdlog::debug("GATEIO websocket thread exit");
}