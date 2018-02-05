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

#include <set>
#include <string>
#include <vector>
#include "Hermit/Foundation/AsyncTaskQueue.h"
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
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;

			//
			class InternalCompletion {
			public:
				//
				InternalCompletion(const HermitPtr& h_,
								   const FilePathPtr& filePath1,
								   const FilePathPtr& filePath2,
								   bool ignoreDates,
								   bool ignoreFinderInfo,
								   const CompareDirectoriesCompletionPtr& completion) :
				mH_(h_),
				mFilePath1(filePath1),
				mFilePath2(filePath2),
				mIgnoreDates(ignoreDates),
				mIgnoreFinderInfo(ignoreFinderInfo),
				mCompletion(completion) {
				}
				
				//
				void CompareFilesComplete(CompareFilesStatus status, bool filesMatch) {
					if (status == CompareFilesStatus::kCancel) {
						mCompletion->Call(CompareDirectoriesStatus::kCancel);
						return;
					}
					if (status != CompareFilesStatus::kSuccess) {
						NOTIFY_ERROR(mH_, "CompareDirectories: CompareFiles step failed");
						mCompletion->Call(CompareDirectoriesStatus::kError);
						return;
					}
					
					bool attributesMatch = true;
					PathIsPackageCallbackClass path1IsPackage;
					PathIsPackage(mH_, mFilePath1, path1IsPackage);
					if (!path1IsPackage.mSuccess) {
						NOTIFY_ERROR(mH_, "CompareDirectories: PathIsPackage failed for:", mFilePath1);
						mCompletion->Call(CompareDirectoriesStatus::kError);
						return;
					}
					
					PathIsPackageCallbackClass path2IsPackage;
					PathIsPackage(mH_, mFilePath2, path2IsPackage);
					if (!path2IsPackage.mSuccess) {
						NOTIFY_ERROR(mH_, "CompareDirectories: PathIsPackage failed for:", mFilePath2);
						mCompletion->Call(CompareDirectoriesStatus::kError);
						return;
					}
					
					bool notifiedPackageStateDifference = false;
					if (path1IsPackage.mIsPackage != path2IsPackage.mIsPackage) {
						attributesMatch = false;
						FileNotificationParams params(kPackageStatesDiffer, mFilePath1, mFilePath2);
						NOTIFY(mH_, kFilesDifferNotification, &params);
						notifiedPackageStateDifference = true;
					}
					
                    bool xattrsMatch = false;
                    auto result = CompareXAttrs(mH_, mFilePath1, mFilePath2, xattrsMatch);
                    if (result != CompareXAttrsResult::kSuccess) {
                        NOTIFY_ERROR(mH_, "CompareXAttrs failed for:", mFilePath1);
                        mCompletion->Call(CompareDirectoriesStatus::kError);
                        return;
                    }
                    if (!xattrsMatch) {
                        attributesMatch = false;
                    }

                    uint32_t permissions1 = 0;
					if (!GetFilePosixPermissions(mH_, mFilePath1, permissions1)) {
						NOTIFY_ERROR(mH_, "CompareDirectories: GetFilePosixPermissions failed for path 1:", mFilePath1);
						mCompletion->Call(CompareDirectoriesStatus::kError);
						return;
					}
					
					uint32_t permissions2 = 0;
					if (!GetFilePosixPermissions(mH_, mFilePath2, permissions2)) {
						NOTIFY_ERROR(mH_, "CompareDirectories: GetFilePosixPermissions failed for path 2:", mFilePath2);
						mCompletion->Call(CompareDirectoriesStatus::kError);
						return;
					}
					
					if (permissions1 != permissions2) {
						attributesMatch = false;
						FileNotificationParams params(kPermissionsDiffer, mFilePath1, mFilePath2);
						NOTIFY(mH_, kFilesDifferNotification, &params);
					}
					
					std::string userOwner1;
					std::string groupOwner1;
					if (!GetFilePosixOwnership(mH_, mFilePath1, userOwner1, groupOwner1)) {
						NOTIFY_ERROR(mH_, "CompareDirectories: GetFilePosixOwnership failed for path 1:", mFilePath1);
						mCompletion->Call(CompareDirectoriesStatus::kError);
						return;
					}
					
					std::string userOwner2;
					std::string groupOwner2;
					if (!GetFilePosixOwnership(mH_, mFilePath2, userOwner2, groupOwner2)) {
						NOTIFY_ERROR(mH_, "CompareDirectories: GetFilePosixOwnership failed for path 2:", mFilePath2);
						mCompletion->Call(CompareDirectoriesStatus::kError);
						return;
					}
					
					if (userOwner1 != userOwner2) {
						attributesMatch = false;
						FileNotificationParams params(kUserOwnersDiffer, mFilePath1, mFilePath2, userOwner1, userOwner2);
						NOTIFY(mH_, kFilesDifferNotification, &params);
					}
					if (groupOwner1 != groupOwner2) {
						attributesMatch = false;
						FileNotificationParams params(kGroupOwnersDiffer, mFilePath1, mFilePath2, groupOwner1, groupOwner2);
						NOTIFY(mH_, kFilesDifferNotification, &params);
					}
					
					if (!mIgnoreFinderInfo) {
						auto compareFinderInfoStatus = CompareFinderInfo(mH_, mFilePath1, mFilePath2);
						if ((compareFinderInfoStatus != kCompareFinderInfoStatus_Match) &&
							(compareFinderInfoStatus != kCompareFinderInfoStatus_FinderInfosDiffer) &&
							(compareFinderInfoStatus != kCompareFinderInfoStatus_FolderPackageStatesDiffer)) {
							NOTIFY_ERROR(mH_, "CompareDirectories: CompareFinderInfo failed for path 1:", mFilePath1);
							NOTIFY_ERROR(mH_, "-- and path 2:", mFilePath2);
							mCompletion->Call(CompareDirectoriesStatus::kError);
							return;
						}
						
						if (compareFinderInfoStatus == kCompareFinderInfoStatus_FolderPackageStatesDiffer) {
							if (!notifiedPackageStateDifference) {
								attributesMatch = false;
								FileNotificationParams params(kPackageStatesDiffer, mFilePath1, mFilePath2);
								NOTIFY(mH_, kFilesDifferNotification, &params);
								notifiedPackageStateDifference = true;
							}
						}
						else if (compareFinderInfoStatus == kCompareFinderInfoStatus_FinderInfosDiffer) {
							attributesMatch = false;
							FileNotificationParams params(kFinderInfosDiffer, mFilePath1, mFilePath2);
							NOTIFY(mH_, kFilesDifferNotification, &params);
						}
					}
					
					if (!mIgnoreDates) {
						GetFileDatesCallbackClassT<std::string> datesCallback1;
						GetFileDates(mH_, mFilePath1, datesCallback1);
						if (!datesCallback1.mSuccess) {
							NOTIFY_ERROR(mH_, "CompareDirectories: GetFileDates failed for:", mFilePath1);
							mCompletion->Call(CompareDirectoriesStatus::kError);
							return;
						}
						
						GetFileDatesCallbackClassT<std::string> datesCallback2;
						GetFileDates(mH_, mFilePath2, datesCallback2);
						if (!datesCallback2.mSuccess) {
							NOTIFY_ERROR(mH_, "CompareDirectories: GetFileDates failed for:", mFilePath2);
							mCompletion->Call(CompareDirectoriesStatus::kError);
							return;
						}
						
						if (datesCallback1.mCreationDate != datesCallback2.mCreationDate) {
							attributesMatch = false;
							FileNotificationParams params(
														  kCreationDatesDiffer,
														  mFilePath1,
														  mFilePath2,
														  datesCallback1.mCreationDate,
														  datesCallback2.mCreationDate);
							NOTIFY(mH_, kFilesDifferNotification, &params);
						}
						if (datesCallback1.mModificationDate != datesCallback2.mModificationDate) {
							attributesMatch = false;
							FileNotificationParams params(
														  kModificationDatesDiffer,
														  mFilePath1,
														  mFilePath2,
														  datesCallback1.mModificationDate,
														  datesCallback2.mModificationDate);
							NOTIFY(mH_, kFilesDifferNotification, &params);
						}
					}
					
					if (!filesMatch) {
						FileNotificationParams params(kFolderContentsDiffer, mFilePath1, mFilePath2);
						NOTIFY(mH_, kFilesDifferNotification, &params);
					}
					else if (attributesMatch) {
						FileNotificationParams params(kFilesMatch, mFilePath1, mFilePath2);
						NOTIFY(mH_, kFilesMatchNotification, &params);
					}
					mCompletion->Call(CompareDirectoriesStatus::kSuccess);
				}
				
				//
				HermitPtr mH_;
				FilePathPtr mFilePath1;
				FilePathPtr mFilePath2;
				bool mIgnoreDates;
				bool mIgnoreFinderInfo;
				CompareDirectoriesCompletionPtr mCompletion;
			};
			typedef std::shared_ptr<InternalCompletion> InternalCompletionPtr;
			
			//
			class HermitProxy : public Hermit {
			public:
				//
				HermitProxy(const HermitPtr& h_) : mH_(h_), mFilesMatch(false) {
				}
				
				//
				virtual bool ShouldAbort() override {
					return mH_->ShouldAbort();
				}
				
				//
				virtual void Notify(const char* name, const void* param) override {
					std::string nameStr(name);
					if (nameStr == kFilesMatchNotification) {
						mFilesMatch = true;
					}
					else if (nameStr == kFilesDifferNotification) {
						mFilesMatch = false;
					}
					NOTIFY(mH_, name, param);
				}
				
				//
				HermitPtr mH_;
				bool mFilesMatch;
			};
			typedef std::shared_ptr<HermitProxy> HermitProxyPtr;
			
			//
			class Aggregator {
			public:
				//
				Aggregator(const HermitPtr& h_, const InternalCompletionPtr& completion) :
				mH_(h_),
				mCompletion(completion),
				mStatus(CompareFilesStatus::kSuccess),
				mFilesMatch(true),
				mPendingTasks(0),
				mAllTasksAdded(false) {
				}
				
				//
				void TaskAdded() {
					mPendingTasks++;
				}

				//
				void AllTasksAdded() {
					mAllTasksAdded = true;
					CheckCompletion();
				}
				
				//
				void TaskComplete(const CompareFilesStatus& status, bool filesMatch) {
					if (status == CompareFilesStatus::kCancel) {
						if (mStatus == CompareFilesStatus::kSuccess) {
							mStatus = CompareFilesStatus::kCancel;
						}
					}
					else if (status != CompareFilesStatus::kSuccess) {
						mStatus = status;
					}
					
					mFilesMatch = mFilesMatch && filesMatch;
					
					mPendingTasks--;
					CheckCompletion();
				}
				
				//
				void CheckCompletion() {
					if (mAllTasksAdded && (mPendingTasks == 0)) {
						mCompletion->CompareFilesComplete(mStatus, mFilesMatch);
					}
				}
				
				//
				HermitPtr mH_;
				InternalCompletionPtr mCompletion;
				CompareFilesStatus mStatus;
				std::atomic_bool mFilesMatch;
				std::atomic_int mPendingTasks;
				std::atomic_bool mAllTasksAdded;
			};
			typedef std::shared_ptr<Aggregator> AggregatorPtr;
			
			//
			class CompareCompletion : public CompareFilesCompletion {
			public:
				//
				CompareCompletion(const HermitProxyPtr& h_,
								  const AggregatorPtr& aggregator,
								  const FilePathPtr& filePath1,
								  const FilePathPtr& filePath2) :
				mH_(h_),
				mAggregator(aggregator),
				mFilePath1(filePath1),
				mFilePath2(filePath2) {
					mAggregator->TaskAdded();
				}
				
				//
				virtual void Call(const CompareFilesStatus& status) override {
					if ((status != CompareFilesStatus::kSuccess) && (status != CompareFilesStatus::kCancel)) {
						NOTIFY_ERROR(mH_, "CompareFiles failed for:", mFilePath1, "and:", mFilePath2);
					}
					mAggregator->TaskComplete(status, mH_->mFilesMatch);
				}
				
				//
				HermitProxyPtr mH_;
				AggregatorPtr mAggregator;
				FilePathPtr mFilePath1;
				FilePathPtr mFilePath2;
			};
			
			//
			struct FileInfo {
				//
				FileInfo(FilePathPtr inPath, const std::string& inName) :
				mPath(inPath),
				mName(inName),
				mLowerName(inName) {
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
			
			//
			typedef std::shared_ptr<FileInfo> FileInfoPtr;
			
			//
			struct SortFileInfoPtrs : public std::binary_function<FileInfoPtr, FileInfoPtr, bool> {
				//
				bool operator ()(const FileInfoPtr& inLHS, const FileInfoPtr& inRHS) const {
					if (inLHS->mLowerName == inRHS->mLowerName) {
						return inLHS->mName < inRHS->mName;
					}
					return inLHS->mLowerName < inRHS->mLowerName;
				}
			};
			
			//
			typedef std::set<FileInfoPtr, SortFileInfoPtrs> FileSet;
			
			//
			class Directory : public ListDirectoryContentsItemCallback {
			public:
				//
				Directory(const PreprocessFileFunctionPtr& inPreprocessFunction) :
				mPreprocessFunction(inPreprocessFunction) {
				}
				
				//
				virtual bool OnItem(const HermitPtr& h_,
									const ListDirectoryContentsResult& result,
									const FilePathPtr& inParentPath,
									const std::string& inItemName) override {
					PreprocessFileInstruction instruction = PreprocessFileInstruction::kContinue;
					if (mPreprocessFunction != nullptr) {
						instruction = mPreprocessFunction->Preprocess(h_, inParentPath, inItemName);
					}
					
					FilePathPtr itemPath;
					AppendToFilePath(h_, inParentPath, inItemName, itemPath);
					if (itemPath == nullptr) {
						NOTIFY_ERROR(h_, "CompareDirectories: Directory::Function: AppendToFilePath failed.");
						return false;
					}
						
					if (instruction == PreprocessFileInstruction::kSkip) {
						FileNotificationParams params(kFileSkipped, itemPath);
						NOTIFY(h_, kFileSkippedNotification, &params);
					}
					else if (instruction != PreprocessFileInstruction::kContinue) {
						NOTIFY_ERROR(h_, "CompareDirectories: preprocess function failed for:", itemPath);
						return false;
					}
					else {
						FileInfoPtr p(new FileInfo(itemPath, inItemName));
						mFiles.insert(p);
					}
					return true;
				}
				
				//
				const FileSet& GetFiles() const {
					return mFiles;
				}
				
				//
				PreprocessFileFunctionPtr mPreprocessFunction;
				FileSet mFiles;
			};
			
			//
			static void CompareDirectoryFiles(const HermitPtr& h_,
											  const Directory& inDir1,
											  const Directory& inDir2,
											  const bool& inIgnoreDates,
											  const bool& inIgnoreFinderInfo,
											  const PreprocessFileFunctionPtr& inPreprocessFunction,
											  const InternalCompletionPtr& inCompletion) {
				const FileSet& dir1Files = inDir1.GetFiles();
				const FileSet& dir2Files = inDir2.GetFiles();
				
				FileSet::const_iterator end1 = dir1Files.end();
				FileSet::const_iterator end2 = dir2Files.end();
				
				FileSet::const_iterator it1 = dir1Files.begin();
				FileSet::const_iterator it2 = dir2Files.begin();
				
				auto aggregator = std::make_shared<Aggregator>(h_, inCompletion);
				bool match = true;
				while (true) {
					if (it1 == end1) {
						while (it2 != end2) {
							match = false;
							FileNotificationParams params(kItemInPath2Only, NULL, (*it2)->mPath);
							NOTIFY(h_, kFilesDifferNotification, &params);
							++it2;
						}
						break;
					}
					if (it2 == end2) {
						while (it1 != end1) {
							match = false;
							FileNotificationParams params(kItemInPath1Only, (*it1)->mPath, NULL);
							NOTIFY(h_, kFilesDifferNotification, &params);
							++it1;
						}
						break;
					}
					
					if ((*it1)->mName < (*it2)->mName) {
						match = false;
						
						FileNotificationParams params(kItemInPath1Only, (*it1)->mPath, NULL);
						NOTIFY(h_, kFilesDifferNotification, &params);
						
						++it1;
					}
					else if ((*it1)->mName > (*it2)->mName) {
						match = false;
						
						FileNotificationParams params(kItemInPath2Only, NULL, (*it2)->mPath);
						NOTIFY(h_, kFilesDifferNotification, &params);
						
						++it2;
					}
					else {
						auto proxy = std::make_shared<HermitProxy>(h_);
						auto completion = std::make_shared<CompareCompletion>(proxy, aggregator, (*it1)->mPath, (*it2)->mPath);
						CompareFiles(proxy,
									 (*it1)->mPath,
									 (*it2)->mPath,
									 inIgnoreDates,
									 inIgnoreFinderInfo,
									 inPreprocessFunction,
									 completion);
						++it1;
						++it2;
					}
				}
				aggregator->AllTasksAdded();
			}
			
			//
			void PerformWork(const HermitPtr& h_,
							 const FilePathPtr& inFilePath1,
							 const FilePathPtr& inFilePath2,
							 const bool& inIgnoreDates,
							 const bool& inIgnoreFinderInfo,
							 const PreprocessFileFunctionPtr& inPreprocessFunction,
							 const CompareDirectoriesCompletionPtr& inCompletion) {
				FileType fileType1 = FileType::kUnknown;
				if (!GetFileType(h_, inFilePath1, fileType1)) {
					NOTIFY_ERROR(h_, "CompareDirectories: GetFileType failed for:", inFilePath1);
					inCompletion->Call(CompareDirectoriesStatus::kError);
					return;
				}
				if (fileType1 != FileType::kDirectory) {
					NOTIFY_ERROR(h_, "CompareDirectories: path not a directory:", inFilePath1);
					inCompletion->Call(CompareDirectoriesStatus::kError);
					return;
				}
				FileType fileType2 = FileType::kUnknown;
				if (!GetFileType(h_, inFilePath2, fileType2)) {
					NOTIFY_ERROR(h_, "CompareDirectories: GetFileType failed for:", inFilePath2);
					inCompletion->Call(CompareDirectoriesStatus::kError);
					return;
				}
				if (fileType2 != FileType::kDirectory) {
					NOTIFY_ERROR(h_, "CompareDirectories: path not a directory:", inFilePath2);
					inCompletion->Call(CompareDirectoriesStatus::kError);
					return;
				}
				
				Directory dir1(inPreprocessFunction);
				auto status1 = ListDirectoryContents(h_, inFilePath1, false, dir1);
				if (status1 != ListDirectoryContentsResult::kSuccess) {
					NOTIFY_ERROR(h_, "CompareDirectories: ListDirectoryContents failed for:", inFilePath1);
					inCompletion->Call(CompareDirectoriesStatus::kError);
					return;
				}
				
				Directory dir2(inPreprocessFunction);
				auto status2 = ListDirectoryContents(h_, inFilePath2, false, dir2);
				if (status2 != ListDirectoryContentsResult::kSuccess) {
					NOTIFY_ERROR(h_, "CompareDirectories: ListDirectoryContents failed for:", inFilePath2);
					inCompletion->Call(CompareDirectoriesStatus::kError);
					return;
				}
				
				auto completion = std::make_shared<InternalCompletion>(h_,
																	   inFilePath1,
																	   inFilePath2,
																	   inIgnoreDates,
																	   inIgnoreFinderInfo,
																	   inCompletion);
				CompareDirectoryFiles(h_, dir1, dir2, inIgnoreDates, inIgnoreFinderInfo, inPreprocessFunction, completion);
			}

			//
			class Task : public AsyncTask {
			public:
				Task(const FilePathPtr& inFilePath1,
					 const FilePathPtr& inFilePath2,
					 const bool& inIgnoreDates,
					 const bool& inIgnoreFinderInfo,
					 const PreprocessFileFunctionPtr& inPreprocessFunction,
					 const CompareDirectoriesCompletionPtr& inCompletion) :
				mFilePath1(inFilePath1),
				mFilePath2(inFilePath2),
				mIgnoreDates(inIgnoreDates),
				mIgnoreFinderInfo(inIgnoreFinderInfo),
				mPreprocessFunction(inPreprocessFunction),
				mCompletion(inCompletion) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PerformWork(h_, mFilePath1, mFilePath2, mIgnoreDates, mIgnoreFinderInfo, mPreprocessFunction, mCompletion);
				}

				FilePathPtr mFilePath1;
				FilePathPtr mFilePath2;
				bool mIgnoreDates;
				bool mIgnoreFinderInfo;
				PreprocessFileFunctionPtr mPreprocessFunction;
				CompareDirectoriesCompletionPtr mCompletion;
			};
			
		} // namespace CompareDirectories_Impl
        using namespace CompareDirectories_Impl;
		
		void CompareDirectories(const HermitPtr& h_,
								const FilePathPtr& inFilePath1,
								const FilePathPtr& inFilePath2,
								const bool& inIgnoreDates,
								const bool& inIgnoreFinderInfo,
								const PreprocessFileFunctionPtr& inPreprocessFunction,
								const CompareDirectoriesCompletionPtr& inCompletion) {
			auto task = std::make_shared<Task>(inFilePath1,
											   inFilePath2,
											   inIgnoreDates,
											   inIgnoreFinderInfo,
											   inPreprocessFunction,
											   inCompletion);
			if (!QueueAsyncTask(h_, task, 100)) {
				NOTIFY_ERROR(h_, "CompareDirectories: QueueAsyncTask failed.");
				inCompletion->Call(CompareDirectoriesStatus::kError);
			}
		}
		
	} // namespace file
} // namespace hermit
