#include "Wsession_Gateio.hpp"

void Wsession_Gateio::on_timer(const boost::system::error_code &ec)
{
	if (ec)
		return fail(ec, "timer");

	if (!*alive_ && ws_.is_open())
		// Close the WebSocket connection
		return ws_.async_close(beast::websocket::close_code::normal,
							   beast::bind_front_handler(&Wsession::on_close,
														 shared_from_this()));

	// Every 5 seconds
	if (--request_timer_ <= 0) {
		request_timer_ = 5;

		// Send ping message
		ws_.write(net::buffer("{\"id\": " + std::to_string(++xmit_id_) + ", \"method\": \"server.ping\", \"params\":[]}"));

		// Iterate over requests
		for (auto single_instrument_request = instruments_requests_map_->begin();
			 single_instrument_request != instruments_requests_map_->end();
			 ++single_instrument_request) {

			auto single_instrument_request_description = instruments_requests_map_->get_entry(single_instrument_request).first;
			auto single_instrument_request_status = instruments_requests_map_->get_entry(single_instrument_request).second->get_status();

			// Check if the request has already been processed
			auto response = instruments_responses_map_->find_entry(single_instrument_request_description);
			auto is_in_responses = (response != nullptr);

			if (!is_in_responses) {
				// Create a request object, fill it with single instrument requests
				// and place an object in the requests_ map as a pair with a unique id
				if (requests_.empty() ||
					(last_request_id_ != 0 && requests_[last_request_id_]
													  ->try_insert(std::make_pair(single_instrument_request_description,
																				  single_instrument_request_status)) != RequestError::Success)) {

					xmit_id_++;
					requests_.insert_or_assign(xmit_id_,
											   std::make_shared<WstreamRequest_Gateio>(xmit_id_,
																					   max_instruments_in_request_));
					requests_[xmit_id_]->try_insert(std::make_pair(single_instrument_request_description,
																   single_instrument_request_status));

					last_request_id_ = xmit_id_;
				}

				// Place a single instrument request into the responses safe map
				// thereby mark it as ready to send
				instruments_responses_map_
					->update_or_add_entry(instruments_requests_map_
											  ->get_entry(single_instrument_request)
											  .first,
										  std::make_shared<InstrumentResponse>(instruments_requests_map_
																				   ->get_entry(single_instrument_request)
																				   .second->get_status()));

				spdlog::trace("Request: Gateio id = {}, '{}', {}",
							  xmit_id_,
							  single_instrument_request_description,
							  single_instrument_request_status);
			}
		}

		// The last request created is probably not completely filled out and needs to be finalized
		if (last_request_id_)
			requests_[last_request_id_]->finalyze();
	}

	// Submit requests one by one
	if (requests_.size() > 0) {
		for (const auto &request : requests_) {
			if (request.second->get_size() > 0 &&
				request.second->get_attempts() > 0) {

				// Send the message
				std::string json = request.second->make_json();
				ws_.write(net::buffer(json));
				spdlog::trace("JSON tree as plain string: Gateio '{}'", json);

				// Since tne names of the instruments on each exchange are different
				// we need to build a table to convert these names to a conventional names
				const auto &conversion_table = request.second->get_reverse_conversion_table_();
				reverse_conversion_table_.insert(conversion_table.begin(),
												 conversion_table.end());

				// Place the request instruments into the responses safe map
				// thereby mark it as processed
				for (const auto &instrument : request.second->get_conventional_responses()) {
					instruments_responses_map_->update_or_add_entry(instrument.first,
																	std::make_shared<InstrumentResponse>(instrument.second));
					spdlog::trace("Instrument: Gateio '{}', '{}'", instrument.first, instrument.second);
				}

				break;
			}
		}
	}

	// Rearming the timer
	timer_.expires_from_now(timer_period_);
	timer_.async_wait(beast::bind_front_handler(&Wsession::on_timer, shared_from_this()));
}

// -----------------------------------------------------------------------------

// Parse stream data
void Wsession_Gateio::output(beast::flat_buffer &buffer)
{
	if (buffer.size() > 0) {
		// Convert received buffer to string
		std::string json_raw = beast::buffers_to_string(buffer.data());

		spdlog::trace("Parse stream data: Gateio '{}'", json_raw);

		// Try parsing the string as a trade detail update message
		auto ticks = WstreamResponse_Gateio::trade_detail(json_raw);
		if (ticks != nullptr && ticks->size() > 0) {
			// Iterate over ticks one by one
			for (auto &tick : *ticks) {
				// Check if there is a tick symbol in the conversion table
				auto conversion = reverse_conversion_table_.find(tick->get_instrument());
				bool is_in_reverse_conversion_table = (conversion != reverse_conversion_table_.end());

				if (is_in_reverse_conversion_table) {
					// Convert the received symbol to a conventional instrument name
					std::string field_symbol = conversion->second;
					tick->set_instrument(field_symbol);

					spdlog::trace("Send price to analyzer: Gateio {}, {}, {}, {}, {}",
								  field_symbol,
								  tick->get_timestamp(),
								  tick->get_price_str(),
								  tick->get_size_str(),
								  tick->is_buyer());

					// Put a tick in the lock-free queue
					while (!ticks_queue_->push(tick))
						usleep(10);
				}
			}
		}
		else {
			// Try parsing the string as a response to a subscription
			uint32_t id = WstreamResponse_Gateio::response(json_raw);
			if (id != 0) {
				spdlog::trace("Response: Gateio id = {}, '{}'", id, json_raw);

				// Check if the response id matches one of the submitted requests
				auto request = requests_.find(id);
				bool is_in_requests = (request != requests_.end());

				if (is_in_requests) {
					// Change insruments status to success in the responses safe map
					for (const auto &instrument : request->second->get_conventional_responses()) {
						instruments_responses_map_->update_or_add_entry(instrument.first,
																		std::make_shared<InstrumentResponse>("success"));
						spdlog::trace("Instrument: Gateio '{}', '{}'", instrument.first, "success");
					}
				}
			}
			else
				spdlog::warn("Unknown response: Gateio '{}'", json_raw);
		}
	}

	buffer.clear();
}