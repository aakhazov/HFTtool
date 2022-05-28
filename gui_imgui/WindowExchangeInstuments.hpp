#ifndef _WINDOW_EXCHANGE_INSTRUMENTS_HPP_
#define _WINDOW_EXCHANGE_INSTRUMENTS_HPP_

#include "imgui.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <stdio.h>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "common/communication.hpp"

#include "InstrumentPlot.hpp"

enum InstrumentsTableColumnType {
	InstrumentsTableColumnType_Number = 0,
	InstrumentsTableColumnType_Instrument = 1,
	InstrumentsTableColumnType_Tradable = 2,
	InstrumentsTableColumnType_TradeDetail = 3,
	InstrumentsTableColumnType_Status = 4
};

//--------------------------------------------------------------------------

struct InstrumentsTableItem {
	int Number = 0;
	char Name[64] = {0};
	char Status[64] = {0};
	bool Tradable = false;

	bool Plot = false;

	// We have a problem which is affecting _only this demo_ and should not affect your code:
	// As we don't rely on std:: or other third-party library to compile dear imgui, we only have reliable access to qsort(),
	// however qsort doesn't allow passing user data to comparing function.
	// As a workaround, we are storing the sort specs in a static/global for the comparing function to access.
	// In your own use case you would probably pass the sort specs to your sorting/comparing functions directly and not use a global.
	// We could technically call ImGui::TableGetSortSpecs() in CompareWithSortSpecs(), but considering that this function is called
	// very often by the sorting algorithm it would be a little wasteful.
	static const ImGuiTableSortSpecs *s_current_sort_specs;

	// Compare function to be used by qsort()
	static int CompareWithSortSpecs(const void *lhs, const void *rhs)
	{
		const InstrumentsTableItem *a = (const InstrumentsTableItem *)lhs;
		const InstrumentsTableItem *b = (const InstrumentsTableItem *)rhs;

		for (int n = 0; n < s_current_sort_specs->SpecsCount; n++) {
			// Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
			// We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
			const ImGuiTableColumnSortSpecs *sort_spec = &s_current_sort_specs->Specs[n];
			int delta = 0;

			switch (sort_spec->ColumnUserID) {
				case InstrumentsTableColumnType_Number:
					delta = (a->Number - b->Number);
					break;
				case InstrumentsTableColumnType_Instrument:
					delta = (strcmp(a->Name, b->Name));
					break;
				case InstrumentsTableColumnType_Tradable:
					delta = (a->Tradable - b->Tradable);
					break;
				case InstrumentsTableColumnType_TradeDetail:
					delta = (strcmp(a->Name, b->Name));
					break;
				case InstrumentsTableColumnType_Status:
					delta = (strcmp(a->Status, b->Status));
					break;
				default:
					IM_ASSERT(0);
					break;
			}

			if (delta > 0)
				return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;

			if (delta < 0)
				return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? -1 : +1;
		}

		// qsort() is instable so always return a way to differenciate items.
		// Your own compare function may want to avoid fallback on implicit sort specs e.g. a Name compare if it wasn't already part of the sort specs.
		return strcmp(a->Name, b->Name);
	}
};

//--------------------------------------------------------------------------

class WindowExchangeInstuments {
private:
	const ImGuiTableFlags flags_ =
		ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

	const float inner_width_with_scroll_ = 0.0f; // Auto-extend
	const bool outer_size_enabled_ = true;
	const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
	const ImVec2 outer_size_value_ = ImVec2(0.0f, TEXT_BASE_HEIGHT * 24);
	const int freeze_cols_ = 1;
	const int freeze_rows_ = 1;
	const bool show_headers_ = true;
	const float row_min_height_ = 0.0f; // Auto

	enum ContentsType { CT_Text,
						CT_Button,
						CT_SmallButton,
						CT_FillButton,
						CT_Selectable,
						CT_SelectableSpanRow };
	const int contents_type_ = CT_SelectableSpanRow;

	ImVector<InstrumentsTableItem> table_items_;
	ImVector<int> table_selection_;
	bool items_need_sort_ = false;

	std::string exchange_name_;
	std::shared_ptr<InstrumentsInfoMap> &instruments_info_map_;
	std::shared_ptr<InstrumentsRequestsMap> &instruments_requests_map_;
	std::shared_ptr<InstrumentsResponsesMap> &instruments_responses_map_;
	std::shared_ptr<TicksOfInstruments> &ticks_of_instruments_;

public:
	WindowExchangeInstuments(std::string &exchange_name,
							 std::shared_ptr<InstrumentsInfoMap> &instruments_info_map,
							 std::shared_ptr<InstrumentsRequestsMap> &instruments_requests_map,
							 std::shared_ptr<InstrumentsResponsesMap> &instruments_responses_map,
							 std::shared_ptr<TicksOfInstruments> &ticks_of_instruments)
		: exchange_name_(exchange_name),
		  instruments_info_map_(instruments_info_map),
		  instruments_requests_map_(instruments_requests_map),
		  instruments_responses_map_(instruments_responses_map),
		  ticks_of_instruments_(ticks_of_instruments)
	{
	}

	//----------------------------------------------------------------------

	void interact(bool *p_open);
};

#endif // _WINDOW_EXCHANGE_INSTRUMENTS_HPP_