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
#include "AppendToFilePath.h"
#include "CompareFiles.h"
#include "CopyFinderInfo.h"
#include "CopySymbolicLink.h"
#include "CreateDirectory.h"
#include "DeleteFile.h"
#include "FileExists.h"
#include "FileSystemCopy.h"
#include "GetFileDates.h"
#include "GetFilePosixPermissions.h"
#include "GetFileType.h"
#include "ListDirectoryContents.h"
#include "PathIsAlias.h"
#include "SetFileDates.h"
#include "SetFilePosixPermissions.h"
#include "Hermit/Foundation/Thread.h"
#include "FileSystemCopyWithVerify.h"

namespace hermit {
namespace file {

namespace {

	//
	//
	class GetTypeCallback
		:
		public GetFileTypeCallback
	{
	public:
		//
		//
		GetTypeCallback()
			:
			mSuccess(false),
			mFileType(FileType::kUnknown)
		{
		}
		
		//
		//
		bool Function(
			const bool& inSuccess,
			const FileType& inFileType)
		{
			mSuccess = inSuccess;
			if (inSuccess)
			{
				mFileType = inFileType;
			}
			return true;
		}
		
		//
		//
		bool mSuccess;
		FileType mFileType;
	};

	//
	//
	class IsAliasCallback
		:
		public PathIsAliasCallback
	{
	public:
		//
		//
		IsAliasCallback()
			:
			mSuccess(false),
			mIsAlias(false)
		{
		}
		
		//
		//
		bool Function(
			const bool& inSuccess,
			const bool& inIsAlias)
		{
			mSuccess = inSuccess;
			if (inSuccess)
			{
				mIsAlias = inIsAlias;
			}
			return true;
		}
		
		//
		//
		bool mSuccess;
		bool mIsAlias;
	};

	//
	//
	class FileDatesCallback
		:
		public GetFileDatesCallback
	{
	public:
		//
		//
		FileDatesCallback()
			:
			mSuccess(false)
		{
		}
		
		//
		//
		bool Function(
			const bool& inSuccess,
			const std::string& inCreationDate,
			const std::string& inModificationDate)
		{
			mSuccess = inSuccess;
			if (inSuccess)
			{
				mCreationDate = inCreationDate;
				mModificationDate = inModificationDate;
			}
			return true;
		}
		
		//
		//
		bool mSuccess;
		std::string mCreationDate;
		std::string mModificationDate;
	};

	//
	//
	class GetPermissionsCallback
		:
		public GetFilePosixPermissionsCallback
	{
	public:
		//
		//
		GetPermissionsCallback()
			:
			mSuccess(false),
			mPermissions(0)
		{
		}
		
		//
		//
		bool Function(
			const bool& inSuccess,
			const uint32_t& inPosixPermissions)
		{
			mSuccess = inSuccess;
			if (inSuccess)
			{
				mPermissions = inPosixPermissions;
			}
			return true;
		}
		
		//
		//
		bool mSuccess;
		uint32_t mPermissions;
	};

	//
	//
	class CopyCallback
		:
		public FileSystemCopyCallback
	{
	public:
		//
		//
		CopyCallback()
			:
			mStatus(kFileSystemCopyStatus_Unknown)
		{
		}
		
		//
		//
		bool Function(
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
	bool CopyOneFile(
		const FilePathPtr& inSourcePath,
		const FilePathPtr& inDestPath,
		const FileSystemCopyCallbackRef& inCallback)
	{
		IsAliasCallback isAliasCallback;
		PathIsAlias(inHub, inSourcePath, isAliasCallback);
		
		if (!isAliasCallback.mSuccess)
		{
			LogFilePath(
				inHub,
				"CopyOneFile(): PathIsAlias failed for: ",
				inSourcePath);
		}
		else if (isAliasCallback.mIsAlias)
		{
			BasicCallbackClass callback;
			CopySymbolicLink(
				inSourcePath,
				inDestPath,
				callback);

			if (!callback.mSuccess)
			{
				LogFilePath(
					inHub,
					"CopyOneFile(): CopySymbolicLink failed for path: ",
					inSourcePath);
								
				return inCallback.Call(kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
			}

			return inCallback.Call(kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
		}

		int retries = 0;
		while (true)
		{
			CopyCallback copyCallback;
			FileSystemCopy(
				inSourcePath,
				inDestPath,
				copyCallback);

			if (copyCallback.mStatus == kFileSystemCopyStatus_Success)
			{
				bool filesMatch = false;
				int compareRetries = 0;
				while (true)
				{
					CompareFilesCallbackClassT<std::string> compareCallback;
					CompareFiles(
						inHub,
						inSourcePath,
						inDestPath,
						true,
						false,
						compareCallback);

					if (compareCallback.mStatus == kCompareFilesStatus_Error)
					{
						Log(
							inHub,
							"FileSystemCopyWithVerify: CopyOneFile(): CompareFiles() failed.");

						LogFilePath(
							inHub,
							"-- file 1: ",
							inSourcePath);

						LogFilePath(
							inHub,
							"-- file 2: ",
							inDestPath);
							
						++compareRetries;
						if (compareRetries == 3)
						{
							inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
							return false;
						}
					}
					else if (compareCallback.mStatus != kCompareFilesStatus_FilesMatch)
					{
						Log(
							inHub,
							"FileSystemCopyWithVerify: CopyOneFile(): Files differ.");

						LogFilePath(
							inHub,
							"-- file 1: ",
							inSourcePath);

						LogFilePath(
							inHub,
							"-- file 2: ",
							inDestPath);
							
						DeleteFileCallbackClass deleteCallback;
						DeleteFile(inHub, inDestPath, deleteCallback);
						break;
					}
					else
					{
						filesMatch = true;
						break;
					}
				}
				if (filesMatch)
				{
					inCallback.Call(inHub, kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
					break;
				}
			}
			
			++retries;
			if ((retries == 3) ||
				(copyCallback.mStatus == kFileSystemCopyStatus_Unknown) ||
				(copyCallback.mStatus == kFileSystemCopyStatus_SourceNotFound) ||
				(copyCallback.mStatus == kFileSystemCopyStatus_TargetAlreadyExists))
			{
				Log(
					inHub,
					"FileSystemCopyWithVerify: CopyOneFile(): FileSystemCopy() failed.");
					
				LogFilePath(
					inHub,
					"-- file 1: ",
					inSourcePath);

				LogFilePath(
					inHub,
					"-- file 2: ",
					inDestPath);

				inCallback.Call(inHub, copyCallback.mStatus, inSourcePath, inDestPath);
				return false;
			}
			Sleep(250);
//			Log(
//				inHub,
//				kLogLevel_Info,
//				"...retrying...");
		}
		return true;
	}
	

	//
	//
	bool CopyOneSymbolicLink(
		const ConstHubPtr& inHub,
		const FilePathPtr& inSourcePath,
		const FilePathPtr& inDestPath,
		const FileSystemCopyCallbackRef& inCallback)
	{
		int retries = 0;
		while (true)
		{
			CopyCallback copyCallback;
			FileSystemCopy(
				inHub,
				inSourcePath,
				inDestPath,
				copyCallback);

			if (copyCallback.mStatus == kFileSystemCopyStatus_Success)
			{
				break;
			}

			++retries;
			if ((retries == 3) ||
				(copyCallback.mStatus == kFileSystemCopyStatus_Unknown) ||
				(copyCallback.mStatus == kFileSystemCopyStatus_SourceNotFound) ||
				(copyCallback.mStatus == kFileSystemCopyStatus_TargetAlreadyExists))
			{
				Log(
					inHub,
					"FileSystemCopyWithVerify: CopyOneSymbolicLink(): FileSystemCopy() failed.");
					
				LogFilePath(
					inHub,
					"-- file 1: ",
					inSourcePath);

				LogFilePath(
					inHub,
					"-- file 2: ",
					inDestPath);

				inCallback.Call(inHub, copyCallback.mStatus, inSourcePath, inDestPath);
				return false;
			}
			Sleep(500);
			Log(
				inHub,
				"...retrying...");
		}
		
//		CompareCallback compareCallback;
//		CompareFilesCallbackT<CompareCallback> compareCallbackFn(&compareCallback, &CompareCallback::Function);
//		CompareFiles(inHub, inSourcePath, inDestPath, compareCallbackFn.F());
//		if (!compareCallback.mSuccess)
//		{
//			Log(
//				inHub,
//				"FileSystemCopyWithVerify: CopyOneFile(): CompareFiles() failed.");
//
//			LogFilePath(
//				inHub,
//				"-- file 1: ",
//				inSourcePath);
//
//			LogFilePath(
//				inHub,
//				"-- file 2: ",
//				inDestPath);
//
//			inCallback.Call(inHub, kFileSystemCopyStatus_VerifyError, inSourcePath, inDestPath);
//			return false;
//		}
//		else if (!compareCallback.mFilesMatch)
//		{
//			Log(
//				inHub,
//				"FileSystemCopyWithVerify: CopyOneFile(): Files differ.");
//
//			LogFilePath(
//				inHub,
//				"-- file 1: ",
//				inSourcePath);
//
//			LogFilePath(
//				inHub,
//				"-- file 2: ",
//				inDestPath);
//
//			inCallback.Call(inHub, kFileSystemCopyStatus_VerifyFailed, inSourcePath, inDestPath);
//			return false;
//		}
		inCallback.Call(inHub, kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
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
		Directory()
			:
			mStatus(kListDirectoryContentsStatus_Unknown)
		{
		}
		
		//
		//
		bool Function(
			const ConstHubPtr& inHub,
			const ListDirectoryContentsStatus& inStatus,
			const FilePathPtr& inParentPath,
			const std::string& inItemName)
		{
			mStatus = inStatus;
			if (inStatus == kListDirectoryContentsStatus_Success)
			{
				FilePathCallbackClassT<FilePathPtr> itemPath;
				AppendToFilePath(inHub, inParentPath, inItemName, itemPath);
				if (itemPath.mFilePath.P() == 0)
				{
					Log(
						inHub,
						"FileSystemCopy: Directory::Function(): AppendToFilePath failed.");
						
					mStatus = kListDirectoryContentsStatus_Error;
					return false;
				}
				
				FileInfoPtr p(new FileInfo(itemPath.mFilePath.P(), inItemName));
				
				GetTypeCallback callback;
				GetFileType(
					inHub,
					itemPath.mFilePath.P(),
					callback);

				if (!callback.mSuccess)
				{
					LogFilePath(
						inHub,
						"FileSystemCopyWithVerify: Directory::Function(): GetFileType failed for: ",
						itemPath.mFilePath.P());

					mStatus = kListDirectoryContentsStatus_Error;
					return false;
				}
				if (callback.mFileType == kFileType_Directory)
				{
					mDirectories.insert(p);
				}
				else if (callback.mFileType == kFileType_SymbolicLink)
				{
					mSymbolicLinks.insert(p);
				}
				else if (callback.mFileType == kFileType_File)
				{
					mFiles.insert(p);
				}
				else
				{
					LogFilePath(
						inHub,
						"FileSystemCopyWithVerify: Directory::Function(): GetFileType returned unexpected type for: ",
						itemPath.mFilePath.P());
						
					return false;
				}
			}
			return true;
		}
		
		//
		//
		const FileSet& GetFiles() const
		{
			return mFiles;
		}
		
		//
		//
		const FileSet& GetDirectories() const
		{
			return mDirectories;
		}

		//
		//
		const FileSet& GetSymbolicLinks() const
		{
			return mSymbolicLinks;
		}
		
		//
		//
		bool IsEqualTo(
			const ConstHubPtr& inHub,
			const Directory& inOtherDir)
		{
			if (!CompareFileSet(inHub, mFiles, inOtherDir.mFiles))
			{
				return false;
			}
			if (!CompareFileSet(inHub, mDirectories, inOtherDir.mDirectories))
			{
				return false;
			}
			if (!CompareFileSet(inHub, mSymbolicLinks, inOtherDir.mSymbolicLinks))
			{
				return false;
			}
			return true;
		}

		//
		//
		static bool CompareFileSet(
			const ConstHubPtr& inHub,
			const FileSet& inFileSet1,
			const FileSet& inFileSet2)
		{
			bool match = true;
			FileSet::const_iterator end1 = inFileSet1.end();
			FileSet::const_iterator end2 = inFileSet2.end();
			FileSet::const_iterator it1 = inFileSet1.begin();
			FileSet::const_iterator it2 = inFileSet2.begin();
			while ((it1 != end1) && (it2 != end2))
			{
				FileInfoPtr info1 = *it1;
				FileInfoPtr info2 = *it2;
				
				if (info1->mName != info2->mName)
				{
					if (info1->mName < info2->mName)
					{
//						LogFilePath(
//							inHub,
//							kLogLevel_Info,
//							"-- CompareFileSet: Path only in first set: ",
//							info1->mPath.P());

						++it1;
					}
					else
					{
//						LogFilePath(
//							inHub,
//							kLogLevel_Info,
//							"-- CompareFileSet: Path only in second set: ",
//							info2->mPath.P());
						
						++it2;
					}
				
					match = false;
				}
				else
				{
					++it1;
					++it2;
				}
			}
			while (it1 != end1)
			{
//				LogFilePath(
//					inHub,
//					kLogLevel_Info,
//					"-- CompareFileSet: Path only in first set: ",
//					(*it1)->mPath.P());

				++it1;
			}
			while (it2 != end2)
			{
//				LogFilePath(
//					inHub,
//					kLogLevel_Info,
//					"-- CompareFileSet: Path only in second set: ",
//					(*it2)->mPath.P());

				++it2;
			}
			
			return match;
		}
		
		//
		//
		ListDirectoryContentsStatus mStatus;
		FileSet mFiles;
		FileSet mDirectories;
		FileSet mSymbolicLinks;
	};

	//
	//
	bool IsHex(
		char inChar)
	{
		if (((inChar >= '0') && (inChar <= '9')) ||
			((inChar >= 'a') && (inChar <= 'f')))
		{
			return true;
		}
		return false;
	}
	
	//
	//
	bool IsDATFileName(
		const ConstHubPtr& inHub,
		const std::string& inFilename)
	{
		//	.dat0219.381
		if (inFilename.size() != 12)
		{
			return false;
		}
		if ((inFilename[0] == '.') &&
			(inFilename[1] == 'd') &&
			(inFilename[2] == 'a') &&
			(inFilename[3] == 't') &&
			IsHex(inFilename[4]) &&
			IsHex(inFilename[5]) &&
			IsHex(inFilename[6]) &&
			IsHex(inFilename[7]) &&
			(inFilename[8] == '.') &&
			IsHex(inFilename[9]) &&
			IsHex(inFilename[10]) &&
			IsHex(inFilename[11]))
		{
			return true;
		}
		return false;		
	}
	
	//
	//
	bool DeleteDotDATFiles(
		const ConstHubPtr& inHub,
		const Directory& inDirectory)
	{
		bool deleted = false;
		FileSet::const_iterator end = inDirectory.mFiles.end();
		FileSet::const_iterator it = inDirectory.mFiles.begin();
		while (it != end)
		{
			FileInfoPtr info = *it;
			if (IsDATFileName(inHub, info->mName))
			{
				DeleteFileCallbackClass deleteCallback;
				DeleteFile(inHub, info->mPath.P(), deleteCallback);
				if (!deleteCallback.Success())
				{
//					LogFilePath(
//						inHub,
//						kLogLevel_Info,
//						"DeleteDotDATFiles(): Couldn't delete apparent .dat file: ",
//						info->mPath.P());
				}
				else
				{
//					LogFilePath(
//						inHub,
//						kLogLevel_Info,
//						">>> Deleted .dat file from target directory: ",
//						info->mPath.P());

					deleted = true;
				}
			}
			++it;
		}
		return deleted;
	}
	
	//
	//
	bool CopyOneDirectory(
		const ConstHubPtr& inHub,
		const FilePathPtr& inSourcePath,
		const FilePathPtr& inDestPath,
		const FileSystemCopyCallbackRef& inCallback)
	{
		int retries = 0;
		while (true)
		{
			BasicCallbackClass createCallback;
			CreateDirectory(inHub, inDestPath, createCallback);
			if (createCallback.mSuccess)
			{
				break;
			}

			++retries;
			if (retries == 3)
			{
				Log(
					inHub,
					"FileSystemCopyWithVerify: CopyOneDirectory(): CreateDirectory() failed.");
					
				LogFilePath(
					inHub,
					"-- path: ",
					inDestPath);

				inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
				return false;
			}
			Sleep(250);
			
			Log(
				inHub,
				"...retrying...");
		}
		
		Directory dir;
		ListDirectoryContents(
			inHub,
			inSourcePath,
			false,
			dir);

		if ((dir.mStatus != kListDirectoryContentsStatus_Success) &&
			(dir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
		{
			inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
			return false;
		}

		bool success = true;
		{
			const FileSet& dirFiles = dir.GetFiles();
			FileSet::const_iterator end = dirFiles.end();
			for (FileSet::const_iterator it = dirFiles.begin(); it != end; ++it)
			{
				FilePathCallbackClassT<FilePathPtr> destFilePath;
				AppendToFilePath(inHub, inDestPath, (*it)->mName, destFilePath);
				if (destFilePath.mFilePath.P() == NULL)
				{
					LogString(
						inHub,
						"CopyOneDirectory(): AppendToFilePath failed for file item: ",
						(*it)->mName);

					inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					return false;
				}
				
				if (!CopyOneFile(inHub, (*it)->mPath.P(), destFilePath.mFilePath.P(), inCallback))
				{
					success = false;
				}
			}
		}

		{
			const FileSet& dirDirs = dir.GetDirectories();
			FileSet::const_iterator end = dirDirs.end();
			for (FileSet::const_iterator it = dirDirs.begin(); it != end; ++it)
			{
				FilePathCallbackClassT<FilePathPtr> destFilePath;
				AppendToFilePath(inHub, inDestPath, (*it)->mName, destFilePath);
				if (destFilePath.mFilePath.P() == NULL)
				{
					LogString(
						inHub,
						"CopyOneDirectory(): AppendToFilePath failed for directory item: ",
						(*it)->mName);

					inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					return false;
				}
				
				if (!CopyOneDirectory(inHub, (*it)->mPath.P(), destFilePath.mFilePath.P(), inCallback))
				{
					success = false;
				}
			}
		}

		{
			const FileSet& dirLinks = dir.GetSymbolicLinks();
			FileSet::const_iterator end = dirLinks.end();
			for (FileSet::const_iterator it = dirLinks.begin(); it != end; ++it)
			{
				FilePathCallbackClassT<FilePathPtr> destFilePath;
				AppendToFilePath(inHub, inDestPath, (*it)->mName, destFilePath);
				if (destFilePath.mFilePath.P() == NULL)
				{
					inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
					return false;
				}
				
				if (!CopyOneSymbolicLink(inHub, (*it)->mPath.P(), destFilePath.mFilePath.P(), inCallback))
				{
					success = false;
				}
			}
		}

		bool permissionsSuccess = false;
		retries = 0;
		while (true)
		{
			GetPermissionsCallback getPermissionsCallback;
			GetFilePosixPermissions(
				inHub,
				inSourcePath,
				getPermissionsCallback);

			if (!getPermissionsCallback.mSuccess)
			{
				LogFilePath(
					inHub,
					"CopyOneDirectory(): GetFilePosixPermissions failed for source path: ",
					inSourcePath);
			}
			else
			{
				BasicCallbackClass setPermissionsCallback;
				SetFilePosixPermissions(
					inHub,
					inDestPath,
					getPermissionsCallback.mPermissions,
					setPermissionsCallback);

				if (!setPermissionsCallback.mSuccess)
				{
					LogFilePath(
						inHub,
						"CopyOneDirectory(): SetFilePosixPermissions failed for dest path: ",
						inDestPath);
				}
				else
				{
					permissionsSuccess = true;
				}			
			}
			if (permissionsSuccess)
			{
				break;
			}
			else
			{
				++retries;
				if (retries == 3)
				{
					break;
				}
				Sleep(250);
				
//				Log(
//					inHub,
//					kLogLevel_Info,
//					"...retrying...");
			}
		}
		if (!permissionsSuccess)
		{
			success = false;
		}

		bool finderInfoSuccess = false;
		retries = 0;
		while (true)
		{
			BasicCallbackClass finderInfoCallback;
			CopyFinderInfo(
				inHub,
				inSourcePath,
				inDestPath,
				finderInfoCallback);
			
			finderInfoSuccess = finderInfoCallback.mSuccess;
			if (finderInfoSuccess)
			{
				break;
			}
			
			LogFilePath(
				inHub,
				"CopyOneDirectory(): CopyFinderInfo failed for source path: ",
				inSourcePath);

			++retries;
			if (retries == 3)
			{
				break;
			}
			Sleep(250);
			
//			Log(
//				inHub,
//				kLogLevel_Info,
//				"...retrying...");
		}

		bool dateSuccess = false;
		retries = 0;
		while (true)
		{
			FileDatesCallback datesCallback;
			GetFileDates(
				inHub,
				inSourcePath,
				datesCallback);

			dateSuccess = datesCallback.mSuccess;
			if (!dateSuccess)
			{
				LogFilePath(
					inHub,
					"CopyOneDirectory(): GetFileDates() failed for: ",
					inSourcePath);
			}
			else
			{
				std::string creationDate(datesCallback.mCreationDate);
				std::string modificationDate(datesCallback.mModificationDate);

				BasicCallbackClass setDatesCallback;
				SetFileDates(inHub, inDestPath, creationDate, modificationDate, setDatesCallback);
				if (!setDatesCallback.mSuccess)
				{
					LogFilePath(
						inHub,
						"CopyOneDirectory(): SetFileDates() failed for: ",
						inDestPath);
					
					dateSuccess = false;
				}
			}
			if (dateSuccess)
			{
				break;
			}
			else
			{
				++retries;
				if (retries == 3)
				{
					break;
				}
				Sleep(250);
				
//				Log(
//					inHub,
//					kLogLevel_Info,
//					"...retrying...");
			}
		}
		if (!dateSuccess)
		{
			success = false;
		}
		if (!success)
		{
			inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
		}
		return success;
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
//			const ConstHubPtr& inHub,
//			ListDirectoryContentsStatus inStatus,
//			FilePathPtr inParentPath,
//			const char* inItemName)
//		{
//			mStatus = inStatus;
//			if (inStatus == kListDirectoryContentsStatus_Success)
//			{
//				FilePathPtr itemPath = 0;
//				AppendToFilePath(inHub, inParentPath, inItemName, itemPath);
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
//					AppendToFilePath(inHub, mDestParentPath, inItemName, destFilePath);
//					if (destFilePath == NULL)
//					{
//						mCallback.Call(inHub, kFileSystemCopyStatus_Error, itemPath, destFilePath);
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
//						return mCallback.Call(inHub, kFileSystemCopyStatus_Error, itemPath, destFilePath);
//					}
//				}
//				else if (callback.mFileType == kFileType_File)
//				{
//					FilePathPtr destFilePath = NULL;
//					AppendToFilePath(inHub, mDestParentPath, inItemName, destFilePath);
//					if (destFilePath == NULL)
//					{
//						mCallback.Call(inHub, kFileSystemCopyStatus_Error, itemPath, destFilePath);
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
//						if (!mCallback.Call(inHub, kFileSystemCopyStatus_Error, itemPath, destFilePath))
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
//							if (!mCallback.Call(inHub, kFileSystemCopyStatus_Error, itemPath, destFilePath))
//							{
//								return false;
//							}
//						}
//						else
//						{
//							if (!mCallback.Call(inHub, kFileSystemCopyStatus_Success, itemPath, destFilePath))
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
//		const ConstHubPtr& inHub,
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
//			return inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
//		}
//
//		return true;
//	}

	//
	//
	bool VerifyDirectory(
		const ConstHubPtr& inHub,
		const FilePathPtr& inSourcePath,
		const FilePathPtr& inDestPath,
		const FileSystemCopyCallbackRef& inCallback)
	{
		Directory dir;
		ListDirectoryContents(
			inHub,
			inSourcePath,
			false,
			dir);

		if ((dir.mStatus != kListDirectoryContentsStatus_Success) &&
			(dir.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
		{
			LogFilePath(
				inHub,
				"VerifyDirectory(): ListDirectoryContents() failed for source path: ",
				inSourcePath);

			inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
			return false;
		}

		Directory dir2;
		ListDirectoryContents(
			inHub,
			inDestPath,
			false,
			dir2);

		if ((dir2.mStatus != kListDirectoryContentsStatus_Success) &&
			(dir2.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
		{
			LogFilePath(
				inHub,
				"VerifyDirectory(): ListDirectoryContents() failed for destination path: ",
				inDestPath);

			inCallback.Call(inHub, kFileSystemCopyStatus_VerifyError, inSourcePath, inDestPath);
			return false;
		}
			
		if (dir2.mFiles.size() != dir.mFiles.size())
		{
			if (DeleteDotDATFiles(inHub, dir2))
			{
				Directory dir3;
				ListDirectoryContents(
					inHub,
					inDestPath,
					false,
					dir3);

				if ((dir3.mStatus != kListDirectoryContentsStatus_Success) &&
					(dir3.mStatus != kListDirectoryContentsStatus_DirectoryEmpty))
				{
					LogFilePath(
						inHub,
						"CopyOneDirectory(): ListDirectoryContents() failed for 2nd attempt at destination path: ",
						inDestPath);

					inCallback.Call(inHub, kFileSystemCopyStatus_VerifyError, inSourcePath, inDestPath);
					return false;
				}
				dir2 = dir3;
			}
		}
			
		if (!dir.IsEqualTo(inHub, dir2))
		{
			LogFilePath(
				inHub,
				"CopyOneDirectory(): Directories not equal after copy: ",
				inDestPath);

			inCallback.Call(inHub, kFileSystemCopyStatus_VerifyFailed, inSourcePath, inDestPath);
			return false;
		}
		else
		{
			inCallback.Call(inHub, kFileSystemCopyStatus_Success, inSourcePath, inDestPath);
		}
		return true;
	}

} // private namespace

//
//
void FileSystemCopyWithVerify(
	const ConstHubPtr& inHub,
	const FilePathPtr& inSourcePath,
	const FilePathPtr& inDestPath,
	const FileSystemCopyCallbackRef& inCallback)
{
	FileExistsCallbackClass sourceFileExistsStatus ;
	FileExists(inHub, inSourcePath, sourceFileExistsStatus);
	if (!sourceFileExistsStatus.mSuccess)
	{
		LogFilePath(
			inHub,
			"FileSystemCopyWithVerify(): FileExists() failed for source path: ",
			inSourcePath);

		inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
		return;
	}
	if (!sourceFileExistsStatus.mExists)
	{
		LogFilePath(
			inHub,
			"FileSystemCopyWithVerify(): Source item doesn't exist at path: ",
			inSourcePath);
		inCallback.Call(inHub, kFileSystemCopyStatus_SourceNotFound, inSourcePath, inDestPath);
		return;
	}

	FileExistsCallbackClass destFileExistsStatus ;
	FileExists(inHub, inDestPath, destFileExistsStatus);
	if (!destFileExistsStatus.mSuccess)
	{
		LogFilePath(
			inHub,
			"FileSystemCopyWithVerify(): FileExists() failed for dest path: ",
			inDestPath);

		inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
		return;
	}
	if (destFileExistsStatus.mExists)
	{
		LogFilePath(
			inHub,
			"FileSystemCopyWithVerify(): Item already exists at dest path: ",
			inDestPath);

		inCallback.Call(inHub, kFileSystemCopyStatus_TargetAlreadyExists, inSourcePath, inDestPath);
		return;
	}
	
	GetTypeCallback callback;
	GetFileType(
		inHub,
		inSourcePath,
		callback);

	if (!callback.mSuccess)
	{
		LogFilePath(
			inHub,
			"FileSystemCopyWithVerify(): GetFileType() failed for path: ",
			inSourcePath);

		inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
	}
	else if (callback.mFileType == kFileType_Directory)
	{
		bool success = CopyOneDirectory(inHub, inSourcePath, inDestPath, inCallback);
		if (success)
		{
			VerifyDirectory(inHub, inSourcePath, inDestPath, inCallback);
		}
	}
	else if (callback.mFileType == kFileType_SymbolicLink)
	{
		CopyOneSymbolicLink(inHub, inSourcePath, inDestPath, inCallback);
	}
	else if (callback.mFileType == kFileType_File)
	{
		CopyOneFile(inHub, inSourcePath, inDestPath, inCallback);
	}
	else
	{
		LogFilePath(
			inHub,
			"FileSystemCopyWithVerify(): GetFileType() returned unexpected file type for path: ",
			inSourcePath);

		inCallback.Call(inHub, kFileSystemCopyStatus_Error, inSourcePath, inDestPath);
	}
}

} // namespace file
} // namespace hermit

#endif


