
#include "gui_imgui.hpp"

#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include "imgui.h"
#include "implot.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <stdio.h>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "common/communication.hpp"

#include "WindowExchangeInstuments.hpp"

#include "InstrumentPlot.hpp"

#include <csignal>

//------------------------------------------------------------------------------

static void glfw_error_callback(int error, const char *description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

//------------------------------------------------------------------------------

void window_exchanges(bool *p_open,
					  AllInstrumentsInfo &all_instruments,
					  AllRequests &all_requests,
					  AllResponses &all_responses,
					  AllTicks &all_ticks)
{
	ImGui::SetNextWindowPos(ImVec2(25, 240), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(125, 125), ImGuiCond_FirstUseEver);

	ImGui::Begin("Exchanges", (bool *)NULL, ImGuiWindowFlags_None);

	// Pointers to the windows of the exchanges
	static std::map<std::string, std::unique_ptr<WindowExchangeInstuments>> window_exchange;

	// Local map for storing all ticks of all instruments of all exchanges
	static TicksOfExchanges ticks_of_exchanges;

	// Walk through the exchanges
	for (const auto &exchange : all_instruments) {

		auto exchange_name = exchange.first;
		auto instruments_info_map = exchange.second;

		// Add an exchange if it doesn't already exist
		if (ticks_of_exchanges.find(exchange_name) == ticks_of_exchanges.end())
			ticks_of_exchanges[exchange_name] = std::make_shared<TicksOfInstruments>();

		// Submap of all ticks of all instruments of the selected exchange
		auto ticks_of_instruments = ticks_of_exchanges[exchange_name];

		// Getting ticks from the lock-free queue
		std::shared_ptr<Tick> tick;
		while (all_ticks[exchange_name]->pop(tick)) {
			auto instrument_name = tick->get_instrument();

			// Add a tick submap of the instrument whose tick is received, if there is no such submap yet
			auto ticks_by_timestamp = ticks_of_instruments->find(instrument_name);
			if (ticks_by_timestamp == ticks_of_instruments->end())
				ticks_of_instruments->insert_or_assign(instrument_name, std::make_unique<TicksByTimestamps>());

			// Add a tick to the instrument ticks submap, sorted by ticks timestamps
			auto ticks_by_timestamps = ticks_of_instruments->find(instrument_name)->second;
			ticks_by_timestamps->insert_or_assign(tick->get_timestamp(), tick);

			// Erase all ticks older than 1 minute
			for (auto tick_to_erase = ticks_by_timestamps->begin();
				 tick_to_erase != ticks_by_timestamps->end();
				 ++tick_to_erase) {

				if (tick_to_erase->first + 60000 < tick->get_timestamp())
					ticks_by_timestamps->erase(tick_to_erase);
				else
					break;
			}
		}

		// Exchange checkbox
		ImGui::Checkbox(exchange_name.c_str(), &instruments_info_map->gui_selected);

		// If the exchange checkbox is checked
		if (instruments_info_map->gui_selected) {
			// Create an exchange window if it doesn't already exist
			if (window_exchange.find(exchange_name) == window_exchange.end())
				window_exchange[exchange_name] = std::make_unique<WindowExchangeInstuments>(exchange_name,
																							instruments_info_map,
																							all_requests[exchange_name],
																							all_responses[exchange_name],
																							ticks_of_exchanges[exchange_name]);

			ImGui::SetNextWindowPos(ImVec2(675, 20), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(420, 470), ImGuiCond_FirstUseEver);

			// Show and interact with the exchange window
			window_exchange[exchange_name]->interact(&instruments_info_map->gui_selected);
		}
		// Remove the exchange window if the exchange checkbox is not checked
		else if (window_exchange.find(exchange_name) != window_exchange.end())
			window_exchange.erase(exchange_name);
	}

	ImGui::End();
}

//------------------------------------------------------------------------------

void gui_imgui_thread_func(std::atomic<bool> *alive,
						   boost::asio::io_context &ioc,
						   AllInstrumentsInfo &all_instruments,
						   AllRequests &all_requests,
						   AllResponses &all_responses,
						   AllTicks &all_ticks)
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit())
		return;

	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	// Create window with graphics context
	GLFWwindow *window = glfwCreateWindow(1280, 720, "HFT tool", NULL, NULL);
	if (window == NULL)
		return;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	bool show_imgui_demo_window = false;
	bool show_implot_demo_window = false;

	// ImVec4 clear_color = ImVec4(0.19f, 0.67f, 0.53f, 1.00f); // Light green
	ImVec4 clear_color = ImVec4(0.257f, 0.407f, 0.363f, 1.00f); // Dark green

	// Experiments
	bool show_exchanges = true;
	ImPlot::GetStyle().AntiAliasedLines = true;

	// Main loop
	while (!glfwWindowShouldClose(window) && *alive) {
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// 1. Show exchanges list
		window_exchanges(&show_exchanges, all_instruments, all_requests, all_responses, all_ticks);

		// 2. Show demo windows to visual demonstrate the capabilities of ImGui and ImPlot
		ImGui::Begin("GUI Demo");

		ImGui::Checkbox("ImGui Demo Window", &show_imgui_demo_window);
		ImGui::Checkbox("ImPlot Demo Window", &show_implot_demo_window);

		ImGui::ColorEdit3("clear color", (float *)&clear_color);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		if (show_imgui_demo_window)
			ImGui::ShowDemoWindow(&show_imgui_demo_window);
		if (show_implot_demo_window)
			ImPlot::ShowDemoWindow(&show_implot_demo_window);

		// Rendering
		ImGui::Render();

		int display_w, display_h;

		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	*alive = false;
	ioc.stop();

	spdlog::debug("ImGui thread exit");
}