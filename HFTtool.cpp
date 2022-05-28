#include <thread>

#include <boost/asio/signal_set.hpp>

#include "exchanges/Binance/threadFuncs_Binance.hpp"
#include "exchanges/Currencycom/threadFuncs_Currencycom.hpp"
#include "exchanges/Gateio/threadFuncs_Gateio.hpp"
#include "exchanges/Huobi/threadFuncs_Huobi.hpp"

#include "gui_imgui/gui_imgui.hpp"

#include "common/communication.hpp"

void signal_handler(const boost::system::error_code &error, int signal_number)
{
	std::cout << std::endl;
	spdlog::debug("App exit due to an interrupt signal {0:d}", signal_number);
}

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	// The io_context is required for all I/O
	boost::asio::io_context ioc;
	// Construct a signal set registered for process termination.
	boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
	// Start an asynchronous wait for one of the signals to occur.
	signals.async_wait(signal_handler);

	// Trading allowed instruments
	AllInstrumentsInfo all_instruments{{"Binance", std::make_shared<InstrumentsInfoMap>()},
									   {"Huobi", std::make_shared<InstrumentsInfoMap>()},
									   {"Gateio", std::make_shared<InstrumentsInfoMap>()},
									   {"Currencycom", std::make_shared<InstrumentsInfoMap>()}};

	// Exchange requests
	AllRequests all_requests{{"Binance", std::make_shared<InstrumentsRequestsMap>()},
							 {"Huobi", std::make_shared<InstrumentsRequestsMap>()},
							 {"Gateio", std::make_shared<InstrumentsRequestsMap>()},
							 {"Currencycom", std::make_shared<InstrumentsRequestsMap>()}};

	// Exchange responses
	AllResponses all_responses{{"Binance", std::make_shared<InstrumentsResponsesMap>()},
							   {"Huobi", std::make_shared<InstrumentsResponsesMap>()},
							   {"Gateio", std::make_shared<InstrumentsResponsesMap>()},
							   {"Currencycom", std::make_shared<InstrumentsResponsesMap>()}};

	// Exchange prices
	AllTicks all_ticks{{"Binance", std::make_shared<TicksQueue>()},
					   {"Huobi", std::make_shared<TicksQueue>()},
					   {"Gateio", std::make_shared<TicksQueue>()},
					   {"Currencycom", std::make_shared<TicksQueue>()}};

	try {
		// Customize msg format for all loggers
		spdlog::set_pattern("[%D %H:%M:%S] [%^%L%$] [thread %t] %v");
		// Set global log level to info
		spdlog::set_level(spdlog::level::debug);

		// Flush all *registered* loggers using a worker thread every 3 seconds.
		// note: registered loggers *must* be thread safe for this to work correctly!
		spdlog::flush_every(std::chrono::seconds(3));
	}
	// Exceptions will only be thrown upon failed logger or sink construction (not during logging).
	catch (const spdlog::spdlog_ex &ex) {
		std::printf("Log initialization failed: %s\n", ex.what());
		exit(1);
	}

	std::atomic<bool> alive{true};

	// Run all threads

	std::thread restapi_thread_binance(&rest_thread_func_binance, &alive, std::ref(all_instruments), std::ref(all_requests), std::ref(all_responses));
	std::thread restapi_thread_huobi(&rest_thread_func_huobi, &alive, std::ref(all_instruments), std::ref(all_requests), std::ref(all_responses));
	std::thread restapi_thread_gateio(&rest_thread_func_gateio, &alive, std::ref(all_instruments), std::ref(all_requests), std::ref(all_responses));
	std::thread restapi_thread_currencycom(&rest_thread_func_currencycom, &alive, std::ref(all_instruments), std::ref(all_requests), std::ref(all_responses));

	std::thread websocket_thread_binance(&ws_thread_func_binance, &alive, std::ref(all_requests), std::ref(all_responses), std::ref(all_ticks));
	std::thread websocket_thread_huobi(&ws_thread_func_huobi, &alive, std::ref(all_requests), std::ref(all_responses), std::ref(all_ticks));
	std::thread websocket_thread_gateio(&ws_thread_func_gateio, &alive, std::ref(all_requests), std::ref(all_responses), std::ref(all_ticks));
	std::thread websocket_thread_currencycom(&ws_thread_func_currencycom, &alive, std::ref(all_requests), std::ref(all_responses), std::ref(all_ticks));

	std::thread gui_imgui_thread(&gui_imgui_thread_func,
								 &alive,
								 std::ref(ioc),
								 std::ref(all_instruments),
								 std::ref(all_requests),
								 std::ref(all_responses),
								 std::ref(all_ticks));

	// Run the I/O services.
	ioc.run();

	alive = false;

	// Join all threads
	spdlog::debug("Join all threads");

	gui_imgui_thread.join();

	websocket_thread_binance.join();
	websocket_thread_huobi.join();
	websocket_thread_gateio.join();
	websocket_thread_currencycom.join();

	restapi_thread_binance.join();
	restapi_thread_huobi.join();
	restapi_thread_gateio.join();
	restapi_thread_currencycom.join();

	// Apply some function on all registered loggers
	spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { l->debug("App exit"); });
	// Release all spdlog resources, and drop all loggers in the registry
	spdlog::shutdown();

	return EXIT_SUCCESS;
}
