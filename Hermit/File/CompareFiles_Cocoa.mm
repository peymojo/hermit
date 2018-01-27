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

#import <Cocoa/Cocoa.h>
#import <string>
#import "Hermit/Foundation/CompareMemory.h"
#import "Hermit/Foundation/Notification.h"
#import "CompareFinderInfo.h"
#import "CompareLinks.h"
#import "CompareXAttrs.h"
#import "FileNotification.h"
#import "FilePathToCocoaPathString.h"
#import "GetAliasTarget.h"
#import "GetCanonicalFilePathString.h"
#import "GetFileACL.h"
#import "GetFileBSDFlags.h"
#import "GetFileDates.h"
#import "GetFileIsDevice.h"
#import "GetFileIsLocked.h"
#import "GetFilePathParent.h"
#import "GetFilePosixOwnership.h"
#import "GetFilePosixPermissions.h"
#import "GetFileTotalSize.h"
#import "GetFileType.h"
#import "GetRelativeFilePath.h"
#import "PathIsAlias.h"
#import "PathIsHardLink.h"
#import "CompareDirectories.h"
#import "CompareFiles.h"

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			bool CompareData(const HermitPtr& h_,
							 const FilePathPtr& inPath1,
							 const FilePathPtr& inPath2,
							 bool& outMatch,
							 uint64_t& outDifferenceOffset) {
				
				std::string path1UTF8;
				FilePathToCocoaPathString(h_, inPath1, path1UTF8);
				
				NSString* path1String = [NSString stringWithUTF8String:path1UTF8.c_str()];
				NSError* error = nil;
				NSFileHandle* file1Handle = [NSFileHandle fileHandleForReadingFromURL:[NSURL fileURLWithPath:path1String] error:&error];
				if (error != nil) {
					NSInteger errorCode = [error code];
					NOTIFY_ERROR(h_,
								 "CompareData: NSFileHandle fileHandleForReadingFromURL failed for path 1:",
								 inPath1,
								 "error code:",
								 (int32_t)errorCode,
								 "error:",
								 [[error localizedDescription] UTF8String]);
					return false;
				}
				if (file1Handle == nil) {
					NOTIFY_ERROR(h_, "CompareData: NSFileHandle fileHandleForReadingAtPath returned nil fileHandle for path : ", inPath1);
					return false;
				}
				
				std::string path2UTF8;
				FilePathToCocoaPathString(h_, inPath2, path2UTF8);
				
				NSString* path2String = [NSString stringWithUTF8String:path2UTF8.c_str()];
				NSFileHandle* file2Handle = [NSFileHandle fileHandleForReadingFromURL:[NSURL fileURLWithPath:path2String] error:&error];
				if (error != nil) {
					NSInteger errorCode = [error code];
					NOTIFY_ERROR(h_,
								 "CompareData: NSFileHandle fileHandleForReadingFromURL failed for path 2:",
								 inPath2,
								 "error code:",
								 (int32_t)errorCode,
								 "error",
								 [[error localizedDescription] UTF8String]);
					return false;
				}
				if (file2Handle == nil) {
					[file1Handle closeFile];
					
					NOTIFY_ERROR(h_, "CompareData: NSFileHandle fileHandleForReadingAtPath returned nil fileHandle for path 2: ", inPath2);
					return false;
				}
				
				bool failed = false;
				uint64_t matchingBytes = 0;
				bool match = false;
				static const int kBufferSize = 32768;
				while (true) {
					@autoreleasepool {
						@try {
							NSData* data1 = nil;
							@try {
								data1 = [file1Handle readDataOfLength:kBufferSize];
							}
							@catch (NSException* ex) {
								NOTIFY_ERROR(h_,
											 "CompareData: exception during [file1Handle readDataOfLength] for path:",
											 inPath1,
											 "exception:",
											 [[ex name] UTF8String],
											 "reason:",
											 [[ex reason] UTF8String]);
								
								failed = true;
								break;
							}
							uint64_t actualCount1 = [data1 length];
							
							NSData* data2 = nil;
							@try {
								data2 = [file2Handle readDataOfLength:kBufferSize];
							}
							@catch (NSException* ex) {
								NOTIFY_ERROR(h_,
											 "CompareData: exception during [file2Handle readDataOfLength] for path:",
											 inPath2,
											 "exception:",
											 [[ex name] UTF8String],
											 "reason:",
											 [[ex reason] UTF8String]);
								
								failed = true;
								break;
							}
							uint64_t actualCount2 = [data2 length];
							
							if (actualCount1 != actualCount2) {
								break;
							}
							if (actualCount1 == 0) {
								match = true;
								break;
							}
							
							typedef const char* const_char_ptr;
							const_char_ptr p1 = (const char*)[data1 bytes];
							const_char_ptr p2 = (const char*)[data2 bytes];
							size_t size = actualCount1;
							
							CompareMemoryCallbackClass compareCallback;
							CompareMemory(p1, p2, size, compareCallback);
							if (!compareCallback.mMatch) {
								matchingBytes += compareCallback.mBytesThatMatch;
								break;
							}
							if (actualCount1 < kBufferSize) {
								match = true;
								break;
							}
							matchingBytes += actualCount1;
						}
						@catch (NSException* ex) {
							NOTIFY_ERROR(h_,
										 "CompareData: Inner Exception:",
										 [[ex name] UTF8String],
										 "reason:",
										 [[ex reason] UTF8String]);
							failed = true;
							break;
						}
					}
				}
				[file1Handle closeFile];
				[file2Handle closeFile];
				
				if (failed) {
					return false;
				}
				if (!match) {
					outDifferenceOffset = matchingBytes;
				}
				outMatch = match;
				return true;
			}
			
			//
			enum CompareAliasFileStatus
			{
				kCompareAliasFileStatus_Match,
				kCompareAliasFileStatus_NonMatch,
				kCompareAliasFileStatus_CouldntResolveAliases,
				kCompareAliseFileStatus_Error
			};
			
			//
			CompareAliasFileStatus CompareAliasFiles(const HermitPtr& h_,
													 const FilePathPtr& inFilePath1,
													 const FilePathPtr& inFilePath2) {
				
				GetAliasTargetCallbackClassT<FilePathPtr> aliasTarget1Callback;
				GetAliasTarget(h_, inFilePath1, aliasTarget1Callback);
				if ((aliasTarget1Callback.mStatus != kGetAliasTargetStatus_Success) &&
					(aliasTarget1Callback.mStatus != kGetAliasTargetStatus_TargetNotFound)) {
					NOTIFY_ERROR(h_, "CompareAliasFiles: GetAliasTarget failed for path: ", inFilePath1);
					return kCompareAliseFileStatus_Error;
				}
				
				GetAliasTargetCallbackClassT<FilePathPtr> aliasTarget2Callback;
				GetAliasTarget(h_, inFilePath2, aliasTarget2Callback);
				if ((aliasTarget2Callback.mStatus != kGetAliasTargetStatus_Success) &&
					(aliasTarget2Callback.mStatus != kGetAliasTargetStatus_TargetNotFound)) {
					NOTIFY_ERROR(h_, "CompareAliasFiles: GetAliasTarget failed for path: ", inFilePath2);
					return kCompareAliseFileStatus_Error;
				}
				
				if ((aliasTarget1Callback.mStatus == kGetAliasTargetStatus_TargetNotFound) ||
					(aliasTarget2Callback.mStatus == kGetAliasTargetStatus_TargetNotFound)) {
					if (aliasTarget1Callback.mStatus != aliasTarget2Callback.mStatus) {
						NOTIFY_ERROR(h_,
									 "CompareAliasFiles: GetAliasTarget target not found mismatch for path:",
									 inFilePath1,
									 "and path:",
									 inFilePath2);
						return kCompareAliseFileStatus_Error;
					}
					return kCompareAliasFileStatus_CouldntResolveAliases;
				}
				
				FilePathPtr aliasTarget1Path(aliasTarget1Callback.mFilePath);
				FilePathPtr aliasTarget2Path(aliasTarget2Callback.mFilePath);
				
				if (!aliasTarget1Callback.mIsRelativePath && !aliasTarget2Callback.mIsRelativePath) {
					std::string canonicalPath1;
					GetCanonicalFilePathString(h_, aliasTarget1Path, canonicalPath1);
					
					std::string canonicalPath2;
					GetCanonicalFilePathString(h_, aliasTarget2Path, canonicalPath2);
					
					if (canonicalPath1 == canonicalPath2) {
						return kCompareAliasFileStatus_Match;
					}
				}
				
				if (!aliasTarget1Callback.mIsRelativePath) {
					FilePathPtr parentPath;
					GetFilePathParent(h_, inFilePath1, parentPath);
					if (parentPath == nullptr) {
						NOTIFY_ERROR(h_, "CompareAliasFiles: GetFilePathParent failed for path: ", inFilePath1);
						return kCompareAliseFileStatus_Error;
					}
					
					GetRelativeFilePathCallbackClassT<FilePathPtr> getRelativePathCallback;
					GetRelativeFilePath(h_, parentPath, aliasTarget1Path, getRelativePathCallback);
					if (!getRelativePathCallback.mSuccess) {
						NOTIFY_ERROR(h_,
									 "CompareAliasFiles: GetRelativeFilePath failed for parent path:",
									 parentPath,
									 "and target path:",
									 aliasTarget1Path);
						return kCompareAliseFileStatus_Error;
					}
					aliasTarget1Path = getRelativePathCallback.mFilePath;
				}
				
				if (!aliasTarget2Callback.mIsRelativePath) {
					FilePathPtr parentPath;
					GetFilePathParent(h_, inFilePath2, parentPath);
					if (parentPath == nullptr) {
						NOTIFY_ERROR(h_, "CompareAliasFiles: GetFilePathParent failed for path: ", inFilePath2);
						return kCompareAliseFileStatus_Error;
					}
					
					GetRelativeFilePathCallbackClassT<FilePathPtr> getRelativePathCallback;
					GetRelativeFilePath(h_, parentPath, aliasTarget2Path, getRelativePathCallback);
					if (!getRelativePathCallback.mSuccess) {
						NOTIFY_ERROR(h_,
									 "CompareAliasFiles: GetRelativeFilePath failed for parent path: ",
									 parentPath,
									 "and target path:",
									 aliasTarget2Path);
						return kCompareAliseFileStatus_Error;
					}
					aliasTarget2Path = getRelativePathCallback.mFilePath;
				}
				
				std::string canonicalPath1;
				GetCanonicalFilePathString(h_, aliasTarget1Path, canonicalPath1);
				
				std::string canonicalPath2;
				GetCanonicalFilePathString(h_, aliasTarget2Path, canonicalPath2);
				
				if (canonicalPath1 != canonicalPath2)
				{
					return kCompareAliasFileStatus_NonMatch;
				}
				return kCompareAliasFileStatus_Match;
			}
			
			//
			class DirectoriesCompletion : public CompareDirectoriesCompletion {
			public:
				//
				DirectoriesCompletion(const HermitPtr& h_,
									  const FilePathPtr& inFilePath1,
									  const FilePathPtr& inFilePath2,
									  const CompareFilesCompletionPtr& inCompletion) :
				mH_(h_),
				mFilePath1(inFilePath1),
				mFilePath2(inFilePath2),
				mCompletion(inCompletion) {
				}
				
				//
				virtual void Call(const CompareDirectoriesStatus& status) override {
					if (status == CompareDirectoriesStatus::kCancel) {
						mCompletion->Call(CompareFilesStatus::kCancel);
						return;
					}
					if (status != CompareDirectoriesStatus::kSuccess) {
						NOTIFY_ERROR(mH_, "CompareFiles: CompareDirectories failed, path 1:", mFilePath1, "path 2:", mFilePath2);
						mCompletion->Call(CompareFilesStatus::kError);
						return;
					}
					mCompletion->Call(CompareFilesStatus::kSuccess);
				}
				
				//
				HermitPtr mH_;
				FilePathPtr mFilePath1;
				FilePathPtr mFilePath2;
				CompareFilesCompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void CompareFiles(const HermitPtr& h_,
						  const FilePathPtr& inFilePath1,
						  const FilePathPtr& inFilePath2,
						  const bool& inIgnoreDates,
						  const bool& inIgnoreFinderInfo,
						  const PreprocessFileFunctionPtr& inPreprocessFunction,
						  const CompareFilesCompletionPtr& inCompletion) {
			
			FileType fileType1 = FileType::kUnknown;
			if (!GetFileType(h_, inFilePath1, fileType1)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileType failed for:", inFilePath1);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			FileType fileType2 = FileType::kUnknown;
			if (!GetFileType(h_, inFilePath2, fileType2)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileType failed for: ", inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			if (fileType1 != fileType2) {
				FileNotificationParams params(kFileTypesDiffer, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			if (fileType1 == FileType::kDirectory) {
				auto completion = std::make_shared<DirectoriesCompletion>(h_, inFilePath1, inFilePath2, inCompletion);
				CompareDirectories(h_, inFilePath1, inFilePath2, inIgnoreDates, inIgnoreFinderInfo, inPreprocessFunction, completion);
				return;
			}
			if (fileType1 == FileType::kSymbolicLink) {
				auto status = CompareLinks(h_, inFilePath1, inFilePath2, inIgnoreDates);
				if (status == kCompareLinksStatus_Cancel) {
					inCompletion->Call(CompareFilesStatus::kCancel);
					return;
				}
				if (status != kCompareLinksStatus_Success) {
					NOTIFY_ERROR(h_,
								 "CompareFiles: CompareLinks failed for path 1:",
								 inFilePath1,
								 "path 2:",
								 inFilePath2);
					inCompletion->Call(CompareFilesStatus::kError);
					return;
				}
				inCompletion->Call(CompareFilesStatus::kSuccess);
				return;
			}
			if (fileType1 != FileType::kFile) {
				NOTIFY_ERROR(h_,
							 "CompareFiles: Unexpected file types for path 1:",
							 inFilePath1,
							 "path 2:",
							 inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileIsDeviceCallbackClass isDeviceCallback1;
			GetFileIsDevice(h_, inFilePath1, isDeviceCallback1);
			if (!isDeviceCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsDevice failed for:", inFilePath1);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileIsDeviceCallbackClass isDeviceCallback2;
			GetFileIsDevice(h_, inFilePath2, isDeviceCallback2);
			if (!isDeviceCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsDevice failed for: ", inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			bool match = true;
			bool isDevicePath = false;
			if (isDeviceCallback1.mIsDevice || isDeviceCallback2.mIsDevice) {
				isDevicePath = true;
				if (isDeviceCallback1.mIsDevice != isDeviceCallback2.mIsDevice) {
					match = false;
					FileNotificationParams params(kFileDeviceStatesDiffer,
												  inFilePath1,
												  inFilePath2,
												  isDeviceCallback1.mIsDevice,
												  isDeviceCallback2.mIsDevice);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				else {
					if (isDeviceCallback1.mDeviceMode != isDeviceCallback2.mDeviceMode) {
						match = false;
						FileNotificationParams params(kFileDeviceModesDiffer,
													  inFilePath1,
													  inFilePath2,
													  isDeviceCallback1.mDeviceMode,
													  isDeviceCallback2.mDeviceMode);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					if (isDeviceCallback1.mDeviceID != isDeviceCallback2.mDeviceID) {
						match = false;
						FileNotificationParams params(kFileDeviceIDsDiffer,
													  inFilePath1,
													  inFilePath2,
													  isDeviceCallback1.mDeviceID,
													  isDeviceCallback2.mDeviceID);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
				}
			}
			else {
				bool compareBits = true;
				
				PathIsHardLinkCallbackClass hardLinkCallback1;
				PathIsHardLink(h_, inFilePath1, hardLinkCallback1);
				if (!hardLinkCallback1.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: PathIsHardLink failed for: ", inFilePath1);
					inCompletion->Call(CompareFilesStatus::kError);
					return;
				}
				
				PathIsHardLinkCallbackClass hardLinkCallback2;
				PathIsHardLink(h_, inFilePath2, hardLinkCallback2);
				if (!hardLinkCallback2.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: PathIsHardLink failed for: ", inFilePath2);
					inCompletion->Call(CompareFilesStatus::kError);
					return;
				}
				
				if (hardLinkCallback1.mIsHardLink != hardLinkCallback2.mIsHardLink) {
					FileNotificationParams params(kIsHardLinkDiffers, inFilePath1, inFilePath2);
					NOTIFY(h_, kFilesDifferNotification, &params);
					inCompletion->Call(CompareFilesStatus::kSuccess);
					return;
				}
				if (hardLinkCallback1.mIsHardLink) {
					bool hardLinkTargetsMatch = true;
					if (hardLinkCallback1.mFileSystemNumber != hardLinkCallback2.mFileSystemNumber) {
						hardLinkTargetsMatch = false;
						match = false;
						FileNotificationParams params(kHardLinkFileSystemNumDiffers, inFilePath1, inFilePath2);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					if (hardLinkCallback1.mFileNumber != hardLinkCallback2.mFileNumber) {
						hardLinkTargetsMatch = false;
						match = false;
						FileNotificationParams params(kHardLinkFileNumDiffers, inFilePath1, inFilePath2);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					if (hardLinkTargetsMatch) {
						compareBits = false;
					}
				}
				else {
					PathIsAliasCallbackClass isAlias1Callback;
					PathIsAlias(h_, inFilePath1, isAlias1Callback);
					if (!isAlias1Callback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: PathIsAlias failed for: ", inFilePath1);
						inCompletion->Call(CompareFilesStatus::kError);
						return;
					}
					
					PathIsAliasCallbackClass isAlias2Callback;
					PathIsAlias(h_, inFilePath2, isAlias2Callback);
					if (!isAlias2Callback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: PathIsAlias failed for: ", inFilePath2);
						inCompletion->Call(CompareFilesStatus::kError);
						return;
					}
					
					if (isAlias1Callback.mIsAlias != isAlias2Callback.mIsAlias) {
						FileNotificationParams params(kIsAliasDiffers, inFilePath1, inFilePath2);
						NOTIFY(h_, kFilesDifferNotification, &params);
						inCompletion->Call(CompareFilesStatus::kSuccess);
						return;
					}
					
					bool isAliasMatch = false;
					if (isAlias1Callback.mIsAlias) {
						compareBits = false;
						CompareAliasFileStatus compareAliasStatus = CompareAliasFiles(h_, inFilePath1, inFilePath2);
						if (compareAliasStatus == kCompareAliseFileStatus_Error) {
							NOTIFY_ERROR(h_, "CompareFiles: CompareAliasFiles failed for: ", inFilePath1);
							NOTIFY_ERROR(h_, "-- and: ", inFilePath2);
							inCompletion->Call(CompareFilesStatus::kError);
							return;
						}
						
						if (compareAliasStatus == kCompareAliasFileStatus_NonMatch) {
							match = false;
							FileNotificationParams params(kAliasTargetsDiffer, inFilePath1, inFilePath2);
							NOTIFY(h_, kFilesDifferNotification, &params);
						}
						else if (compareAliasStatus == kCompareAliasFileStatus_Match) {
							isAliasMatch = true;
						}
					}
				}
				
				if (compareBits) {
					GetFileTotalSizeCallbackClass sizeCallback;
					GetFileTotalSize(h_, inFilePath1, sizeCallback);
					if (!sizeCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: GetFileForkSize failed for: ", inFilePath1);
						inCompletion->Call(CompareFilesStatus::kError);
						return;
					}
					uint64_t fileSize1 = sizeCallback.mTotalSize;
					
					GetFileTotalSize(h_, inFilePath2, sizeCallback);
					if (!sizeCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: GetFileForkSize failed for: ", inFilePath2);
						inCompletion->Call(CompareFilesStatus::kError);
						return;
					}
					uint64_t fileSize2 = sizeCallback.mTotalSize;
					
					if (fileSize1 != fileSize2) {
						match = false;
						FileNotificationParams params(kFileSizesDiffer, inFilePath1, inFilePath2, fileSize1, fileSize2);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					else if (fileSize1 > 0) {
						@autoreleasepool {
							uint64_t differenceOffset = 0;
							bool dataMatch = false;
							bool result = CompareData(h_, inFilePath1, inFilePath2, dataMatch, differenceOffset);
							if (!result) {
								inCompletion->Call(CompareFilesStatus::kError);
								return;
							}
							if (!dataMatch) {
								match = false;
								FileNotificationParams params(kFileContentsDiffer, inFilePath1, inFilePath2, differenceOffset, 0);
								NOTIFY(h_, kFilesDifferNotification, &params);
							}
						}
					}
				}
			}
			
			GetFileIsLockedCallbackClass lockedCallback1;
			GetFileIsLocked(h_, inFilePath1, lockedCallback1);
			if (!lockedCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsLocked failed for: ", inFilePath1);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileIsLockedCallbackClass lockedCallback2;
			GetFileIsLocked(h_, inFilePath2, lockedCallback2);
			if (!lockedCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsLocked failed for: ", inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (lockedCallback1.mIsLocked != lockedCallback2.mIsLocked) {
				match = false;
				FileNotificationParams params(kLockedStatesDiffer, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			GetFileBSDFlagsCallbackClass bsdFlagsCallback1;
			GetFileBSDFlags(h_, inFilePath1, bsdFlagsCallback1);
			if (!bsdFlagsCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileBSDFlags failed for: ", inFilePath1);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileBSDFlagsCallbackClass bsdFlagsCallback2;
			GetFileBSDFlags(h_, inFilePath2, bsdFlagsCallback2);
			if (!bsdFlagsCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileBSDFlags failed for: ", inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (bsdFlagsCallback1.mFlags != bsdFlagsCallback2.mFlags) {
				match = false;
				FileNotificationParams params(kBSDFlagsDiffer, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			GetFileACLCallbackClass<std::string> aclCallback1;
			GetFileACL(h_, inFilePath1, aclCallback1);
			if (!aclCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileACL failed for: ", inFilePath1);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileACLCallbackClass<std::string> aclCallback2;
			GetFileACL(h_, inFilePath2, aclCallback2);
			if (!aclCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileACL failed for: ", inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (aclCallback1.mACL != aclCallback2.mACL) {
				match = false;
				FileNotificationParams params(kACLsDiffer, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			if (!isDevicePath) {
                bool xattrsMatch = false;
                auto result = CompareXAttrs(h_, inFilePath1, inFilePath2, xattrsMatch);
                if (result != CompareXAttrsResult::kSuccess) {
                    NOTIFY_ERROR(h_, "CompareXAttrs failed for:", inFilePath1);
                    inCompletion->Call(CompareFilesStatus::kError);
                    return;
                }
                if (!xattrsMatch) {
                    match = false;
                }
			}
			
			uint32_t permissions1 = 0;
			if (!GetFilePosixPermissions(h_, inFilePath1, permissions1)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixPermissions failed for path 1: ", inFilePath1);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			uint32_t permissions2 = 0;
			if (!GetFilePosixPermissions(h_, inFilePath2, permissions2)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixPermissions failed for path 2: ", inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (permissions1 != permissions2) {
				match = false;
				FileNotificationParams params(kPermissionsDiffer, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			std::string userOwner1;
			std::string groupOwner1;
			if (!GetFilePosixOwnership(h_, inFilePath1, userOwner1, groupOwner1)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixOwnership failed for path 1: ", inFilePath1);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			std::string userOwner2;
			std::string groupOwner2;
			if (!GetFilePosixOwnership(h_, inFilePath2, userOwner2, groupOwner2)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixOwnership failed for path 2: ", inFilePath2);
				inCompletion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (userOwner1 != userOwner2) {
				match = false;
				FileNotificationParams params(kUserOwnersDiffer,
											  inFilePath1,
											  inFilePath2,
											  userOwner1,
											  userOwner2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			if (groupOwner1 != groupOwner2) {
				match = false;
				FileNotificationParams params(kGroupOwnersDiffer,
											  inFilePath1,
											  inFilePath2,
											  groupOwner1,
											  groupOwner2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			if (!isDevicePath && !inIgnoreFinderInfo) {
				auto status = CompareFinderInfo(h_, inFilePath1, inFilePath2);
				if ((status != kCompareFinderInfoStatus_Match) &&
					(status != kCompareFinderInfoStatus_FinderInfosDiffer)) {
					NOTIFY_ERROR(h_,
								 "CompareFiles: CompareFinderInfo failed for path 1:",
								 inFilePath1,
								 "and path 2:",
								 inFilePath2);
					FileNotificationParams params(kErrorComparingFinderInfo, inFilePath1, inFilePath2);
					NOTIFY(h_, kFileErrorNotification, &params);
					inCompletion->Call(CompareFilesStatus::kError);
					return;
				}
				
				if (status == kCompareFinderInfoStatus_FinderInfosDiffer) {
					match = false;
					FileNotificationParams params(kFinderInfosDiffer, inFilePath1, inFilePath2);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
			}
			
			if (!inIgnoreDates) {
				GetFileDatesCallbackClassT<std::string> datesCallback1;
				GetFileDates(h_, inFilePath1, datesCallback1);
				if (!datesCallback1.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: GetFileDates failed for: ", inFilePath1);
					inCompletion->Call(CompareFilesStatus::kError);
					return;
				}
				
				GetFileDatesCallbackClassT<std::string> datesCallback2;
				GetFileDates(h_, inFilePath2, datesCallback2);
				if (!datesCallback2.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: GetFileDates failed for: ", inFilePath2);
					inCompletion->Call(CompareFilesStatus::kError);
					return;
				}
				
				if (datesCallback1.mCreationDate != datesCallback2.mCreationDate) {
					match = false;
					FileNotificationParams params(kCreationDatesDiffer,
												  inFilePath1,
												  inFilePath2,
												  datesCallback1.mCreationDate,
												  datesCallback2.mCreationDate);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				
				if (datesCallback1.mModificationDate != datesCallback2.mModificationDate) {
					match = false;
					FileNotificationParams params(kModificationDatesDiffer,
												  inFilePath1,
												  inFilePath2,
												  datesCallback1.mModificationDate,
												  datesCallback2.mModificationDate);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
			}
			
			if (match) {
				FileNotificationParams params(kFilesMatch, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesMatchNotification, &params);
			}
			
			inCompletion->Call(CompareFilesStatus::kSuccess);
		}
		
	} // namespace file
} // namespace hermit

