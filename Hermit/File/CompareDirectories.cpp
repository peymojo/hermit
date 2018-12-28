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

#include <list>
#include <set>
#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "AppendToFilePath.h"
#include "CompareFiles.h"
#include "CompareFinderInfo.h"
#include "CompareXAttrs.h"
#include "FileNotification.h"
#include "GetFileDates.h"
#include "GetFilePosixOwnership.h"
#include "GetFilePosixPermissions.h"
#include "GetFileType.h"
#include "ListDirectoryContents.h"
#include "PathIsPackage.h"
#include "CompareDirectories.h"

namespace hermit {
	namespace file {
		namespace CompareDirectories_Impl {

			//
			void CompareAttributes(const HermitPtr& h_,
								   const FilePathPtr& filePath1,
								   const FilePathPtr& filePath2,
								   CompareFilesStatus status,
								   bool filesMatch,
								   const IgnoreDates& ignoreDates,
								   const IgnoreFinderInfo& ignoreFinderInfo,
								   const CompareDirectoriesCompletionPtr& completion) {
				if (status == CompareFilesStatus::kCancel) {
					completion->Call(h_, CompareDirectoriesStatus::kCancel);
					return;
				}
				if (status != CompareFilesStatus::kSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles step failed");
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				
				bool attributesMatch = true;
				PathIsPackageCallbackClass path1IsPackage;
				PathIsPackage(h_, filePath1, path1IsPackage);
				if (!path1IsPackage.mSuccess) {
					NOTIFY_ERROR(h_, "PathIsPackage failed for:", filePath1);
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				
				PathIsPackageCallbackClass path2IsPackage;
				PathIsPackage(h_, filePath2, path2IsPackage);
				if (!path2IsPackage.mSuccess) {
					NOTIFY_ERROR(h_, "PathIsPackage failed for:", filePath2);
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				
				bool notifiedPackageStateDifference = false;
				if (path1IsPackage.mIsPackage != path2IsPackage.mIsPackage) {
					attributesMatch = false;
					FileNotificationParams params(kPackageStatesDiffer, filePath1, filePath2);
					NOTIFY(h_, kFilesDifferNotification, &params);
					notifiedPackageStateDifference = true;
				}
				
				bool xattrsMatch = false;
				auto result = CompareXAttrs(h_, filePath1, filePath2, xattrsMatch);
				if (result != CompareXAttrsResult::kSuccess) {
					NOTIFY_ERROR(h_, "CompareXAttrs failed for:", filePath1);
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				if (!xattrsMatch) {
					attributesMatch = false;
				}
				
				uint32_t permissions1 = 0;
				if (!GetFilePosixPermissions(h_, filePath1, permissions1)) {
					NOTIFY_ERROR(h_, "GetFilePosixPermissions failed for path 1:", filePath1);
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				
				uint32_t permissions2 = 0;
				if (!GetFilePosixPermissions(h_, filePath2, permissions2)) {
					NOTIFY_ERROR(h_, "GetFilePosixPermissions failed for path 2:", filePath2);
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				
				if (permissions1 != permissions2) {
					attributesMatch = false;
					FileNotificationParams params(kPermissionsDiffer, filePath1, filePath2);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				
				std::string userOwner1;
				std::string groupOwner1;
				if (!GetFilePosixOwnership(h_, filePath1, userOwner1, groupOwner1)) {
					NOTIFY_ERROR(h_, "GetFilePosixOwnership failed for path 1:", filePath1);
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				
				std::string userOwner2;
				std::string groupOwner2;
				if (!GetFilePosixOwnership(h_, filePath2, userOwner2, groupOwner2)) {
					NOTIFY_ERROR(h_, "GetFilePosixOwnership failed for path 2:", filePath2);
					completion->Call(h_, CompareDirectoriesStatus::kError);
					return;
				}
				
				if (userOwner1 != userOwner2) {
					attributesMatch = false;
					FileNotificationParams params(kUserOwnersDiffer, filePath1, filePath2, userOwner1, userOwner2);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				if (groupOwner1 != groupOwner2) {
					attributesMatch = false;
					FileNotificationParams params(kGroupOwnersDiffer, filePath1, filePath2, groupOwner1, groupOwner2);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				
				if (ignoreFinderInfo == IgnoreFinderInfo::kNo) {
					auto compareFinderInfoStatus = CompareFinderInfo(h_, filePath1, filePath2);
					if ((compareFinderInfoStatus != kCompareFinderInfoStatus_Match) &&
						(compareFinderInfoStatus != kCompareFinderInfoStatus_FinderInfosDiffer) &&
						(compareFinderInfoStatus != kCompareFinderInfoStatus_FolderPackageStatesDiffer)) {
						NOTIFY_ERROR(h_,
									 "CompareFinderInfo failed for path 1:", filePath1,
									 "and path 2:", filePath2);
						completion->Call(h_, CompareDirectoriesStatus::kError);
						return;
					}
					
					if (compareFinderInfoStatus == kCompareFinderInfoStatus_FolderPackageStatesDiffer) {
						if (!notifiedPackageStateDifference) {
							attributesMatch = false;
							FileNotificationParams params(kPackageStatesDiffer, filePath1, filePath2);
							NOTIFY(h_, kFilesDifferNotification, &params);
							notifiedPackageStateDifference = true;
						}
					}
					else if (compareFinderInfoStatus == kCompareFinderInfoStatus_FinderInfosDiffer) {
						attributesMatch = false;
						FileNotificationParams params(kFinderInfosDiffer, filePath1, filePath2);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
				}
				
				if (ignoreDates == IgnoreDates::kNo) {
					GetFileDatesCallbackClassT<std::string> datesCallback1;
					GetFileDates(h_, filePath1, datesCallback1);
					if (!datesCallback1.mSuccess) {
						NOTIFY_ERROR(h_, "GetFileDates failed for:", filePath1);
						completion->Call(h_, CompareDirectoriesStatus::kError);
						return;
					}
					
					GetFileDatesCallbackClassT<std::string> datesCallback2;
					GetFileDates(h_, filePath2, datesCallback2);
					if (!datesCallback2.mSuccess) {
						NOTIFY_ERROR(h_, "GetFileDates failed for:", filePath2);
						completion->Call(h_, CompareDirectoriesStatus::kError);
						return;
					}
					
					if (datesCallback1.mCreationDate != datesCallback2.mCreationDate) {
						attributesMatch = false;
						FileNotificationParams params(kCreationDatesDiffer,
													  filePath1,
													  filePath2,
													  datesCallback1.mCreationDate,
													  datesCallback2.mCreationDate);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					if (datesCallback1.mModificationDate != datesCallback2.mModificationDate) {
						attributesMatch = false;
						FileNotificationParams params(kModificationDatesDiffer,
													  filePath1,
													  filePath2,
													  datesCallback1.mModificationDate,
													  datesCallback2.mModificationDate);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
				}
				
				if (!filesMatch) {
					FileNotificationParams params(kFolderContentsDiffer, filePath1, filePath2);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				else if (attributesMatch) {
					FileNotificationParams params(kFilesMatch, filePath1, filePath2);
					NOTIFY(h_, kFilesMatchNotification, &params);
				}
				completion->Call(h_, CompareDirectoriesStatus::kSuccess);
			}

			
			//
			enum class MatchStatus {
				kUnknown,
				kFilesMatch,
				kFilesDontMatch
			};
			
			//
			class HermitProxy : public Hermit {
			public:
				//
				HermitProxy(const HermitPtr& h_) : mH_(h_), mMatchStatus(MatchStatus::kUnknown) {
				}
				
				//
				virtual bool ShouldAbort() override {
					return mH_->ShouldAbort();
				}
				
				//
				virtual void Notify(const char* name, const void* param) override {
					std::string nameStr(name);
					if (nameStr == kFilesMatchNotification) {
						mMatchStatus = MatchStatus::kFilesMatch;
					}
					else if (nameStr == kFilesDifferNotification) {
						mMatchStatus = MatchStatus::kFilesDontMatch;
					}
					NOTIFY(mH_, name, param);
				}
				
				//
				HermitPtr mH_;
				MatchStatus mMatchStatus;
			};
			typedef std::shared_ptr<HermitProxy> HermitProxyPtr;
			
//			//
//			class Aggregator {
//			public:
//				//
//				Aggregator(const HermitPtr& h_, const InternalCompletionPtr& completion) :
//				mH_(h_),
//				mCompletion(completion),
//				mStatus(CompareFilesStatus::kSuccess),
//				mFilesMatch(true),
//				mPendingTasks(0),
//				mAllTasksAdded(false) {
//				}
//
//				//
//				void TaskAdded() {
//					mPendingTasks++;
//				}
//
//				//
//				void AllTasksAdded(const HermitPtr& h_) {
//					mAllTasksAdded = true;
//					CheckCompletion(h_);
//				}
//
//				//
//				void TaskComplete(const HermitPtr& h_, const CompareFilesStatus& status, MatchStatus matchStatus) {
//					if (status == CompareFilesStatus::kCancel) {
//						if (mStatus == CompareFilesStatus::kSuccess) {
//							mStatus = CompareFilesStatus::kCancel;
//						}
//					}
//					else if (status != CompareFilesStatus::kSuccess) {
//						mStatus = status;
//					}
//					if (matchStatus == MatchStatus::kUnknown) {
//						NOTIFY_ERROR(mH_, "matchStatus == MatchStatus::kUnknown");
//						mStatus = CompareFilesStatus::kError;
//					}
//					else if (matchStatus == MatchStatus::kFilesDontMatch) {
//						mFilesMatch = false;
//					}
//
//					mPendingTasks--;
//					CheckCompletion(h_);
//				}
//
//				//
//				void CheckCompletion(const HermitPtr& h_) {
//					if (mAllTasksAdded && (mPendingTasks == 0)) {
//						mCompletion->CompareFilesComplete(h_, mStatus, mFilesMatch);
//					}
//				}
//
//				//
//				HermitPtr mH_;
//				InternalCompletionPtr mCompletion;
//				CompareFilesStatus mStatus;
//				std::atomic_bool mFilesMatch;
//				std::atomic_int mPendingTasks;
//				std::atomic_bool mAllTasksAdded;
//			};
//			typedef std::shared_ptr<Aggregator> AggregatorPtr;
			
//			//
//			class CompareCompletion : public CompareFilesCompletion {
//			public:
//				//
//				CompareCompletion(const HermitProxyPtr& proxy,
//								  const AggregatorPtr& aggregator,
//								  const FilePathPtr& filePath1,
//								  const FilePathPtr& filePath2) :
//				mProxy(proxy),
//				mAggregator(aggregator),
//				mFilePath1(filePath1),
//				mFilePath2(filePath2) {
//					mAggregator->TaskAdded();
//				}
//
//				//
//				virtual void Call(const HermitPtr& h_, const CompareFilesStatus& status) override {
//					if ((status != CompareFilesStatus::kSuccess) && (status != CompareFilesStatus::kCancel)) {
//						NOTIFY_ERROR(h_, "CompareFiles failed for:", mFilePath1, "and:", mFilePath2);
//					}
//					mAggregator->TaskComplete(h_, status, mProxy->mMatchStatus);
//				}
//
//				//
//				HermitProxyPtr mProxy;
//				AggregatorPtr mAggregator;
//				FilePathPtr mFilePath1;
//				FilePathPtr mFilePath2;
//			};
			
			
			
//			//
//			void CompareTwoItems(const HermitPtr& h_,
//								 const FilePathPtr& filePath1,
//								 const FilePathPtr& filePath2,
//								 ) {
//				FileType fileType1 = FileType::kUnknown;
//				if (!GetFileType(h_, filePath1, fileType1)) {
//					NOTIFY_ERROR(h_, "CompareFiles: GetFileType failed for:", filePath1);
//					completion->Call(h_, CompareFilesStatus::kError);
//					return;
//				}
//				FileType fileType2 = FileType::kUnknown;
//				if (!GetFileType(h_, filePath2, fileType2)) {
//					NOTIFY_ERROR(h_, "CompareFiles: GetFileType failed for:", filePath2);
//					completion->Call(h_, CompareFilesStatus::kError);
//					return;
//				}
//
//				// Are these even the same fundamental type of file?
//				if (fileType1 != fileType2) {
//					FileNotificationParams params(kFileTypesDiffer, filePath1, filePath2);
//					NOTIFY(h_, kFilesDifferNotification, &params);
//
//					// Different fundamental file types, no reason to continue comparing the details.
//					completion->Call(h_, CompareFilesStatus::kSuccess);
//					return;
//				}
//			}
			
			
			
			
			
			//
			struct FileInfo {
				//
				FileInfo(FilePathPtr path, const std::string& name) :
				mPath(path),
				mName(name),
				mLowerName(name) {
					std::transform(mLowerName.begin(), mLowerName.end(), mLowerName.begin(), ::tolower);
				}
				
				//
				~FileInfo() {
				}
				
				//
				FilePathPtr mPath;
				std::string mName;
				std::string mLowerName;
			};
			typedef std::shared_ptr<FileInfo> FileInfoPtr;
			
			//
			struct CompareFileInfoPtrs : public std::binary_function<FileInfoPtr, FileInfoPtr, bool> {
				//
				bool operator ()(const FileInfoPtr& inLHS, const FileInfoPtr& inRHS) const {
					if (inLHS->mLowerName == inRHS->mLowerName) {
						return inLHS->mName < inRHS->mName;
					}
					return inLHS->mLowerName < inRHS->mLowerName;
				}
			};
			
			//
			typedef std::set<FileInfoPtr, CompareFileInfoPtrs> FileSet;
			
			//
			class Directory : public ListDirectoryContentsItemCallback {
			public:
				//
				Directory(const PreprocessFileFunctionPtr& preprocessFunction) :
				mPreprocessFunction(preprocessFunction) {
				}
				
				//
				virtual bool OnItem(const HermitPtr& h_,
									const ListDirectoryContentsResult& result,
									const FilePathPtr& parentPath,
									const std::string& itemName) override {
					PreprocessFileInstruction instruction = PreprocessFileInstruction::kContinue;
					if (mPreprocessFunction != nullptr) {
						instruction = mPreprocessFunction->Preprocess(h_, parentPath, itemName);
					}
					
					FilePathPtr itemPath;
					AppendToFilePath(h_, parentPath, itemName, itemPath);
					if (itemPath == nullptr) {
						NOTIFY_ERROR(h_, "AppendToFilePath failed.");
						return false;
					}
						
					if (instruction == PreprocessFileInstruction::kSkip) {
						FileNotificationParams params(kFileSkipped, itemPath);
						NOTIFY(h_, kFileSkippedNotification, &params);
					}
					else if (instruction != PreprocessFileInstruction::kContinue) {
						NOTIFY_ERROR(h_, "preprocess function failed for:", itemPath);
						return false;
					}
					else {
						FileInfoPtr p(new FileInfo(itemPath, itemName));
						mFiles.insert(p);
					}
					return true;
				}
				
				//
				PreprocessFileFunctionPtr mPreprocessFunction;
				FileSet mFiles;
			};
			typedef std::shared_ptr<Directory> DirectoryPtr;
			
			//
			struct DirectoryContext {
				//
				DirectoryContext(const DirectoryPtr& dir1, const DirectoryPtr& dir2) :
				mDir1(dir1),
				mDir2(dir2) {
					mIt1 = begin(dir1->mFiles);
					mEnd1 = end(dir1->mFiles);
					mIt2 = begin(dir2->mFiles);
					mEnd2 = end(dir2->mFiles);
				}
				
				//
				DirectoryPtr mDir1;
				DirectoryPtr mDir2;
				FileSet::const_iterator mIt1;
				FileSet::const_iterator mEnd1;
				FileSet::const_iterator mIt2;
				FileSet::const_iterator mEnd2;
			};
			typedef std::shared_ptr<DirectoryContext> DirectoryContextPtr;
			
//			//
//			static void CompareDirectoryFiles(const HermitPtr& h_,
//											  const Directory& dir1,
//											  const Directory& dir2,
//											  const HardLinkMapPtr& hardLinkMap1,
//											  const HardLinkMapPtr& hardLinkMap2,
//											  const IgnoreDates& ignoreDates,
//											  const IgnoreFinderInfo& ignoreFinderInfo,
//											  const PreprocessFileFunctionPtr& preprocessFunction,
//											  const InternalCompletionPtr& completion) {
//				const FileSet& dir1Files = dir1.GetFiles();
//				const FileSet& dir2Files = dir2.GetFiles();
//
//				FileSet::const_iterator end1 = dir1Files.end();
//				FileSet::const_iterator end2 = dir2Files.end();
//
//				FileSet::const_iterator it1 = dir1Files.begin();
//				FileSet::const_iterator it2 = dir2Files.begin();
//
//				CompareFileInfoPtrs firstComesBeforeSecondFn;
//				auto aggregator = std::make_shared<Aggregator>(h_, completion);
//				bool match = true;
//				while (true) {
//					if (it1 == end1) {
//						while (it2 != end2) {
//							match = false;
//							FileNotificationParams params(kItemInPath2Only, NULL, (*it2)->mPath);
//							NOTIFY(h_, kFilesDifferNotification, &params);
//							++it2;
//						}
//						break;
//					}
//					if (it2 == end2) {
//						while (it1 != end1) {
//							match = false;
//							FileNotificationParams params(kItemInPath1Only, (*it1)->mPath, NULL);
//							NOTIFY(h_, kFilesDifferNotification, &params);
//							++it1;
//						}
//						break;
//					}
//
//					if (firstComesBeforeSecondFn((*it1), (*it2))) {
//						match = false;
//						FileNotificationParams params(kItemInPath1Only, (*it1)->mPath, NULL);
//						NOTIFY(h_, kFilesDifferNotification, &params);
//						++it1;
//					}
//					else if (firstComesBeforeSecondFn((*it2), (*it1))) {
//						match = false;
//						FileNotificationParams params(kItemInPath2Only, NULL, (*it2)->mPath);
//						NOTIFY(h_, kFilesDifferNotification, &params);
//						++it2;
//					}
//					else {
//						auto proxy = std::make_shared<HermitProxy>(h_);
//						auto completion = std::make_shared<CompareCompletion>(proxy, aggregator, (*it1)->mPath, (*it2)->mPath);
//						CompareFiles(proxy,
//									 (*it1)->mPath,
//									 (*it2)->mPath,
//									 hardLinkMap1,
//									 hardLinkMap2,
//									 ignoreDates,
//									 ignoreFinderInfo,
//									 preprocessFunction,
//									 completion);
//						++it1;
//						++it2;
//					}
//				}
//				aggregator->AllTasksAdded(h_);
//			}
			
			//
			class Comparator : public std::enable_shared_from_this<Comparator> {
			public:
				//
				Comparator(const FilePathPtr& filePath1,
						   const FilePathPtr& filePath2,
						   const HardLinkMapPtr& hardLinkMap1,
						   const HardLinkMapPtr& hardLinkMap2,
						   const IgnoreDates& ignoreDates,
						   const IgnoreFinderInfo& ignoreFinderInfo,
						   const PreprocessFileFunctionPtr& preprocessFunction,
						   const CompareDirectoriesCompletionPtr& completion) :
				mFilePath1(filePath1),
				mFilePath2(filePath2),
				mHardLinkMap1(hardLinkMap1),
				mHardLinkMap2(hardLinkMap2),
				mIgnoreDates(ignoreDates),
				mIgnoreFinderInfo(ignoreFinderInfo),
				mPreprocessFunction(preprocessFunction),
				mCompletion(completion),
				mStatus(CompareFilesStatus::kSuccess),
				mMatch(true),
				mBusy(false) {
					pthread_mutex_init(&mMutex, nullptr);
					pthread_cond_init(&mCondition, nullptr);
				}
				
				//
				void Begin(const hermit::HermitPtr& h_) {
					if (!PrepareDirectories(h_, mFilePath1, mFilePath2)) {
						NOTIFY_ERROR(h_, "PrepareDirectories failed for:", mFilePath1, mFilePath2);
						mCompletion->Call(h_, CompareDirectoriesStatus::kError);
						return;
					}
					
					while (true) {
						pthread_mutex_lock(&mMutex);
						while (mBusy) {
							pthread_cond_wait(&mCondition, &mMutex);
						}
						mBusy = true;
						pthread_mutex_unlock(&mMutex);
						
						if (mContextList.empty()) {
							break;
						}
						auto context = mContextList.front();
						ProcessContext(h_, context);
					}
					
					CompareAttributes(h_,
									  mFilePath2,
									  mFilePath2,
									  mStatus,
									  mMatch,
									  mIgnoreDates,
									  mIgnoreFinderInfo,
									  mCompletion);
				}
				
				//
				void ProcessContext(const hermit::HermitPtr& h_, const DirectoryContextPtr& context) {
					if (context->mIt1 == context->mEnd1) {
						while (context->mIt2 != context->mEnd2) {
							mMatch = false;
							FileNotificationParams params(kItemInPath2Only, NULL, (*context->mIt2)->mPath);
							NOTIFY(h_, kFilesDifferNotification, &params);
							++context->mIt2;
						}
						
						pthread_mutex_lock(&mMutex);
						mContextList.pop_front();
						mBusy = false;
						pthread_cond_signal(&mCondition);
						pthread_mutex_unlock(&mMutex);
						return;
					}
					
					if (context->mIt2 == context->mEnd2) {
						while (context->mIt1 != context->mEnd1) {
							mMatch = false;
							FileNotificationParams params(kItemInPath1Only, (*context->mIt1)->mPath, NULL);
							NOTIFY(h_, kFilesDifferNotification, &params);
							++context->mIt1;
						}
						
						pthread_mutex_lock(&mMutex);
						mContextList.pop_front();
						mBusy = false;
						pthread_cond_signal(&mCondition);
						pthread_mutex_unlock(&mMutex);
						return;
					}
						
					CompareFileInfoPtrs firstComesBeforeSecondFn;
					if (firstComesBeforeSecondFn((*context->mIt1), (*context->mIt2))) {
						mMatch = false;
						FileNotificationParams params(kItemInPath1Only, (*context->mIt1)->mPath, NULL);
						NOTIFY(h_, kFilesDifferNotification, &params);
						++context->mIt1;
						
						pthread_mutex_lock(&mMutex);
						mBusy = false;
						pthread_cond_signal(&mCondition);
						pthread_mutex_unlock(&mMutex);
						return;
					}
					
					if (firstComesBeforeSecondFn((*context->mIt2), (*context->mIt1))) {
						mMatch = false;
						FileNotificationParams params(kItemInPath2Only, NULL, (*context->mIt2)->mPath);
						NOTIFY(h_, kFilesDifferNotification, &params);
						++context->mIt2;

						pthread_mutex_lock(&mMutex);
						mBusy = false;
						pthread_cond_signal(&mCondition);
						pthread_mutex_unlock(&mMutex);
						return;
					}
					
					auto path1 = (*context->mIt1)->mPath;
					auto path2 = (*context->mIt2)->mPath;
					++context->mIt1;
					++context->mIt2;

					auto proxy = std::make_shared<HermitProxy>(h_);
					auto completion = std::make_shared<CompareCompletion>(shared_from_this(), proxy, path1, path2);
					CompareFiles(proxy,
								 path1,
								 path2,
								 mHardLinkMap1,
								 mHardLinkMap2,
								 mIgnoreDates,
								 mIgnoreFinderInfo,
								 mPreprocessFunction,
								 completion);
				}
				
				//
				class CompareCompletion : public CompareFilesCompletion {
				public:
					//
					CompareCompletion(const std::shared_ptr<Comparator>& this_,
									  const HermitProxyPtr& proxy,
									  const FilePathPtr& filePath1,
									  const FilePathPtr& filePath2) :
					mThis(this_),
					mProxy(proxy),
					mFilePath1(filePath1),
					mFilePath2(filePath2) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const CompareFilesStatus& status) override {
						mThis->CompareComplete(h_, status, mFilePath1, mFilePath2, mProxy->mMatchStatus);
					}
					
					//
					std::shared_ptr<Comparator> mThis;
					HermitProxyPtr mProxy;
					FilePathPtr mFilePath1;
					FilePathPtr mFilePath2;
				};

				//
				void CompareComplete(const HermitPtr& h_,
									 const CompareFilesStatus& status,
									 const FilePathPtr& filePath1,
									 const FilePathPtr& filePath2,
									 const MatchStatus& matchStatus) {
					if ((status != CompareFilesStatus::kSuccess) && (status != CompareFilesStatus::kCancel)) {
						NOTIFY_ERROR(h_, "CompareFiles failed for:", filePath1, "and:", filePath2);
					}

					if (status == CompareFilesStatus::kCancel) {
						if (mStatus == CompareFilesStatus::kSuccess) {
							mStatus = CompareFilesStatus::kCancel;
						}
					}
					else if (status != CompareFilesStatus::kSuccess) {
						mStatus = status;
					}
					if (matchStatus == MatchStatus::kUnknown) {
						NOTIFY_ERROR(h_, "matchStatus == MatchStatus::kUnknown");
						mStatus = CompareFilesStatus::kError;
					}
					else if (matchStatus == MatchStatus::kFilesDontMatch) {
						mMatch = false;
					}
					
					pthread_mutex_lock(&mMutex);
					mBusy = false;
					pthread_cond_signal(&mCondition);
					pthread_mutex_unlock(&mMutex);
				}

				//
				bool PrepareDirectories(const HermitPtr& h_,
										const FilePathPtr& filePath1,
										const FilePathPtr& filePath2) {
					auto dir1 = std::make_shared<Directory>(mPreprocessFunction);
					auto status1 = ListDirectoryContents(h_, filePath1, false, *dir1);
					if ((status1 != ListDirectoryContentsResult::kSuccess) && (status1 != ListDirectoryContentsResult::kPermissionDenied)) {
						NOTIFY_ERROR(h_, "ListDirectoryContents failed for:", filePath1);
						return false;
					}
					
					auto dir2 = std::make_shared<Directory>(mPreprocessFunction);
					auto status2 = ListDirectoryContents(h_, filePath2, false, *dir2);
					if ((status2 != ListDirectoryContentsResult::kSuccess) && (status2 != ListDirectoryContentsResult::kPermissionDenied)) {
						NOTIFY_ERROR(h_, "ListDirectoryContents failed for:", filePath2);
						return false;
					}
					
					if ((status1 == ListDirectoryContentsResult::kPermissionDenied) || (status2 == ListDirectoryContentsResult::kPermissionDenied)) {
						if (status1 != status2) {
							FileNotificationParams params(kDirectoryAccessDiffers, filePath1, filePath2);
							NOTIFY(h_, kFilesDifferNotification, &params);
						}
						if (status1 == ListDirectoryContentsResult::kPermissionDenied) {
							FileNotificationParams params(kPermissionDenied, filePath1);
							NOTIFY(h_, kPermissionDeniedNotification, &params);
						}
						if (status2 == ListDirectoryContentsResult::kPermissionDenied) {
							FileNotificationParams params(kPermissionDenied, filePath2);
							NOTIFY(h_, kPermissionDeniedNotification, &params);
						}
						// Can't really continue to test attributes and so forth since those calls will
						// likely also encounter permission denied errors. But having reported this, we can
						// continue to compare other items in the directory tree.
						pthread_mutex_lock(&mMutex);
						mMatch = false;
						mBusy = false;
						pthread_cond_signal(&mCondition);
						pthread_mutex_unlock(&mMutex);
						return true;
					}
					
					auto context = std::make_shared<DirectoryContext>(dir1, dir2);
					
					pthread_mutex_lock(&mMutex);
					mContextList.push_back(context);
					mBusy = false;
					pthread_cond_signal(&mCondition);
					pthread_mutex_unlock(&mMutex);
					
					return true;
				}
				
				//
				FilePathPtr mFilePath1;
				FilePathPtr mFilePath2;
				HardLinkMapPtr mHardLinkMap1;
				HardLinkMapPtr mHardLinkMap2;
				IgnoreDates mIgnoreDates;
				IgnoreFinderInfo mIgnoreFinderInfo;
				PreprocessFileFunctionPtr mPreprocessFunction;
				CompareDirectoriesCompletionPtr mCompletion;
				
				CompareFilesStatus mStatus;
				std::atomic_bool mMatch;
				std::list<DirectoryContextPtr> mContextList;
				bool mBusy;
				pthread_mutex_t mMutex;
				pthread_cond_t mCondition;
			};
			
		} // namespace CompareDirectories_Impl
        using namespace CompareDirectories_Impl;
		
		void CompareDirectories(const HermitPtr& h_,
								const FilePathPtr& filePath1,
								const FilePathPtr& filePath2,
								const HardLinkMapPtr& hardLinkMap1,
								const HardLinkMapPtr& hardLinkMap2,
								const IgnoreDates& ignoreDates,
								const IgnoreFinderInfo& ignoreFinderInfo,
								const PreprocessFileFunctionPtr& preprocessFunction,
								const CompareDirectoriesCompletionPtr& completion) {
			FileType fileType1 = FileType::kUnknown;
			if (!GetFileType(h_, filePath1, fileType1)) {
				NOTIFY_ERROR(h_, "GetFileType failed for:", filePath1);
				completion->Call(h_, CompareDirectoriesStatus::kError);
				return;
			}
			if (fileType1 != FileType::kDirectory) {
				NOTIFY_ERROR(h_, "path not a directory:", filePath1);
				completion->Call(h_, CompareDirectoriesStatus::kError);
				return;
			}
			FileType fileType2 = FileType::kUnknown;
			if (!GetFileType(h_, filePath2, fileType2)) {
				NOTIFY_ERROR(h_, "GetFileType failed for:", filePath2);
				completion->Call(h_, CompareDirectoriesStatus::kError);
				return;
			}
			if (fileType2 != FileType::kDirectory) {
				NOTIFY_ERROR(h_, "path not a directory:", filePath2);
				completion->Call(h_, CompareDirectoriesStatus::kError);
				return;
			}
			
			auto comparator = std::make_shared<Comparator>(filePath1,
														   filePath2,
														   hardLinkMap1,
														   hardLinkMap2,
														   ignoreDates,
														   ignoreFinderInfo,
														   preprocessFunction,
														   completion);
			comparator->Begin(h_);
		}
		
	} // namespace file
} // namespace hermit
