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

#if 000

#include <set>
#include <string>
#include <vector>
#include "Hermit/File/AppendToFilePath.h"
#include "Hermit/File/CopyFinderInfo.h"
#include "Hermit/File/CopySymbolicLink.h"
#include "Hermit/File/CreateDirectory.h"
#include "Hermit/File/CreateEmptyFileFork.h"
#include "Hermit/File/CreateSymbolicLink.h"
#include "Hermit/File/DeleteFile.h"
#include "Hermit/File/FileExists.h"
#include "Hermit/File/GetAliasTarget.h"
#include "Hermit/File/GetFileDates.h"
#include "Hermit/File/GetFileForkSize.h"
#include "Hermit/File/GetFilePathUTF8String.h"
#include "Hermit/File/GetFilePosixOwnership.h"
#include "Hermit/File/GetFilePosixPermissions.h"
#include "Hermit/File/GetFileType.h"
#include "Hermit/File/GetRelativeFilePath.h"
#include "Hermit/File/GetSymbolicLinkTarget.h"
#include "Hermit/File/ListDirectoryContents.h"
#include "Hermit/File/PathIsAlias.h"
#include "Hermit/File/PathIsPackage.h"
#include "Hermit/File/SetDirectoryIsPackage.h"
#include "Hermit/File/SetFileDates.h"
#include "Hermit/File/SetFilePosixOwnership.h"
#include "Hermit/File/SetFilePosixPermissions.h"
#include "Hermit/File/StreamInFileFork.h"
#include "Hermit/File/StreamOutFileFork.h"
#include "Hermit/Foundation/Hermit.h"
#include "FileSystemCopy.h"

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			//
			typedef std::vector<std::string> StringVector;
			
			//
			//
			class StreamOutFunction
			:
			public StreamDataFunction
			{
			public:
				//
				//
				StreamOutFunction(
								  FilePathPtr inSourceFile,
								  const std::string& inForkName)
				:
				mSourceFile(inSourceFile),
				mForkName(inForkName),
				mSuccess(false)
				{
				}
				
				//
				//
				bool Function(const HermitPtr& h_,
							  const StreamDataHandlerFunctionRef& inDataHandler,
							  const StreamDataStatusCallbackRef& inCallback)
				{
					StreamInFileForkCallbackClass callback;
					StreamInFileFork(mSourceFile,
									 mForkName,
									 1024 * 1024,
									 inDataHandler,
									 h_,
									 callback);
					
					if (callback.mResult == kStreamInFileForkResult_Canceled)
					{
						return inCallback.Call(kStreamDataStatus_Canceled);
					}
					if (callback.mResult != kStreamInFileForkResult_Success)
					{
						return inCallback.Call(kStreamDataStatus_Error);
					}
					return inCallback.Call(kStreamDataStatus_Success);
				}
				
				//
				//
				FilePathPtr mSourceFile;
				std::string mForkName;
				bool mSuccess;
			};
			
			//
			bool CopyOneFileFork(const HermitPtr& h_,
								 const FilePathPtr& inSourcePath,
								 const FilePathPtr& inDestPath,
								 const std::string& inForkName,
								 const FileSystemCopyCallbackRef& inCallback) {
				StreamOutFunction streamOutFunction(inSourcePath, inForkName);
				StreamOutFileForkCallbackClass streamOutCallback;
				StreamOutFileFork(inDestPath,
								  inForkName,
								  streamOutFunction,
								  h_,
								  streamOutCallback);
				
				if (streamOutCallback.mStatus != kStreamOutFileForkStatus_Success) {
					NOTIFY_ERROR(h_,
								 "CopyOneFileFork() Copy failed for source path:",
								 inSourcePath,
								 ", dest path:",
								 inSourcePath,
								 ", fork name:",
								 inForkName);
					
					inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					return false;
				}
				return true;
			}
			
			//
			bool CopyOneFile(const HermitPtr& h_,
							 const FilePathPtr& inSourcePath,
							 const FilePathPtr& inDestPath,
							 const FileSystemCopyCallbackRef& inCallback) {
				
				PathIsAliasCallbackClass isAliasCallback;
				PathIsAlias(inSourcePath, h_, isAliasCallback);
				if (!isAliasCallback.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneFile(): PathIsAlias failed for:", inSourcePath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				else if (isAliasCallback.mIsAlias) {
					BasicCallbackClass callback;
					CopySymbolicLink(inSourcePath, inDestPath, h_, callback);
					if (!callback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneFile(): CopySymbolicLink failed for path:", inSourcePath);
						return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					}
					
					return inCallback.Call(kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
				}
				
				StringVector forkNames;
				forkNames.push_back("");
				forkNames.push_back("rsrc");
				
				bool success = true;
				auto end = forkNames.end();
				for (auto it = forkNames.begin(); it != end; ++it) {
					GetFileForkSizeCallbackClass sizeCallback;
					GetFileForkSize(inSourcePath, *it, h_, sizeCallback);
					if (!sizeCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneFile(): GetFileForkSize() failed for:", inSourcePath);
						inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						return false;
					}
					
					uint64_t forkSize = sizeCallback.mSize;
					if (forkSize > 0) {
						if (!CopyOneFileFork(inSourcePath, inDestPath, *it, h_, inCallback)) {
							success = false;
							break;
						}
					}
					else if (*it == "") {
						BasicCallbackClass createCallback;
						CreateEmptyFileFork(inDestPath, *it, h_, createCallback);
						if (!createCallback.mSuccess) {
							NOTIFY_ERROR(h_, "CopyOneFile(): CreateEmptyFileFork() failed for:", inSourcePath);
							success = false;
							break;
						}
					}
				}
				
				if (success) {
					GetFilePosixPermissionsCallbackClass getPermissionsCallback;
					GetFilePosixPermissions(inSourcePath, h_, getPermissionsCallback);
					if (!getPermissionsCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneFile(): GetFilePosixPermissions failed for source path:", inSourcePath);
						success = false;
					}
					else {
						BasicCallbackClass setPermissionsCallback;
						SetFilePosixPermissions(inDestPath,
												getPermissionsCallback.mPermissions,
												h_,
												setPermissionsCallback);
						
						if (!setPermissionsCallback.mSuccess) {
							NOTIFY_ERROR(h_, "CopyOneFile(): SetFilePosixPermissions failed for dest path:", inDestPath);
							success = false;
						}
					}
				}
				
				if (success) {
					GetFilePosixOwnershipCallbackClassT<std::string> ownership;
					GetFilePosixOwnership(inSourcePath, h_, ownership);
					if (!ownership.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneFile(): GetFilePosixOwnership failed for source path:", inSourcePath);
						return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					}
					
					SetFilePosixOwnershipCallbackClass setOwnershipCallback;
					SetFilePosixOwnership(inDestPath,
										  ownership.mUserOwner,
										  ownership.mGroupOwner,
										  h_,
										  setOwnershipCallback);
					if (setOwnershipCallback.mResult != SetFilePosixOwnershipResult::kSuccess) {
						NOTIFY_ERROR(h_, "CopyOneFile(): SetFilePosixOwnership failed for dest path:", inDestPath);
						return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					}
				}
				
				if (success) {
					BasicCallbackClass finderInfoCallback;
					CopyFinderInfo(inSourcePath, inDestPath, finderInfoCallback);
					if (!finderInfoCallback.mSuccess)
					{
						NOTIFY_ERROR(h_, "CopyOneFile(): CopyFinderInfo failed for source path:", inSourcePath);
						success = false;
					}
				}
				
				if (success) {
					GetFileDatesCallbackClassT<std::string> datesCallback;
					GetFileDates(inSourcePath, h_, datesCallback);
					if (!datesCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneFile(): GetFileDates() failed for:", inSourcePath);
						success = false;
					}
					else {
						std::string creationDate(datesCallback.mCreationDate);
						std::string modificationDate(datesCallback.mModificationDate);
						
						BasicCallbackClass setDatesCallback;
						SetFileDates(inDestPath, creationDate, modificationDate, h_, setDatesCallback);
						if (!setDatesCallback.mSuccess) {
							NOTIFY_ERROR(h_, "CopyOneFile(): SetFileDates() failed for:", inDestPath);
							success = false;
						}
					}
				}
				
				if (success) {
					inCallback.Call(kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
				}
				else {
					DeleteFileCallbackClass deleteCallback;
					DeleteFile(inDestPath, h_, deleteCallback);
					inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				return success;
			}
			
			//
			bool CopyOneSymbolicLink(const HermitPtr& h_,
									 const FilePathPtr& inSourcePath,
									 const FilePathPtr& inDestPath,
									 const FileSystemCopyCallbackRef& inCallback) {
				
				BasicCallbackClass callback;
				CopySymbolicLink(inSourcePath, inDestPath, h_, callback);
				if (!callback.mSuccess) {
					NOTIFY_ERROR(h_,
								 "CopyOneSymbolicLink: CopySymbolicLink falied for:",
								 inSourcePath,
								 "... trying manual symbolic link duplication ...");
					
					StringCallbackClass stringCallback;
					GetFilePathUTF8String(inSourcePath, h_, stringCallback);
					std::string linkPathUTF8(stringCallback.mValue);
					
					GetSymbolicLinkTargetCallbackClassT<FilePathPtr> linkTargetCallback;
					GetSymbolicLinkTarget(inSourcePath, h_, linkTargetCallback);
					if (!linkTargetCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneSymbolicLink(): GetSymbolicLinkTarget failed for path:", inSourcePath);
						return false;
					}
					
					GetRelativeFilePathCallbackClassT<FilePathPtr> getRelativePathCallback;
					GetRelativeFilePath(inSourcePath,
										linkTargetCallback.mFilePath,
										h_,
										getRelativePathCallback);
					
					if (!getRelativePathCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneSymbolicLink(): GetRelativeFilePath failed for path:", inSourcePath);
						return false;
					}
					
					FilePathPtr targetPath = getRelativePathCallback.mFilePath;
					if (targetPath == 0) {
						targetPath = linkTargetCallback.mFilePath;
						NOTIFY_ERROR(h_,
									 "CopyOneSymbolicLink(): Link path has no common root to target, using absolute target path:",
									 targetPath);
					}
					
					BasicCallbackClass linkCallback;
					CreateSymbolicLink(inDestPath, targetPath, h_, linkCallback);
					if (!linkCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneSymbolicLink(): CreateSymbolicLink() failed for:", inDestPath);
						inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						return false;
					}
					
					BasicCallbackClass finderInfoCallback;
					CopyFinderInfo(inSourcePath, inDestPath, finderInfoCallback);
					if (!finderInfoCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneSymbolicLink(): CopyFinderInfo failed for source path:", inSourcePath);
						inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						return false;
					}
					
					GetFilePosixOwnershipCallbackClassT<std::string> ownership;
					GetFilePosixOwnership(inSourcePath, h_, ownership);
					if (!ownership.mSuccess) {
						NOTIFY_ERROR(h_,
									 "CopyOneSymbolicLink(): GetFilePosixOwnership failed for source path:",
									 inSourcePath);
						return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					}
					
					SetFilePosixOwnershipCallbackClass setOwnershipCallback;
					SetFilePosixOwnership(inDestPath,
										  ownership.mUserOwner,
										  ownership.mGroupOwner,
										  h_,
										  setOwnershipCallback);
					if (setOwnershipCallback.mResult != SetFilePosixOwnershipResult::kSuccess) {
						NOTIFY_ERROR(h_, "CopyOneSymbolicLink(): SetFilePosixOwnership failed for dest path:", inDestPath);
						return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					}
					
					GetFileDatesCallbackClassT<std::string> datesCallback;
					GetFileDates(inSourcePath, h_, datesCallback);
					if (!datesCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneSymbolicLink(): GetFileDates() failed for:", inSourcePath);
						
						DeleteFileCallbackClass deleteCallback;
						DeleteFile(inDestPath, h_, deleteCallback);
						
						inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						return false;
					}
					
					std::string creationDate(datesCallback.mCreationDate);
					std::string modificationDate(datesCallback.mModificationDate);
					
					BasicCallbackClass setDatesCallback;
					SetFileDates(inDestPath, creationDate, modificationDate, h_, setDatesCallback);
					if (!setDatesCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneSymbolicLink(): SetFileDates() failed for:", inDestPath);
						
						DeleteFileCallbackClass deleteCallback;
						DeleteFile(inDestPath, h_, deleteCallback);
						
						inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						return false;
					}
					
					inCallback.Call(kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
					return true;
				}
				
				inCallback.Call(kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
				return true;
			}
			
			//
			//
			struct FileInfo {
				//
				//
				FileInfo(
						 FilePathPtr inPath,
						 const char* inName)
				:
				mPath(inPath),
				mName(inName)
				{
				}
				
				//
				//
				~FileInfo()
				{
				}
				
				//
				//
				FilePathPtr mPath;
				std::string mName;
			};
			
			//
			//
			typedef std::shared_ptr<FileInfo> FileInfoPtr;
			
			//
			//
			struct SortFileInfoPtrs
			:
			public std::binary_function<FileInfoPtr, FileInfoPtr, bool>
			{
				//
				//
				bool operator ()(
								 const FileInfoPtr& inLHS,
								 const FileInfoPtr& inRHS) const
				{
					return inLHS->mName < inRHS->mName;
				}
			};
			
			//
			typedef std::set<FileInfoPtr, SortFileInfoPtrs> FileSet;
			
			//
			class Directory : public ListDirectoryContentsCallback {
			public:
				//
				Directory(FilePathPtr inDestParentPath, const FileSystemCopyCallbackRef& inCallback)
				:
				mDestParentPath(inDestParentPath),
				mCallback(inCallback),
				mStatus(kListDirectoryContentsStatus_Unknown) {
				}
				
				//
				//
				bool Function(const HermitPtr& h_,
							  const ListDirectoryContentsStatus& inStatus,
							  const FilePathPtr& inParentPath,
							  const std::string& inItemName) {
					
					mStatus = inStatus;
					if (inStatus == kListDirectoryContentsStatus_Success) {
						FilePathCallbackClassT<FilePathPtr> itemPath;
						AppendToFilePath(inParentPath, inItemName, h_, itemPath);
						if (itemPath.mFilePath == nullptr) {
							NOTIFY_ERROR(h_, "FileSystemCopy: Directory::Function(): AppendToFilePath failed.");
							mStatus = kListDirectoryContentsStatus_Error;
							return false;
						}
						
						FileInfoPtr p(new FileInfo(itemPath.mFilePath, inItemName));
						
						GetFileTypeCallbackClass callback;
						GetFileType(itemPath.mFilePath, h_, callback);
						if (!callback.mSuccess) {
							NOTIFY_ERROR(h_,
										 "FileSystemCopy: Directory::Function(): GetFileType failed for:",
										 itemPath.mFilePath);
							
							mStatus = kListDirectoryContentsStatus_Error;
							return false;
						}
						if (callback.mFileType == FileType::kDirectory) {
							mDirectories.insert(p);
						}
						else if (callback.mFileType == FileType::kSymbolicLink) {
							mSymbolicLinks.insert(p);
						}
						else if (callback.mFileType == FileType::kFile) {
							FilePathCallbackClassT<FilePathPtr> destFilePath;
							AppendToFilePath(mDestParentPath, inItemName, h_, destFilePath);
							if (destFilePath.mFilePath == nullptr) {
								mCallback.Call(kFileSystemCopyStatus_Error, itemPath.mFilePath, destFilePath.mFilePath);
								mStatus = kListDirectoryContentsStatus_Error;
								return false;
							}
							
							if (!CopyOneFile(itemPath.mFilePath, destFilePath.mFilePath, h_, mCallback)) {
								mCallback.Call(kFileSystemCopyStatus_Error, itemPath.mFilePath, destFilePath.mFilePath);
								mStatus = kListDirectoryContentsStatus_Error;
								return false;
							}
						}
						else {
							NOTIFY_ERROR(h_,
										 "FileSystemCopy: Directory::Function(): GetFileType returned unexpected type for:",
										 itemPath.mFilePath);
							return false;
						}
					}
					return true;
				}
				
				//
				FilePathPtr mDestParentPath;
				const FileSystemCopyCallbackRef& mCallback;
				ListDirectoryContentsStatus mStatus;
				FileSet mDirectories;
				FileSet mSymbolicLinks;
			};
			
			//
			//
			bool CopyOneDirectory(const HermitPtr& h_,
								  const FilePathPtr& inSourcePath,
								  const FilePathPtr& inDestPath,
								  const FileSystemCopyCallbackRef& inCallback) {
				
				CreateDirectoryCallbackClass createCallback;
				CreateDirectory(inDestPath, h_, createCallback);
				if (createCallback.mResult != kCreateDirectoryResult_Success) {
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				Directory dir(inDestPath, inCallback);
				ListDirectoryContents(inSourcePath, false, h_, dir);
				
				if ((dir.mStatus != kListDirectoryContentsStatus_Success) &&
					(dir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty)) {
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				{
					const FileSet& dirDirs = dir.mDirectories;
					FileSet::const_iterator end = dirDirs.end();
					for (FileSet::const_iterator it = dirDirs.begin(); it != end; ++it) {
						FilePathCallbackClassT<FilePathPtr> destFilePath;
						AppendToFilePath(inDestPath, (*it)->mName, h_, destFilePath);
						if (destFilePath.mFilePath == nullptr) {
							return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						}
						
						if (!CopyOneDirectory((*it)->mPath, destFilePath.mFilePath, h_, inCallback)) {
							return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						}
					}
				}
				
				{
					const FileSet& dirLinks = dir.mSymbolicLinks;
					FileSet::const_iterator end = dirLinks.end();
					for (FileSet::const_iterator it = dirLinks.begin(); it != end; ++it) {
						FilePathCallbackClassT<FilePathPtr> destFilePath;
						AppendToFilePath(inDestPath, (*it)->mName, h_, destFilePath);
						if (destFilePath.mFilePath == nullptr) {
							return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						}
						
						if (!CopyOneSymbolicLink((*it)->mPath, destFilePath.mFilePath, h_, inCallback)) {
							return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						}
					}
				}
				
				GetFilePosixOwnershipCallbackClassT<std::string> ownership;
				GetFilePosixOwnership(inSourcePath, h_, ownership);
				if (!ownership.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): GetFilePosixOwnership failed for source path:", inSourcePath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				SetFilePosixOwnershipCallbackClass setOwnershipCallback;
				SetFilePosixOwnership(inDestPath,
									  ownership.mUserOwner,
									  ownership.mGroupOwner,
									  h_,
									  setOwnershipCallback);
				if (setOwnershipCallback.mResult != SetFilePosixOwnershipResult::kSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): SetFilePosixOwnership failed for dest path:", inDestPath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				GetFilePosixPermissionsCallbackClass getPermissionsCallback;
				GetFilePosixPermissions(inSourcePath, h_, getPermissionsCallback);
				if (!getPermissionsCallback.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): GetFilePosixPermissions failed for source path:", inSourcePath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				BasicCallbackClass setPermissionsCallback;
				SetFilePosixPermissions(inDestPath, getPermissionsCallback.mPermissions, h_, setPermissionsCallback);
				if (!setPermissionsCallback.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): SetFilePosixPermissions() failed for dest path:", inDestPath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				BasicCallbackClass finderInfoCallback;
				CopyFinderInfo(inSourcePath, inDestPath, finderInfoCallback);
				if (!finderInfoCallback.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): CopyFinderInfo failed for source path:", inSourcePath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				PathIsPackageCallbackClass pathIsPackage;
				PathIsPackage(inSourcePath, h_, pathIsPackage);
				if (!pathIsPackage.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): PathIsPackage failed for:", inSourcePath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				if (pathIsPackage.mIsPackage) {
					PathIsPackageCallbackClass path2IsPackage;
					PathIsPackage(inDestPath, h_, path2IsPackage);
					if (!path2IsPackage.mSuccess) {
						NOTIFY_ERROR(h_, "CopyOneDirectory(): PathIsPackage failed for:", inDestPath);
						return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					}
					
					if (!path2IsPackage.mIsPackage) {
						BasicCallbackClass setPackageCallback;
						SetDirectoryIsPackage(inDestPath, true, h_, setPackageCallback);
						if (!setPackageCallback.mSuccess) {
							NOTIFY_ERROR(h_, "CopyOneDirectory(): SetDirectoryIsPackage failed for:", inDestPath);
							return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
						}
					}
				}
				
				GetFileDatesCallbackClassT<std::string> datesCallback;
				GetFileDates(inSourcePath, h_, datesCallback);
				if (!datesCallback.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): GetFileDates() failed for:", inSourcePath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				std::string creationDate(datesCallback.mCreationDate);
				std::string modificationDate(datesCallback.mModificationDate);
				
				BasicCallbackClass setDatesCallback;
				SetFileDates(inDestPath, creationDate, modificationDate, h_, setDatesCallback);
				if (!setDatesCallback.mSuccess) {
					NOTIFY_ERROR(h_, "CopyOneDirectory(): SetFileDates() failed for:", inDestPath);
					return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				}
				
				return inCallback.Call(kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
			}
			
			//	//
			//	//
			//	class AliasesDirectory
			//	{
			//	public:
			//		//
			//		//
			//		AliasesDirectory(
			//			FilePathPtr inDestParentPath,
			//			FileSystemCopyCallback inCallback)
			//			:
			//			mDestParentPath(inDestParentPath),
			//			mCallback(inCallback),
			//			mStatus(kListDirectoryContentsStatus_Unknown)
			//		{
			//		}
			//
			//		//
			//		//
			//		bool Function(
			//			ListDirectoryContentsStatus inStatus,
			//			FilePathPtr inParentPath,
			//			const char* inItemName)
			//		{
			//			mStatus = inStatus;
			//			if (inStatus == kListDirectoryContentsStatus_Success)
			//			{
			//				FilePathPtr itemPath = 0;
			//				AppendToFilePath(inParentPath, inItemName, itemPath);
			//				if (itemPath == 0)
			//				{
			//					Log(
			//						inHub,
			//						"FileSystemCopy: AliasesDirectory::Function(): AppendToFilePath failed.");
			//
			//					mStatus = kListDirectoryContentsStatus_Error;
			//					return false;
			//				}
			//
			//				GetTypeCallback callback;
			//				GetFileType(
			//					inHub,
			//					itemPath,
			//					GetFileTypeCallbackT<GetTypeCallback>(callback));
			//
			//				if (!callback.mSuccess)
			//				{
			//					LogFilePath(
			//						inHub,
			//						"FileSystemCopy: AliasesDirectory::Function(): GetFileType failed for: ",
			//						itemPath);
			//
			//					mStatus = kListDirectoryContentsStatus_Error;
			//					return false;
			//				}
			//				if (callback.mFileType == kFileType_Directory)
			//				{
			//					FilePathPtr destFilePath = NULL;
			//					AppendToFilePath(mDestParentPath, inItemName, destFilePath);
			//					if (destFilePath == NULL)
			//					{
			//						mCallback.Call(kFileSystemCopyStatus_Error, itemPath, destFilePath);
			//						mStatus = kListDirectoryContentsStatus_Error;
			//						return false;
			//					}
			//					FilePathReleaser destFilePathReleaser(destFilePath);
			//
			//					AliasesDirectory dir(destFilePath, mCallback);
			//					ListDirectoryContents(
			//						inHub,
			//						itemPath,
			//						false,
			//						ListDirectoryContentsCallbackT<AliasesDirectory>(dir));
			//
			//					if ((dir.mStatus != kListDirectoryContentsStatus_Success) &&
			//						(dir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
			//					{
			//						return mCallback.Call(kFileSystemCopyStatus_Error, itemPath, destFilePath);
			//					}
			//				}
			//				else if (callback.mFileType == kFileType_File)
			//				{
			//					FilePathPtr destFilePath = NULL;
			//					AppendToFilePath(mDestParentPath, inItemName, destFilePath);
			//					if (destFilePath == NULL)
			//					{
			//						mCallback.Call(kFileSystemCopyStatus_Error, itemPath, destFilePath);
			//						mStatus = kListDirectoryContentsStatus_Error;
			//						return false;
			//					}
			//					FilePathReleaser destFilePathReleaser(destFilePath);
			//
			//					IsAliasCallback isAliasCallback;
			//					PathIsAlias(
			//						inHub,
			//						itemPath,
			//						PathIsAliasCallbackT<IsAliasCallback>(isAliasCallback));
			//
			//					if (!isAliasCallback.mSuccess)
			//					{
			//						LogFilePath(
			//							inHub,
			//							"FileSystemCopy: AliasesDirectory::Function(): PathIsAlias failed for: ",
			//							itemPath);
			//
			//						if (!mCallback.Call(kFileSystemCopyStatus_Error, itemPath, destFilePath))
			//						{
			//							return false;
			//						}
			//					}
			//					else if (isAliasCallback.mIsAlias)
			//					{
			//						BasicCallback callback;
			//						CopySymbolicLink(
			//							inHub,
			//							itemPath,
			//							destFilePath,
			//							CopySymbolicLinkCallbackT<BasicCallback>(callback));
			//
			//						if (!callback.mSuccess)
			//						{
			//							LogFilePath(
			//								inHub,
			//								"FileSystemCopy: AliasesDirectory::Function(): CopySymbolicLink failed for path: ",
			//								itemPath);
			//
			//							if (!mCallback.Call(kFileSystemCopyStatus_Error, itemPath, destFilePath))
			//							{
			//								return false;
			//							}
			//						}
			//						else
			//						{
			//							if (!mCallback.Call(kFileSystemCopyStatus_Success, itemPath, destFilePath))
			//							{
			//								return false;
			//							}
			//						}
			//					}
			//				}
			//			}
			//			return true;
			//		}
			//
			//		//
			//		//
			//		FilePathPtr mDestParentPath;
			//		FileSystemCopyCallback mCallback;
			//		ListDirectoryContentsStatus mStatus;
			//		FileSet mDirectories;
			//		FileSet mSymbolicLinks;
			//	};
			//
			//	//
			//	//
			//	bool CopyAliases(
			//		const FilePathPtr& inSourcePath,
			//		const FilePathPtr& inDestPath,
			//		const FileSystemCopyCallback& inCallback)
			//	{
			//		AliasesDirectory dir(inDestPath, inCallback);
			//		ListDirectoryContents(
			//			inHub,
			//			inSourcePath,
			//			false,
			//			ListDirectoryContentsCallbackT<AliasesDirectory>(dir));
			//
			//		if ((dir.mStatus != kListDirectoryContentsStatus_Success) &&
			//			(dir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
			//		{
			//			return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
			//		}
			//
			//		return true;
			//	}
			
		} // private namespace
		
		//
		void FileSystemCopy(const HermitPtr& h_,
							const FilePathPtr& inSourcePath,
							const FilePathPtr& inDestPath,
							const FileSystemCopyCallbackRef& inCallback) {
			
			FileExistsCallbackClass sourceFileExists;
			FileExists(inSourcePath, h_, sourceFileExists);
			if (!sourceFileExists.mSuccess) {
				NOTIFY_ERROR(h_, "FileSystemCopy(): FileExists() failed for source path:", inSourcePath);
				inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				return;
			}
			if (!sourceFileExists.mExists) {
				NOTIFY_ERROR(h_, "FileSystemCopy(): Source item doesn't exist at path:", inSourcePath);
				inCallback.Call(kFileSystemCopyStatus_SourceNotFound, inSourcePath, inDestPath);
				return;
			}
			
			FileExistsCallbackClass destFileExists;
			FileExists(inDestPath, h_, destFileExists);
			if (!destFileExists.mSuccess) {
				NOTIFY_ERROR(h_, "FileSystemCopy(): FileExists() failed for dest path:", inDestPath);
				inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				return;
			}
			if (destFileExists.mExists) {
				NOTIFY_ERROR(h_, "FileSystemCopy(): Item already exists at dest path:", inDestPath);
				inCallback.Call(kFileSystemCopyStatus_TargetAlreadyExists, inSourcePath, inDestPath);
				return;
			}
			
			GetFileTypeCallbackClass callback;
			GetFileType(inSourcePath, h_, callback);
			if (!callback.mSuccess) {
				NOTIFY_ERROR(h_, "FileSystemCopy(): GetFileType() failed for path:", inSourcePath);
				inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
			}
			else if (callback.mFileType == FileType::kDirectory) {
				CopyOneDirectory(inSourcePath, inDestPath, h_, inCallback);
			}
			else if (callback.mFileType == FileType::kSymbolicLink) {
				CopyOneSymbolicLink(inSourcePath, inDestPath, h_, inCallback);
			}
			else if (callback.mFileType == FileType::kFile) {
				CopyOneFile(inSourcePath, inDestPath, h_, inCallback);
			}
			else
			{
				NOTIFY_ERROR(h_, "FileSystemCopy(): GetFileType() returned unexpected file type for path:", inSourcePath);
				inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
			}
		}
		
	} // namespace file
} // namespace hermit

#endif


