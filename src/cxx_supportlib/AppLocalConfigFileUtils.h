/*
 *  Phusion Passenger - https://www.phusionpassenger.com/
 *  Copyright (c) 2018 Phusion Holding B.V.
 *
 *  "Passenger", "Phusion Passenger" and "Union Station" are registered
 *  trademarks of Phusion Holding B.V.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#ifndef _PASSENGER_APP_LOCAL_CONFIG_FILE_UTILS_H_
#define _PASSENGER_APP_LOCAL_CONFIG_FILE_UTILS_H_

#include <oxt/system_calls.hpp>
#include <oxt/backtrace.hpp>

#include <cerrno>
#include <fcntl.h>

#include <jsoncpp/json.h>

#include <StaticString.h>
#include <Exceptions.h>
#include <FileTools/FileManip.h>
#include <Utils/ScopeGuard.h>

namespace Passenger {

using namespace std;


inline Json::Value
parseAppLocalConfigFile(const StaticString appRoot) {
	TRACE_POINT();

	if (!fileExists(appRoot + "/Passengerfile.json")) {
		return Json::Value();
	}

	int dirfd = syscalls::open(appRoot.toString().c_str(), O_RDONLY);
	if (dirfd == -1) {
		int e = errno;
		throw FileSystemException("Cannot open " + appRoot, e, appRoot);
	}

	FdGuard dirfdGuard(dirfd, __FILE__, __LINE__);
	// This read is safe because the appRoot path comes from the web server.
	pair<string, bool> contents;
	try {
		contents = safeReadFile(dirfd, "Passengerfile.json",
		1024 * 512);
	} catch (const FileSystemException &e) {
		throw FileSystemException("Cannot open '" + appRoot
			+ "/Passengerfile.json' for reading",
			e.code(),
			appRoot + "/Passengerfile.json");
	}
	if (!contents.second) {
		throw SecurityException("Error parsing " + appRoot
			+ "/Passengerfile.json: file exceeds size limit of 512 KB");
	}

	Json::Reader reader;
	Json::Value config;
	if (!reader.parse(contents.first, config)) {
		throw RuntimeException("Error parsing " + appRoot
			+ "/Passengerfile.json: " + reader.getFormattedErrorMessages());
	}

	// TODO: validate JSON and its keys

	return config;
}


} // namespace Passenger

#endif /* _PASSENGER_APP_LOCAL_CONFIG_FILE_UTILS_H_ */
