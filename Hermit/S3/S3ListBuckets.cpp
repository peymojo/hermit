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
#include "SendS3Command.h"
#include "SignAWSRequestVersion2.h"
#include "S3ListBuckets.h"

namespace hermit {
	namespace s3 {
		
		namespace {

			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			class SendCommandCallback : public SendS3CommandCallback {
			public:
				//
				SendCommandCallback() : mStatus(S3Result::kUnknown) {
				}
				
				//
				bool Function(const S3Result& inStatus,
							  const EnumerateStringValuesFunctionRef& inParamFunction,
							  const DataBuffer& inData) {
					mStatus = inStatus;
					if (inStatus == S3Result::kSuccess) {
						mResponseData = std::string(inData.first, inData.second);
					}
					return true;
				}
				
				//
				S3Result mStatus;
				std::string mResponseData;
			};
			
			//
			class ProcessS3ListBucketsXMLClass : xml::ParseXMLClient {
			private:
				//
				enum ParseState {
					kParseState_New,
					kParseState_ListAllMyBucketsResult,
					kParseState_Buckets,
					kParseState_Bucket,
					kParseState_Name,
					kParseState_IgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessS3ListBucketsXMLClass(const HermitPtr& h_, BucketNameReceiver& receiver) :
				mH_(h_),
				mReceiver(receiver),
				mParseState(kParseState_New),
				mProcessedAtLeastOneBucket(false) {
				}
								
				//
				xml::ParseXMLStatus Process(const std::string& inXMLData) {
					return xml::ParseXMLData(mH_, inXMLData, *this);
				}
				
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == kParseState_New) {
						if (inStartTag == "ListAllMyBucketsResult") {
							PushState(kParseState_ListAllMyBucketsResult);
						}
						else if (inStartTag != "?xml") {
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_ListAllMyBucketsResult) {
						if (inStartTag == "Buckets") {
							PushState(kParseState_Buckets);
						}
						else {
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Buckets) {
						if (inStartTag == "Bucket") {
							PushState(kParseState_Bucket);
						}
						else {
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Bucket) {
						if (inStartTag == "Name") {
							PushState(kParseState_Name);
						}
						else {
							PushState(kParseState_IgnoredElement);
						}
					}
					else {
						PushState(kParseState_IgnoredElement);
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnContent(const std::string& inContent) override {
					if (mParseState == kParseState_Name) {
						mProcessedAtLeastOneBucket = true;
						if (!mReceiver(inContent)) {
							return xml::kParseXMLStatus_Cancel;
						}
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
					PopState();
					return xml::kParseXMLStatus_OK;
				}
				
				//
				void PushState(ParseState inNewState) {
					mParseStateStack.push(mParseState);
					mParseState = inNewState;
				}
				
				//
				void PopState() {
					mParseState = mParseStateStack.top();
					mParseStateStack.pop();
				}
				
				//
				HermitPtr mH_;
				BucketNameReceiver& mReceiver;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				bool mProcessedAtLeastOneBucket;
			};
			
			// ListBuckets operation class
			class ListBuckets {
			private:
				std::string mAWSPublicKey;
				std::string mAWSPrivateKey;
				BucketNameReceiver& mReceiver;
				S3Result mResult;
				int mAccessDeniedRetries;
				
			public:
				typedef S3Result ResultType;
				static const S3Result kDefaultResult = S3Result::kUnknown;
				static const int kMaxRetries = 5;
				
				//
				const char* OpName() const {
					return "ListBuckets";
				}
				
				//
				ListBuckets(const std::string& awsPublicKey, const std::string& awsPrivateKey, BucketNameReceiver& receiver) :
				mAWSPublicKey(awsPublicKey),
				mAWSPrivateKey(awsPrivateKey),
				mReceiver(receiver),
				mResult(S3Result::kUnknown),
				mAccessDeniedRetries(0) {
				}
				
				ResultType AttemptOnce(const HermitPtr& h_) {
					std::string method("GET");
					std::string contentType;
					std::string urlToSign("/");
					
					SignAWSRequestVersion2CallbackClass authCallback;
					SignAWSRequestVersion2(mAWSPublicKey,
										   mAWSPrivateKey,
										   method,
										   "",
										   contentType,
										   "",
										   urlToSign,
										   authCallback);
					
					if (!authCallback.mSuccess) {
						NOTIFY_ERROR(h_, "S3ListBuckets: SignAWSRequestVersion2 failed");
						return S3Result::kError;
					}
					
					StringPairVector params;
					params.push_back(StringPair("Date", authCallback.mDateString));
					params.push_back(StringPair("Authorization", authCallback.mAuthorizationString));
					
					std::string url("https://s3.amazonaws.com/");
					
					EnumerateStringValuesFunctionClass s3Params(params);
					SendCommandCallback result;
					SendS3Command(h_, url, method, s3Params, result);
					
					if (result.mStatus == S3Result::kSuccess) {
						ProcessS3ListBucketsXMLClass pc(h_, mReceiver);
						pc.Process(result.mResponseData);
					}
					return result.mStatus;
				}
				
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
				
				void ProcessResult(const HermitPtr& h_, const ResultType& result) {
					if (result == S3Result::kCanceled) {
						mResult = result;
						return;
					}
					if (result == S3Result::kSuccess) {
						mResult = result;
						return;
					}
					NOTIFY_ERROR(h_, "S3ListBuckets: error result encountered:", (int)result);
					mResult = result;
				}
				
				void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
					NOTIFY_ERROR(h_, "S3ListBuckets: maximum retries exceeded, most recent result:", (int)result);
					mResult = result;
				}
				
				void Canceled() {
					mResult = S3Result::kCanceled;
				}
				
				S3Result GetResult() const {
					return mResult;
				}
			};
			
		} // private namespace
		
		//
		//
		S3Result S3ListBuckets(const HermitPtr& h_,
							   const std::string& awsPublicKey,
							   const std::string& awsPrivateKey,
							   BucketNameReceiver& receiver) {
			ListBuckets list(awsPublicKey, awsPrivateKey, receiver);
			RetryClassT<ListBuckets> retry;
			retry.AttemptWithRetry(h_, list);
			return list.GetResult();
		}
		
	} // namespace s3
} // namespace hermit
