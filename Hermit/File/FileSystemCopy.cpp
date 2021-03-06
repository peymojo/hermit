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
#include "Hermit/File/AppendToFilePath.h"
#include "Hermit/File/CopySymbolicLink.h"
#include "Hermit/File/CopyXAttrs.h"
#include "Hermit/File/CreateDirectory.h"
#include "Hermit/File/CreateEmptyFile.h"
#include "Hermit/File/CreateSymbolicLink.h"
#include "Hermit/File/DeleteFile.h"
#include "Hermit/File/FileExists.h"
#include "Hermit/File/GetAliasTarget.h"
#include "Hermit/File/GetFileBSDFlags.h"
#include "Hermit/File/GetFileDataSize.h"
#include "Hermit/File/GetFileDates.h"
#include "Hermit/File/GetFilePathUTF8String.h"
#include "Hermit/File/GetFilePosixOwnership.h"
#include "Hermit/File/GetFilePosixPermissions.h"
#include "Hermit/File/GetFileType.h"
#include "Hermit/File/GetRelativeFilePath.h"
#include "Hermit/File/GetSymbolicLinkTarget.h"
#include "Hermit/File/ListDirectoryContentsWithType.h"
#include "Hermit/File/PathIsAlias.h"
#include "Hermit/File/PathIsPackage.h"
#include "Hermit/File/ReadFileData.h"
#include "Hermit/File/SetDirectoryIsPackage.h"
#include "Hermit/File/SetFileBSDFlags.h"
#include "Hermit/File/SetFileDates.h"
#include "Hermit/File/SetFilePosixOwnership.h"
#include "Hermit/File/SetFilePosixPermissions.h"
#include "Hermit/File/StreamOutFileData.h"
#include "Hermit/Foundation/AsyncTaskQueue.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Foundation/Notification.h"
#include "FileSystemCopy.h"

namespace hermit {
	namespace file {
		namespace FileSystemCopy_Impl {
			
			//
			void CopyAttributes(const HermitPtr& h_,
								const FilePathPtr& sourcePath,
								const FilePathPtr& destPath,
								const FileSystemCopyCompletionPtr& completion) {
				bool success = true;
				auto copyXAttrsResult = CopyXAttrs(h_, sourcePath, destPath);
				if (copyXAttrsResult != CopyXAttrsResult::kSuccess) {
					NOTIFY_ERROR(h_, "CopyXAttrs failed for source path:", sourcePath, "dest path:", destPath);
					success = false;
				}

				GetFileBSDFlagsCallbackClass bsdFlags;
				GetFileBSDFlags(h_, sourcePath, bsdFlags);
				if (!bsdFlags.mSuccess) {
					NOTIFY_ERROR(h_, "GetFileBSDFlags failed for source path:", sourcePath);
					success = false;
				}
				else {
					if (!SetFileBSDFlags(h_, destPath, bsdFlags.mFlags)) {
						NOTIFY_ERROR(h_, "SetFileBSDFlags failed for dest path:", destPath);
						success = false;
					}
				}
				
				uint32_t permissions = 0;
				if (!GetFilePosixPermissions(h_, sourcePath, permissions)) {
					NOTIFY_ERROR(h_, "GetFilePosixPermissions failed for source path:", sourcePath);
					success = false;
				}
				else {
					if (!SetFilePosixPermissions(h_, destPath, permissions)) {
						NOTIFY_ERROR(h_, "SetFilePosixPermissions failed for dest path:", destPath);
						success = false;
					}
				}
				
				std::string userOwner;
				std::string groupOwner;
				if (!GetFilePosixOwnership(h_, sourcePath, userOwner, groupOwner)) {
					NOTIFY_ERROR(h_, "GetFilePosixOwnership failed for source path:", sourcePath);
					success = false;
				}
				else {
					auto result = SetFilePosixOwnership(h_, destPath, userOwner, groupOwner);
					if (result != SetFilePosixOwnershipResult::kSuccess) {
						NOTIFY_ERROR(h_, "SetFilePosixOwnership failed for dest path:", destPath);
						success = false;
					}
				}
				
				GetFileDatesCallbackClassT<std::string> datesCallback;
				GetFileDates(h_, sourcePath, datesCallback);
				if (!datesCallback.mSuccess) {
					NOTIFY_ERROR(h_, "GetFileDates() failed for:", sourcePath);
					success = false;
				}
				else {
					std::string creationDate(datesCallback.mCreationDate);
					std::string modificationDate(datesCallback.mModificationDate);
					if (!SetFileDates(h_, destPath, creationDate, modificationDate)) {
						NOTIFY_ERROR(h_, "SetFileDates() failed for:", destPath);
						success = false;
					}
				}
				
				completion->Call(h_, success ? FileSystemCopyResult::kSuccess : FileSystemCopyResult::kError);
			}
			
			//
			class CopyDataCompletion : public DataCompletion {
			public:
				//
				CopyDataCompletion(const FilePathPtr& source,
								   const FilePathPtr& dest,
								   const FileSystemCopyCompletionPtr& completion) :
				mSource(source),
				mDest(dest),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const StreamDataResult& result) override {
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "CopyFileData failed for source path:", mSource, "dest path:", mDest);
						mCompletion->Call(h_, FileSystemCopyResult::kError);
						return;
					}
					
					CopyAttributes(h_, mSource, mDest, mCompletion);
				}
				
				//
				FilePathPtr mSource;
				FilePathPtr mDest;
				FileSystemCopyCompletionPtr mCompletion;
			};

			//
			class FileDataProvider : public DataProvider {
			public:
				//
				FileDataProvider(FilePathPtr sourceFile) : mSourceFile(sourceFile) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const DataReceiverPtr& dataReceiver,
								  const DataCompletionPtr& completion) override {
					ReadFileData(h_, mSourceFile, dataReceiver, completion);
				}
				
				//
				FilePathPtr mSourceFile;
			};
			
			//
			void CopyOneFile(const HermitPtr& h_,
							 const FilePathPtr& sourcePath,
							 const FilePathPtr& destPath,
							 const FileSystemCopyCompletionPtr& completion) {
				PathIsAliasCallbackClass isAliasCallback;
				PathIsAlias(h_, sourcePath, isAliasCallback);
				if (!isAliasCallback.mSuccess) {
					NOTIFY_ERROR(h_, "PathIsAlias failed for:", sourcePath);
					completion->Call(h_, FileSystemCopyResult::kError);
					return;
				}
				if (isAliasCallback.mIsAlias) {
					if (!CopySymbolicLink(h_, sourcePath, destPath)) {
						NOTIFY_ERROR(h_, "CopySymbolicLink failed for:", sourcePath);
						completion->Call(h_, FileSystemCopyResult::kError);
						return;
					}
					
					completion->Call(h_, FileSystemCopyResult::kSuccess);
					return;
				}
				
				uint64_t dataSize = 0;
				if (!GetFileDataSize(h_, sourcePath, dataSize)) {
					NOTIFY_ERROR(h_, "GetFileDataSize failed for:", sourcePath);
					completion->Call(h_, FileSystemCopyResult::kError);
					return;
				}
				if (dataSize == 0) {
					if (!CreateEmptyFile(h_, destPath)) {
						NOTIFY_ERROR(h_, "CreateEmptyFile failed for:", destPath);
						completion->Call(h_, FileSystemCopyResult::kError);
						return;
					}
					CopyAttributes(h_, sourcePath, destPath, completion);
					return;
				}
				
				auto dataProvider = std::make_shared<FileDataProvider>(sourcePath);
				auto dataCompletion = std::make_shared<CopyDataCompletion>(sourcePath, destPath, completion);
				StreamOutFileData(h_, destPath, dataProvider, dataCompletion);
			}
			
			//
			void CopyOneSymbolicLink(const HermitPtr& h_,
									 const FilePathPtr& sourcePath,
									 const FilePathPtr& destPath,
									 const FileSystemCopyCompletionPtr& completion) {
				if (!CopySymbolicLink(h_, sourcePath, destPath)) {
					NOTIFY_ERROR(h_,
								 "CopyOneSymbolicLink: CopySymbolicLink falied for:", sourcePath,
								 "dest path:", destPath,
								 "... trying manual symbolic link duplication ...");
					
					std::string linkPathUTF8;
					GetFilePathUTF8String(h_, sourcePath, linkPathUTF8);
					
					FilePathPtr linkTargetPath;
					bool linkTargetIsRelative = false;
					if (!GetSymbolicLinkTarget(h_, sourcePath, linkTargetPath, linkTargetIsRelative)) {
						NOTIFY_ERROR(h_, "GetSymbolicLinkTarget failed for path:", sourcePath);
						completion->Call(h_, FileSystemCopyResult::kError);
						return;
					}
					
					GetRelativeFilePathCallbackClassT<FilePathPtr> getRelativePathCallback;
					GetRelativeFilePath(h_, sourcePath, linkTargetPath, getRelativePathCallback);
					if (!getRelativePathCallback.mSuccess) {
						NOTIFY_ERROR(h_,
									 "GetRelativeFilePath failed for path:", sourcePath,
									 "target:", linkTargetPath);
						completion->Call(h_, FileSystemCopyResult::kError);
						return;
					}
					
					FilePathPtr targetPath = getRelativePathCallback.mFilePath;
					if (targetPath == nullptr) {
						targetPath = linkTargetPath;
						NOTIFY_ERROR(h_,
									 "Link path has no common root to target, using absolute target path:",
									 targetPath);
					}
					
					if (!CreateSymbolicLink(h_, destPath, targetPath)) {
						NOTIFY_ERROR(h_, "CreateSymbolicLink() failed for:", destPath);
						completion->Call(h_, FileSystemCopyResult::kError);
						return;
					}
					
					CopyAttributes(h_, sourcePath, destPath, completion);
					return;
				}
				completion->Call(h_, FileSystemCopyResult::kSuccess);
			}
			
			//
			struct FileInfo {
				//
				FileInfo(FilePathPtr path, const std::string& name) : mPath(path), mName(name) {
				}
				
				//
				~FileInfo() = default;
				
				//
				FilePathPtr mPath;
				std::string mName;
			};
			typedef std::shared_ptr<FileInfo> FileInfoPtr;
			
			//
			struct SortFileInfoPtrs : public std::binary_function<FileInfoPtr, FileInfoPtr, bool> {
				//
				bool operator ()(const FileInfoPtr& lhs, const FileInfoPtr& rhs) const {
					return lhs->mName < rhs->mName;
				}
			};
			
			//
			typedef std::set<FileInfoPtr, SortFileInfoPtrs> FileSet;
			
			//
			class Directory : public ListDirectoryContentsWithTypeItemCallback {
			public:
				//
				Directory() {
				}
				
				//
				virtual bool OnItem(const HermitPtr& h_,
									const ListDirectoryContentsResult& result,
									const FilePathPtr& parentPath,
									const std::string& itemName,
									const FileType& fileType) override {
					FilePathPtr itemPath;
					AppendToFilePath(h_, parentPath, itemName, itemPath);
					if (itemPath == nullptr) {
						NOTIFY_ERROR(h_,
									 "FileSystemCopy: Directory::Function(): AppendToFilePath failed, parent path:", parentPath,
									 "item name:", itemName);
					}
					else {
						auto fileInfo = std::make_shared<FileInfo>(itemPath, itemName);
						if ((fileType == FileType::kFile) || (fileType == FileType::kDevice)) {
							mFiles.insert(fileInfo);
						}
						else if (fileType == FileType::kDirectory) {
							mDirectories.insert(fileInfo);
						}
						else if (fileType == FileType::kSymbolicLink) {
							mSymbolicLinks.insert(fileInfo);
						}
						else {
							NOTIFY_ERROR(h_, "FileSystemCopy: Directory::Function(): Unexpected file type for:", itemPath);
						}
					}
					return true;
				}
				
				//
				FileSet mFiles;
				FileSet mDirectories;
				FileSet mSymbolicLinks;
			};
			typedef std::shared_ptr<Directory> DirectoryPtr;
			
			//
			class CopyDirectoryLinks;
			typedef std::shared_ptr<CopyDirectoryLinks> CopyDirectoryLinksPtr;
			
			//
			class CopyDirectoryLinks : public std::enable_shared_from_this<CopyDirectoryLinks> {
			public:
				//
				CopyDirectoryLinks(const DirectoryPtr& directory,
								   const FilePathPtr& sourcePath,
								   const FilePathPtr& destPath,
								   const FileSystemCopyIntermediateUpdateCallbackPtr& updateCallback,
								   const FileSystemCopyCompletionPtr& completion) :
				mDirectory(directory),
				mSourcePath(sourcePath),
				mDestPath(destPath),
				mUpdateCallback(updateCallback),
				mCompletion(completion),
				mIt(begin(directory->mSymbolicLinks)) {
				}
				
				//
				class Task : public AsyncTask {
				public:
					//
					Task(const CopyDirectoryLinksPtr& owner, const FileInfoPtr& fileInfo) :
					mOwner(owner),
					mFileInfo(fileInfo) {
					}
					
					//
					virtual void PerformTask(const HermitPtr& h_) override {
						mOwner->PerformTask(h_, mFileInfo);
					}
					
					//
					CopyDirectoryLinksPtr mOwner;
					FileInfoPtr mFileInfo;
				};
				
				//
				void ProcessNextItem(const HermitPtr& h_) {
					if (mIt == end(mDirectory->mSymbolicLinks)) {
						CopyAttributes(h_, mSourcePath, mDestPath, mCompletion);
						return;
					}
					auto fileInfo = *mIt++;
					auto task = std::make_shared<Task>(shared_from_this(), fileInfo);
					if (!QueueAsyncTask(h_, task, 10)) {
						NOTIFY_ERROR(h_, "CopyDirectoryLinks: QueueAsyncTask failed.");
						mCompletion->Call(h_, FileSystemCopyResult::kError);
					}
				}
				
				//
				class Completion : public FileSystemCopyCompletion {
				public:
					//
					Completion(const CopyDirectoryLinksPtr& owner, const FilePathPtr& sourcePath, const FilePathPtr& destPath) :
					mOwner(owner),
					mSourcePath(sourcePath),
					mDestPath(destPath) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const FileSystemCopyResult& result) override {
						mOwner->OnCompletion(h_, result, mSourcePath, mDestPath);
					}
					
					//
					CopyDirectoryLinksPtr mOwner;
					FilePathPtr mSourcePath;
					FilePathPtr mDestPath;
				};
				
				//
				void PerformTask(const HermitPtr& h_, const FileInfoPtr& fileInfo) {
					FilePathPtr destFilePath;
					AppendToFilePath(h_, mDestPath, fileInfo->mName, destFilePath);
					if (destFilePath == nullptr) {
						NOTIFY_ERROR(h_, "CopyDirectoryLinks: AppendToFilePath failed, path:", mDestPath, "item name:", fileInfo->mName);
						ProcessNextItem(h_);
						return;
					}
					
					auto completion = std::make_shared<Completion>(shared_from_this(), fileInfo->mPath, destFilePath);
					CopyOneSymbolicLink(h_, fileInfo->mPath, destFilePath, completion);
				}

				//
				void OnCompletion(const HermitPtr& h_,
								  const FileSystemCopyResult& result,
								  const FilePathPtr& sourcePath,
								  const FilePathPtr& destPath) {
					if (!mUpdateCallback->OnUpdate(h_, result, sourcePath, destPath)) {
						mCompletion->Call(h_, FileSystemCopyResult::kStoppedViaUpdateCallback);
						return;
					}
					ProcessNextItem(h_);
				}
				
				//
				DirectoryPtr mDirectory;
				FilePathPtr mSourcePath;
				FilePathPtr mDestPath;
				FileSystemCopyCompletionPtr mCompletion;
				FileSystemCopyIntermediateUpdateCallbackPtr mUpdateCallback;
				FileSet::const_iterator mIt;
			};
			
			//
			void CopyOneDirectory(const HermitPtr& h_,
								  const FilePathPtr& sourcePath,
								  const FilePathPtr& destPath,
								  const FileSystemCopyIntermediateUpdateCallbackPtr& updateCallback,
								  const FileSystemCopyCompletionPtr& completion);

			//
			class CopyDirectoryDirectories;
			typedef std::shared_ptr<CopyDirectoryDirectories> CopyDirectoryDirectoriesPtr;
			
			//
			class CopyDirectoryDirectories : public std::enable_shared_from_this<CopyDirectoryDirectories> {
			public:
				//
				CopyDirectoryDirectories(const DirectoryPtr& directory,
										 const FilePathPtr& sourcePath,
										 const FilePathPtr& destPath,
										 const FileSystemCopyIntermediateUpdateCallbackPtr& updateCallback,
										 const FileSystemCopyCompletionPtr& completion) :
				mDirectory(directory),
				mSourcePath(sourcePath),
				mDestPath(destPath),
				mUpdateCallback(updateCallback),
				mCompletion(completion),
				mIt(begin(directory->mDirectories)) {
				}

				//
				class Task : public AsyncTask {
				public:
					//
					Task(const CopyDirectoryDirectoriesPtr& owner, const FileInfoPtr& fileInfo) :
					mOwner(owner),
					mFileInfo(fileInfo) {
					}
					
					//
					virtual void PerformTask(const HermitPtr& h_) override {
						mOwner->PerformTask(h_, mFileInfo);
					}
					
					//
					CopyDirectoryDirectoriesPtr mOwner;
					FileInfoPtr mFileInfo;
				};

				//
				void ProcessNextItem(const HermitPtr& h_) {
					if (mIt == end(mDirectory->mDirectories)) {
						auto copyLinks = std::make_shared<CopyDirectoryLinks>(mDirectory,
																			  mSourcePath,
																			  mDestPath,
																			  mUpdateCallback,
																			  mCompletion);
						copyLinks->ProcessNextItem(h_);
						return;
					}
					auto fileInfo = *mIt++;
					auto task = std::make_shared<Task>(shared_from_this(), fileInfo);
					if (!QueueAsyncTask(h_, task, 10)) {
						NOTIFY_ERROR(h_, "CopyDirectoryDirectories: QueueAsyncTask failed.");
						mCompletion->Call(h_, FileSystemCopyResult::kError);
					}
				}
				
				//
				class Completion : public FileSystemCopyCompletion {
				public:
					//
					Completion(const CopyDirectoryDirectoriesPtr& owner, const FilePathPtr& sourcePath, const FilePathPtr& destPath) :
					mOwner(owner),
					mSourcePath(sourcePath),
					mDestPath(destPath) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const FileSystemCopyResult& result) override {
						mOwner->OnCompletion(h_, result, mSourcePath, mDestPath);
					}
					
					//
					CopyDirectoryDirectoriesPtr mOwner;
					FilePathPtr mSourcePath;
					FilePathPtr mDestPath;
				};
				
				//
				void PerformTask(const HermitPtr& h_, const FileInfoPtr& fileInfo) {
					FilePathPtr destFilePath;
					AppendToFilePath(h_, mDestPath, fileInfo->mName, destFilePath);
					if (destFilePath == nullptr) {
						NOTIFY_ERROR(h_, "CopyDirectoryDirectories: AppendToFilePath failed, path:", mDestPath, "item name:", fileInfo->mName);
						ProcessNextItem(h_);
						return;
					}
					
					auto completion = std::make_shared<Completion>(shared_from_this(), fileInfo->mPath, destFilePath);
					CopyOneDirectory(h_, fileInfo->mPath, destFilePath, mUpdateCallback, completion);
				}

				//
				void OnCompletion(const HermitPtr& h_,
								  const FileSystemCopyResult& result,
								  const FilePathPtr& sourcePath,
								  const FilePathPtr& destPath) {
					if (!mUpdateCallback->OnUpdate(h_, result, sourcePath, destPath)) {
						mCompletion->Call(h_, FileSystemCopyResult::kStoppedViaUpdateCallback);
						return;
					}
					ProcessNextItem(h_);
				}

				//
				DirectoryPtr mDirectory;
				FilePathPtr mSourcePath;
				FilePathPtr mDestPath;
				FileSystemCopyIntermediateUpdateCallbackPtr mUpdateCallback;
				FileSystemCopyCompletionPtr mCompletion;
				FileSet::const_iterator mIt;
			};

			//
			class CopyDirectoryFiles;
			typedef std::shared_ptr<CopyDirectoryFiles> CopyDirectoryFilesPtr;
			
			//
			class CopyDirectoryFiles : public std::enable_shared_from_this<CopyDirectoryFiles> {
			public:
				//
				CopyDirectoryFiles(const DirectoryPtr& directory,
								   const FilePathPtr& sourcePath,
								   const FilePathPtr& destPath,
								   const FileSystemCopyIntermediateUpdateCallbackPtr& updateCallback,
								   const FileSystemCopyCompletionPtr& completion) :
				mDirectory(directory),
				mSourcePath(sourcePath),
				mDestPath(destPath),
				mUpdateCallback(updateCallback),
				mCompletion(completion),
				mIt(begin(directory->mFiles)) {
				}

				//
				class Task : public AsyncTask {
				public:
					//
					Task(const CopyDirectoryFilesPtr& owner, const FileInfoPtr& fileInfo) :
					mOwner(owner),
					mFileInfo(fileInfo) {
					}
					
					//
					virtual void PerformTask(const HermitPtr& h_) override {
						mOwner->PerformTask(h_, mFileInfo);
					}
					
					//
					CopyDirectoryFilesPtr mOwner;
					FileInfoPtr mFileInfo;
				};

				//
				void ProcessNextItem(const HermitPtr& h_) {
					if (mIt == end(mDirectory->mFiles)) {
						auto copyDirectories = std::make_shared<CopyDirectoryDirectories>(mDirectory,
																						  mSourcePath,
																						  mDestPath,
																						  mUpdateCallback,
																						  mCompletion);
						copyDirectories->ProcessNextItem(h_);
						return;
					}
					auto fileInfo = *mIt++;
					auto task = std::make_shared<Task>(shared_from_this(), fileInfo);
					if (!QueueAsyncTask(h_, task, 10)) {
						NOTIFY_ERROR(h_, "CopyDirectoryFiles: QueueAsyncTask failed.");
						mCompletion->Call(h_, FileSystemCopyResult::kError);
					}
				}
				
				//
				class Completion : public FileSystemCopyCompletion {
				public:
					//
					Completion(const CopyDirectoryFilesPtr& owner, const FilePathPtr& sourcePath, const FilePathPtr& destPath) :
					mOwner(owner),
					mSourcePath(sourcePath),
					mDestPath(destPath) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const FileSystemCopyResult& result) override {
						mOwner->OnCompletion(h_, result, mSourcePath, mDestPath);
					}
					
					//
					CopyDirectoryFilesPtr mOwner;
					FilePathPtr mSourcePath;
					FilePathPtr mDestPath;
				};
				
				//
				void PerformTask(const HermitPtr& h_, const FileInfoPtr& fileInfo) {
					FilePathPtr destFilePath;
					AppendToFilePath(h_, mDestPath, fileInfo->mName, destFilePath);
					if (destFilePath == nullptr) {
						NOTIFY_ERROR(h_, "CopyDirectoryFiles: AppendToFilePath failed, path:", mDestPath, "item name:", fileInfo->mName);
						ProcessNextItem(h_);
						return;
					}
					
					auto completion = std::make_shared<Completion>(shared_from_this(), fileInfo->mPath, destFilePath);
					CopyOneFile(h_, fileInfo->mPath, destFilePath, completion);
				}
				
				//
				void OnCompletion(const HermitPtr& h_,
								  const FileSystemCopyResult& result,
								  const FilePathPtr& sourcePath,
								  const FilePathPtr& destPath) {
					if (!mUpdateCallback->OnUpdate(h_, result, sourcePath, destPath)) {
						mCompletion->Call(h_, FileSystemCopyResult::kStoppedViaUpdateCallback);
						return;
					}
					ProcessNextItem(h_);
				}

				//
				DirectoryPtr mDirectory;
				FilePathPtr mSourcePath;
				FilePathPtr mDestPath;
				FileSystemCopyCompletionPtr mCompletion;
				FileSystemCopyIntermediateUpdateCallbackPtr mUpdateCallback;
				FileSet::const_iterator mIt;
			};

			//
			void CopyOneDirectory(const HermitPtr& h_,
								  const FilePathPtr& sourcePath,
								  const FilePathPtr& destPath,
								  const FileSystemCopyIntermediateUpdateCallbackPtr& updateCallback,
								  const FileSystemCopyCompletionPtr& completion) {
				auto createResult = CreateDirectory(h_, destPath);
				if (createResult != CreateDirectoryResult::kSuccess) {
					NOTIFY_ERROR(h_, "CreateDirectory failed for:", destPath);
					completion->Call(h_, FileSystemCopyResult::kError);
					return;
				}
				
				auto directory = std::make_shared<Directory>();
				auto listResult = ListDirectoryContentsWithType(h_, sourcePath, false, *directory);
				if (listResult != ListDirectoryContentsResult::kSuccess) {
					NOTIFY_ERROR(h_, "ListDirectoryContentsWithType failed for:", sourcePath);
					completion->Call(h_, FileSystemCopyResult::kError);
					return;
				}
				
				auto copyFiles = std::make_shared<CopyDirectoryFiles>(directory,
																	  sourcePath,
																	  destPath,
																	  updateCallback,
																	  completion);
				copyFiles->ProcessNextItem(h_);
			}
						
		} // namespace FileSystemCopy_Impl
		using namespace FileSystemCopy_Impl;
		
		//
		void FileSystemCopy(const HermitPtr& h_,
							const FilePathPtr& sourcePath,
							const FilePathPtr& destPath,
							const FileSystemCopyIntermediateUpdateCallbackPtr& updateCallback,
							const FileSystemCopyCompletionPtr& completion) {
			bool sourceFileExists = false;
			if (!FileExists(h_, sourcePath, sourceFileExists)) {
				NOTIFY_ERROR(h_, "FileExists failed for source path:", sourcePath);
				completion->Call(h_, FileSystemCopyResult::kError);
				return;
			}
			if (!sourceFileExists) {
				NOTIFY_ERROR(h_, "Source item doesn't exist at path:", sourcePath);
				completion->Call(h_, FileSystemCopyResult::kSourceNotFound);
				return;
			}
			
			bool destFileExists = false;
			if (!FileExists(h_, destPath, destFileExists)) {
				NOTIFY_ERROR(h_, "FileExists failed for dest path:", destPath);
				completion->Call(h_, FileSystemCopyResult::kError);
				return;
			}
			if (destFileExists) {
				NOTIFY_ERROR(h_, "Item already exists at dest path:", destPath);
				completion->Call(h_, FileSystemCopyResult::kTargetAlreadyExists);
				return;
			}
			
			FileType fileType = FileType::kUnknown;
			if (!GetFileType(h_, sourcePath, fileType)) {
				NOTIFY_ERROR(h_, "GetFileType failed for path:", sourcePath);
				completion->Call(h_, FileSystemCopyResult::kError);
				return;
			}
			if (fileType == FileType::kDirectory) {
				CopyOneDirectory(h_, sourcePath, destPath, updateCallback, completion);
				return;
			}
			if (fileType == FileType::kSymbolicLink) {
				CopyOneSymbolicLink(h_, sourcePath, destPath, completion);
				return;
			}
			if ((fileType == FileType::kFile) || (fileType == FileType::kDevice)) {
				CopyOneFile(h_, sourcePath, destPath, completion);
				return;
			}
			NOTIFY_ERROR(h_, "GetFileType returned unexpected file type for path:", sourcePath);
			completion->Call(h_, FileSystemCopyResult::kError);
		}
		
	} // namespace file
} // namespace hermit
