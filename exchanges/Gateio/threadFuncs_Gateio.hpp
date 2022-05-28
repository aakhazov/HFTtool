#ifndef _THREADFUNCS_GATEIO_HPP_
#define _THREADFUNCS_GATEIO_HPP_

#include <thread>

#include "common/communication.hpp"

void rest_thread_func_gateio(std::atomic<bool> *alive,
							 AllInstrumentsInfo &all_instruments,
							 AllRequests &all_requests,
							 AllResponses &all_responses);

void ws_thread_func_gateio(std::atomic<bool> *alive,
						   AllRequests &all_requests,
						   AllResponses &all_responses,
						   AllTicks &all_prices);

#endif // _THREADFUNCS_GATEIO_HPP_