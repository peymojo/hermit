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
		namespace CompareFiles_Cocoa_Impl {
			
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
					NOTIFY_ERROR(h_, "CompareData: NSFileHandle fileHandleForReadingAtPath returned nil fileHandle for path :", inPath1);
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
					
					NOTIFY_ERROR(h_, "CompareData: NSFileHandle fileHandleForReadingAtPath returned nil fileHandle for path 2:", inPath2);
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
			enum class CompareAliasFileStatus {
				kMatch,
				kNonMatch,
				kCouldntResolveAliases,
				kError
			};
			
			//
			CompareAliasFileStatus CompareAliasFiles(const HermitPtr& h_, const FilePathPtr& filePath1, const FilePathPtr& filePath2) {
				GetAliasTargetCallbackClassT<FilePathPtr> aliasTarget1Callback;
				GetAliasTarget(h_, filePath1, aliasTarget1Callback);
				if ((aliasTarget1Callback.mStatus != kGetAliasTargetStatus_Success) &&
					(aliasTarget1Callback.mStatus != kGetAliasTargetStatus_TargetNotFound)) {
					NOTIFY_ERROR(h_, "CompareAliasFiles: GetAliasTarget failed for path:", filePath1);
					return CompareAliasFileStatus::kError;
				}
				
				GetAliasTargetCallbackClassT<FilePathPtr> aliasTarget2Callback;
				GetAliasTarget(h_, filePath2, aliasTarget2Callback);
				if ((aliasTarget2Callback.mStatus != kGetAliasTargetStatus_Success) &&
					(aliasTarget2Callback.mStatus != kGetAliasTargetStatus_TargetNotFound)) {
					NOTIFY_ERROR(h_, "CompareAliasFiles: GetAliasTarget failed for path:", filePath2);
					return CompareAliasFileStatus::kError;
				}
				
				if ((aliasTarget1Callback.mStatus == kGetAliasTargetStatus_TargetNotFound) ||
					(aliasTarget2Callback.mStatus == kGetAliasTargetStatus_TargetNotFound)) {
					if (aliasTarget1Callback.mStatus != aliasTarget2Callback.mStatus) {
						NOTIFY_ERROR(h_,
									 "CompareAliasFiles: GetAliasTarget target not found mismatch for path:", filePath1,
									 "and path:", filePath2);
						return CompareAliasFileStatus::kError;
					}
					return CompareAliasFileStatus::kCouldntResolveAliases;
				}
				
				FilePathPtr aliasTarget1Path(aliasTarget1Callback.mFilePath);
				FilePathPtr aliasTarget2Path(aliasTarget2Callback.mFilePath);
				
				if (!aliasTarget1Callback.mIsRelativePath && !aliasTarget2Callback.mIsRelativePath) {
					std::string canonicalPath1;
					GetCanonicalFilePathString(h_, aliasTarget1Path, canonicalPath1);
					
					std::string canonicalPath2;
					GetCanonicalFilePathString(h_, aliasTarget2Path, canonicalPath2);
					
					if (canonicalPath1 == canonicalPath2) {
						return CompareAliasFileStatus::kMatch;
					}
				}
				
				if (!aliasTarget1Callback.mIsRelativePath) {
					FilePathPtr parentPath;
					GetFilePathParent(h_, filePath1, parentPath);
					if (parentPath == nullptr) {
						NOTIFY_ERROR(h_, "CompareAliasFiles: GetFilePathParent failed for path:", filePath1);
						return CompareAliasFileStatus::kError;
					}
					
					GetRelativeFilePathCallbackClassT<FilePathPtr> getRelativePathCallback;
					GetRelativeFilePath(h_, parentPath, aliasTarget1Path, getRelativePathCallback);
					if (!getRelativePathCallback.mSuccess) {
						NOTIFY_ERROR(h_,
									 "CompareAliasFiles: GetRelativeFilePath failed for parent path:", parentPath,
									 "and target path:", aliasTarget1Path);
						return CompareAliasFileStatus::kError;
					}
					aliasTarget1Path = getRelativePathCallback.mFilePath;
				}
				
				if (!aliasTarget2Callback.mIsRelativePath) {
					FilePathPtr parentPath;
					GetFilePathParent(h_, filePath2, parentPath);
					if (parentPath == nullptr) {
						NOTIFY_ERROR(h_, "CompareAliasFiles: GetFilePathParent failed for path:", filePath2);
						return CompareAliasFileStatus::kError;
					}
					
					GetRelativeFilePathCallbackClassT<FilePathPtr> getRelativePathCallback;
					GetRelativeFilePath(h_, parentPath, aliasTarget2Path, getRelativePathCallback);
					if (!getRelativePathCallback.mSuccess) {
						NOTIFY_ERROR(h_,
									 "CompareAliasFiles: GetRelativeFilePath failed for parent path:", parentPath,
									 "and target path:", aliasTarget2Path);
						return CompareAliasFileStatus::kError;
					}
					aliasTarget2Path = getRelativePathCallback.mFilePath;
				}
				
				std::string canonicalPath1;
				GetCanonicalFilePathString(h_, aliasTarget1Path, canonicalPath1);
				
				std::string canonicalPath2;
				GetCanonicalFilePathString(h_, aliasTarget2Path, canonicalPath2);
				
				if (canonicalPath1 != canonicalPath2) {
					return CompareAliasFileStatus::kNonMatch;
				}
				return CompareAliasFileStatus::kMatch;
			}
			
			//
			class DirectoriesCompletion : public CompareDirectoriesCompletion {
			public:
				//
				DirectoriesCompletion(const HermitPtr& h_,
									  const FilePathPtr& filePath1,
									  const FilePathPtr& filePath2,
									  const CompareFilesCompletionPtr& completion) :
				mH_(h_),
				mFilePath1(filePath1),
				mFilePath2(filePath2),
				mCompletion(completion) {
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
			
		} // namespace CompareFiles_Cocoa_Impl
		using namespace CompareFiles_Cocoa_Impl;
		
		//
		void CompareFiles(const HermitPtr& h_,
						  const FilePathPtr& filePath1,
						  const FilePathPtr& filePath2,
						  const HardLinkMapPtr& hardLinkMap1,
						  const HardLinkMapPtr& hardLinkMap2,
						  const bool& ignoreDates,
						  const bool& ignoreFinderInfo,
						  const PreprocessFileFunctionPtr& preprocessFunction,
						  const CompareFilesCompletionPtr& completion) {
			FileType fileType1 = FileType::kUnknown;
			if (!GetFileType(h_, filePath1, fileType1)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileType failed for:", filePath1);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			FileType fileType2 = FileType::kUnknown;
			if (!GetFileType(h_, filePath2, fileType2)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileType failed for:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			if (fileType1 != fileType2) {
				FileNotificationParams params(kFileTypesDiffer, filePath1, filePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			if (fileType1 == FileType::kDirectory) {
				auto compareCompletion = std::make_shared<DirectoriesCompletion>(h_, filePath1, filePath2, completion);
				CompareDirectories(h_,
								   filePath1,
								   filePath2,
								   hardLinkMap1,
								   hardLinkMap2,
								   ignoreDates,
								   ignoreFinderInfo,
								   preprocessFunction,
								   compareCompletion);
				return;
			}
			if (fileType1 == FileType::kSymbolicLink) {
				auto status = CompareLinks(h_, filePath1, filePath2, ignoreDates);
				if (status == kCompareLinksStatus_Cancel) {
					completion->Call(CompareFilesStatus::kCancel);
					return;
				}
				if (status != kCompareLinksStatus_Success) {
					NOTIFY_ERROR(h_, "CompareFiles: CompareLinks failed for path 1:", filePath1, "path 2:", filePath2);
					completion->Call(CompareFilesStatus::kError);
					return;
				}
				completion->Call(CompareFilesStatus::kSuccess);
				return;
			}
			if (fileType1 != FileType::kFile) {
				NOTIFY_ERROR(h_, "CompareFiles: Unexpected file types for path 1:", filePath1, "path 2:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileIsDeviceCallbackClass isDeviceCallback1;
			GetFileIsDevice(h_, filePath1, isDeviceCallback1);
			if (!isDeviceCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsDevice failed for:", filePath1);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileIsDeviceCallbackClass isDeviceCallback2;
			GetFileIsDevice(h_, filePath2, isDeviceCallback2);
			if (!isDeviceCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsDevice failed for:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			bool match = true;
			bool isDevicePath = false;
			if (isDeviceCallback1.mIsDevice || isDeviceCallback2.mIsDevice) {
				isDevicePath = true;
				if (isDeviceCallback1.mIsDevice != isDeviceCallback2.mIsDevice) {
					match = false;
					FileNotificationParams params(kFileDeviceStatesDiffer,
												  filePath1,
												  filePath2,
												  isDeviceCallback1.mIsDevice,
												  isDeviceCallback2.mIsDevice);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				else {
					if (isDeviceCallback1.mDeviceMode != isDeviceCallback2.mDeviceMode) {
						match = false;
						FileNotificationParams params(kFileDeviceModesDiffer,
													  filePath1,
													  filePath2,
													  isDeviceCallback1.mDeviceMode,
													  isDeviceCallback2.mDeviceMode);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					if (isDeviceCallback1.mDeviceID != isDeviceCallback2.mDeviceID) {
						match = false;
						FileNotificationParams params(kFileDeviceIDsDiffer,
													  filePath1,
													  filePath2,
													  isDeviceCallback1.mDeviceID,
													  isDeviceCallback2.mDeviceID);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
				}
			}
			else {
				bool compareBits = true;
				
				PathIsHardLinkCallbackClass hardLinkCallback1;
				PathIsHardLink(h_, filePath1, hardLinkCallback1);
				if (!hardLinkCallback1.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: PathIsHardLink failed for:", filePath1);
					completion->Call(CompareFilesStatus::kError);
					return;
				}
				
				PathIsHardLinkCallbackClass hardLinkCallback2;
				PathIsHardLink(h_, filePath2, hardLinkCallback2);
				if (!hardLinkCallback2.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: PathIsHardLink failed for:", filePath2);
					completion->Call(CompareFilesStatus::kError);
					return;
				}
				
				if (hardLinkCallback1.mIsHardLink != hardLinkCallback2.mIsHardLink) {
					FileNotificationParams params(kIsHardLinkDiffers, filePath1, filePath2);
					NOTIFY(h_, kFilesDifferNotification, &params);
					completion->Call(CompareFilesStatus::kSuccess);
					return;
				}
				if (hardLinkCallback1.mIsHardLink) {
					bool hardLinkTargetsMatch = true;
					if (hardLinkCallback1.mFileSystemNumber != hardLinkCallback2.mFileSystemNumber) {
						hardLinkTargetsMatch = false;
						match = false;
						FileNotificationParams params(kHardLinkFileSystemNumDiffers, filePath1, filePath2);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					if (hardLinkCallback1.mFileNumber != hardLinkCallback2.mFileNumber) {
						hardLinkTargetsMatch = false;
						match = false;
						FileNotificationParams params(kHardLinkFileNumDiffers, filePath1, filePath2);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					if (hardLinkTargetsMatch) {
						compareBits = false;
					}
				}
				else {
					PathIsAliasCallbackClass isAlias1Callback;
					PathIsAlias(h_, filePath1, isAlias1Callback);
					if (!isAlias1Callback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: PathIsAlias failed for:", filePath1);
						completion->Call(CompareFilesStatus::kError);
						return;
					}
					
					PathIsAliasCallbackClass isAlias2Callback;
					PathIsAlias(h_, filePath2, isAlias2Callback);
					if (!isAlias2Callback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: PathIsAlias failed for:", filePath2);
						completion->Call(CompareFilesStatus::kError);
						return;
					}
					
					if (isAlias1Callback.mIsAlias != isAlias2Callback.mIsAlias) {
						FileNotificationParams params(kIsAliasDiffers, filePath1, filePath2);
						NOTIFY(h_, kFilesDifferNotification, &params);
						completion->Call(CompareFilesStatus::kSuccess);
						return;
					}
					
					bool isAliasMatch = false;
					if (isAlias1Callback.mIsAlias) {
						compareBits = false;
						CompareAliasFileStatus compareAliasStatus = CompareAliasFiles(h_, filePath1, filePath2);
						if (compareAliasStatus == CompareAliasFileStatus::kError) {
							NOTIFY_ERROR(h_, "CompareFiles: CompareAliasFiles failed for:", filePath1, "and:", filePath2);
							completion->Call(CompareFilesStatus::kError);
							return;
						}
						
						if (compareAliasStatus == CompareAliasFileStatus::kNonMatch) {
							match = false;
							FileNotificationParams params(kAliasTargetsDiffer, filePath1, filePath2);
							NOTIFY(h_, kFilesDifferNotification, &params);
						}
						else if (compareAliasStatus == CompareAliasFileStatus::kMatch) {
							isAliasMatch = true;
						}
					}
				}
				
				if (compareBits) {
					GetFileTotalSizeCallbackClass sizeCallback;
					GetFileTotalSize(h_, filePath1, sizeCallback);
					if (!sizeCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: GetFileForkSize failed for:", filePath1);
						completion->Call(CompareFilesStatus::kError);
						return;
					}
					uint64_t fileSize1 = sizeCallback.mTotalSize;
					
					GetFileTotalSize(h_, filePath2, sizeCallback);
					if (!sizeCallback.mSuccess) {
						NOTIFY_ERROR(h_, "CompareFiles: GetFileForkSize failed for:", filePath2);
						completion->Call(CompareFilesStatus::kError);
						return;
					}
					uint64_t fileSize2 = sizeCallback.mTotalSize;
					
					if (fileSize1 != fileSize2) {
						match = false;
						FileNotificationParams params(kFileSizesDiffer, filePath1, filePath2, fileSize1, fileSize2);
						NOTIFY(h_, kFilesDifferNotification, &params);
					}
					else if (fileSize1 > 0) {
						@autoreleasepool {
							uint64_t differenceOffset = 0;
							bool dataMatch = false;
							bool result = CompareData(h_, filePath1, filePath2, dataMatch, differenceOffset);
							if (!result) {
								completion->Call(CompareFilesStatus::kError);
								return;
							}
							if (!dataMatch) {
								match = false;
								FileNotificationParams params(kFileContentsDiffer, filePath1, filePath2, differenceOffset, 0);
								NOTIFY(h_, kFilesDifferNotification, &params);
							}
						}
					}
				}
			}
			
			GetFileIsLockedCallbackClass lockedCallback1;
			GetFileIsLocked(h_, filePath1, lockedCallback1);
			if (!lockedCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsLocked failed for:", filePath1);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileIsLockedCallbackClass lockedCallback2;
			GetFileIsLocked(h_, filePath2, lockedCallback2);
			if (!lockedCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileIsLocked failed for:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (lockedCallback1.mIsLocked != lockedCallback2.mIsLocked) {
				match = false;
				FileNotificationParams params(kLockedStatesDiffer, filePath1, filePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			GetFileBSDFlagsCallbackClass bsdFlagsCallback1;
			GetFileBSDFlags(h_, filePath1, bsdFlagsCallback1);
			if (!bsdFlagsCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileBSDFlags failed for:", filePath1);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileBSDFlagsCallbackClass bsdFlagsCallback2;
			GetFileBSDFlags(h_, filePath2, bsdFlagsCallback2);
			if (!bsdFlagsCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileBSDFlags failed for:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (bsdFlagsCallback1.mFlags != bsdFlagsCallback2.mFlags) {
				match = false;
				FileNotificationParams params(kBSDFlagsDiffer, filePath1, filePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			GetFileACLCallbackClass<std::string> aclCallback1;
			GetFileACL(h_, filePath1, aclCallback1);
			if (!aclCallback1.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileACL failed for:", filePath1);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			GetFileACLCallbackClass<std::string> aclCallback2;
			GetFileACL(h_, filePath2, aclCallback2);
			if (!aclCallback2.mSuccess) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFileACL failed for:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (aclCallback1.mACL != aclCallback2.mACL) {
				match = false;
				FileNotificationParams params(kACLsDiffer, filePath1, filePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			if (!isDevicePath) {
                bool xattrsMatch = false;
                auto result = CompareXAttrs(h_, filePath1, filePath2, xattrsMatch);
                if (result != CompareXAttrsResult::kSuccess) {
                    NOTIFY_ERROR(h_, "CompareXAttrs failed for:", filePath1);
                    completion->Call(CompareFilesStatus::kError);
                    return;
                }
                if (!xattrsMatch) {
                    match = false;
                }
			}
			
			uint32_t permissions1 = 0;
			if (!GetFilePosixPermissions(h_, filePath1, permissions1)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixPermissions failed for path 1:", filePath1);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			uint32_t permissions2 = 0;
			if (!GetFilePosixPermissions(h_, filePath2, permissions2)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixPermissions failed for path 2:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (permissions1 != permissions2) {
				match = false;
				FileNotificationParams params(kPermissionsDiffer, filePath1, filePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			std::string userOwner1;
			std::string groupOwner1;
			if (!GetFilePosixOwnership(h_, filePath1, userOwner1, groupOwner1)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixOwnership failed for path 1:", filePath1);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			std::string userOwner2;
			std::string groupOwner2;
			if (!GetFilePosixOwnership(h_, filePath2, userOwner2, groupOwner2)) {
				NOTIFY_ERROR(h_, "CompareFiles: GetFilePosixOwnership failed for path 2:", filePath2);
				completion->Call(CompareFilesStatus::kError);
				return;
			}
			
			if (userOwner1 != userOwner2) {
				match = false;
				FileNotificationParams params(kUserOwnersDiffer,
											  filePath1,
											  filePath2,
											  userOwner1,
											  userOwner2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			if (groupOwner1 != groupOwner2) {
				match = false;
				FileNotificationParams params(kGroupOwnersDiffer,
											  filePath1,
											  filePath2,
											  groupOwner1,
											  groupOwner2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			if (!isDevicePath && !ignoreFinderInfo) {
				auto status = CompareFinderInfo(h_, filePath1, filePath2);
				if ((status != kCompareFinderInfoStatus_Match) &&
					(status != kCompareFinderInfoStatus_FinderInfosDiffer)) {
					NOTIFY_ERROR(h_,
								 "CompareFiles: CompareFinderInfo failed for path 1:", filePath1,
								 "and path 2:", filePath2);
					FileNotificationParams params(kErrorComparingFinderInfo, filePath1, filePath2);
					NOTIFY(h_, kFileErrorNotification, &params);
					completion->Call(CompareFilesStatus::kError);
					return;
				}
				
				if (status == kCompareFinderInfoStatus_FinderInfosDiffer) {
					match = false;
					FileNotificationParams params(kFinderInfosDiffer, filePath1, filePath2);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
			}
			
			if (!ignoreDates) {
				GetFileDatesCallbackClassT<std::string> datesCallback1;
				GetFileDates(h_, filePath1, datesCallback1);
				if (!datesCallback1.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: GetFileDates failed for:", filePath1);
					completion->Call(CompareFilesStatus::kError);
					return;
				}
				
				GetFileDatesCallbackClassT<std::string> datesCallback2;
				GetFileDates(h_, filePath2, datesCallback2);
				if (!datesCallback2.mSuccess) {
					NOTIFY_ERROR(h_, "CompareFiles: GetFileDates failed for:", filePath2);
					completion->Call(CompareFilesStatus::kError);
					return;
				}
				
				if (datesCallback1.mCreationDate != datesCallback2.mCreationDate) {
					match = false;
					FileNotificationParams params(kCreationDatesDiffer,
												  filePath1,
												  filePath2,
												  datesCallback1.mCreationDate,
												  datesCallback2.mCreationDate);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				
				if (datesCallback1.mModificationDate != datesCallback2.mModificationDate) {
					match = false;
					FileNotificationParams params(kModificationDatesDiffer,
												  filePath1,
												  filePath2,
												  datesCallback1.mModificationDate,
												  datesCallback2.mModificationDate);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
			}
			
			if (match) {
				FileNotificationParams params(kFilesMatch, filePath1, filePath2);
				NOTIFY(h_, kFilesMatchNotification, &params);
			}
			
			completion->Call(CompareFilesStatus::kSuccess);
		}
		
	} // namespace file
} // namespace hermit

