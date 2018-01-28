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

#import <Cocoa/Cocoa.h>
#import <string>
#import <vector>
#import "Hermit/Foundation/Notification.h"
#import "CreateFilePathFromComponents.h"
#import "FilePathToCocoaPathString.h"
#import "GetSymbolicLinkTarget.h"

namespace hermit {
	namespace file {
		namespace GetSymbolicLinkTarget_Cocoa_Impl {

			//
			typedef std::vector<std::string> StringVector;
			

			//
			std::string SwapSeparators(const std::string& inPath) {
				if (inPath == "/") {
					return inPath;
				}
				std::string result;
				for (std::string::size_type n = 0; n < inPath.size(); ++n) {
					std::string::value_type ch = inPath[n];
					if (ch == '/') {
						ch = ':';
					}
					else if (ch == ':') {
						ch = '/';
					}
					result.push_back(ch);
				}
				return result;
			}
			
			//
			void SplitPath(const std::string& inPath, StringVector& outNodes) {
				StringVector nodes;
				std::string path(inPath);
				while (true) {
					if (path.empty()) {
						break;
					}
					std::string::size_type sepPos = path.find("/");
					if (sepPos == std::string::npos) {
						nodes.push_back(SwapSeparators(path));
						break;
					}
					if (sepPos == 0) {
						if (nodes.empty()) {
							nodes.push_back(std::string("/"));
						}
					}
					else {
						nodes.push_back(SwapSeparators(path.substr(0, sepPos)));
					}
					path = path.substr(sepPos + 1);
				}
				outNodes.swap(nodes);
			}
			
		} // namespace GetSymbolicLinkTarget_Cocoa_Impl
		using namespace  GetSymbolicLinkTarget_Cocoa_Impl;
		
		//
		void GetSymbolicLinkTarget(const HermitPtr& h_,
								   const FilePathPtr& inLinkPath,
								   const GetSymbolicLinkTargetCallbackRef& inCallback) {
			
			@autoreleasepool {
				std::string linkPathUTF8;
				FilePathToCocoaPathString(h_, inLinkPath, linkPathUTF8);
				
				NSString* path = [NSString stringWithUTF8String:linkPathUTF8.c_str()];
				NSError* error = nil;
				NSString* destination = [[NSFileManager defaultManager] destinationOfSymbolicLinkAtPath:path error:&error];
				if (error != nil) {
					NOTIFY_ERROR(h_,
								 "GetSymbolicLinkTarget(): NSFileHandle destinationOfSymbolicLinkAtPath failed for path:", inLinkPath,
								 "error code:", (int32_t)[error code],
								 "error:", [[error localizedDescription] UTF8String]);
					inCallback.Call(false, 0, false);
				}
				else if (destination == nil) {
					NOTIFY_ERROR(h_, "GetSymbolicLinkTarget: no error but destination is nil path:", inLinkPath);
					inCallback.Call(false, 0, false);
				}
				else {
					std::string destinationPathUTF8([destination UTF8String]);
					if (destinationPathUTF8.empty()) {
						NOTIFY_ERROR(h_, "GetSymbolicLinkTarget: destinationPathUTF8 is empty for link at path:", inLinkPath);
						inCallback.Call(false, 0, false);
					}
					else {
						StringVector nodes;
						SplitPath(destinationPathUTF8, nodes);
						
						FilePathPtr destinationPath;
						CreateFilePathFromComponents(nodes, destinationPath);
						if (destinationPath == 0) {
							NOTIFY_ERROR(h_, "GetSymbolicLinkTarget: CreateFilePathFromComponents failed for string:", destinationPathUTF8);
							inCallback.Call(false, 0, false);
							return;
						}
						
						bool isRelativePath = (destinationPathUTF8[0] != '/');
						inCallback.Call(true, destinationPath, isRelativePath);
					}
				}
			}
		}
		
	} // namespace file
} // namespace hermit
