#ifndef _THREADFUNCS_CURRENCYCOM_HPP_
#define _THREADFUNCS_CURRENCYCOM_HPP_

#include <thread>

#include "common/communication.hpp"

void rest_thread_func_currencycom(std::atomic<bool> *alive,
								  AllInstrumentsInfo &all_instruments,
								  AllRequests &all_requests,
								  AllResponses &all_responses);

void ws_thread_func_currencycom(std::atomic<bool> *alive,
								AllRequests &all_requests,
								AllResponses &all_responses,
								AllTicks &all_prices);

#endif // _THREADFUNCS_CURRENCYCOM_HPP_