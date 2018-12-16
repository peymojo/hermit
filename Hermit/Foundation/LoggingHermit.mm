//
//	Hermit
//	Copyright (C) 2017 Paul Young (aka peymojo)
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <Foundation/Foundation.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "IsDebuggerActive.h"
#include "LoggingHermit.h"

namespace hermit {

	//
	class LoggingHermit::impl {
	public:
		//
		impl() {
		}
		
		//
		impl(const char* logFilePath) : mLogFilePath(logFilePath) {
		}
		
		//
		void Notify(const char* name, const void* param) {
			if (strcmp(name, kMessageNotification) == 0) {
				auto p = (MessageParams*)param;
				
				std::stringstream strm;
				if (p->severity == MessageSeverity::kWarning) {
					strm << "Warning: ";
				}
				else if (p->severity == MessageSeverity::kError) {
					strm << "ERROR: ";
					
					std::lock_guard<std::mutex> guard(mErrorsMutex);
					mErrors.push_back(p->message);
				}
				strm << p->message;
				
				OutputLine(strm.str());
			}
		}
		
		//
		void PrintErrors(bool clearErrors) {
			std::lock_guard<std::mutex> guard(mErrorsMutex);
			if (!mErrors.empty()) {
				OutputLine("");
				OutputLine("-----");
				OutputLine("There were errors:");
				
				for (auto it = begin(mErrors); it != end(mErrors); ++it) {
					OutputLine((*it).c_str());
				}
				
				OutputLine("-----");
				OutputLine("");
				
				if (clearErrors) {
					mErrors.clear();
				}
			}
		}
		
		//
		void FinishLogFileOutput() {
			std::lock_guard<std::mutex> guard(mErrorsMutex);
			OutputLine("Logging completed.");
			
			mFileStream.close();
			mLogFilePath.clear();
		}
		
	private:
		//
		void OutputLine(const std::string& line) {
			std::lock_guard<std::recursive_mutex> guard(mStreamMutex);
			
			if (!mLogFilePath.empty()) {
				if (!mFileStream.is_open()) {
					mFileStream.open(mLogFilePath);
					if (!mFileStream.is_open()) {
						NSLog(@"LoggingHermit: couldn't open log file: %s", mLogFilePath.c_str());
						mLogFilePath.clear();
					}
					else {
						OutputLine("Logging started.");
					}
				}
				if (mFileStream.is_open()) {
					time_t now = 0;
					time(&now);
					struct tm localTime;
					memset(&localTime, 0, sizeof(struct tm));
					localtime_r(&now, &localTime);
					char localBuf[256];
					memset(localBuf, 0, 256);
					strftime(localBuf, 256, "%Y-%m-%d %H:%M:%S %Z", &localTime);
					
					uint64_t threadId = 0;
					pthread_threadid_np(pthread_self(), &threadId);
					mFileStream << localBuf << ": [" << threadId << "] " << line << "\n";
					mFileStream.flush();
				}
			}
			if (mLogFilePath.empty() || IsDebuggerActive()) {
				std::cout << line << std::endl;
			}
		}
		
		//
		std::string mLogFilePath;
		std::ofstream mFileStream;
		std::recursive_mutex mStreamMutex;
		std::mutex mErrorsMutex;
		std::vector<std::string> mErrors;
	};
	
	//
	LoggingHermit::LoggingHermit() : pimpl(new impl()) {
	}
	
	//
	LoggingHermit::LoggingHermit(const char* logFilePath) : pimpl(new impl(logFilePath)) {
	}
	
	//
	LoggingHermit::~LoggingHermit() {
	}
	
	//
	bool LoggingHermit::ShouldAbort() {
		return false;
	}
	
	//
	void LoggingHermit::Notify(const char* name, const void* param) {
		return pimpl->Notify(name, param);
	}
	
	//
	void LoggingHermit::PrintErrors(bool clearErrors) {
		pimpl->PrintErrors(clearErrors);
	}
	
	//
	void LoggingHermit::FinishLogFileOutput() {
		pimpl->FinishLogFileOutput();
	}
	
} // namespace hermit
