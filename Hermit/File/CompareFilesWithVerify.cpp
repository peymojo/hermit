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

#include <string>
#include <map>
#include "Hermit/Encoding+File/CalculateFileForkCRC32.h"
#include "Hermit/Encoding+File/CalculateFileForkCRC32s.h"
#include "CompareFiles.h"
#include "CompareFilesWithVerify.h"

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			//
			struct CRC32Result
			{
				encoding_file::CalculateFileForkCRC32Result mResult;
				uint32_t mCRC32;
			};
			bool operator ==(const CRC32Result& inLHS, const CRC32Result& inRHS)
			{
				return (inLHS.mResult == inRHS.mResult) && (inLHS.mCRC32 == inRHS.mCRC32);
			}
			
			//
			//
			typedef std::map<std::string, CRC32Result> CRC32ResultMap;
			
			//
			//
			class CRC32sCallback
			:
			public encoding_file::CalculateFileForkCRC32sCallback
			{
			public:
				//
				//
				CRC32sCallback()
				{
				}
				
				//
				//
				bool Function(
							  const std::string& inForkName,
							  const encoding_file::CalculateFileForkCRC32Result& inResult,
							  const uint32_t& inCRC32)
				{
					CRC32Result result;
					result.mResult = inResult;
					result.mCRC32 = inCRC32;
					mCRCs.insert(CRC32ResultMap::value_type(inForkName, result));
					return true;
				}
				
				//
				//
				CRC32ResultMap mCRCs;
			};
			
		} // private namespace
		
		//
		void CompareFilesWithVerify(const HermitPtr& h_,
									const FilePathPtr& inFirstFilePath,
									const FilePathPtr& inSecondFilePath,
									const bool& inIgnoreDates,
									const bool& inIgnoreFinderInfo,
									const PreprocessFileFunctionRef& inPreprocessFunction,
									const CompareFilesCallbackRef& inCallback) {
			
			int retries = 0;
			while (true) {
				CompareFilesCallbackClass compareFilesCallback;
				CompareFiles(inFirstFilePath,
							 inSecondFilePath,
							 inIgnoreDates,
							 inIgnoreFinderInfo,
							 inPreprocessFunction,
							 h_,
							 compareFilesCallback);
				
				if (compareFilesCallback.mStatus == kCompareFilesStatus_Error) {
					NOTIFY_ERROR(h_,
								 "CompareFilesWithVerify: CompareFiles failed for:",
								 inFirstFilePath,
								 "and:",
								 inSecondFilePath);
					
					++retries;
					if (retries < 3) {
						NOTIFY_ERROR(h_, "... retrying ...\n");
						continue;
					}
					
					inCallback.Call(kCompareFilesStatus_Error);
					return;
				}
				else if ((compareFilesCallback.mStatus != kCompareFilesStatus_FilesMatch) &&
						 (compareFilesCallback.mStatus != kCompareFilesStatus_LinkTargetsMatch))
				{
					if (compareFilesCallback.mStatus == kCompareFilesStatus_FileSizesDiffer)
					{
						inCallback.Call(
										kCompareFilesStatus_FileSizesDiffer,
										compareFilesCallback.mInfoString1,
										"",
										0);
						
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_FileTypesDiffer)
					{
						inCallback.Call(kCompareFilesStatus_FileTypesDiffer, "", "", 0);
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_XAttrsDiffer)
					{
						inCallback.Call(kCompareFilesStatus_XAttrsDiffer, "", "", 0);
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_FinderInfosDiffer)
					{
						inCallback.Call(kCompareFilesStatus_FinderInfosDiffer, "", "", 0);
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_CreationDatesDiffer)
					{
						inCallback.Call(
										kCompareFilesStatus_CreationDatesDiffer,
										compareFilesCallback.mInfoString1,
										compareFilesCallback.mInfoString2,
										0);
						
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_ModificationDatesDiffer)
					{
						inCallback.Call(
										kCompareFilesStatus_ModificationDatesDiffer,
										compareFilesCallback.mInfoString1,
										compareFilesCallback.mInfoString2,
										0);
						
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_PermissionsDiffer)
					{
						inCallback.Call(
										kCompareFilesStatus_PermissionsDiffer,
										compareFilesCallback.mInfoString1,
										compareFilesCallback.mInfoString2,
										0);
						
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_UserOwnersDiffer)
					{
						inCallback.Call(
										kCompareFilesStatus_UserOwnersDiffer,
										compareFilesCallback.mInfoString1,
										compareFilesCallback.mInfoString2,
										0);
						
						return;
					}
					else if (compareFilesCallback.mStatus == kCompareFilesStatus_GroupOwnersDiffer)
					{
						inCallback.Call(
										kCompareFilesStatus_GroupOwnersDiffer,
										compareFilesCallback.mInfoString1,
										compareFilesCallback.mInfoString2,
										0);
						
						return;
					}
					else
					{
						encoding_file::CalculateFileForkCRC32CallbackClass crcCallback;
						CalculateFileForkCRC32(
											   inFirstFilePath,
											   compareFilesCallback.mInfoString1,
											   crcCallback);
						
						if (crcCallback.mResult != encoding_file::kCalculateFileForkCRC32Result_Success)
						{
							LogFilePath("CompareFilesWithVerify: CalculateFileForkCRC32 failed for: ", inFirstFilePath);
							inCallback.Call(kCompareFilesStatus_Error, "", "", 0);
							return;
						}
						uint32_t file1CRC32 = crcCallback.mCRC32;
						
						CalculateFileForkCRC32(
											   inSecondFilePath,
											   compareFilesCallback.mInfoString1,
											   crcCallback);
						
						if (crcCallback.mResult != encoding_file::kCalculateFileForkCRC32Result_Success)
						{
							LogFilePath("CompareFilesWithVerify: CalculateFileForkCRC32 failed for: ", inSecondFilePath);
							inCallback.Call(kCompareFilesStatus_Error, "", "", 0);
							return;
						}
						uint32_t file2CRC32 = crcCallback.mCRC32;
						
						if (file1CRC32 == file2CRC32)
						{
							//	Hmmmm... The bit compare thinks they differ, the CRC32s match.
							//	This might be a rare CRC32 collision, or it could be a (similarly rare if not moreso) blip on the file reads
							//	when we did the bit compare. We'll treat it as a retry case, though only retry once.
							
							LogFilePath("CompareFilesWithVerify: Data mismatch but CRC32 match for: ", inFirstFilePath);
							LogFilePath("-- and: ", inSecondFilePath);
							
							++retries;
							if (retries < 2)
							{
								Log("... retrying ...\n");
								continue;
							}
						}
						
						inCallback.Call(
										kCompareFilesStatus_FileContentsDiffer,
										compareFilesCallback.mInfoString1,
										"",
										compareFilesCallback.mInfoUInt64);
						
						return;
					}
				}
				else if (compareFilesCallback.mStatus == kCompareFilesStatus_LinkTargetsMatch)
				{
					inCallback.Call(kCompareFilesStatus_LinkTargetsMatch, "", "", 0);
					return;
				}
				else
				{
					CRC32sCallback file1CRCsCallback;
					CalculateFileForkCRC32s(inFirstFilePath, file1CRCsCallback);
					CRC32ResultMap::const_iterator end1 = file1CRCsCallback.mCRCs.end();
					for (CRC32ResultMap::const_iterator it1 = file1CRCsCallback.mCRCs.begin(); it1 != end1; ++it1)
					{
						if (((*it1).second.mResult != encoding_file::kCalculateFileForkCRC32Result_Success) &&
							((*it1).second.mResult != encoding_file::kCalculateFileForkCRC32Result_ForkNotFound))
						{
							LogFilePath("CompareFilesWithVerify: CalculateFileForkCRC32s failed for first path: ", inFirstFilePath);
							inCallback.Call(kCompareFilesStatus_Error, "", "", 0);
							return;
						}
					}
					
					CRC32sCallback file2CRCsCallback;
					CalculateFileForkCRC32s(inSecondFilePath, file2CRCsCallback);
					CRC32ResultMap::const_iterator end2 = file2CRCsCallback.mCRCs.end();
					for (CRC32ResultMap::const_iterator it2 = file2CRCsCallback.mCRCs.begin(); it2 != end2; ++it2)
					{
						if (((*it2).second.mResult != encoding_file::kCalculateFileForkCRC32Result_Success) &&
							((*it2).second.mResult != encoding_file::kCalculateFileForkCRC32Result_ForkNotFound))
						{
							LogFilePath("CompareFilesWithVerify: CalculateFileForkCRC32s failed for second path: ", inSecondFilePath);
							inCallback.Call(kCompareFilesStatus_Error, "", "", 0);
							return;
						}
					}
					
					if (file1CRCsCallback.mCRCs != file2CRCsCallback.mCRCs)
					{
						//	Hmmmm... The bit compare thinks they're the same, but the CRC32s don't match.
						//	We'll treat it as a retry case.
						
						LogFilePath("CompareFilesWithVerify: Data match but CRC32 mismatch for: ", inFirstFilePath);
						LogFilePath("-- and: ", inSecondFilePath);
						
						++retries;
						if (retries < 3)
						{
							Log("... retrying ...\n");
							continue;
						}
					}
					
					inCallback.Call(kCompareFilesStatus_FilesMatch, "", "", 0);
					return;
				}
			}
		}
		
	} // namespace file
} // namespace hermit

#endif

