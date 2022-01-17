/**
 * This file is part of Darling.
 *
 * Copyright (C) 2021 Darling developers
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <darlingserver/logging.hpp>
#include <darlingserver/server.hpp>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>

#ifndef LOG_TO_STDOUT
	//#define LOG_TO_STDOUT 0

	// FOR DARLINGSERVER TESTING:
	#define LOG_TO_STDOUT 1
#endif

DarlingServer::Log::Log(std::string category):
	_category(category)
	{};

DarlingServer::Log::Stream::Stream(Type type, const Log& log):
	_type(type),
	_log(log)
	{};

DarlingServer::Log::Stream::~Stream() {
	*this << endLog;
};

template<>
DarlingServer::Log::Stream& DarlingServer::Log::Stream::operator<<<EndLog>(EndLog value) {
	auto str = _buffer.str();
	if (!str.empty()) {
		_log._log(_type, str);
		_buffer.clear();
	}
	return *this;
};

DarlingServer::Log::Stream DarlingServer::Log::debug() const {
	return Stream(Type::Debug, *this);
};

DarlingServer::Log::Stream DarlingServer::Log::info() const {
	return Stream(Type::Info, *this);
};

DarlingServer::Log::Stream DarlingServer::Log::warning() const {
	return Stream(Type::Warning, *this);
};

DarlingServer::Log::Stream DarlingServer::Log::error() const {
	return Stream(Type::Error, *this);
};

std::string DarlingServer::Log::_typeToString(Type type) {
	switch (type) {
		case Type::Debug:
			return "Debug";
		case Type::Info:
			return "Info";
		case Type::Warning:
			return "Warning";
		case Type::Error:
			return "Error";
		default:
			return "Unknown";
	}
};

void DarlingServer::Log::_log(Type type, std::string message) const {
	// NOTE: we use POSIX file APIs because we want to append each message to the log file atomically,
	//       and as far as i can tell, C++ fstreams provide no such guarantee (that they won't write in chunks).
	static int logFile = []() {
		std::filesystem::path path(Server::sharedInstance().prefix() + "/private/var/log/");
		std::filesystem::create_directories(path.parent_path());
		return open(path.c_str(), O_WRONLY | O_APPEND);
	}();

	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	double secs = (double)time.tv_sec + ((double)time.tv_nsec / 1.0e9);
	std::string messageToLog = "[" + std::to_string(secs) + "](" + _category + ", " + _typeToString(type) + ") " + message + "\n";

	write(logFile, messageToLog.c_str(), messageToLog.size());

#if LOG_TO_STDOUT
	write(STDOUT_FILENO, messageToLog.c_str(), messageToLog.size());
#endif
};
