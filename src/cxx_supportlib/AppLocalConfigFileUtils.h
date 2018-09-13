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

#include <ResourceLocator/Locator.h>
#include <StaticString.h>
#include <Constants.h>
#include <Exceptions.h>
#include <FileTools/FileManip.h>
#include <ProcessManagement/Spawn.h>
#include <Utils/ScopeGuard.h>

namespace Passenger {

using namespace std;


inline Json::Value
parseAppLocalConfigFile(const ResourceLocator &resourceLocator, const StaticString appRoot,
	StaticString user)
{
	TRACE_POINT();
	string path = appRoot + "/Passengerfile.json";

	if (!fileExists(path)) {
		return Json::Value();
	}

	// Reading from Passengerfile.json from a root process is unsafe
	// because of symlink attacks.
	// We are unable to use safeReadFile() here because we do not
	// control the safety of the directories leading up to appRoot.
	// So we fork a child process with limited privileges to do the
	// read.

	string agentExe = resourceLocator.findSupportBinary(AGENT_EXE);
	string userNt = user.toString();
	const char *command[] = {
		agentExe.c_str(),
		agentExe.c_str(),
		"exec-helper",
		"--user",
		userNt.c_str(),

		agentExe.c_str(),
		"file-read-helper",
		path.c_str(),

		NULL
	};
	SubprocessInfo info;
	SubprocessOutput output;

	try {
		runCommandAndCaptureOutput(command, info, output, 1024 * 512);
	} catch (const SystemException &e) {
		throw SystemException("Cannot read from '" + path
			+ "': error reading from reader subprocess",
			e.code());
	}
	if (!output.eof) {
		throw SecurityException("Error parsing " + path
			+ ": file exceeds size limit of 512 KB");
	}

	// Parse JSON.

	Json::Reader reader;
	Json::Value config;
	if (!reader.parse(output.data, config)) {
		throw RuntimeException("Error parsing " + path + ": "
			+ reader.getFormattedErrorMessages());
	}
	// We no longer need the raw data so free the memory.
	output.data.resize(0);

	// TODO: validate JSON and its keys

	return config;
}


} // namespace Passenger

#endif /* _PASSENGER_APP_LOCAL_CONFIG_FILE_UTILS_H_ */
