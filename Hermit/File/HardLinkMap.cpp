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
#include "Hermit/Foundation/AsyncTaskQueue.h"
#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/ThreadLock.h"
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
			bool GetRelativePath(const HermitPtr& h_,
								 const FilePathPtr& root,
								 const FilePathPtr& target,
								 std::string& outRelativePath) {
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
			typedef std::vector<GetHardLinkInfoCompletionFunctionPtr> HardLinkFunctionVector;
			
			//
			class HardLinkInfo {
			public:
				//
				HardLinkInfo() :
				mStatus(HardLinkInfoStatus::kUnknown),
				mDataSize(0),
				mFetchingDataObjectID(false) {
				}
				
				//
				HardLinkInfoStatus mStatus;
				std::string mDataObjectID;
				uint64_t mDataSize;
				std::string mDataHash;
				std::vector<std::string> mPaths;
				bool mFetchingDataObjectID;
				HardLinkFunctionVector mFunctions;
				ThreadLock mLock;
			};
			typedef std::shared_ptr<HardLinkInfo> HardLinkInfoPtr;
			
			//
			typedef std::map<std::string, HardLinkInfoPtr> HardLinkInfoMap;
			
			//
			typedef std::pair<uint32_t, uint32_t> FileNumberPair;
			typedef std::multimap<FileNumberPair, FilePathPtr> HardLinkMap;
			
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
												  const bool inDescendSubdirectories,
												  const GetItemChildrenItemCallbackRef& itemCallback) {
				ItemCallback directoryItemCallback;
				auto result = ListDirectoryContentsWithType(h_,
															filePath,
															inDescendSubdirectories,
															directoryItemCallback);
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
				GetChildrenCallback(const HermitPtr& h_, HardLinkMap& inHardLinkMap) :
				mH_(h_),
				mHardLinkMap(inHardLinkMap),
				mCanceled(false) {
				}
				
				//
				virtual bool Function(const HermitPtr& h_, const FilePathPtr& filePath, const FileType& fileType) override {
					if (CHECK_FOR_ABORT(mH_)) {
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
							mHardLinkMap.insert(HardLinkMap::value_type(fileNumberPair, filePath));
						}
					}
					return true;
				}
				
				//
				HermitPtr mH_;
				HardLinkMap& mHardLinkMap;
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
			BuildHardLinkMapResult BuildHardLinkMap(const HermitPtr& h_,
													const FilePathPtr& inRootItem,
													HardLinkMap& inHardLinkMap) {
				
				GetChildrenCallback itemCallback(h_, inHardLinkMap);
				auto result = GetItemChildren(h_, inRootItem, true, itemCallback);
				if (result == GetItemChildrenResult::kCanceled) {
					return BuildHardLinkMapResult::kCanceled;
				}
				if (result != GetItemChildrenResult::kSuccess) {
					// Log the error but continue to try to build the hard link map
					NOTIFY_ERROR(h_, "GetChildren failed for:", inRootItem);
				}
				return BuildHardLinkMapResult::kSuccess;
			}
			
			//
			BuildHardLinkMapResult BuildHardLinkInfoMap(const HermitPtr& h_,
														const FilePathPtr& inRootItem,
														const HardLinkMap& inHardLinkMap,
														HardLinkInfoMap& inHardLinkInfoMap) {
				
				typedef std::map<FileNumberPair, HardLinkInfoPtr> FileNumberValueMap;
				FileNumberValueMap fileNumberValueMap;
				
				auto end = inHardLinkMap.end();
				for (auto it = inHardLinkMap.begin(); it != end; ++it) {
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
					if (!GetRelativePath(h_, inRootItem, it->second, relativePath)) {
						NOTIFY_ERROR(h_, "GetRelativePath failed for item:", it->second, "root item:", inRootItem);
						return BuildHardLinkMapResult::kError;
					}
					numberValueIt->second->mPaths.push_back(relativePath);
					inHardLinkInfoMap.insert(HardLinkInfoMap::value_type(relativePath, numberValueIt->second));
				}
				return BuildHardLinkMapResult::kSuccess;
			}
			
			//
			class IngestLinkTargetCallback : public IngestHardLinkTargetCompletionFunction {
			public:
				//
				IngestLinkTargetCallback(const HardLinkInfoPtr& inHardLinkInfo,
										 const GetHardLinkInfoCompletionFunctionPtr& inCompletionFunction) :
				mHardLinkInfo(inHardLinkInfo),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const HardLinkInfoStatus& inStatus,
								  const std::string& inDataObjectID,
								  const uint64_t& inDataSize,
								  const std::string& inDataHash) override {
					HardLinkFunctionVector otherFunctions;
					std::vector<std::string> paths;
					{
						ThreadLockScope lock(mHardLinkInfo->mLock);
						mHardLinkInfo->mStatus = inStatus;
						if (inStatus == HardLinkInfoStatus::kSuccess) {
							mHardLinkInfo->mDataObjectID = inDataObjectID;
							mHardLinkInfo->mDataSize = inDataSize;
							mHardLinkInfo->mDataHash = inDataHash;
						}
						paths = mHardLinkInfo->mPaths;
						otherFunctions.swap(mHardLinkInfo->mFunctions);
					}
					
					auto end = otherFunctions.end();
					for (auto it = otherFunctions.begin(); it != end; ++it) {
						(*it)->Call(h_, inStatus, paths, inDataObjectID, inDataSize, inDataHash);
					}
					
					mCompletionFunction->Call(h_, inStatus, paths, inDataObjectID, inDataSize, inDataHash);
				}
				
				//
				HardLinkInfoPtr mHardLinkInfo;
				GetHardLinkInfoCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			struct HardLinkInfoParams {
				HermitPtr mH_;
				FilePathPtr mItem;
				IngestHardLinkTargetFunctionPtr mIngestFunction;
				GetHardLinkInfoCompletionFunctionPtr mCompletionFunction;
			};
			typedef std::vector<HardLinkInfoParams> HardLinkInfoFunctionVector;
			
			//
			class HardLinkInfoContext {
			public:
				//
				HardLinkInfoContext(const FilePathPtr& inRootItem) :
				mRootItem(inRootItem),
				mBuildingMap(false),
				mBuildMapResult(BuildHardLinkMapResult::kUnknown) {
				}
				
				//
				FilePathPtr mRootItem;
				HardLinkMap mHardLinkMap;
				HardLinkInfoMap mHardLinkInfoMap;
				ThreadLock mLock;
				
				bool mBuildingMap;
				BuildHardLinkMapResult mBuildMapResult;
				HardLinkInfoFunctionVector mFunctions;
			};
			typedef std::shared_ptr<HardLinkInfoContext> HardLinkInfoContextPtr;
			
			//
			void ProcessOneItem(const HermitPtr& h_,
								const HardLinkInfoContextPtr& inContext,
								const FilePathPtr& inItem,
								const IngestHardLinkTargetFunctionPtr& inIngestTargetFunction,
								const GetHardLinkInfoCompletionFunctionPtr& inCompletionFunction) {
				HardLinkInfoPtr hardLinkInfo;
				{
					ThreadLockScope lock(inContext->mLock);
					
					std::string relativePath;
					if (!GetRelativePath(h_, inContext->mRootItem, inItem, relativePath)) {
						NOTIFY_ERROR(h_, "GetRelativePath failed for item:", inItem, "root:", inContext->mRootItem);
						inCompletionFunction->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
						return;
					}
					
					auto path = relativePath;
					auto it = inContext->mHardLinkInfoMap.find(path);
					if (it == inContext->mHardLinkInfoMap.end()) {
						NOTIFY_ERROR(h_, "it == mHardLinkInfoMap.end() for relative path:", path);
						inCompletionFunction->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
						return;
					}
					hardLinkInfo = it->second;
				}
				
				bool callIngester = false;
				bool callCallback = false;
				HardLinkInfoStatus status = HardLinkInfoStatus::kUnknown;
				std::string dataObjectID;
				uint64_t dataSize = 0;
				std::string dataHash;
				std::vector<std::string> paths;
				{
					ThreadLockScope lock(hardLinkInfo->mLock);
					status = hardLinkInfo->mStatus;
					if (status == HardLinkInfoStatus::kUnknown) {
						if (!hardLinkInfo->mFetchingDataObjectID) {
							hardLinkInfo->mFetchingDataObjectID = true;
							callIngester = true;
						}
						else {
							hardLinkInfo->mFunctions.push_back(inCompletionFunction);
						}
					}
					else {
						if (status == HardLinkInfoStatus::kSuccess) {
							dataObjectID = hardLinkInfo->mDataObjectID;
							paths = hardLinkInfo->mPaths;
							dataSize = hardLinkInfo->mDataSize;
							dataHash = hardLinkInfo->mDataHash;
						}
						callCallback = true;
					}
				}
				
				if (callIngester) {
					auto completion = std::make_shared<IngestLinkTargetCallback>(hardLinkInfo, inCompletionFunction);
					inIngestTargetFunction->Call(h_, completion);
				}
				else if (callCallback) {
					inCompletionFunction->Call(h_, status, paths, dataObjectID, dataSize, dataHash);
				}
			}
			
			//
			void PerformWork(const HermitPtr& h_,
							 const HardLinkInfoContextPtr& inContext,
							 const FilePathPtr& inItem,
							 const IngestHardLinkTargetFunctionPtr& inIngestTargetFunction,
							 const GetHardLinkInfoCompletionFunctionPtr& inCompletionFunction) {
				FilePathPtr rootItem;
				auto priorResult = BuildHardLinkMapResult::kUnknown;
				{
					ThreadLockScope lock(inContext->mLock);
					priorResult = inContext->mBuildMapResult;
					if (priorResult == BuildHardLinkMapResult::kUnknown) {
						if (!inContext->mBuildingMap) {
							// We'll build the map below.
							rootItem = inContext->mRootItem;
							inContext->mBuildingMap = true;
						}
						else {
							// Another thread is building the map, we can just tag along and get
							// notified when it's done.
							HardLinkInfoParams params;
							params.mItem = inItem;
							params.mIngestFunction = inIngestTargetFunction;
							params.mH_ = h_;
							params.mCompletionFunction = inCompletionFunction;
							inContext->mFunctions.push_back(params);
							return;
						}
					}
				}
				
				if ((priorResult == BuildHardLinkMapResult::kCanceled) || (CHECK_FOR_ABORT(h_))) {
					inCompletionFunction->Call(h_, HardLinkInfoStatus::kCanceled, std::vector<std::string>(), "", 0, "");
					return;
				}
				if (priorResult == BuildHardLinkMapResult::kError) {
					inCompletionFunction->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
					return;
				}
				
				if (priorResult == BuildHardLinkMapResult::kUnknown) {
					HardLinkMap hardLinkMap;
					HardLinkInfoMap hardLinkInfoMap;
					auto status = BuildHardLinkMap(h_, rootItem, hardLinkMap);
					if (status == BuildHardLinkMapResult::kSuccess) {
						status = BuildHardLinkInfoMap(h_, rootItem, hardLinkMap, hardLinkInfoMap);
						if (status == BuildHardLinkMapResult::kError) {
							NOTIFY_ERROR(h_, "BuildHardLinkInfoMap failed.");
						}
					}
					else if (status != BuildHardLinkMapResult::kCanceled) {
						NOTIFY_ERROR(h_, "BuildHardLinkMap failed.");
					}
					
					ThreadLockScope lock(inContext->mLock);
					inContext->mBuildMapResult = status;
					inContext->mBuildingMap = false;
					
					if (status == BuildHardLinkMapResult::kCanceled) {
						inCompletionFunction->Call(h_, HardLinkInfoStatus::kCanceled, std::vector<std::string>(), "", 0, "");
						
						auto end = inContext->mFunctions.end();
						for (auto it = inContext->mFunctions.begin(); it != end; ++it) {
							(*it).mCompletionFunction->Call(h_, HardLinkInfoStatus::kCanceled, std::vector<std::string>(), "", 0, "");
						}
						inContext->mFunctions.clear();
						return;
					}
					if (status != BuildHardLinkMapResult::kSuccess) {
						inCompletionFunction->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
						
						auto end = inContext->mFunctions.end();
						for (auto it = inContext->mFunctions.begin(); it != end; ++it) {
							(*it).mCompletionFunction->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
						}
						inContext->mFunctions.clear();
						return;
					}
					
					inContext->mHardLinkMap.swap(hardLinkMap);
					inContext->mHardLinkInfoMap.swap(hardLinkInfoMap);
				}
				
				ProcessOneItem(h_, inContext, inItem, inIngestTargetFunction, inCompletionFunction);
				
				// Now pick up any other clients that are waiting to be notified when the
				// map build has completed.
				ThreadLockScope lock(inContext->mLock);
				auto end = inContext->mFunctions.end();
				for (auto it = inContext->mFunctions.begin(); it != end; ++it) {
					ProcessOneItem((*it).mH_,
								   inContext,
								   (*it).mItem,
								   (*it).mIngestFunction,
								   (*it).mCompletionFunction);
				}
				inContext->mFunctions.clear();
			}
			
			//
			class Task : public AsyncTask {
			public:
				//
				Task(const HardLinkInfoContextPtr& inContext,
					 const FilePathPtr& inItem,
					 const IngestHardLinkTargetFunctionPtr& inIngestTargetFunction,
					 const GetHardLinkInfoCompletionFunctionPtr& inCompletionFunction) :
				mContext(inContext),
				mItem(inItem),
				mIngestTargetFunction(inIngestTargetFunction),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PerformWork(h_,
								mContext,
								mItem,
								mIngestTargetFunction,
								mCompletionFunction);
				}
				
				//
				HardLinkInfoContextPtr mContext;
				FilePathPtr mItem;
				IngestHardLinkTargetFunctionPtr mIngestTargetFunction;
				GetHardLinkInfoCompletionFunctionPtr mCompletionFunction;
			};
			
		} // namespace HardLinkMap_Impl
		using namespace HardLinkMap_Impl;
		
		//
		class HardLinkInfoHandlerImpl {
		public:
			//
			HardLinkInfoHandlerImpl(const FilePathPtr& inRootItem) :
			mContext(std::make_shared<HardLinkInfoContext>(inRootItem)) {
			}
			
			//
			HardLinkInfoContextPtr mContext;
		};
		
		//
		HardLinkInfoHandler::HardLinkInfoHandler(const FilePathPtr& inRootItem) :
		mImpl(new HardLinkInfoHandlerImpl(inRootItem)) {
		}
		
		//
		HardLinkInfoHandler::~HardLinkInfoHandler() {
			delete mImpl;
		}
		
		//
		void HardLinkInfoHandler::Call(const HermitPtr& h_,
									   const FilePathPtr& inItem,
									   const IngestHardLinkTargetFunctionPtr& inIngestTargetFunction,
									   const GetHardLinkInfoCompletionFunctionPtr& inCompletionFunction) {
			auto task = std::make_shared<Task>(mImpl->mContext,
											   inItem,
											   inIngestTargetFunction,
											   inCompletionFunction);
			if (!QueueAsyncTask(h_, task, 60)) {
				NOTIFY_ERROR(h_, "QueueAsyncTask failed");
				inCompletionFunction->Call(h_, HardLinkInfoStatus::kError, std::vector<std::string>(), "", 0, "");
			}
		}
		
	} // namespace file
} // namespace hermit
