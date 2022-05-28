#include "threadFuncs_Currencycom.hpp"

#include <boost/asio/spawn.hpp>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include "RestSession_Currencycom.hpp"
#include "Wsession_Currencycom.hpp"
#include "common/root_certificates.hpp"

void rest_thread_func_currencycom(std::atomic<bool> *alive,
								  AllInstrumentsInfo &all_instruments,
								  AllRequests &all_requests,
								  AllResponses &all_responses)
{
	int timer = 0;

	do {
		if (--timer <= 0) {
			timer = 30;

			spdlog::trace("!--------- Begin CURRENCYCOM GET request ----------!");

			// List of instruments: curl -X GET "https://api-adapter.backend.currency.com/api/v1/exchangeInfo"
			// or
			// ! List of instruments: curl -X GET "https://api-adapter.backend.currency.com/api/v2/exchangeInfo"
			std::string const host = "api-adapter.backend.currency.com";
			std::string const port = "443";
			std::string const target = "/api/v2/exchangeInfo";
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
			boost::asio::spawn(ioc, std::bind(&RestSession_Currencycom::do_session,
											  host,
											  port,
											  target,
											  all_instruments.at("Currencycom"),
											  version,
											  std::ref(ioc),
											  std::ref(ctx),
											  std::placeholders::_1));

			// Run the I/O service. The call will return when
			// the get operation is complete.
			ioc.run();

			spdlog::trace("!--------- End CURRENCYCOM GET request ----------!");
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while (*alive);

	spdlog::debug("CURRENCYCOM RestAPI thread exit");
}

// -----------------------------------------------------------------------------

void ws_thread_func_currencycom(std::atomic<bool> *alive,
								AllRequests &all_requests,
								AllResponses &all_responses,
								AllTicks &all_prices)
{
	do {
		spdlog::trace("!--------- Start CURRENCYCOM stream ----------!");

		// The io_context is required for all I/O
		net::io_context io_context_currencycom;
		// The SSL context is required, and holds certificates
		ssl::context ssl_context_currencycom{ssl::context::tlsv12_client};
		// This holds the root certificate used for verification
		load_root_certificates(ssl_context_currencycom);

		// Launch the asynchronous operation
		// https://apitradedoc.currency.com/swagger-ui.html
		// List of instruments: {\"destination\": \"/api/v2/exchangeInfo\", \"correlationId\": 0, \"payload\": {}}
		std::make_shared<Wsession_Currencycom>(io_context_currencycom,
											   ssl_context_currencycom,
											   alive,
											   all_requests.at("Currencycom"),
											   all_responses.at("Currencycom"),
											   all_prices.at("Currencycom"))
			->start("api-adapter.backend.currency.com",
					"443",
					"/connect",
					"");

		io_context_currencycom.run();

		spdlog::trace("!--------- Close CURRENCYCOM stream ----------!");
	} while (*alive);

	spdlog::debug("CURRENCYCOM websocket thread exit");
}