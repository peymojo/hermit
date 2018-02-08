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
#include <string>
#include <vector>
#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/TaskQueue.h"
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
				HardLinkInfo() : mStatus(HardLinkInfoStatus::kUnknown), mDataSize(0) {
				}
				
				//
				HardLinkInfoStatus mStatus;
				std::string mDataObjectId;
				uint64_t mDataSize;
				std::string mDataHash;
				std::vector<std::string> mPaths;
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
					numberValueIt->second->mPaths.push_back(relativePath);
					infoMap.insert(HardLinkInfoMap::value_type(relativePath, numberValueIt->second));
				}
				return BuildHardLinkMapResult::kSuccess;
			}
			
			//
			class ProcessCallback : public ProcessHardLinkCompletionFunction {
			public:
				//
				ProcessCallback(const HardLinkInfoPtr& inHardLinkInfo,
								const GetHardLinkInfoCompletionFunctionPtr& inCompletion) :
				mHardLinkInfo(inHardLinkInfo),
				mCompletionFunction(inCompletion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const HardLinkInfoStatus& status,
								  const std::string& dataObjectId,
								  const uint64_t& dataSize,
								  const std::string& dataHash) override {
					mHardLinkInfo->mStatus = status;
					if (status == HardLinkInfoStatus::kSuccess) {
						mHardLinkInfo->mDataObjectId = dataObjectId;
						mHardLinkInfo->mDataSize = dataSize;
						mHardLinkInfo->mDataHash = dataHash;
					}
					mCompletionFunction->Call(h_, status, mHardLinkInfo->mPaths, dataObjectId, dataSize, dataHash);
				}
				
				//
				HardLinkInfoPtr mHardLinkInfo;
				GetHardLinkInfoCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			struct HardLinkInfoParams {
				HermitPtr mH_;
				FilePathPtr mItem;
				ProcessHardLinkFunctionPtr mProcessFunction;
				GetHardLinkInfoCompletionFunctionPtr mCompletion;
			};
			typedef std::vector<HardLinkInfoParams> HardLinkInfoFunctionVector;
			
			//
			class HardLinkInfoContext {
			public:
				//
				HardLinkInfoContext(const FilePathPtr& root) : mRoot(root), mBuildMapResult(BuildHardLinkMapResult::kUnknown) {
				}
				
				//
				FilePathPtr mRoot;
				BuildHardLinkMapResult mBuildMapResult;
				HardLinkLookupMap mLookupMap;
				HardLinkInfoMap mInfoMap;
			};
			typedef std::shared_ptr<HardLinkInfoContext> HardLinkInfoContextPtr;
			
			//
			void ProcessOneItem(const HermitPtr& h_,
								const HardLinkInfoContextPtr& context,
								const FilePathPtr& item,
								const ProcessHardLinkFunctionPtr& processFunction,
								const GetHardLinkInfoCompletionFunctionPtr& completion) {
				std::string relativePath;
				if (!GetRelativePath(h_, context->mRoot, item, relativePath)) {
					NOTIFY_ERROR(h_, "GetRelativePath failed for item:", item, "root:", context->mRoot);
					completion->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
					return;
				}
				auto it = context->mInfoMap.find(relativePath);
				if (it == context->mInfoMap.end()) {
					NOTIFY_ERROR(h_, "it == context->mInfoMap.end() for relative path:", relativePath);
					completion->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
					return;
				}
				HardLinkInfoPtr hardLinkInfo = it->second;
				if (hardLinkInfo->mStatus == HardLinkInfoStatus::kUnknown) {
					auto processCompletion = std::make_shared<ProcessCallback>(hardLinkInfo, completion);
					processFunction->Call(h_, processCompletion);
					return;
				}
				completion->Call(h_,
								 hardLinkInfo->mStatus,
								 hardLinkInfo->mPaths,
								 hardLinkInfo->mDataObjectId,
								 hardLinkInfo->mDataSize,
								 hardLinkInfo->mDataHash);
			}
			
			//
			void PerformWork(const HermitPtr& h_,
							 const HardLinkInfoContextPtr& context,
							 const FilePathPtr& item,
							 const ProcessHardLinkFunctionPtr& processFunction,
							 const GetHardLinkInfoCompletionFunctionPtr& completion) {
				if (context->mBuildMapResult == BuildHardLinkMapResult::kUnknown) {
					HardLinkLookupMap lookupMap;
					HardLinkInfoMap infoMap;
					auto status = BuildHardLinkMap(h_, context->mRoot, lookupMap);
					if (status == BuildHardLinkMapResult::kSuccess) {
						status = BuildHardLinkInfoMap(h_, context->mRoot, lookupMap, infoMap);
						if (status == BuildHardLinkMapResult::kError) {
							NOTIFY_ERROR(h_, "BuildHardLinkInfoMap failed.");
						}
					}
					else if (status != BuildHardLinkMapResult::kCanceled) {
						NOTIFY_ERROR(h_, "BuildHardLinkMap failed.");
					}
					context->mBuildMapResult = status;
					
					if (status == BuildHardLinkMapResult::kCanceled) {
						completion->Call(h_, HardLinkInfoStatus::kCanceled, std::vector<std::string>(), "", 0, "");
						return;
					}
					if (status != BuildHardLinkMapResult::kSuccess) {
						completion->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
						return;
					}
					context->mLookupMap.swap(lookupMap);
					context->mInfoMap.swap(infoMap);
				}
				
				if ((context->mBuildMapResult == BuildHardLinkMapResult::kCanceled) || (CHECK_FOR_ABORT(h_))) {
					completion->Call(h_, HardLinkInfoStatus::kCanceled, std::vector<std::string>(), "", 0, "");
					return;
				}
				if (context->mBuildMapResult == BuildHardLinkMapResult::kError) {
					completion->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
					return;
				}
				
				ProcessOneItem(h_, context, item, processFunction, completion);
			}
			
			//
			class Task : public AsyncTask {
			public:
				//
				Task(const HardLinkInfoContextPtr& inContext,
					 const FilePathPtr& item,
					 const ProcessHardLinkFunctionPtr& processFunction,
					 const GetHardLinkInfoCompletionFunctionPtr& completion) :
				mContext(inContext),
				mItem(item),
				mProcessFunction(processFunction),
				mCompletion(completion) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PerformWork(h_,
								mContext,
								mItem,
								mProcessFunction,
								mCompletion);
				}
				
				//
				HardLinkInfoContextPtr mContext;
				FilePathPtr mItem;
				ProcessHardLinkFunctionPtr mProcessFunction;
				GetHardLinkInfoCompletionFunctionPtr mCompletion;
			};
			
			//
			class CompletionProxy : public GetHardLinkInfoCompletionFunction {
			public:
				//
				CompletionProxy(const HardLinkMapPtr& hardLinkMap,
								const GetHardLinkInfoCompletionFunctionPtr& completion) :
				mHardLinkMap(hardLinkMap),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const HardLinkInfoStatus& status,
								  const std::vector<std::string>& paths,
								  const std::string& objectDataId,
								  const uint64_t& dataSize,
								  const std::string& dataHash) override {
					mCompletion->Call(h_, status, paths, objectDataId, dataSize, dataHash);
					mHardLinkMap->TaskComplete();
				}

				//
				HardLinkMapPtr mHardLinkMap;
				GetHardLinkInfoCompletionFunctionPtr mCompletion;
			};
			
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
							   const GetHardLinkInfoCompletionFunctionPtr& completion) {
			auto completionProxy = std::make_shared<CompletionProxy>(shared_from_this(), completion);
			auto task = std::make_shared<Task>(mImpl->mContext, item, processFunction, completionProxy);
			if (!QueueTask(h_, task)) {
				NOTIFY_ERROR(h_, "QueueAsyncTask failed");
				completion->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
			}
		}
		
	} // namespace file
} // namespace hermit
