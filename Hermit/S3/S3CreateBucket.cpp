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

#include <stack>
#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "Hermit/XML/ParseXMLData.h"
#include "S3RetryClass.h"
#include "SendS3CommandWithData.h"
#include "SignAWSRequestVersion2.h"
#include "S3CreateBucket.h"

namespace hermit {
	namespace s3 {
		
		namespace
		{
			//
			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			//
			class SendCommandCallback
			:
			public SendS3CommandCallback
			{
			public:
				//
				//
				SendCommandCallback()
				:
				mStatus(S3Result::kUnknown)
				{
				}
				
				//
				//
				bool Function(
							  const S3Result& inStatus,
							  const EnumerateStringValuesFunctionRef& inParamFunction,
							  const DataBuffer& inData)
				{
					mStatus = inStatus;
					if (inStatus == S3Result::kSuccess)
					{
						mResponseData = std::string(inData.first, inData.second);
					}
					return true;
				}
				
				//
				//
				S3Result mStatus;
				std::string mResponseData;
			};
			
			//
			//
			class ProcessS3CreateBucketXMLClass : xml::ParseXMLClient
			{
			private:
				//
				//
				enum ParseState
				{
					kParseState_New,
					kParseState_Error,
					kParseState_Error_Code,
					kParseState_Error_Message,
					kParseState_Error_StringToSignBytes,
					kParseState_ListAllMyBucketsResult,
					kParseState_Buckets,
					kParseState_Bucket,
					kParseState_Name,
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				//
				ProcessS3CreateBucketXMLClass(const HermitPtr& h_)
				:
				mH_(h_),
				mParseState(kParseState_New),
				mIsError(false)
				{
				}
				
				//
				//
				bool GetIsError() const
				{
					return mIsError;
				}
								
				//
				xml::ParseXMLStatus Process(const std::string& inXMLData) {
					return xml::ParseXMLData(mH_, inXMLData, *this);
				}
				
			private:
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == kParseState_New)
					{
						if (inStartTag == "ListAllMyBucketsResult")
						{
							PushState(kParseState_ListAllMyBucketsResult);
						}
						else if (inStartTag == "Error")
						{
							mIsError = true;
							PushState(kParseState_Error);
						}
						else if (inStartTag != "?xml")
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Error)
					{
						if (inStartTag == "Code")
						{
							PushState(kParseState_Error_Code);
						}
						else if (inStartTag == "Message")
						{
							PushState(kParseState_Error_Message);
						}
						else if (inStartTag == "StringToSignBytes")
						{
							PushState(kParseState_Error_StringToSignBytes);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_ListAllMyBucketsResult)
					{
						if (inStartTag == "Buckets")
						{
							PushState(kParseState_Buckets);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Buckets)
					{
						if (inStartTag == "Bucket")
						{
							PushState(kParseState_Bucket);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Bucket)
					{
						if (inStartTag == "Name")
						{
							PushState(kParseState_Name);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else
					{
						PushState(kParseState_IgnoredElement);
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnContent(const std::string& inContent) override {
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
					PopState();
					return xml::kParseXMLStatus_OK;
				}
				
				//
				//
				void PushState(ParseState inNewState)
				{
					mParseStateStack.push(mParseState);
					mParseState = inNewState;
				}
				
				//
				//
				void PopState()
				{
					mParseState = mParseStateStack.top();
					mParseStateStack.pop();
				}
				
				//
				//
				HermitPtr mH_;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				bool mIsError;
			};
			
			// CreateBucket operation class
			class CreateBucket {
			private:
				std::string mBucketName;
				std::string mRegion;
				std::string mAWSPublicKey;
				std::string mAWSPrivateKey;
				S3Result mResult;
				int mAccessDeniedRetries;
				
			public:
				typedef S3Result ResultType;
				static const ResultType kDefaultResult = S3Result::kUnknown;
				static const int kMaxRetries = 5;
				
				//
				const char* OpName() const {
					return "CreateBucket";
				}
				
				//
				CreateBucket(const std::string& bucketName,
							 const std::string& region,
							 const std::string& awsPublicKey,
							 const std::string& awsPrivateKey) :
				mBucketName(bucketName),
				mRegion(region),
				mAWSPublicKey(awsPublicKey),
				mAWSPrivateKey(awsPrivateKey),
				mResult(S3Result::kUnknown),
				mAccessDeniedRetries(0) {
				}
				
				//
				ResultType AttemptOnce(const HermitPtr& h_) {
					
					std::string method("PUT");
					std::string contentType;
					std::string urlToSign("/");
					urlToSign += mBucketName;
					urlToSign += "/";
					
					SignAWSRequestVersion2CallbackClass authCallback;
					SignAWSRequestVersion2(
										   mAWSPublicKey,
										   mAWSPrivateKey,
										   method,
										   "",
										   contentType,
										   "",
										   urlToSign,
										   authCallback);
					
					if (!authCallback.mSuccess) {
						NOTIFY_ERROR(h_, "S3CreateBucket: SignAWSRequestVersion2 failed for URL:", urlToSign);
						return S3Result::kError;
					}
					
					StringPairVector params;
					params.push_back(StringPair("Date", authCallback.mDateString));
					params.push_back(StringPair("Authorization", authCallback.mAuthorizationString));
					
					std::string url("https://");
					url += mBucketName;
					url += ".s3.amazonaws.com";
					
					std::string regionBody;
					// aws can reject the call if us-east-1 is specified, should be omitted for that case
					if (mRegion != "us-east-1") {
						regionBody = "<CreateBucketConfiguration xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\"><LocationConstraint>";
						regionBody += mRegion;
						regionBody += "</LocationConstraint></CreateBucketConfiguration>";
					}
					
					EnumerateStringValuesFunctionClass headerParams(params);
					SendCommandCallback result;
					SendS3CommandWithData(h_,
										  url,
										  method,
										  headerParams,
										  DataBuffer(regionBody.data(), regionBody.size()),
										  result);
					
					if (result.mStatus == S3Result::kSuccess) {
						ProcessS3CreateBucketXMLClass pc(h_);
						pc.Process(result.mResponseData);
						if (pc.GetIsError())
						{
							NOTIFY_ERROR(h_, "S3CreateBucket: Result indicated an error.");
							return S3Result::kError;
						}
					}
					return result.mStatus;
				}
				
				//
				bool ShouldRetry(const ResultType& result) {
					if ((result == S3Result::kTimedOut) ||
						(result == S3Result::kNetworkConnectionLost) ||
						(result == S3Result::kS3InternalError) ||
						(result == S3Result::k500InternalServerError) ||
						(result == S3Result::k503ServiceUnavailable) ||
						// borderline candidate for retry, but I've seen it recover "in the wild":
						(result == s3::S3Result::kHostNotFound)) {
						return true;
					}
					// we allow a single retry on PermissionDenied since i've seen this fail due to
					// flaky network behavior in the wild. (but we don't want to spam the server in
					// cases where access is indeed denied so we only do it once.)
					if ((result == S3Result::k403AccessDenied) && (mAccessDeniedRetries == 0)) {
						++mAccessDeniedRetries;
						return true;
					}
					return false;
				}
				
				//
				void ProcessResult(const HermitPtr& h_, const ResultType& result) {
					
					if (result == S3Result::kCanceled) {
						mResult = result;
						return;
					}
					if (result == S3Result::kSuccess) {
						mResult = result;
						return;
					}
					NOTIFY_ERROR(h_, "S3CreateBucket: error result encountered:", (int)result);
					mResult = result;
				}
				
				//
				void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
					NOTIFY_ERROR(h_, "S3CreateBucket: maximum retries exceeded, most recent result:", (int)result);
					mResult = result;
				}
				
				//
				void Canceled() {
					mResult = S3Result::kCanceled;
				}
				
				//
				S3Result GetResult() const {
					return mResult;
				}
			};
			
		} // private namespace
		
		//
		S3Result S3CreateBucket(const HermitPtr& h_,
								const std::string& inBucketName,
								const std::string& inRegion,
								const std::string& inAWSPublicKey,
								const std::string& inAWSPrivateKey) {
			CreateBucket create(inBucketName, inRegion, inAWSPublicKey, inAWSPrivateKey);
			RetryClassT<CreateBucket> retry;
			retry.AttemptWithRetry(h_, create);
			return create.GetResult();
		}
		
	} // namespace s3
} // namespace hermit
