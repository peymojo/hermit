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

#include "Hermit/Foundation/Notification.h"
#include "CreateDirectoryIfNeeded.h"
#include "FileExists.h"
#include "GetFilePathParent.h"
#include "PathIsDirectory.h"
#include "CreateDirectoryAndParentsAsNeeded.h"

namespace hermit {
	namespace file {
		
		namespace {

			//
			class CreateParentDirectoryCallback : public CreateDirectoryAndParentsAsNeededCallback {
			public:
				//
				CreateParentDirectoryCallback() :
				mStatus(kCreateDirectoryAndParentsAsNeededStatus_Unknown) {
				}
				
				//
				bool Function(const FilePathPtr& inFilePath, const CreateDirectoryAndParentsAsNeededStatus& inStatus) {
					mStatus = inStatus;
					return true;
				}
				
				//
				CreateDirectoryAndParentsAsNeededStatus mStatus;
			};
			
		} // private namespace
		
		//
		void CreateDirectoryAndParentsAsNeeded(const HermitPtr& h_,
											   const FilePathPtr& inFilePath,
											   const CreateDirectoryAndParentsAsNeededCallbackRef& inCallback) {
			
			FileExistsCallbackClass existsCallback;
			FileExists(h_, inFilePath, existsCallback);
			if (!existsCallback.mSuccess) {
				NOTIFY_ERROR(h_, "CreateDirectoryAndParentsAsNeeded: FileExists failed for path:", inFilePath);
				inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_Error);
				return;
			}
			
			if (existsCallback.mExists) {
				bool isDirectory = false;
				auto status = PathIsDirectory(h_, inFilePath, isDirectory);
				if (status != PathIsDirectoryStatus::kSuccess) {
					NOTIFY_ERROR(h_, "CreateDirectoryAndParentsAsNeeded: PathIsDirectory failed for path:", inFilePath);
					inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_Error);
				}
				else if (isDirectory) {
					inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_AlreadyPresent);
				}
				else {
					inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_ConflictAtPath);
				}
			}
			else {
				FilePathPtr parentPath;
				GetFilePathParent(h_, inFilePath, parentPath);
				if (parentPath == nullptr) {
					NOTIFY_ERROR(h_, "CreateDirectoryAndParentsAsNeeded(): Couldn't get parent for path:", inFilePath);
					inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_Error);
				}
				else {
					CreateParentDirectoryCallback parentCallback;
					CreateDirectoryAndParentsAsNeeded(h_, parentPath, parentCallback);
					if ((parentCallback.mStatus != kCreateDirectoryAndParentsAsNeededStatus_Created) &&
						(parentCallback.mStatus != kCreateDirectoryAndParentsAsNeededStatus_AlreadyPresent)) {
						inCallback.Call(inFilePath, parentCallback.mStatus);
					}
					else {
						auto result = CreateDirectoryIfNeeded(h_, inFilePath);
						if (result.first == kCreateDirectoryIfNeededStatus_Success) {
							if (result.second) {
								inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_Created);
							}
							else {
								inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_AlreadyPresent);
							}
						}
						else if (result.first == kCreateDirectoryIfNeededStatus_ConflictAtPath) {
							inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_ConflictAtPath);
						}
						else if (result.first == kCreateDirectoryIfNeededStatus_DiskFull) {
							inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_DiskFull);
						}
						else {
							inCallback.Call(inFilePath, kCreateDirectoryAndParentsAsNeededStatus_Error);
						}
					}
				}
			}
		}
		
	} // namespace file
} // namespace hermit
