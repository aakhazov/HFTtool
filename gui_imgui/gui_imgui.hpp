#ifndef _GUI_IMGUI_HPP_
#define _GUI_IMGUI_HPP_

#include <thread>

#include <boost/asio/signal_set.hpp>

#include "common/communication.hpp"

void gui_imgui_thread_func(std::atomic<bool> *alive,
						   boost::asio::io_context &ioc,
						   AllInstrumentsInfo &all_instruments,
						   AllRequests &all_requests,
						   AllResponses &all_responses,
						   AllTicks &all_ticks);

#endif // _GUI_IMGUI_HPP_