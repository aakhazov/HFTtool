#include "InstrumentPlot.hpp"

#include "implot_internal.h"

//------------------------------------------------------------------------------

template <typename MapItr>
auto get_closest_map_iterator(const MapItr &prev, const MapItr &lower, std::int64_t key) -> const MapItr &
{
	return abs(static_cast<std::int64_t>(prev->first) - key) < abs(static_cast<std::int64_t>(lower->first) - key)
			   ? prev
			   : lower;
}

//------------------------------------------------------------------------------

void plot_chart_and_tooltip(const char *label_id,
							std::shared_ptr<TicksByTimestamps> &ticks_by_timestamps,
							ImVec4 const &color_price,
							ImVec4 const &color_price_selected,
							ImVec4 const &color_size,
							ImVec4 const &color_size_selected)
{

	// get ImGui window DrawList
	ImDrawList *draw_list = ImPlot::GetPlotDrawList();

	// Show chart tooltip on mouse hover
	if (ImPlot::IsPlotHovered()) {
		// Get mouse hover timestamp on 'price' chart
		ImPlotPoint mouse_price = ImPlot::GetPlotMousePos(ImAxis_X1, ImAxis_Y1);
		mouse_price.x = ImPlot::RoundTime(ImPlotTime::FromDouble(mouse_price.x), ImPlotTimeUnit_Ms).ToDouble();

		// The minimum grey bar width is 0.5 ms
		double half_width = 0.00025;

		// X - coordinates of the grey bar when the chart is scaled down
		float tool_l_big = ImPlot::PlotToPixels(mouse_price.x, mouse_price.y).x - 1.5;
		float tool_r_big = ImPlot::PlotToPixels(mouse_price.x, mouse_price.y).x + 1.5;

		// X - coordinates of the grey bar when the chart is zoomed in to see milliseconds dimensions
		float tool_l_small = ImPlot::PlotToPixels(mouse_price.x - half_width * 1.5, mouse_price.y).x;
		float tool_r_small = ImPlot::PlotToPixels(mouse_price.x + half_width * 1.5, mouse_price.y).x;

		float tool_l = tool_l_big;
		float tool_r = tool_r_big;

		// Select the grey bar width between the calculated value and the hardcoded value
		// according to the current chart scale
		if ((tool_r_small - tool_l_small) > (tool_r_big - tool_l_big)) {
			tool_l = tool_l_small;
			tool_r = tool_r_small;
		}

		// Y - coordinates of the grey bar
		float tool_t = ImPlot::GetPlotPos().y;
		float tool_b = tool_t + ImPlot::GetPlotSize().y;

		// Calculating the closest timestamp where the mouse cursor is
		auto mouse_timestamp = static_cast<std::int64_t>(std::round(mouse_price.x * 1000));
		auto tick_by_timestamp_lower = ticks_by_timestamps->lower_bound(mouse_timestamp);
		auto tick_by_timestamp_prev = (tick_by_timestamp_lower == ticks_by_timestamps->begin())
										  ? tick_by_timestamp_lower
										  : tick_by_timestamp_lower--;
		auto tick_by_timestamp = get_closest_map_iterator(tick_by_timestamp_prev,
														  tick_by_timestamp_lower,
														  mouse_timestamp);

		// Draw grey bar
		ImPlot::PushPlotClipRect();
		draw_list->AddRectFilled(ImVec2(tool_l, tool_t), ImVec2(tool_r, tool_b), IM_COL32(128, 128, 128, 64));
		ImPlot::PopPlotClipRect();

		// Render tooltip
		if (tick_by_timestamp != ticks_by_timestamps->end()) {
			// Highlight a 'price' point
			{
				ImVec2 point_price = ImPlot::PlotToPixels(tick_by_timestamp->first / 1000.0,
														  tick_by_timestamp->second->get_price_double(),
														  ImAxis_X1, ImAxis_Y1);

				ImU32 u32_color_price = ImGui::GetColorU32(color_price_selected);

				draw_list->AddCircleFilled(point_price, 3.5, u32_color_price, 6);
			}

			// Highlight a 'size' point
			{
				ImVec2 point_size = ImPlot::PlotToPixels(tick_by_timestamp->first / 1000.0,
														 tick_by_timestamp->second->get_size_double(),
														 ImAxis_X1, ImAxis_Y2);

				ImU32 u32_color_size = ImGui::GetColorU32(color_size_selected);

				draw_list->AddCircleFilled(point_size, 3.5, u32_color_size, 6);
			}

			// Print tooltip values
			{
				ImGui::BeginTooltip();

				char buff[32];

				ImPlot::FormatDate(ImPlotTime::FromDouble(tick_by_timestamp->first / 1000.0),
								   buff,
								   32,
								   ImPlotDateFmt_DayMoYr,
								   ImPlot::GetStyle().UseISO8601);
				ImGui::Text("Date:  %s", buff);

				ImPlot::FormatTime(ImPlotTime::FromDouble(tick_by_timestamp->first / 1000.0),
								   buff,
								   32,
								   ImPlotTimeFmt_HrMinSMs,
								   true);
				ImGui::Text("Time:  %s", buff);

				ImGui::Text("Price: %s", tick_by_timestamp->second->get_price_str().c_str());
				ImGui::Text("Size:  %s", tick_by_timestamp->second->get_size_str().c_str());

				ImGui::Text("%s", tick_by_timestamp->second->is_buyer() ? "Buyer" : "Seller");

				ImGui::EndTooltip();
			}
		}
	}

	ImU32 u32_color_price = ImGui::GetColorU32(color_price);
	ImU32 u32_color_size = ImGui::GetColorU32(color_size);

	// Left position of the 'price' axis
	ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
	// Begin plot 'price' chart
	if (ImPlot::BeginItem("price")) {
		// Override legend icon color
		ImPlot::GetCurrentItem()->Color = u32_color_price;

		// Fit data if requested
		if (ImPlot::FitThisFrame()) {
			for (const auto &tick_by_timestamp : *ticks_by_timestamps) {
				ImPlot::FitPoint(ImPlotPoint(tick_by_timestamp.first / 1000.0,
											 tick_by_timestamp.second->get_price_double()));
			}
		}

		// Previous point of first line of the chart
		ImVec2 prev_point = ImPlot::PlotToPixels(ticks_by_timestamps->begin()->first / 1000.0,
												 ticks_by_timestamps->begin()->second->get_price_double());

		// Render data
		for (const auto &tick_by_timestamp : *ticks_by_timestamps) {
			ImVec2 point = ImPlot::PlotToPixels(tick_by_timestamp.first / 1000.0,
												tick_by_timestamp.second->get_price_double());
			// Draw the point
			draw_list->AddCircleFilled(point, 2, u32_color_price, 6);
			// and line between points
			draw_list->AddLine(prev_point, point, u32_color_price);
			prev_point = point;
		}

		// End plot 'price' chart
		ImPlot::EndItem();
	}

	// Right position of the 'price' axis
	ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
	// Begin plot 'size' chart
	if (ImPlot::BeginItem("size")) {
		// Override legend icon color
		ImPlot::GetCurrentItem()->Color = u32_color_size;

		// Fit data if requested
		if (ImPlot::FitThisFrame()) {
			for (const auto &tick_by_timestamp : *ticks_by_timestamps) {
				ImPlot::FitPoint(ImPlotPoint(tick_by_timestamp.first / 1000.0,
											 tick_by_timestamp.second->get_size_double()));
			}
		}

		// Previous point of first line of the chart
		ImVec2 prev_point = ImPlot::PlotToPixels(ticks_by_timestamps->begin()->first / 1000.0,
												 ticks_by_timestamps->begin()->second->get_price_double());

		// Render data
		for (const auto &tick_by_timestamp : *ticks_by_timestamps) {
			ImVec2 point = ImPlot::PlotToPixels(tick_by_timestamp.first / 1000.0,
												tick_by_timestamp.second->get_size_double());
			// Draw the point
			draw_list->AddCircleFilled(point, 2, u32_color_size, 6);
			// and line between points
			draw_list->AddLine(prev_point, point, u32_color_size);
			prev_point = point;
		}

		// End plot 'size' chart
		ImPlot::EndItem();
	}
}

void instrument_plot(bool *p_open,
					 std::string const &exchange_name,
					 std::string const &instrument_name,
					 std::shared_ptr<TicksByTimestamps> &ticks_by_timestamps)
{
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(600, 335), ImGuiCond_FirstUseEver);

	// Begin drawing the chart window
	auto plot_name = exchange_name + ": " + instrument_name;
	ImGui::Begin(plot_name.c_str(), p_open, ImGuiWindowFlags_NoBackground);

	// Color of 'price' chart
	static ImVec4 color_price = ImVec4(0.0f, 1.0f, 0.0f, 0.5f);
	// Color of 'price' mouse hovered points
	static ImVec4 color_price_selected = ImVec4(1.0f, 1.0f, 1.0f, 1.000f);
	// Color adjustment dialog
	// ImGui::ColorEdit4("##color_price", &color_price.x, ImGuiColorEditFlags_NoInputs);
	// ImGui::SameLine();
	// ImGui::ColorEdit4("##color_price_selected", &color_price_selected.x, ImGuiColorEditFlags_NoInputs);

	// Color of 'size' chart
	static ImVec4 color_size = ImVec4(0.0f, 0.0f, 1.0f, 0.5f);
	// Color of 'size' mouse hovered points
	static ImVec4 color_size_selected = ImVec4(1.0f, 0.8f, 0.8f, 1.000f);
	// Color adjustment dialog
	// ImGui::ColorEdit4("##color_size", &color_size.x, ImGuiColorEditFlags_NoInputs);
	// ImGui::SameLine();
	// ImGui::ColorEdit4("##color_size_selected", &color_size_selected.x, ImGuiColorEditFlags_NoInputs);

	ImPlot::GetStyle().UseLocalTime = false;
	ImPlot::GetStyle().Use24HourClock = true;

	// ###chart_head - the name of chart inside ImGui always 'chart_head'
	// but the display name is the number of ticks in the chart
	std::string chart_head = (ticks_by_timestamps == nullptr)
								 ? ("Ticks total: 0###chart_head")
								 : ("Ticks total: " + std::to_string(ticks_by_timestamps->size()) + "###chart_head");

	// Begin drawing the chart
	if (ImPlot::BeginPlot(chart_head.c_str(), ImVec2(-1, 0), ImPlotFlags_NoMouseText)) {
		ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_Time);
		ImPlot::SetupAxis(ImAxis_Y1, "price", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit);

		ImPlot::SetupAxisLimits(ImAxis_X1, std::time(nullptr), std::time(nullptr) + 60);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);

		ImPlot::SetupAxis(ImAxis_Y2, "size", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit | ImPlotAxisFlags_AuxDefault);
		ImPlot::SetupAxisLimits(ImAxis_Y2, 0, 1);

		// The chart
		if (ticks_by_timestamps != nullptr)
			plot_chart_and_tooltip("trade.detail",
								   ticks_by_timestamps,
								   color_price,
								   color_price_selected,
								   color_size,
								   color_size_selected);

		ImPlot::EndPlot();
	}

	ImGui::End();
}