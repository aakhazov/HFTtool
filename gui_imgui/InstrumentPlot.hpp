#ifndef _INSTRUMENT_PLOT_HPP_
#define _INSTRUMENT_PLOT_HPP_

#include "imgui.h"
#include "implot.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <stdio.h>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "common/communication.hpp"

// Hierarchy of maps for storing ticks
typedef std::map<std::uint64_t, std::shared_ptr<Tick>> TicksByTimestamps;
typedef std::map<std::string, std::shared_ptr<TicksByTimestamps>> TicksOfInstruments;
typedef std::map<std::string, std::shared_ptr<TicksOfInstruments>> TicksOfExchanges;

void instrument_plot(bool *p_open,
					 std::string const &exchange_name,
					 std::string const &instrument_name,
					 std::shared_ptr<TicksByTimestamps> &ticks_by_timestamps);

#endif // _INSTRUMENT_PLOT_HPP_