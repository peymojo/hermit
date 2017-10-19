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
#include "Hermit/String/GetRelativePath.h"
#include "AppendToFilePath.h"
#include "CompareFiles.h"
#include "CompareFinderInfo.h"
#include "CopyFinderInfo.h"
#include "CopySymbolicLink.h"
#include "DeleteFile.h"
#include "FileSystemCopy.h"
#include "GetFileDates.h"
#include "GetFileForkSize.h"
#include "GetFilePathUTF8String.h"
#include "GetFilePosixOwnership.h"
#include "GetFilePosixPermissions.h"
#include "GetFileType.h"
#include "GetSymbolicLinkTarget.h"
#include "ListDirectoryContents.h"
#include "PathIsPackage.h"
#include "SetDirectoryIsPackage.h"
#include "SetFileDates.h"
#include "SetFilePosixOwnership.h"
#include "SetFilePosixPermissions.h"
#include "SyncDirectories.h"

#if 000

namespace hermit {
namespace file {

namespace {

	//
	bool CompareFiles(const HermitPtr& h_,
					  const FilePathPtr& inSourcePath,
					  const FilePathPtr& inDestPath,
					  bool& outQuickMatch)
	{
		GetFileDatesCallbackClassT<std::string> sourceDates;
		GetFileDates(inSourcePath, h_, sourceDates);
		if (!sourceDates.mSuccess)
		{
			NOTIFY_ERROR(h_,
						 "CompareFiles: GetFileDates failed for source: ",
						 inSourcePath);

			return false;
		}

		GetFileDatesCallbackClassT<std::string> destDates;
		GetFileDates(inDestPath, h_, destDates);
		if (!destDates.mSuccess)
		{
			NOTIFY_ERROR(h_,
						 "CompareFiles: GetFileDates failed for dest: ",
						 inDestPath);

			return false;
		}

		if ((sourceDates.mCreationDate != destDates.mCreationDate) ||
			(sourceDates.mModificationDate != destDates.mModificationDate))
		{
			outQuickMatch = false;
			return true;
		}
		
		GetFileForkSizeCallbackClass sourceSize;
		GetFileForkSize(inSourcePath, "", h_, sourceSize);
		if (!sourceSize.mSuccess)
		{
			NOTIFY_ERROR(h_,
						 "CompareFiles: GetFileForkSize failed for source: ",
						 inSourcePath);

			return false;
		}
		
		GetFileForkSizeCallbackClass destSize;
		GetFileForkSize(inDestPath, "", h_, destSize);
		if (!destSize.mSuccess)
		{
			NOTIFY_ERROR(h_,
						 "CompareFiles: GetFileForkSize failed for dest: ",
						 inDestPath);

			return false;
		}
		
		outQuickMatch = (sourceSize.mSize == destSize.mSize);
		return true;
	}
		
	//
	//
	bool SyncDates(const HermitPtr& h_,
				   const FilePathPtr& inSourcePath,
				   const FilePathPtr& inDestPath,
				   const bool& inPreviewOnly,
				   const SyncDirectoriesActionCallbackRef& inActionCallback)
	{
		GetFileDatesCallbackClassT<std::string> sourceDates;
		GetFileDates(inSourcePath, h_, sourceDates);
		if (!sourceDates.mSuccess)
		{
			NOTIFY_ERROR(h_,
				"SyncDates: GetFileDates failed for source: ",
				inSourcePath);

			return false;
		}

		GetFileDatesCallbackClassT<std::string> destDates;
		GetFileDates(inDestPath, h_, destDates);
		if (!destDates.mSuccess)
		{
			NOTIFY_ERROR(h_,
				"SyncDates: GetFileDates failed for dest: ",
				inDestPath);

			return false;
		}

		if ((sourceDates.mCreationDate != destDates.mCreationDate) ||
			(sourceDates.mModificationDate != destDates.mModificationDate))
		{
			if (!inPreviewOnly)
			{
				BasicCallbackClass callback;
				SetFileDates(inDestPath,
							 sourceDates.mCreationDate,
							 sourceDates.mModificationDate,
							 h_,
							 callback);
				
				if (!callback.mSuccess)
				{
					NOTIFY_ERROR(h_,
								 "SyncDates: SetFileDates failed for dest: ",
								 inDestPath);

					return false;
				}
			}

			inActionCallback.Call(kSyncDirectoriesActionStatus_UpdatedDatesForItem, inDestPath);
		}

		return true;
	}

	//
	//
	bool SyncPosixOwnership(const HermitPtr& h_,
							const FilePathPtr& inSourcePath,
							const FilePathPtr& inDestPath,
							const bool& inPreviewOnly,
							const SyncDirectoriesActionCallbackRef& inActionCallback)
	{
		GetFilePosixOwnershipCallbackClassT<std::string> sourceOwnership;
		GetFilePosixOwnership(inSourcePath, h_, sourceOwnership);
		if (!sourceOwnership.mSuccess)
		{
			NOTIFY_ERROR(h_,
						 "SyncPosixOwnership: GetFilePosixOwnership failed for source path: ",
						 inSourcePath);
			
			return false;
		}
		
		GetFilePosixOwnershipCallbackClassT<std::string> destOwnership;
		GetFilePosixOwnership(inDestPath, h_, destOwnership);
		if (!destOwnership.mSuccess)
		{
			NOTIFY_ERROR(h_,
						 "SyncPosixOwnership: GetFilePosixOwnership failed for dest path: ",
						 inDestPath);
			
			return false;
		}
		
		if ((sourceOwnership.mUserOwner == destOwnership.mUserOwner) &&
			(sourceOwnership.mGroupOwner == destOwnership.mGroupOwner))
		{
			return true;
		}
		
		if (!inPreviewOnly)
		{
			SetFilePosixOwnershipCallbackClass callback;
			SetFilePosixOwnership(inDestPath,
								  sourceOwnership.mUserOwner,
								  sourceOwnership.mGroupOwner,
								  h_,
								  callback);
			
			if (callback.mResult != SetFilePosixOwnershipResult::kSuccess)
			{
				NOTIFY_ERROR(h_,
							 "SyncPosixOwnership: SetFilePosixOwnership failed for source path:",
							 inSourcePath,
							 "dest path:",
							 inDestPath);

				return false;
			}
		}
		inActionCallback.Call(kSyncDirectoriesActionStatus_UpdatedPosixOwnershipForItem, inDestPath);
		return true;
	}

	//
	//
	bool SyncPosixPermissions(const HermitPtr& h_,
							  const FilePathPtr& inSourcePath,
							  const FilePathPtr& inDestPath,
							  const bool& inPreviewOnly,
							  const SyncDirectoriesActionCallbackRef& inActionCallback)
	{
		GetFilePosixPermissionsCallbackClass sourcePermissions;
		GetFilePosixPermissions(inSourcePath, h_, sourcePermissions);
		if (!sourcePermissions.mSuccess) {
			NOTIFY_ERROR(h_,
						 "SyncPosixPermissions: GetFilePosixPermissions failed for source path: ",
						 inSourcePath);
			
			return false;
		}
		
		GetFilePosixPermissionsCallbackClass destPermissions;
		GetFilePosixPermissions(inDestPath, h_, destPermissions);
		if (!destPermissions.mSuccess)
		{
			NOTIFY_ERROR(h_,
						 "SyncPosixPermissions: GetFilePosixPermissions failed for dest path: ",
						 inDestPath);
			
			return false;
		}
		
		if (sourcePermissions.mPermissions == destPermissions.mPermissions)
		{
			return true;
		}
		
		if (!inPreviewOnly)
		{
			BasicCallbackClass callback;
			SetFilePosixPermissions(inDestPath,
									sourcePermissions.mPermissions,
									h_,
									callback);
			
			if (!callback.mSuccess)
			{
				NOTIFY_ERROR(h_,
							 "SyncPosixPermissions: SetFilePosixPermissions failed for source path:",
							 inSourcePath,
							 "dest path:",
							 inDestPath);

				return false;
			}
		}
		inActionCallback.Call(kSyncDirectoriesActionStatus_UpdatedPosixPermissionsForItem, inDestPath);
		return true;
	}

	//
	//
	bool SyncFinderInfo(const HermitPtr& h_,
						const FilePathPtr& inSourcePath,
						const FilePathPtr& inDestPath,
						const bool& inPreviewOnly,
						const SyncDirectoriesActionCallbackRef& inActionCallback)
	{
		CompareFinderInfoCallbackClass compareFinderInfoCallback;
		CompareFinderInfo(inSourcePath,
						  inDestPath,
						  compareFinderInfoCallback);
		
		if (compareFinderInfoCallback.mStatus == kCompareFinderInfoStatus_Match)
		{
			return true;
		}
		
		if ((compareFinderInfoCallback.mStatus != kCompareFinderInfoStatus_FinderInfosDiffer) &&
			(compareFinderInfoCallback.mStatus != kCompareFinderInfoStatus_FolderPackageStatesDiffer))
		{
			NOTIFY_ERROR(h_,
						 "SyncFinderInfo: CompareFinderInfo failed for source path: ",
						 inSourcePath,
						 "-- dest path: ",
						 inDestPath);

			return false;
		}
		
		if (compareFinderInfoCallback.mStatus == kCompareFinderInfoStatus_FolderPackageStatesDiffer)
		{
			//	This should've been handled elsewhere.
			NOTIFY_ERROR(h_,
						 "SyncFinderInfo: Unexpected state, kCompareFinderInfoStatus_FolderPackageStatesDiffer for source path:",
						 inSourcePath,
						 "dest path:",
						 inDestPath);

			return false;
		}

		if (compareFinderInfoCallback.mStatus != kCompareFinderInfoStatus_FinderInfosDiffer)
		{
			NOTIFY_ERROR(h_,
						 "SyncFinderInfo: Unexpected state, != kCompareFinderInfoStatus_FinderInfosDiffer for source path:",
						 inSourcePath,
						 "dest path:",
						 inDestPath);

			return false;
		}

		if (!inPreviewOnly)
		{
			BasicCallbackClass callback;
			CopyFinderInfo(inSourcePath, inDestPath, callback);
			if (!callback.mSuccess)
			{
				NOTIFY_ERROR(h_,
							 "SyncFinderInfo: CopyFinderInfo failed for source path:",
							 inSourcePath,
							 "dest path:",
							 inDestPath);

				return false;
			}
		}
		inActionCallback.Call(kSyncDirectoriesActionStatus_UpdatedFinderInfoForItem, inDestPath);
		return true;
	}

	//
	//
	class CopyFileCallback
		:
		public FileSystemCopyCallback
	{
	public:
		//
		//
		CopyFileCallback()
			:
			mStatus(kFileSystemCopyStatus_Unknown)
		{
		}
		
		//
		//
		virtual bool Function(
			const FileSystemCopyStatus& inStatus,
			const FilePathPtr& inSourcePath,
			const FilePathPtr& inDestPath)
		{
			mStatus = inStatus;
			return true;
		}
		
		//
		//
		FileSystemCopyStatus mStatus;
	};
	
	//
	//
	bool CopyFileWithOverwrite(const HermitPtr& h_,
							   const FilePathPtr& inSourcePath,
							   const FilePathPtr& inDestParentPath,
							   const std::string& inFilename,
							   const bool& inPreviewOnly,
							   const SyncDirectoriesActionCallbackRef& inActionCallback)
	{
		FilePathCallbackClassT<FilePathPtr> destPath;
		AppendToFilePath(inDestParentPath, inFilename, h_, destPath);
		if (destPath.mFilePath == nullptr)
		{
			NOTIFY_ERROR(h_,
						 "CopyFileWithOverwrite: AppendToFilePath failed for destParentPath:",
						 inDestParentPath,
						 "item name:",
						 inFilename);

			return false;
		}

		if (!inPreviewOnly)
		{
			DeleteFileCallbackClass deleteCallback;
			DeleteFile(destPath.mFilePath, h_, deleteCallback);
			if (!deleteCallback.Success())
			{
				NOTIFY_ERROR(h_,
							 "CopyFileWithOverwrite: DeleteFile failed for dest path: ",
							 destPath.mFilePath);

				return false;
			}
		
			//	??? If this is a directory, should exclusions be taken into account?
			CopyFileCallback copyCallback;
			FileSystemCopy(inSourcePath, destPath.mFilePath, copyCallback);
			if (copyCallback.mStatus != kFileSystemCopyStatus_Success)
			{
				NOTIFY_ERROR(h_,
							 "CopyFileWithOverwrite: FileSystemCopy failed for source path:",
							 inSourcePath,
							 "dest path:",
							 destPath.mFilePath);

				return false;
			}
		}
		inActionCallback.Call(kSyncDirectoriesActionStatus_CopiedItem, destPath.mFilePath);
		return true;
	}

	//
	//
	bool CopyLinkWithOverwrite(const HermitPtr& h_,
							   const FilePathPtr& inSourcePath,
							   const FilePathPtr& inDestPath,
							   const bool& inPreviewOnly,
							   const SyncDirectoriesActionCallbackRef& inActionCallback)
	{
		if (!inPreviewOnly)
		{
			DeleteFileCallbackClass deleteCallback;
			DeleteFile(inDestPath, h_, deleteCallback);
			if (!deleteCallback.Success())
			{
				NOTIFY_ERROR(h_,
							 "CopyLinkWithOverwrite: DeleteFile failed for dest path:",
							 inDestPath);

				return false;
			}
		
			BasicCallbackClass copyCallback;
			CopySymbolicLink(inSourcePath, inDestPath, h_, copyCallback);
			if (!copyCallback.mSuccess)
			{
				NOTIFY_ERROR(h_,
							 "CopyLinkWithOverwrite: CopySymbolicLink failed for source path:",
							 inSourcePath,
							 "dest path:",
							 inDestPath);

				return false;
			}
		}
		inActionCallback.Call(kSyncDirectoriesActionStatus_CopiedItem, inDestPath);
		return true;
	}

	//
	//
	bool CopyLinkWithOverwrite(const HermitPtr& h_,
							   const FilePathPtr& inSourcePath,
							   const FilePathPtr& inDestParentPath,
							   const std::string& inFilename,
							   const bool& inPreviewOnly,
							   const SyncDirectoriesActionCallbackRef& inActionCallback)
	{
		FilePathCallbackClassT<FilePathPtr> destPath;
		AppendToFilePath(inDestParentPath, inFilename, h_, destPath);
		if (destPath.mFilePath == nullptr)
		{
			NOTIFY_ERROR(h_,
						 "CopyLinkWithOverwrite: AppendToFilePath failed for destParentPath:",
						 inDestParentPath,
						 "item name:",
						 inFilename);

			return false;
		}
		
		return CopyLinkWithOverwrite(inSourcePath, destPath.mFilePath, inPreviewOnly, h_, inActionCallback);
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
	typedef std::set<std::string> StringSet;
	
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
	//
	typedef std::set<FileInfoPtr, SortFileInfoPtrs> FileSet;
	
	//
	//
	class Directory
		:
		public ListDirectoryContentsCallback
	{
	public:
		//
		//
		Directory(
			FilePathPtr inFilePath,
			const StringSet& inExclusions)
			:
			mFilePath(inFilePath),
			mExclusions(inExclusions),
			mStatus(kListDirectoryContentsStatus_Unknown)
		{
		}
		
		//
		//
		bool Function(const HermitPtr& h_,
					  const ListDirectoryContentsStatus& inStatus,
					  const FilePathPtr& inParentPath,
					  const std::string& inItemName)
		{
			mStatus = inStatus;
			if (inStatus == kListDirectoryContentsStatus_Success)
			{				
				FilePathCallbackClassT<FilePathPtr> itemPath;
				AppendToFilePath(inParentPath, inItemName, h_, itemPath);
				if (itemPath.mFilePath == 0)
				{
					NOTIFY_ERROR(h_, "Directory::Function: AppendToFilePath failed.");
					return false;
				}

				if (mExclusions.find(inItemName) != mExclusions.end())
				{
					return true;
				}
				
				FileInfoPtr p(new FileInfo(itemPath.mFilePath, inItemName));
				
				GetFileTypeCallbackClass callback;
				GetFileType(itemPath.mFilePath, h_, callback);
				if (!callback.mSuccess)
				{
					NOTIFY_ERROR(
						h_,
						"Directory::Function: GetFileType failed for: ",
						itemPath.mFilePath);

					mStatus = kListDirectoryContentsStatus_Error;
					return false;
				}
				if (callback.mFileType == FileType::kDirectory)
				{
					mDirectories.insert(p);
				}
				else if (callback.mFileType == FileType::kSymbolicLink)
				{
					mSymbolicLinks.insert(p);
				}
				else if (callback.mFileType == FileType::kFile)
				{
					mFiles.insert(p);
				}
				else
				{
					NOTIFY_ERROR(
						inHub,
						"Directory::Function: GetFileType returned unexpected type for: ",
						itemPath.mFilePath.P());
						
					return false;
				}
			}
			return true;
		}
		
		//
		//
		FilePathPtr mFilePath;
		const StringSet& mExclusions;
		ListDirectoryContentsStatus mStatus;
		FileSet mFiles;
		FileSet mDirectories;
		FileSet mSymbolicLinks;
	};
	
	//
	//
	class SyncLinksClass
	{
	public:
		//
		//
		static bool SyncDirectoryLinks(
			const ConstHubPtr& inHub,
			const FilePathPtr& inSourceDirectoryPath,
			const FilePathPtr& inDestDirectoryPath,
			const bool& inPreviewOnly,
			const bool& inIgnoreDates,
			const StringSet& inExclusions,
			const SyncDirectoriesActionCallbackRef& inActionCallback)
		{
			Directory sourceDir(inSourceDirectoryPath, inExclusions);
			ListDirectoryContents(inSourceDirectoryPath, false, sourceDir);
			if ((sourceDir.mStatus != kListDirectoryContentsStatus_Success) &&
				(sourceDir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncDirectoryLinks: ListDirectoryContents failed for source dir: ",
					inSourceDirectoryPath);

				return false;
			}
			
			Directory destDir(inDestDirectoryPath, inExclusions);
			ListDirectoryContents(inDestDirectoryPath, false, destDir);
			if ((destDir.mStatus != kListDirectoryContentsStatus_Success) &&
				(destDir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncDirectoryLinks: ListDirectoryContents failed for dest dir: ",
					inDestDirectoryPath);

				return false;
			}
		
			if (!SyncLinks(sourceDir, inDestDirectoryPath, destDir, inPreviewOnly, inIgnoreDates, inActionCallback))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncDirectoryLinks: SyncLinks failed for source dir: ",
					inSourceDirectoryPath);

				NOTIFY_ERROR(
					inHub,
					"-- dest dir: ",
					inDestDirectoryPath);

				return false;
			}
			
			if (!SyncDirectories(sourceDir, destDir, inPreviewOnly, inIgnoreDates, inExclusions, inActionCallback))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncDirectoryLinks: SyncDirectories failed for source dir: ",
					inSourceDirectoryPath);

				NOTIFY_ERROR(
					inHub,
					"-- dest dir: ",
					inDestDirectoryPath);

				return false;
			}
						
			if (!inIgnoreDates)
			{
				if (!SyncDates(inSourceDirectoryPath, inDestDirectoryPath, inPreviewOnly, inActionCallback))
				{
					NOTIFY_ERROR(
						inHub,
						"SyncDirectoryLinks: SyncDates failed for source dir: ",
						inSourceDirectoryPath);

					NOTIFY_ERROR(
						inHub,
						"-- dest dir: ",
						inDestDirectoryPath);
				
					return false;
				}
			}
			return true;
		}
		
		//
		//
		static bool SyncLinks(
			const ConstHubPtr& inHub,
			const Directory& inSourceDir,
			const FilePathPtr& inDestDirPath,
			const Directory& inDestDir,
			const bool& inPreviewOnly,
			const bool& inIgnoreDates,
			const SyncDirectoriesActionCallbackRef& inActionCallback)
		{
			const FileSet& sourceLinks = inSourceDir.mSymbolicLinks;
			const FileSet& destLinks = inDestDir.mSymbolicLinks;
			
			FileSet::const_iterator sourceEnd = sourceLinks.end();
			FileSet::const_iterator destEnd = destLinks.end();
			
			FileSet::const_iterator sourceIt = sourceLinks.begin();
			FileSet::const_iterator destIt = destLinks.begin();
			
			while (true)
			{
				if (sourceIt == sourceEnd)
				{
					while (destIt != destEnd)
					{
						if (!inPreviewOnly)
						{
							DeleteFileCallbackClass callback;
							DeleteFile((*destIt)->mPath.P(), callback);
							if (!callback.Success())
							{
								NOTIFY_ERROR(
									inHub,
									"SyncLinks: DeleteFile failed for dest path: ",
									(*destIt)->mPath.P());

								return false;
							}
						}
						inActionCallback.Call(kSyncDirectoriesActionStatus_DeletedItem, (*destIt)->mPath.P());

						++destIt;
					}
					break;
				}
				if (destIt == destEnd)
				{
					while (sourceIt != sourceEnd)
					{
						if (!CopyLinkWithOverwrite((*sourceIt)->mPath.P(), inDestDirPath, (*sourceIt)->mName, inActionCallback))
						{
							NOTIFY_ERROR(
								inHub,
								"SyncLinks: CopyLinkWithOverwrite failed for source path: ",
								(*sourceIt)->mPath.P());

							return false;
						}

						++sourceIt;
					}
					break;
				}
				
				if ((*sourceIt)->mName < (*destIt)->mName)
				{
					if (!CopyLinkWithOverwrite((*sourceIt)->mPath.P(), inDestDirPath, (*sourceIt)->mName, inActionCallback))
					{
						NOTIFY_ERROR(
							inHub,
							"SyncLinks: CopyLinkWithOverwrite failed for source path: ",
							(*sourceIt)->mPath.P());

						return false;
					}

					++sourceIt;
				}
				else if ((*sourceIt)->mName > (*destIt)->mName)
				{
					if (!inPreviewOnly)
					{
						DeleteFileCallbackClass callback;
						DeleteFile((*destIt)->mPath.P(), callback);
						if (!callback.Success())
						{
							NOTIFY_ERROR(
								inHub,
								"SyncLinks: DeleteFile failed for dest path: ",
								(*destIt)->mPath.P());

							return false;
						}
					}
					inActionCallback.Call(kSyncDirectoriesActionStatus_DeletedItem, (*destIt)->mPath.P());

					++destIt;
				}
				else
				{
					if (!SyncOneLink((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
					{
						NOTIFY_ERROR(
							inHub,
							"SyncLinks: SyncOneLink failed for source path: ",
							(*sourceIt)->mPath.P());
								
						NOTIFY_ERROR(
							inHub,
							"-- dest path: ",
							(*destIt)->mPath.P());
							
						return false;
					}
					
					if (!SyncPosixOwnership((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
					{
							NOTIFY_ERROR(
								inHub,
								"SyncLinks: SyncPosixOwnership failed for source dir: ",
								(*sourceIt)->mPath.P());

							NOTIFY_ERROR(
								inHub,
								"-- dest dir: ",
								(*destIt)->mPath.P());

							return false;
					}

					if (!SyncPosixPermissions((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
					{
							NOTIFY_ERROR(
								inHub,
								"SyncLinks: SyncPosixPermissions failed for source dir: ",
								(*sourceIt)->mPath.P());

							NOTIFY_ERROR(
								inHub,
								"-- dest dir: ",
								(*destIt)->mPath.P());

							return false;
					}

					if (!inIgnoreDates)
					{
						if (!SyncDates((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
						{
							NOTIFY_ERROR(
								inHub,
								"SyncLinks: SyncDates failed for source dir: ",
								(*sourceIt)->mPath.P());

							NOTIFY_ERROR(
								inHub,
								"-- dest dir: ",
								(*destIt)->mPath.P());
						
							return false;
						}
					}
					
					++sourceIt;
					++destIt;
				}
			}
			return true;
		}

		//
		//
		static bool SyncOneLink(
			const ConstHubPtr& inHub,
			const FilePathPtr& inLinkPath1,
			const FilePathPtr& inLinkPath2,
			const bool& inPreviewOnly,
			const SyncDirectoriesActionCallbackRef& inActionCallback)
		{
			StringCallbackClass stringCallback;
			GetFilePathUTF8String(inLinkPath1, stringCallback);
			std::string link1PathUTF8(stringCallback.mValue);

			GetSymbolicLinkTargetCallbackClassT<FilePathPtr> link1TargetCallback;
			GetSymbolicLinkTarget(inLinkPath1, link1TargetCallback);
			if (!link1TargetCallback.mSuccess)
			{
				NOTIFY_ERROR(
					inHub,
					"SyncOneLink: GetSymbolicLinkTarget failed for path: ",
					inLinkPath1);
					
				return false;
			}

			GetFilePathUTF8String(inLinkPath2, stringCallback);
			std::string link2PathUTF8(stringCallback.mValue);

			GetSymbolicLinkTargetCallbackClassT<FilePathPtr> link2TargetCallback;
			GetSymbolicLinkTarget(inLinkPath2, link2TargetCallback);
			if (!link2TargetCallback.mSuccess)
			{
				NOTIFY_ERROR(
					inHub,
					"SyncOneLink: GetSymbolicLinkTarget failed for path: ",
					inLinkPath2);
						
				return false;
			}

			GetFilePathUTF8String(link1TargetCallback.mFilePath.P(), stringCallback);
			std::string link1TargetPathUTF8(stringCallback.mValue);
						
			GetFilePathUTF8String(link2TargetCallback.mFilePath.P(), stringCallback);
			std::string link2TargetPathUTF8(stringCallback.mValue);
		
			if (link1TargetPathUTF8 == link2TargetPathUTF8)
			{
				return true;
			}

			std::string originalLink1TargetPathUTF8(link1TargetPathUTF8);
			if ((link1PathUTF8.find("/Volumes/") == 0) &&
				(link1TargetPathUTF8.find("/Volumes/") == 0))
			{
				link1PathUTF8 = link1PathUTF8.substr(8);
				link1TargetPathUTF8 = link1TargetPathUTF8.substr(8);
			}

			GetRelativePath(link1PathUTF8, link1TargetPathUTF8, stringCallback);
			std::string link1TargetRelativePathUTF8(stringCallback.mValue);
			if (link1TargetRelativePathUTF8.empty())
			{
				link1TargetPathUTF8 = originalLink1TargetPathUTF8;
			}
			else
			{
				link1TargetPathUTF8 = link1TargetRelativePathUTF8;
			}

			std::string originalLink2TargetPathUTF8(link2TargetPathUTF8);
			if ((link2PathUTF8.find("/Volumes/") == 0) &&
				(link2TargetPathUTF8.find("/Volumes/") == 0))
			{
				link2PathUTF8 = link2PathUTF8.substr(8);
				link2TargetPathUTF8 = link2TargetPathUTF8.substr(8);
			}

			GetRelativePath(link2PathUTF8, link2TargetPathUTF8, stringCallback);
			std::string link2TargetRelativePathUTF8(stringCallback.mValue);
			if (link2TargetRelativePathUTF8.empty())
			{
				link2TargetPathUTF8 = originalLink2TargetPathUTF8;
			}
			else
			{
				link2TargetPathUTF8 = link2TargetRelativePathUTF8;
			}

			if (link1TargetPathUTF8 == link2TargetPathUTF8)
			{
				return true;
			}
			
			if (!CopyLinkWithOverwrite(inLinkPath1, inLinkPath2, inPreviewOnly, inActionCallback))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncOneLink: CopyLinkWithOverwrite failed for source path: ",
					inLinkPath1);

				return false;
			}

			return true;
		}

		//
		//
		static bool SyncDirectories(
			const ConstHubPtr& inHub,
			const Directory& inSourceDir,
			const Directory& inDestDir,
			const bool& inPreviewOnly,
			const bool& inIgnoreDates,
			const StringSet& inExclusions,
			const SyncDirectoriesActionCallbackRef& inActionCallback)
		{
			const FileSet& sourceDirs = inSourceDir.mDirectories;
			const FileSet& destDirs = inDestDir.mDirectories;
			
			FileSet::const_iterator sourceEnd = sourceDirs.end();
			FileSet::const_iterator destEnd = destDirs.end();
			
			FileSet::const_iterator sourceIt = sourceDirs.begin();
			FileSet::const_iterator destIt = destDirs.begin();

			while (true)
			{
				//	since this is used for the second pass, the directory structures should already match.
				//	so each variation of mismatch is an unexpected error condition.
				if (sourceIt == sourceEnd)
				{
					if ((destIt != destEnd) && !inPreviewOnly)
					{
						NOTIFY_ERROR(
							inHub,
							"SyncDirectories: Unexpected case, sourceIt == sourceEnd but destIt != destEnd for source dir: ",
							inSourceDir.mFilePath.P());
								
						NOTIFY_ERROR(
							inHub,
							"-- dest dir: ",
							inDestDir.mFilePath.P());
							
						LogString("-- (*destIt)->mName: ", (*destIt)->mName);

						return false;
					}
					break;
				}
				if (destIt == destEnd)
				{
					if ((sourceIt != sourceEnd) && !inPreviewOnly)
					{
						NOTIFY_ERROR(
							inHub,
							"SyncDirectories: Unexpected case, destIt == destEnd but sourceIt != sourceEnd for source dir: ",
							inSourceDir.mFilePath.P());
								
						NOTIFY_ERROR(
							inHub,
							"-- dest dir: ",
							inDestDir.mFilePath.P());

						LogString("-- (*sourceIt)->mName: ", (*sourceIt)->mName);

						return false;
					}
					break;
				}
				if ((*sourceIt)->mName < (*destIt)->mName)
				{
					if (!inPreviewOnly)
					{
						NOTIFY_ERROR(
							inHub,
							"SyncDirectories: Unexpected case, (*sourceIt)->mName < (*destIt)->mName for source dir: ",
							inSourceDir.mFilePath.P());
								
						NOTIFY_ERROR(
							inHub,
							"-- dest dir: ",
							inDestDir.mFilePath.P());

						LogString("-- (*sourceIt)->mName: ", (*sourceIt)->mName);
						LogString("-- (*destIt)->mName: ", (*destIt)->mName);

						return false;
					}
					++sourceIt;
				}
				else if ((*sourceIt)->mName > (*destIt)->mName)
				{
					if (!inPreviewOnly)
					{
						NOTIFY_ERROR(
							inHub,
							"SyncDirectories: Unexpected case, (*sourceIt)->mName < (*destIt)->mName for source dir: ",
							inSourceDir.mFilePath.P());
						
						NOTIFY_ERROR(
							inHub,
							"-- dest dir: ",
							inDestDir.mFilePath.P());

						LogString("-- (*sourceIt)->mName: ", (*sourceIt)->mName);
						LogString("-- (*destIt)->mName: ", (*destIt)->mName);

						return false;
					}
					++destIt;
				}
				else
				{
					if (!SyncDirectoryLinks(
						inHub,
						(*sourceIt)->mPath.P(),
						(*destIt)->mPath.P(),
						inPreviewOnly,
						inIgnoreDates,
						inExclusions,
						inActionCallback))
					{
						NOTIFY_ERROR(
							inHub,
							"SyncDirectories: SyncDirectoryLinks failed for source path: ",
							(*sourceIt)->mPath.P());
								
						NOTIFY_ERROR(
							inHub,
							"-- dest path: ",
							(*destIt)->mPath.P());

						return false;
					}
					
					++sourceIt;
					++destIt;
				}
			}

			return true;
		}
	};

	//
	//
	class SyncFilesAndDirectoriesClass
	{
	public:
		//
		//
		static bool SyncFilesAndDirectories(
			const ConstHubPtr& inHub,
			const FilePathPtr& inSourceDirectoryPath,
			const FilePathPtr& inDestDirectoryPath,
			const bool& inPreviewOnly,
			const bool& inIgnoreDates,
			const bool& inIgnoreFinderInfo,
			const StringSet& inExclusions,
			const SyncDirectoriesActionCallbackRef& inActionCallback)
		{
			Directory sourceDir(inSourceDirectoryPath, inExclusions);
			ListDirectoryContents(inSourceDirectoryPath, false, sourceDir);
			if ((sourceDir.mStatus != kListDirectoryContentsStatus_Success) &&
				(sourceDir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncFilesAndDirectories: ListDirectoryContents failed for: ",
					inSourceDirectoryPath);

				return false;
			}
			
			Directory destDir(inDestDirectoryPath, inExclusions);
			ListDirectoryContents(inDestDirectoryPath, false, destDir);
			if ((destDir.mStatus != kListDirectoryContentsStatus_Success) &&
				(destDir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncFilesAndDirectories: ListDirectoryContents failed for: ",
					inDestDirectoryPath);

				return false;
			}
			
			if (!SyncFiles(
				inHub,
				sourceDir,
				inDestDirectoryPath,
				destDir,
				inPreviewOnly,
				inIgnoreDates,
				inIgnoreFinderInfo,
				inActionCallback))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncFilesAndDirectories: SyncFiles failed for source dir: ",
					inSourceDirectoryPath);

				NOTIFY_ERROR(
					inHub,
					"-- dest dir: ",
					inDestDirectoryPath);

				return false;
			}

			if (!SyncDirectories(
				inHub,
				sourceDir,
				inDestDirectoryPath,
				destDir,
				inPreviewOnly,
				inIgnoreDates,
				inIgnoreFinderInfo,
				inExclusions,
				inActionCallback))
			{
				NOTIFY_ERROR(
					inHub,
					"SyncFilesAndDirectories: SyncDirectories failed for source dir: ",
					inSourceDirectoryPath);

				NOTIFY_ERROR(
					inHub,
					"-- dest dir: ",
					inDestDirectoryPath);

				return false;
			}
			
			PathIsPackageCallbackClass sourceIsPackage;
			PathIsPackage(inSourceDirectoryPath, sourceIsPackage);
			if (!sourceIsPackage.mSuccess)
			{
				NOTIFY_ERROR(
					inHub,
					"SyncFilesAndDirectories: PathIsPackage failed for soure dir: ",
					inSourceDirectoryPath);

				return false;
			}
			
			PathIsPackageCallbackClass destIsPackage;
			PathIsPackage(inDestDirectoryPath, destIsPackage);
			if (!destIsPackage.mSuccess)
			{
				NOTIFY_ERROR(
					inHub,
					"SyncFilesAndDirectories: PathIsPackage failed for dest dir: ",
					inDestDirectoryPath);

				return false;
			}
			
			if (sourceIsPackage.mIsPackage != destIsPackage.mIsPackage)
			{
				if (!inPreviewOnly)
				{
					BasicCallbackClass callback;
					SetDirectoryIsPackage(inDestDirectoryPath, sourceIsPackage.mIsPackage, callback);
					if (!callback.mSuccess)
					{
						NOTIFY_ERROR(
							inHub,
							"SyncFilesAndDirectories: SetDirectoryIsPackage failed for dest dir: ",
							inDestDirectoryPath);

						return false;
					}
				}
				inActionCallback.Call(kSyncDirectoriesActionStatus_UpdatedPackageStateForDirectory, inDestDirectoryPath);
			}
			
			if (!SyncPosixOwnership(inSourceDirectoryPath, inDestDirectoryPath, inPreviewOnly, inActionCallback))
			{
					NOTIFY_ERROR(
						inHub,
						"SyncFilesAndDirectories: SyncPosixOwnership failed for source dir: ",
						inSourceDirectoryPath);

					NOTIFY_ERROR(
						inHub,
						"-- dest dir: ",
						inDestDirectoryPath);

					return false;
			}

			if (!SyncPosixPermissions(inSourceDirectoryPath, inDestDirectoryPath, inPreviewOnly, inActionCallback))
			{
					NOTIFY_ERROR(
						inHub,
						"SyncFilesAndDirectories: SyncPosixPermissions failed for source dir: ",
						inSourceDirectoryPath);

					NOTIFY_ERROR(
						inHub,
						"-- dest dir: ",
						inDestDirectoryPath);

					return false;
			}

			if (!inIgnoreFinderInfo)
			{
				if (!SyncFinderInfo(inSourceDirectoryPath, inDestDirectoryPath, inPreviewOnly, inActionCallback))
				{
					NOTIFY_ERROR(
						inHub,
						"SyncFilesAndDirectories: SyncFinderInfo failed for source dir: ",
						inSourceDirectoryPath);

					NOTIFY_ERROR(
						inHub,
						"-- dest dir: ",
						inDestDirectoryPath);

					return false;
				}
			}
			
			if (!inIgnoreDates)
			{
				if (!SyncDates(inSourceDirectoryPath, inDestDirectoryPath, inPreviewOnly, inActionCallback))
				{
					NOTIFY_ERROR(
						inHub,
						"SyncFilesAndDirectories: SyncDates failed for source dir: ",
						inSourceDirectoryPath);

					NOTIFY_ERROR(
						inHub,
						"-- dest dir: ",
						inDestDirectoryPath);

					return false;
				}
			}

			return true;
		}
		
		//
		//
		static bool SyncFiles(
			const ConstHubPtr& inHub,
			const Directory& inSourceDir,
			const FilePathPtr& inDestDirPath,
			const Directory& inDestDir,
			const bool& inPreviewOnly,
			const bool& inIgnoreDates,
			const bool& inIgnoreFinderInfo,
			const SyncDirectoriesActionCallbackRef& inActionCallback)
		{
			const FileSet& sourceFiles = inSourceDir.mFiles;
			const FileSet& destFiles = inDestDir.mFiles;
			
			FileSet::const_iterator sourceEnd = sourceFiles.end();
			FileSet::const_iterator destEnd = destFiles.end();
			
			FileSet::const_iterator sourceIt = sourceFiles.begin();
			FileSet::const_iterator destIt = destFiles.begin();
			
			while (true)
			{
				if (sourceIt == sourceEnd)
				{
					while (destIt != destEnd)
					{
						if (!inPreviewOnly)
						{
							DeleteFileCallbackClass callback;
							DeleteFile((*destIt)->mPath.P(), callback);
							if (!callback.Success())
							{
								NOTIFY_ERROR(
									inHub,
									"SyncFiles: DeleteFile failed for dest path: ",
									(*destIt)->mPath.P());

								return false;
							}
						}
						inActionCallback.Call(kSyncDirectoriesActionStatus_DeletedItem, (*destIt)->mPath.P());

						++destIt;
					}
					break;
				}
				if (destIt == destEnd)
				{
					while (sourceIt != sourceEnd)
					{
						if (!CopyFileWithOverwrite(
							inHub,
							(*sourceIt)->mPath.P(),
							inDestDirPath,
							(*sourceIt)->mName,
							inPreviewOnly,
							inActionCallback))
						{
							NOTIFY_ERROR(
								inHub,
								"SyncFiles: CopyFileWithOverwrite failed for source path: ",
								(*destIt)->mPath.P());
								
							return false;
						}

						++sourceIt;
					}
					break;
				}
				
				if ((*sourceIt)->mName < (*destIt)->mName)
				{
					if (!CopyFileWithOverwrite(
						inHub,
						(*sourceIt)->mPath.P(),
						inDestDirPath,
						(*sourceIt)->mName,
						inPreviewOnly,
						inActionCallback))
					{
						NOTIFY_ERROR(
							inHub,
							"SyncFiles: CopyFileWithOverwrite failed for source path: ",
							(*destIt)->mPath.P());
								
						return false;
					}

					++sourceIt;
				}
				else if ((*sourceIt)->mName > (*destIt)->mName)
				{
					if (!inPreviewOnly)
					{
						DeleteFileCallbackClass callback;
						DeleteFile((*destIt)->mPath.P(), callback);
						if (!callback.Success())
						{
							NOTIFY_ERROR(
								inHub,
								"SyncFiles: DeleteFile failed for dest path: ",
								(*destIt)->mPath.P());

							return false;
						}
					}
					inActionCallback.Call(kSyncDirectoriesActionStatus_DeletedItem, (*destIt)->mPath.P());

					++destIt;
				}
				else
				{
					bool quickMatch = false;
					if (!CompareFiles((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), quickMatch))
					{
						NOTIFY_ERROR(
							inHub,
							"SyncFiles: CompareFiles failed for source path: ",
							(*sourceIt)->mPath.P());
								
						NOTIFY_ERROR(
							inHub,
							"-- dest path: ",
							(*destIt)->mPath.P());

						return false;
					}
					
					if (!quickMatch)
					{
						if (!CopyFileWithOverwrite(
							inHub,
							(*sourceIt)->mPath.P(),
							inDestDirPath,
							(*sourceIt)->mName,
							inPreviewOnly,
							inActionCallback))
						{
							NOTIFY_ERROR(
								inHub,
								"SyncFiles: CopyFileWithOverwrite failed for source path: ",
								(*destIt)->mPath.P());
								
							return false;
						}
					}

					if (!SyncPosixOwnership((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
					{
							NOTIFY_ERROR(
								inHub,
								"SyncFiles: SyncPosixOwnership failed for source dir: ",
								(*sourceIt)->mPath.P());

							NOTIFY_ERROR(
								inHub,
								"-- dest dir: ",
								(*destIt)->mPath.P());

							return false;
					}

					if (!SyncPosixPermissions((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
					{
							NOTIFY_ERROR(
								inHub,
								"SyncFiles: SyncPosixPermissions failed for source dir: ",
								(*sourceIt)->mPath.P());

							NOTIFY_ERROR(
								inHub,
								"-- dest dir: ",
								(*destIt)->mPath.P());

							return false;
					}

					if (!inIgnoreFinderInfo)
					{
						if (!SyncFinderInfo((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
						{
							NOTIFY_ERROR(
								inHub,
								"SyncFiles: SyncFinderInfo failed for source dir: ",
								(*sourceIt)->mPath.P());

							NOTIFY_ERROR(
								inHub,
								"-- dest dir: ",
								(*destIt)->mPath.P());

							return false;
						}
					}

					if (!inIgnoreDates)
					{
						if (!SyncDates((*sourceIt)->mPath.P(), (*destIt)->mPath.P(), inPreviewOnly, inActionCallback))
						{
							NOTIFY_ERROR(
								inHub,
								"SyncFiles: SyncDates failed for source dir: ",
								(*sourceIt)->mPath.P());

							NOTIFY_ERROR(
								inHub,
								"-- dest dir: ",
								(*destIt)->mPath.P());
						
							return false;
						}
					}
										
					++sourceIt;
					++destIt;
				}
			}
			return true;
		}

		//
		//
		static bool SyncDirectories(
			const ConstHubPtr& inHub,
			const Directory& inSourceDir,
			const FilePathPtr& inDestDirPath,
			const Directory& inDestDir,
			const bool& inPreviewOnly,
			const bool& inIgnoreDates,
			const bool& inIgnoreFinderInfo,
			const StringSet& inExclusions,
			const SyncDirectoriesActionCallbackRef& inActionCallback)
		{
			const FileSet& sourceDirs = inSourceDir.mDirectories;
			const FileSet& destDirs = inDestDir.mDirectories;
			
			FileSet::const_iterator sourceEnd = sourceDirs.end();
			FileSet::const_iterator destEnd = destDirs.end();
			
			FileSet::const_iterator sourceIt = sourceDirs.begin();
			FileSet::const_iterator destIt = destDirs.begin();

			while (true)
			{
				if (sourceIt == sourceEnd)
				{
					while (destIt != destEnd)
					{
						if (!inPreviewOnly)
						{
							DeleteFileCallbackClass callback;
							DeleteFile((*destIt)->mPath.P(), callback);
							if (!callback.Success())
							{
								NOTIFY_ERROR(
									inHub,
									"SyncDirectories: DeleteFile failed for dest path: ",
									(*destIt)->mPath.P());

								return false;
							}
						}
						inActionCallback.Call(kSyncDirectoriesActionStatus_DeletedItem, (*destIt)->mPath.P());

						++destIt;
					}
					break;
				}
				if (destIt == destEnd)
				{
					while (sourceIt != sourceEnd)
					{
						if (!CopyFileWithOverwrite(
							inHub,
							(*sourceIt)->mPath.P(),
							inDestDirPath,
							(*sourceIt)->mName,
							inPreviewOnly,
							inActionCallback))
						{
							NOTIFY_ERROR(
								inHub,
								"SyncDirectories: CopyFileWithOverwrite failed for source path: ",
								(*sourceIt)->mPath.P());
								
							return false;
						}

						++sourceIt;
					}
					break;
				}
				
				if ((*sourceIt)->mName < (*destIt)->mName)
				{
					if (!CopyFileWithOverwrite(
						inHub,
						(*sourceIt)->mPath.P(),
						inDestDirPath,
						(*sourceIt)->mName,
						inPreviewOnly,
						inActionCallback))
					{
						NOTIFY_ERROR(
							inHub,
							"SyncDirectories: CopyFileWithOverwrite failed for source path: ",
							(*destIt)->mPath.P());
								
						return false;
					}

					++sourceIt;
				}
				else if ((*sourceIt)->mName > (*destIt)->mName)
				{
					if (!inPreviewOnly)
					{
						DeleteFileCallbackClass callback;
						DeleteFile((*destIt)->mPath.P(), callback);
						if (!callback.Success())
						{
							NOTIFY_ERROR(
								inHub,
								"SyncDirectories: DeleteFile failed for dest path: ",
								(*destIt)->mPath.P());

							return false;
						}
					}
					inActionCallback.Call(kSyncDirectoriesActionStatus_DeletedItem, (*destIt)->mPath.P());

					++destIt;
				}
				else
				{
					if (!SyncFilesAndDirectories(
						inHub,
						(*sourceIt)->mPath.P(),
						(*destIt)->mPath.P(),
						inPreviewOnly,
						inIgnoreDates,
						inIgnoreFinderInfo,
						inExclusions,
						inActionCallback))
					{
						NOTIFY_ERROR(
							inHub,
							"SyncDirectories: SyncFilesAndDirectories failed for source path: ",
							(*sourceIt)->mPath.P());
								
						NOTIFY_ERROR(
							inHub,
							"-- dest path: ",
							(*destIt)->mPath.P());

						return false;
					}
					
					++sourceIt;
					++destIt;
				}
			}

			return true;
		}
	};

} // private namespace

//
//
void SyncDirectories(
	const ConstHubPtr& inHub,
	const FilePathPtr& inSourcePath,
	const FilePathPtr& inDestPath,
	const bool& inPreviewOnly,
	const SyncDirectoriesActionCallbackRef& inActionCallback,
	const BasicCallbackRef& inCallback)
{
	//	??? if source is case-sensitive and dest is not... here be dragons
	//	??? posix permissions?
	
	StringSet exclusions;
	exclusions.insert(".DS_Store");

	if (!SyncFilesAndDirectoriesClass::SyncFilesAndDirectories(
		inHub,
		inSourcePath,
		inDestPath,
		inPreviewOnly,
		false,
		false,
		exclusions,
		inActionCallback))
	{
		NOTIFY_ERROR(
			inHub,
			"SyncDirectories: SyncFilesAndDirectories failed for source dir: ",
			inSourcePath);

		NOTIFY_ERROR(
			inHub,
			"-- dest dir: ",
			inDestPath);
			
		inCallback.Call(false);
		return;
	}

	//	??? are there cases which require multiple passes? link1 -> link2 but link2 isn't synced yet?
	if (!SyncLinksClass::SyncDirectoryLinks(
		inHub,
		inSourcePath,
		inDestPath,
		inPreviewOnly,
		false,
		exclusions,
		inActionCallback))
	{
		NOTIFY_ERROR(
			inHub,
			"SyncDirectories: SyncDirectoryLinks failed for source dir: ",
			inSourcePath);

		NOTIFY_ERROR(
			inHub,
			"-- dest dir: ",
			inDestPath);
				
		inCallback.Call(false);
		return;
	}
		
	inCallback.Call(true);
}
	
} // namespace file
} // namespace hermit

#endif

