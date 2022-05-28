#include "threadFuncs_Huobi.hpp"

#include <boost/asio/spawn.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include "RestSession_Huobi.hpp"
#include "Wsession_Huobi.hpp"
#include "common/root_certificates.hpp"

void rest_thread_func_huobi(std::atomic<bool> *alive,
							AllInstrumentsInfo &all_instruments,
							AllRequests &all_requests,
							AllResponses &all_responses)
{
	int timer = 0;

	do {
		if (--timer <= 0) {
			timer = 30;

			spdlog::trace("!--------- Begin HUOBI GET request ----------!");

			// List of instruments: curl -X GET "https://api.huobi.pro/v1/common/symbols"
			// or
			// ! List of instruments: curl -X GET "https://api.huobi.pro/v2/settings/common/symbols"
			std::string const host = "api.huobi.pro";
			std::string const port = "443";
			std::string const target = "/v2/settings/common/symbols";
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
			boost::asio::spawn(ioc, std::bind(&RestSession_Huobi::do_session,
											  host,
											  port,
											  target,
											  all_instruments.at("Huobi"),
											  version,
											  std::ref(ioc),
											  std::ref(ctx),
											  std::placeholders::_1));

			// Run the I/O service. The call will return when
			// the get operation is complete.
			ioc.run();

			spdlog::trace("!--------- End HUOBI GET request ----------!");
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while (*alive);

	spdlog::debug("HUOBI RestAPI thread exit");
}

// -----------------------------------------------------------------------------

void ws_thread_func_huobi(std::atomic<bool> *alive,
						  AllRequests &all_requests,
						  AllResponses &all_responses,
						  AllTicks &all_prices)
{
	do {
		spdlog::trace("!--------- Start HUOBI stream ----------!");

		// The io_context is required for all I/O
		net::io_context io_context_huobi;
		// The SSL context is required, and holds certificates
		ssl::context ssl_context_huobi{ssl::context::tlsv12_client};
		// This holds the root certificate used for verification
		load_root_certificates(ssl_context_huobi);

		// Launch the asynchronous operation
		// https://huobiapi.github.io/docs/spot/v1/en/#websocket-market-data
		std::make_shared<Wsession_Huobi>(io_context_huobi,
										 ssl_context_huobi,
										 alive,
										 all_requests.at("Huobi"),
										 all_responses.at("Huobi"),
										 all_prices.at("Huobi"))
			->start("api.huobi.pro",
					"443",
					"/ws",
					"{\"sub\": \"market.btcusdt.trade.detail\", \"id\": \"55555\"}");

		io_context_huobi.run();

		spdlog::trace("!--------- Close HUOBI stream ----------!");
	} while (*alive);

	spdlog::debug("HUOBI websocket thread exit");
}