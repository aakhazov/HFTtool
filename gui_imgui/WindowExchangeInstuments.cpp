#include "WindowExchangeInstuments.hpp"

const ImGuiTableSortSpecs *InstrumentsTableItem::s_current_sort_specs = nullptr;

//--------------------------------------------------------------------------

void WindowExchangeInstuments::interact(bool *p_open)
{
	ImGui::Begin(exchange_name_.c_str(), p_open, ImGuiWindowFlags_None);
	ImGui::Text("%ld instruments total", instruments_info_map_->size());

	int instruments_info_map_size = instruments_info_map_->size();

	// Update the table if the number of instruments has changed
	if (table_items_.Size != instruments_info_map_size) {

		table_items_.resize(instruments_info_map_size, InstrumentsTableItem());

		int n = 0;

		// Go through all instruments
		for (auto instrument_info = instruments_info_map_->begin();
			 instrument_info != instruments_info_map_->end();
			 ++instrument_info) {

			auto instrument = instruments_info_map_->get_entry(instrument_info);

			std::string instrument_name = instrument.first;

			// Insert instrument into the table
			InstrumentsTableItem &item = table_items_[n];
			item.Number = n;
			strcpy(item.Name, instrument_name.c_str());
			strcpy(item.Status, "none");
			item.Tradable = instrument.second->is_tradable();

			// Table update completed
			if (++n == instruments_info_map_size)
				break;
		}
	}

	// Submit table
	const float inner_width_to_use = (flags_ & ImGuiTableFlags_ScrollX) ? inner_width_with_scroll_ : 0.0f;

	if (ImGui::BeginTable("instruments_table", 5, flags_, outer_size_enabled_ ? outer_size_value_ : ImVec2(0, 0), inner_width_to_use)) {

		// float indent_step = (float)((int)TEXT_BASE_WIDTH / 2);
		// ImGui::Indent(indent_step);
		// ImGui::Unindent(indent_step * 8.0f);
		// ImGui::SetNextItemWidth(TEXT_BASE_WIDTH * 30);
		// ImGui::PushItemWidth(TEXT_BASE_WIDTH * 30);
		// ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 15.0f);

		// Declare columns
		ImGui::TableSetupColumn("Num",
								ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed,
								33.0f,
								InstrumentsTableColumnType_Number);
		ImGui::TableSetupColumn("Instrument",
								ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed,
								103.0f,
								InstrumentsTableColumnType_Instrument);
		ImGui::TableSetupColumn("tradable",
								ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed,
								67.0f,
								InstrumentsTableColumnType_Tradable);
		ImGui::TableSetupColumn("trade.detail",
								ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed,
								86.0f,
								InstrumentsTableColumnType_TradeDetail);
		ImGui::TableSetupColumn("status",
								ImGuiTableColumnFlags_WidthFixed,
								51.0f,
								InstrumentsTableColumnType_Status);

		ImGui::TableSetupScrollFreeze(freeze_cols_, freeze_rows_);

		// Sort our data if sort specs have been changed!
		ImGuiTableSortSpecs *sorts_specs = ImGui::TableGetSortSpecs();

		if (sorts_specs && sorts_specs->SpecsDirty)
			items_need_sort_ = true;

		if (sorts_specs && items_need_sort_ && table_items_.Size > 1) {
			InstrumentsTableItem::s_current_sort_specs = sorts_specs; // Store in variable accessible by the sort function.

			qsort(&table_items_[0], (size_t)table_items_.Size, sizeof(table_items_[0]), InstrumentsTableItem::CompareWithSortSpecs);

			InstrumentsTableItem::s_current_sort_specs = NULL;

			sorts_specs->SpecsDirty = false;
		}

		items_need_sort_ = false;

		// Take note of whether we are currently sorting based on the 'status' field,
		// we will use this to trigger sorting when we know the data of this column has been modified.
		const bool sorts_specs_using_status = (ImGui::TableGetColumnFlags(InstrumentsTableColumnType_Status) &
											   ImGuiTableColumnFlags_IsSorted) != 0;

		// Show headers
		if (show_headers_)
			ImGui::TableHeadersRow();

		// Show data
		// FIXME-TABLE FIXME-NAV: How we can get decent up/down even though we have the buttons here?
		ImGui::PushButtonRepeat(true);

#if 1
		// Using clipper for large vertical lists
		ImGuiListClipper clipper;
		clipper.Begin(table_items_.Size);
		while (clipper.Step()) {
			for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
#else
		// Without clipper
		{
			for (int row_n = 0; row_n < table_items_.Size; row_n++) {
#endif
				InstrumentsTableItem *item = &table_items_[row_n];

				ImGui::PushID(item->Name);
				ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height_);

				// 'Num' column
				if (ImGui::TableSetColumnIndex(InstrumentsTableColumnType_Number)) {
					char label[32];
					sprintf(label, "%04d", item->Number);

					ImGui::TextUnformatted(label);
				}

				// 'Instrument' column
				if (ImGui::TableSetColumnIndex(InstrumentsTableColumnType_Instrument))
					ImGui::TextUnformatted(item->Name);

				// 'tradable' column
				if (ImGui::TableSetColumnIndex(InstrumentsTableColumnType_Tradable))
					ImGui::TextUnformatted(item->Tradable ? "yes" : "no");

				std::string instrument_name(item->Name);

				// 'trade.detail' column
				if (ImGui::TableSetColumnIndex(InstrumentsTableColumnType_TradeDetail)) {
					// Show chart interaction button if the instrument has a subscription
					if (0 != strcmp(item->Status, "none")) {
						const std::string text = item->Plot ? "hide" : "show";

						// Enable or disable plotting
						if (ImGui::SmallButton(text.c_str()))
							item->Plot = !item->Plot;

						// Chart plotting
						if (item->Plot) {
							auto ticks_of_instrument = ticks_of_instruments_->find(instrument_name);
							// Instrument ticks map, sorted by ticks timestamps
							auto ticks_by_timestamp = (ticks_of_instrument != ticks_of_instruments_->end()) ? ticks_of_instrument->second : nullptr;

							// Plot the chart
							instrument_plot(&item->Plot,
											exchange_name_,
											instrument_name,
											ticks_by_timestamp);
						}
					}
					// If the instrument does not have a subscription, show the 'subscribe' button
					else if (item->Tradable && ImGui::SmallButton("subscribe")) {
						// Submitting a subscription request
						instruments_requests_map_->update_or_add_entry(instrument_name + " trade.detail subscribe",
																	   std::make_shared<InstrumentRequest>("start"));
					}
				}

				// 'status' column
				if (ImGui::TableSetColumnIndex(InstrumentsTableColumnType_Status)) {
					// See an instrument responses
					auto instrument_entry = instruments_responses_map_->find_entry(instrument_name + " trade.detail subscribe");
					if (instrument_entry != nullptr) {
						// 'trade.detail' subscription status
						auto status = instrument_entry->get_status();

						// Update subscription status if it has changed
						if (0 != strcmp(item->Status, status.c_str())) {
							strcpy(item->Status, status.c_str());

							// Sort the table by the 'status' column, if the appropriate sort type is specified
							if (sorts_specs_using_status) {
								items_need_sort_ = true;
							}
						}
					}

					ImGui::TextUnformatted(item->Status);
				}

				ImGui::PopID();
			}
		}

		ImGui::PopButtonRepeat();

		ImGui::EndTable();
	}

	ImGui::End();
}
