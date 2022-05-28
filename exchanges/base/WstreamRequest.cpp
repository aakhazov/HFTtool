#include "WstreamRequest.hpp"

RequestError WstreamRequest::try_insert(const std::pair<std::string, std::string> &request)
{
	std::vector<std::string> instrument_to_subscribe;
	boost::split(instrument_to_subscribe, request.first, boost::is_any_of(" "));

	if (instrument_to_subscribe.size() != 3)
		return RequestError::WrongFormat;

	if (request_status_ > RequestStatus::Fill)
		return RequestError::Busy;

	if (method_.empty())
		method_ = instrument_to_subscribe[2];
	else if (method_ != instrument_to_subscribe[2])
		return RequestError::MethodMismatch;

	if (subscription_channel_.empty())
		subscription_channel_ = instrument_to_subscribe[1];
	else if (subscription_channel_ != instrument_to_subscribe[1])
		return RequestError::ChannelMismatch;

	if (instruments_number_ == max_instruments_) {
		finalyze();
		return RequestError::SizeLimit;
	}

	instruments_.push_back(instrument_to_subscribe[0]);

	request_status_ = RequestStatus::Fill;

	instruments_number_++;

	return RequestError::Success;
}

// -----------------------------------------------------------------------------

std::map<std::string, std::string> &WstreamRequest::get_conventional_responses()
{
	std::string status_string;

	switch (request_status_) {
		case RequestStatus::Empty:
			status_string = "empty";
			break;

		case RequestStatus::Fill:
			status_string = "fill";
			break;

		case RequestStatus::Ready:
			status_string = "ready";
			break;

		case RequestStatus::Processing:
			status_string = "processing";
			break;

		default:
			status_string = "undefined";
			break;
	}

	responses_.clear();

	for (const auto &instrument : instruments_) {
		responses_.insert_or_assign(instrument + " " +
										subscription_channel_ + " " +
										method_,
									status_string);
	}

	return responses_;
}