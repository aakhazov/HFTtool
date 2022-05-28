#include "Wsession_Huobi.hpp"

void Wsession_Huobi::on_timer(const boost::system::error_code &ec)
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
											   std::make_shared<WstreamRequest_Huobi>(xmit_id_,
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

				spdlog::trace("Request: Huobi id = {}, '{}', {}",
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
				spdlog::trace("JSON tree as plain string: Huobi '{}'", json);

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
					spdlog::trace("Instrument: Huobi '{}', '{}'", instrument.first, instrument.second);
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
void Wsession_Huobi::output(beast::flat_buffer &buffer)
{
	if (buffer.size() > 0) {
		// Unzip received buffer to string
		std::vector<char> compressed(static_cast<char *>(buffer.data().data()),
									 static_cast<char *>(buffer.data().data()) + buffer.size());
		const std::vector<char> decompressed = unzip(compressed);

		std::string json_raw(std::string(decompressed.begin(), decompressed.end()));

		spdlog::trace("Parse stream data: Huobi '{}'", json_raw);

		// Try parsing the string as a trade detail update message
		auto ticks = WstreamResponse_Huobi::trade_detail(json_raw);
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

					spdlog::trace("Send price to analyzer: Huobi {}, {}, {}, {}, {}",
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
			uint32_t id = WstreamResponse_Huobi::response(json_raw);
			if (id != 0) {
				spdlog::trace("Response: Huobi id = {}, '{}'", id, json_raw);

				// Check if the response id matches one of the submitted requests
				auto request = requests_.find(id);
				bool is_in_requests = (request != requests_.end());

				if (is_in_requests) {
					// Change insruments status to success in the responses safe map
					for (const auto &instrument : request->second->get_conventional_responses()) {
						instruments_responses_map_->update_or_add_entry(instrument.first,
																		std::make_shared<InstrumentResponse>("success"));
						spdlog::trace("Instrument: Huobi '{}', '{}'", instrument.first, "success");
					}
				}
			}
			// Try parsing the string as an incoming ping message and answer to it with a pong message
			else if (0 == pong(json_raw))
				spdlog::warn("Unknown response: Huobi '{}'", json_raw);
		}
	}

	buffer.clear();
}

// -----------------------------------------------------------------------------

std::vector<char> Wsession_Huobi::unzip(const std::vector<char> compressed)
{
	std::vector<char> decompressed;

	boost::iostreams::filtering_ostream o_stream;

	o_stream.push(boost::iostreams::gzip_decompressor());
	o_stream.push(boost::iostreams::back_inserter(decompressed));

	boost::iostreams::write(o_stream, &compressed[0], compressed.size());

	return decompressed;
}

// -----------------------------------------------------------------------------

std::uint64_t Wsession_Huobi::pong(std::string const &json)
{
	std::uint64_t ping_time = 0;

	boost::iostreams::array_source as(&json[0], json.size());
	boost::iostreams::stream<boost::iostreams::array_source> is(as);

	boost::property_tree::ptree json_tree_root;

	boost::property_tree::read_json(is, json_tree_root);

	try {
		ping_time = json_tree_root.get<std::uint64_t>("ping");

		// Send pong message
		if (ping_time)
			ws_.write(net::buffer("{\"pong\":" + std::to_string(ping_time) + "}"));
	}
	catch (const boost::property_tree::ptree_error &e) {
		spdlog::trace("Catch boost::property_tree::ptree_error: Huobi pong '{}'",
					  e.what());
	}
	catch (std::exception const &e) {
		spdlog::warn("Catch std::exception: Huobi pong '{}'",
					 e.what());
	}

	return ping_time;
}