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

#include <string>
#include "Hermit/Foundation/Notification.h"
#include "AppendToFilePath.h"
#include "ListDirectoryContentsWithType.h"
#include "CountDirectoryContents.h"

namespace hermit {
	namespace file {
		namespace CountDirectoryContents_Impl {
			
			//
			class ListDirectoryContentsCallback : public ListDirectoryContentsWithTypeItemCallback {
			public:
				//
				ListDirectoryContentsCallback(const bool& inDescendSubdirectories,
											  const bool& inIgnorePermissionsErrors,
											  const PreprocessFileFunctionPtr& inPreprocessFunction) :
				mDescendSubdirectories(inDescendSubdirectories),
				mIgnorePermissionsErrors(inIgnorePermissionsErrors),
				mPreprocessFunction(inPreprocessFunction),
				mFileCount(0),
				mErrorOccurred(false) {
				}
				
				//
				virtual bool OnItem(const HermitPtr& h_,
									const ListDirectoryContentsResult& result,
									const FilePathPtr& parentPath,
									const std::string& itemName,
									const FileType& itemType) override {
					if (CHECK_FOR_ABORT(h_)) {
						return false;
					}
					
					if (mPreprocessFunction != nullptr) {
						auto instruction = mPreprocessFunction->Preprocess(h_, parentPath, itemName);
						if (instruction == PreprocessFileInstruction::kSkip) {
							return true;
						}
						if (instruction != PreprocessFileInstruction::kContinue) {
							NOTIFY_ERROR(h_,
										 "CountDirectoryContents: mPreprocessFunction failed for:", parentPath,
										 "inItemName:", itemName);
							mErrorOccurred = true;
							return false;
						}
					}
					
					if ((itemType == FileType::kDirectory) && mDescendSubdirectories) {
						FilePathPtr directoryPath;
						AppendToFilePath(h_, parentPath, itemName, directoryPath);
						if (directoryPath == nullptr) {
							NOTIFY_ERROR(h_,
										 "CountDirectoryContents: AppendToFilePath failed for:", parentPath,
										 "inItemName:", itemName);
							mErrorOccurred = true;
							return false;
						}
						
						ListDirectoryContentsCallback itemCallback(true, mIgnorePermissionsErrors, mPreprocessFunction);
						auto result = ListDirectoryContentsWithType(h_, directoryPath, false, itemCallback);
						if (result == ListDirectoryContentsResult::kCanceled) {
							return false;
						}
						if (mIgnorePermissionsErrors && (result == ListDirectoryContentsResult::kPermissionDenied)) {
							return false;
						}
						if (result != ListDirectoryContentsResult::kSuccess) {
							if ((result == ListDirectoryContentsResult::kPermissionDenied) && mIgnorePermissionsErrors) {
								return true;
							}
							NOTIFY_ERROR(h_, "CountDirectoryContents: ListDirectoryContentsWithType failed for:", directoryPath);
							mErrorOccurred = true;
							return false;
						}
						mFileCount += itemCallback.mFileCount;
					}
					
					++mFileCount;
					return true;
				}
				
				//
				bool mDescendSubdirectories;
				bool mIgnorePermissionsErrors;
				PreprocessFileFunctionPtr mPreprocessFunction;
				uint64_t mFileCount;
				bool mErrorOccurred;
			};
			
		} // namespace CountDirectoryContents_Impl
		using namespace CountDirectoryContents_Impl;
		
		//
		void CountDirectoryContents(const HermitPtr& h_,
									const FilePathPtr& inDirectoryPath,
									const bool& inDescendSubdirectories,
									const bool& inIgnorePermissionsErrors,
									const PreprocessFileFunctionPtr& inPreprocessFunction,
									const CountDirectoryContentsCallbackRef& inCallback) {
			
			ListDirectoryContentsCallback itemCallback(inDescendSubdirectories,
													   inIgnorePermissionsErrors,
													   inPreprocessFunction);
			auto result = ListDirectoryContentsWithType(h_, inDirectoryPath, false, itemCallback);
			if (result == ListDirectoryContentsResult::kCanceled) {
				inCallback.Call(kCountDirectoryContentsStatus_Canceled, 0);
				return;
			}
			if (itemCallback.mErrorOccurred) {
				NOTIFY_ERROR(h_, "itemCallback.mErrorOccurred for path:", inDirectoryPath);
				inCallback.Call(kCountDirectoryContentsStatus_Error, 0);
				return;
			}
			if (result == ListDirectoryContentsResult::kDirectoryNotFound) {
				NOTIFY_ERROR(h_, "DirectoryNotFound for path:", inDirectoryPath);
				inCallback.Call(kCountDirectoryContentsStatus_DirectoryNotFound, 0);
				return;
			}
			if (result == ListDirectoryContentsResult::kPermissionDenied) {
				if (!inIgnorePermissionsErrors) {
					NOTIFY_ERROR(h_, "PermissionDenied for path:", inDirectoryPath);
					inCallback.Call(kCountDirectoryContentsStatus_PermissionDenied, 0);
					return;
				}
			}
			else if (result != ListDirectoryContentsResult::kSuccess) {
				NOTIFY_ERROR(h_, "ListDirectoryContents failed for path:", inDirectoryPath);
				inCallback.Call(kCountDirectoryContentsStatus_Error, 0);
				return;
			}
			inCallback.Call(kCountDirectoryContentsStatus_Success, itemCallback.mFileCount);
		}
		
	} // namespace file
} // namespace hermit
