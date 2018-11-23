//
//	Hermit
//	Copyright (C) 2018 Paul Young (aka peymojo)
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

#include <map>
#include <memory>
#include <stack>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "AppendToFilePath.h"
#include "GetFileType.h"
#include "ListDirectoryContentsWithType.h"
#include "DirectoryEnumerator.h"

namespace hermit {
	namespace file {
		namespace NewDirectoryEnumerator_Impl {
			
			//
			int CompareChar(const char& c1, const char& c2) {
				if (c1 == c2) {
					return 0;
				}
				if (std::toupper(c1) == std::toupper(c2)) {
					return 0;
				}
				if (std::toupper(c1) < std::toupper(c2)) {
					return -1;
				}
				return 1;
			}
			
			//
			int CaseInsensitiveStringCompare(const std::string& str1, const std::string& str2) {
				std::string::size_type i = 0;
				while (true) {
					if ((i >= str1.size()) || (i >= str2.size())) {
						break;
					}
					int chVal = CompareChar(str1[i], str2[i]);
					if (chVal != 0) {
						return chVal;
					}
					i++;
				}
				if (i < str2.size()) {
					return -1;
				}
				if (i < str1.size()) {
					return 1;
				}
				return 0;
			}
			
			//
			struct CaseInsensitiveCompare {
				bool operator() (const std::string& a, const std::string& b) const {
					return CaseInsensitiveStringCompare(a, b) < 0;
				}
			};

			//
			typedef std::pair<FilePathPtr, FileType> FileItem;
			
			//
			typedef std::map<std::string, FileItem, CaseInsensitiveCompare> CaseInsensitiveItemMap;

			//
			class ItemCallback : public ListDirectoryContentsWithTypeItemCallback {
			public:
				//
				ItemCallback(CaseInsensitiveItemMap& items) : mItems(items) {
				}
				
				//
				virtual bool OnItem(const hermit::HermitPtr& h_,
									const ListDirectoryContentsResult& result,
									const FilePathPtr& parentPath,
									const std::string& itemName,
									const FileType& itemType) override {
					FilePathPtr filePath;
					AppendToFilePath(h_, parentPath, itemName, filePath);
					if (filePath == nullptr) {
						// Skip this one but keep going.
						NOTIFY_ERROR(h_, "AppendToFilePath failed for parent:", parentPath, "item name:", itemName);
						return true;
					}
					auto item = std::make_pair(filePath, itemType);
					mItems.insert(std::make_pair(itemName, item));
					return true;
				}
				
				//
				CaseInsensitiveItemMap& mItems;
			};

			//
			class DirectoryEnumerator {
			public:
				//
				DirectoryEnumerator(const FilePathPtr& directoryPath) : mDirectoryPath(directoryPath), mLoaded(false) {
				}
				
				//
				GetNextDirectoryItemResult NextItem(const HermitPtr& h_, FilePathPtr& outPath, FileType& outType) {
					std::lock_guard<std::mutex> guard(mMutex);

					if (!mLoaded) {
						auto result = LoadItems(h_);
						mLoaded = true;
						if (result == ListDirectoryContentsResult::kCanceled) {
							return GetNextDirectoryItemResult::kCanceled;
						}
						if (result == ListDirectoryContentsResult::kPermissionDenied) {
							return GetNextDirectoryItemResult::kPermissionDenied;
						}
						if (result != ListDirectoryContentsResult::kSuccess) {
							NOTIFY_ERROR(h_, "ListDirectoryContentsWithType failed for:", mDirectoryPath);
							return GetNextDirectoryItemResult::kError;
						}
						mIt = begin(mItems);
					}
					
					if (mIt == end(mItems)) {
						return GetNextDirectoryItemResult::kNoMoreItems;
					}
					auto item = *mIt;
					mIt++;
					outPath = item.second.first;
					outType = item.second.second;
					return GetNextDirectoryItemResult::kSuccess;
				}
				
				//
				ListDirectoryContentsResult LoadItems(const HermitPtr& h_) {
					ItemCallback directoryItemCallback(mItems);
					return ListDirectoryContentsWithType(h_, mDirectoryPath, false, directoryItemCallback);
				}
				
				//
				FilePathPtr mDirectoryPath;
				std::mutex mMutex;
				bool mLoaded;
				CaseInsensitiveItemMap mItems;
				CaseInsensitiveItemMap::const_iterator mIt;
			};
			typedef std::shared_ptr<DirectoryEnumerator> DirectoryEnumeratorPtr;
			
			//
			class NextItemGetter : public GetNextDirectoryItemFunction {
			public:
				//
				NextItemGetter(DirectoryEnumeratorPtr root) {
					mStack.push(root);
				}
				
				//
				virtual void Call(const HermitPtr& h_, const GetNextDirectoryItemCallbackPtr& callback) override {
					auto result = GetNextDirectoryItemResult::kUnknown;
					FilePathPtr itemPath;
					FileType itemType = FileType::kUnknown;

					{
						std::lock_guard<std::mutex> guard(mMutex);

						while (true) {
							if (mStack.empty()) {
								result = GetNextDirectoryItemResult::kNoMoreItems;
								break;
							}
							result = mStack.top()->NextItem(h_, itemPath, itemType);
							if (result == GetNextDirectoryItemResult::kCanceled) {
								break;
							}
							if (result == GetNextDirectoryItemResult::kPermissionDenied) {
								// Pass the parent info so the client knows where the permission error occurred.
								itemPath = mStack.top()->mDirectoryPath;
								itemType = FileType::kDirectory;
								mStack.pop();
								break;
							}
							if (result == GetNextDirectoryItemResult::kNoMoreItems) {
								mStack.pop();
								result = GetNextDirectoryItemResult::kEndOfDirectory;
								break;
							}
							if (result != GetNextDirectoryItemResult::kSuccess) {
								NOTIFY_ERROR(h_, "mStack.top()->NextItem failed");
								break;
							}
							if (itemType == FileType::kDirectory) {
								auto enumerator = std::make_shared<DirectoryEnumerator>(itemPath);
								mStack.push(enumerator);
							}
							result = GetNextDirectoryItemResult::kSuccess;
							break;
						}
					}

					callback->Call(h_, result, itemPath, itemType);
				}
				
				//
				std::stack<DirectoryEnumeratorPtr> mStack;
				std::mutex mMutex;
			};
			
		} // namespace NewDirectoryEnumerator_Impl
		using namespace NewDirectoryEnumerator_Impl;
		
		//
		void NewDirectoryEnumerator(const HermitPtr& h_,
									const FilePathPtr& directoryPath,
									const NewDirectoryEnumeratorCallbackPtr callback) {
			FileType fileType = FileType::kUnknown;
			auto status = GetFileType(h_, directoryPath, fileType);
			if (status != GetFileTypeStatus::kSuccess) {
				NOTIFY_ERROR(h_, "GetFileType failed for:", directoryPath);
				callback->Call(h_, NewDirectoryEnumeratorResult::kError, nullptr);
				return;
			}
			if (fileType != FileType::kDirectory) {
				callback->Call(h_, NewDirectoryEnumeratorResult::kNotADirectory, nullptr);
				return;
			}
			
			auto enumerator = std::make_shared<DirectoryEnumerator>(directoryPath);
			auto getter = std::make_shared<NextItemGetter>(enumerator);
			callback->Call(h_, NewDirectoryEnumeratorResult::kSuccess, getter);
		}
		
	} // namespace file
} // namespace hermit
