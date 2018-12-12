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
#include <set>
#include <string>
#include <vector>
#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Notification.h"
#include "AppendToFilePath.h"
#include "GetCanonicalFilePathString.h"
#include "GetRelativeFilePath.h"
#include "HardLinkMap.h"
#include "ListDirectoryContentsWithType.h"
#include "PathIsHardLink.h"

namespace hermit {
	namespace file {
		namespace HardLinkMap_Impl {
			
			//
			bool GetRelativePath(const HermitPtr& h_, const FilePathPtr& root, const FilePathPtr& target, std::string& outRelativePath) {
				GetRelativeFilePathCallbackClassT<FilePathPtr> relativePath;
				GetRelativeFilePath(h_, root, target, relativePath);
				if (!relativePath.mSuccess) {
					NOTIFY_ERROR(h_, "GetRelativeFilePath failed for root:", root, "target", target);
					return false;
				}
				
				std::string canonicalPath;
				GetCanonicalFilePathString(h_, relativePath.mFilePath, canonicalPath);
				if (canonicalPath.empty()) {
					NOTIFY_ERROR(h_, "GetCanonicalFilePathString failed for relative path:", relativePath.mFilePath);
					return false;
				}
				outRelativePath = canonicalPath;
				return true;
			}
			
			//
			class HardLinkInfo {
			public:
				//
				HardLinkInfo() : mProcessing(false), mResult(HardLinkInfoResult::kUnknown), mDataSize(0) {
				}
				
				//
				bool mProcessing;
				HardLinkInfoResult mResult;
				std::vector<std::pair<hermit::HermitPtr, GetHardLinkInfoCompletionPtr>> mClients;
				std::string mDataObjectId;
				uint64_t mDataSize;
				std::string mDataHash;
				std::string mHashAlgorithm;
				std::set<std::string> mPaths;
				std::mutex mMutex;
			};
			typedef std::shared_ptr<HardLinkInfo> HardLinkInfoPtr;
			
			//
			typedef std::map<std::string, HardLinkInfoPtr> HardLinkInfoMap;
			
			//
			typedef std::pair<uint32_t, uint32_t> FileNumberPair;
			typedef std::multimap<FileNumberPair, FilePathPtr> HardLinkLookupMap;
			
			//
			DEFINE_CALLBACK_3A(GetItemChildrenItemCallback, HermitPtr, FilePathPtr, FileType);
			
			//
			enum class GetItemChildrenResult {
				kUnknown,
				kSuccess,
				kCanceled,
				kPermissionDenied,
				kError
			};

			//
			class ItemCallback : public ListDirectoryContentsWithTypeItemCallback {
			public:
				//
				ItemCallback() {
				}
				
				//
				virtual bool OnItem(const HermitPtr& h_,
									const ListDirectoryContentsResult& result,
									const FilePathPtr& parentPath,
									const std::string& itemName,
									const FileType& itemType) override {
					FilePathPtr filePath;
					AppendToFilePath(h_, parentPath, itemName, filePath);
					if (filePath == nullptr) {
						NOTIFY_ERROR(h_, "AppendToFilePath failed for parent:", parentPath, "item name:", itemName);
						return false;
					}
					mItems.push_back(std::make_pair(filePath, itemType));
					return true;
				}
				
				//
				std::vector<std::pair<FilePathPtr, FileType>> mItems;
			};

			//
			GetItemChildrenResult GetItemChildren(const HermitPtr& h_,
												  const FilePathPtr& filePath,
												  const bool descendSubdirectories,
												  const GetItemChildrenItemCallbackRef& itemCallback) {
				ItemCallback directoryItemCallback;
				auto result = ListDirectoryContentsWithType(h_, filePath, descendSubdirectories, directoryItemCallback);
				if (result == ListDirectoryContentsResult::kCanceled) {
					return GetItemChildrenResult::kCanceled;
				}
				if (result == ListDirectoryContentsResult::kPermissionDenied) {
					return GetItemChildrenResult::kPermissionDenied;
				}
				if (result != ListDirectoryContentsResult::kSuccess) {
					NOTIFY_ERROR(h_, "ListDirectoryContentsWithType failed for:", filePath);
					return GetItemChildrenResult::kError;
				}
				auto end = directoryItemCallback.mItems.end();
				for (auto it = directoryItemCallback.mItems.begin(); it != end; ++it) {
					itemCallback.Call(h_, it->first, it->second);
				}
				return GetItemChildrenResult::kSuccess;
			}

			//
			class GetChildrenCallback : public GetItemChildrenItemCallback {
			public:
				//
				GetChildrenCallback(HardLinkLookupMap& lookupMap) :
				mLookupMap(lookupMap),
				mCanceled(false) {
				}
				
				//
				virtual bool Function(const HermitPtr& h_, const FilePathPtr& filePath, const FileType& fileType) override {
					if (CHECK_FOR_ABORT(h_)) {
						mCanceled = true;
						return false;
					}
					
					if (fileType == FileType::kFile) {
						PathIsHardLinkCallbackClass hardLinkCallback;
						PathIsHardLink(h_, filePath, hardLinkCallback);
						if (!hardLinkCallback.mSuccess) {
							// We'll log an error here but continue processing, we don't want the
							// whole hard link map building process to abort.
							NOTIFY_ERROR(h_, "PathIsHardLink failed for:", filePath);
							return true;
						}
						if (hardLinkCallback.mIsHardLink) {
							FileNumberPair fileNumberPair(hardLinkCallback.mFileSystemNumber, hardLinkCallback.mFileNumber);
							mLookupMap.insert(HardLinkLookupMap::value_type(fileNumberPair, filePath));
						}
					}
					return true;
				}
				
				//
				HardLinkLookupMap& mLookupMap;
				bool mCanceled;
			};
			
			//
			enum class BuildHardLinkMapResult {
				kUnknown,
				kSuccess,
				kCanceled,
				kError
			};
			
			//
			BuildHardLinkMapResult BuildHardLinkMap(const HermitPtr& h_, const FilePathPtr& root, HardLinkLookupMap& lookupMap) {
				GetChildrenCallback itemCallback(lookupMap);
				auto result = GetItemChildren(h_, root, true, itemCallback);
				if ((result == GetItemChildrenResult::kCanceled) || itemCallback.mCanceled) {
					return BuildHardLinkMapResult::kCanceled;
				}
				if (result != GetItemChildrenResult::kSuccess) {
					// Log the error but continue to try to build the hard link map
					NOTIFY_ERROR(h_, "GetChildren failed for:", root);
				}
				return BuildHardLinkMapResult::kSuccess;
			}
			
			//
			BuildHardLinkMapResult BuildHardLinkInfoMap(const HermitPtr& h_,
														const FilePathPtr& root,
														const HardLinkLookupMap& lookupMap,
														HardLinkInfoMap& infoMap) {
				typedef std::map<FileNumberPair, HardLinkInfoPtr> FileNumberValueMap;
				FileNumberValueMap fileNumberValueMap;
				
				auto end = lookupMap.end();
				for (auto it = lookupMap.begin(); it != end; ++it) {
					auto numberValueIt = fileNumberValueMap.find(it->first);
					if (numberValueIt == fileNumberValueMap.end()) {
						auto hardLinkInfo = std::make_shared<HardLinkInfo>();
						auto insertResult = fileNumberValueMap.insert(FileNumberValueMap::value_type(it->first, hardLinkInfo));
						if (!insertResult.second) {
							NOTIFY_ERROR(h_, "fileNumberValueMap.insert failed.");
							return BuildHardLinkMapResult::kError;
						}
						numberValueIt = insertResult.first;
					}
					
					std::string relativePath;
					if (!GetRelativePath(h_, root, it->second, relativePath)) {
						NOTIFY_ERROR(h_, "GetRelativePath failed for item:", it->second, "root item:", root);
						return BuildHardLinkMapResult::kError;
					}
					numberValueIt->second->mPaths.insert(relativePath);
					infoMap.insert(HardLinkInfoMap::value_type(relativePath, numberValueIt->second));
				}
				return BuildHardLinkMapResult::kSuccess;
			}
			
			//
			class ProcessCallback : public ProcessHardLinkCompletion {
			public:
				//
				ProcessCallback(const HardLinkInfoPtr& hardLinkInfo) :
				mHardLinkInfo(hardLinkInfo) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const HardLinkInfoResult& result,
								  const std::string& dataObjectId,
								  const uint64_t& dataSize,
								  const std::string& dataHash,
								  const std::string& hashAlgorithm) override {
					auto clients = std::vector<std::pair<hermit::HermitPtr, GetHardLinkInfoCompletionPtr>>();
					{
						std::lock_guard<std::mutex> lock(mHardLinkInfo->mMutex);
						mHardLinkInfo->mResult = result;
						if (result == HardLinkInfoResult::kSuccess) {
							mHardLinkInfo->mDataObjectId = dataObjectId;
							mHardLinkInfo->mDataSize = dataSize;
							mHardLinkInfo->mDataHash = dataHash;
							mHardLinkInfo->mHashAlgorithm = hashAlgorithm;
						}
						clients = mHardLinkInfo->mClients;
					}
					
					for (auto it = begin(clients); it != end(clients); ++it) {
						(*it).second->Call((*it).first,
										   result,
										   mHardLinkInfo->mPaths,
										   dataObjectId,
										   dataSize,
										   dataHash,
										   hashAlgorithm);
					}
				}
				
				//
				HardLinkInfoPtr mHardLinkInfo;
				GetHardLinkInfoCompletionPtr mCompletionFunction;
			};
			
			//
			struct HardLinkInfoParams {
				HermitPtr mH_;
				FilePathPtr mItem;
				ProcessHardLinkFunctionPtr mProcessFunction;
				GetHardLinkInfoCompletionPtr mCompletion;
			};
			typedef std::vector<HardLinkInfoParams> HardLinkInfoFunctionVector;
			
			//
			class HardLinkInfoContext {
			public:
				//
				HardLinkInfoContext(const FilePathPtr& root) :
				mRoot(root),
				mBuildMapResult(BuildHardLinkMapResult::kUnknown) {
				}
				
				//
				FilePathPtr mRoot;
				BuildHardLinkMapResult mBuildMapResult;
				HardLinkInfoMap mInfoMap;
				std::mutex mMutex;
			};
			typedef std::shared_ptr<HardLinkInfoContext> HardLinkInfoContextPtr;
			
			//
			void ProcessOneItem(const HermitPtr& h_,
								const FilePathPtr& item,
								const FilePathPtr& root,
								const HardLinkInfoMap& infoMap,
								const ProcessHardLinkFunctionPtr& processFunction,
								const GetHardLinkInfoCompletionPtr& completion) {
				std::string relativePath;
				if (!GetRelativePath(h_, root, item, relativePath)) {
					NOTIFY_ERROR(h_, "GetRelativePath failed for item:", item, "root:", root);
					completion->Call(h_, HardLinkInfoResult::kError, std::set<std::string>(), "", 0, "", "");
					return;
				}
				auto it = infoMap.find(relativePath);
				if (it == end(infoMap)) {
					NOTIFY_ERROR(h_, "it == context->mInfoMap.end() for relative path:", relativePath);
					completion->Call(h_, HardLinkInfoResult::kError, std::set<std::string>(), "", 0, "", "");
					return;
				}
				HardLinkInfoPtr hardLinkInfo = it->second;
				
				auto result = HardLinkInfoResult::kUnknown;
				bool weShouldProcessItem = false;
				{
					std::lock_guard<std::mutex> lock(hardLinkInfo->mMutex);
					result = hardLinkInfo->mResult;
					if (result == HardLinkInfoResult::kUnknown) {
						hardLinkInfo->mClients.push_back(std::make_pair(h_, completion));
						if (!hardLinkInfo->mProcessing) {
							weShouldProcessItem = true;
							hardLinkInfo->mProcessing = true;
						}
					}
				}
				if (weShouldProcessItem) {
					auto processCompletion = std::make_shared<ProcessCallback>(hardLinkInfo);
					processFunction->Call(h_, processCompletion);
					return;
				}

				if (result == HardLinkInfoResult::kUnknown) {
					// will be handled upon process completion
					return;
				}
				if (result == HardLinkInfoResult::kCanceled) {
					completion->Call(h_, HardLinkInfoResult::kCanceled, std::set<std::string>(), "", 0, "", "");
					return;
				}
				if (result == HardLinkInfoResult::kSuccess) {
					completion->Call(h_,
									 HardLinkInfoResult::kSuccess,
									 hardLinkInfo->mPaths,
									 hardLinkInfo->mDataObjectId,
									 hardLinkInfo->mDataSize,
									 hardLinkInfo->mDataHash,
									 hardLinkInfo->mHashAlgorithm);
					return;
				}
				NOTIFY_ERROR(h_, "HardLinkInfoResult error");
				completion->Call(h_, result, std::set<std::string>(), "", 0, "", "");
			}
			
			//
			void PerformWork(const HermitPtr& h_,
							 const HardLinkInfoContextPtr& context,
							 const FilePathPtr& item,
							 const ProcessHardLinkFunctionPtr& processFunction,
							 const GetHardLinkInfoCompletionPtr& completion) {
				auto result = BuildHardLinkMapResult::kUnknown;
				{
					std::lock_guard<std::mutex> lock(context->mMutex);
					if (context->mBuildMapResult == BuildHardLinkMapResult::kUnknown) {
						HardLinkLookupMap lookupMap;
						HardLinkInfoMap infoMap;
						auto result = BuildHardLinkMap(h_, context->mRoot, lookupMap);
						if (result == BuildHardLinkMapResult::kSuccess) {
							result = BuildHardLinkInfoMap(h_, context->mRoot, lookupMap, infoMap);
							if (result == BuildHardLinkMapResult::kError) {
								NOTIFY_ERROR(h_, "BuildHardLinkInfoMap failed.");
							}
						}
						else if (result != BuildHardLinkMapResult::kCanceled) {
							NOTIFY_ERROR(h_, "BuildHardLinkMap failed.");
						}
						if (result == BuildHardLinkMapResult::kSuccess) {
							context->mInfoMap.swap(infoMap);
						}
						context->mBuildMapResult = result;
					}
					result = context->mBuildMapResult;
				}
				if ((result == BuildHardLinkMapResult::kCanceled) || (CHECK_FOR_ABORT(h_))) {
					completion->Call(h_, HardLinkInfoResult::kCanceled, std::set<std::string>(), "", 0, "", "");
					return;
				}
				if (result != BuildHardLinkMapResult::kSuccess) {
					NOTIFY_ERROR(h_, "result != BuildHardLinkMapResult::kSuccess");
					completion->Call(h_, HardLinkInfoResult::kError, std::set<std::string>(), "", 0, "", "");
					return;
				}
				
				ProcessOneItem(h_, item, context->mRoot, context->mInfoMap, processFunction, completion);
			}
			
		} // namespace HardLinkMap_Impl
		using namespace HardLinkMap_Impl;
		
		//
		class HardLinkMapImpl {
		public:
			//
			HardLinkMapImpl(const FilePathPtr& inRootItem) :
			mContext(std::make_shared<HardLinkInfoContext>(inRootItem)) {
			}
			
			//
			HardLinkInfoContextPtr mContext;
		};
		
		//
		HardLinkMap::HardLinkMap(const FilePathPtr& inRootItem) :
		mImpl(new HardLinkMapImpl(inRootItem)) {
		}
		
		//
		HardLinkMap::~HardLinkMap() {
		}
		
		//
		void HardLinkMap::Call(const HermitPtr& h_,
							   const FilePathPtr& item,
							   const ProcessHardLinkFunctionPtr& processFunction,
							   const GetHardLinkInfoCompletionPtr& completion) {
			PerformWork(h_, mImpl->mContext, item, processFunction, completion);
		}
		
	} // namespace file
} // namespace hermit
