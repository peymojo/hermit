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
#include <thread>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SendS3CommandWithData.h"
#include "SignAWSRequestVersion2.h"
#include "S3CreateBucket.h"
#include "S3Notification.h"

namespace hermit {
	namespace s3 {
		namespace S3CreateBucket_Impl {
			
			//
			class ProcessS3CreateBucketXMLClass : xml::ParseXMLClient {
			private:
				//
				enum class ParseState {
					kNew,
					kError,
					kError_Code,
					kError_Message,
					kError_StringToSignBytes,
					kListAllMyBucketsResult,
					kBuckets,
					kBucket,
					kName,
					kIgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessS3CreateBucketXMLClass(const HermitPtr& h_) :
				mH_(h_),
				mParseState(ParseState::kNew),
				mIsError(false) {
				}
				
				//
				bool GetIsError() const {
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
					if (mParseState == ParseState::kNew) {
						if (inStartTag == "ListAllMyBucketsResult") {
							PushState(ParseState::kListAllMyBucketsResult);
						}
						else if (inStartTag == "Error") {
							mIsError = true;
							PushState(ParseState::kError);
						}
						else if (inStartTag != "?xml") {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kError) {
						if (inStartTag == "Code") {
							PushState(ParseState::kError_Code);
						}
						else if (inStartTag == "Message") {
							PushState(ParseState::kError_Message);
						}
						else if (inStartTag == "StringToSignBytes") {
							PushState(ParseState::kError_StringToSignBytes);
						}
						else {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kListAllMyBucketsResult) {
						if (inStartTag == "Buckets") {
							PushState(ParseState::kBuckets);
						}
						else {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kBuckets) {
						if (inStartTag == "Bucket") {
							PushState(ParseState::kBucket);
						}
						else {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kBucket) {
						if (inStartTag == "Name") {
							PushState(ParseState::kName);
						}
						else {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else {
						PushState(ParseState::kIgnoredElement);
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
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				bool mIsError;
			};
						
			//
			class CreateBucketClass;
			typedef std::shared_ptr<CreateBucketClass> CreateBucketClassPtr;
			
			//
			class CreateBucketCompletion : public SendS3CommandCompletion {
			public:
				//
				CreateBucketCompletion(const CreateBucketClassPtr& createBucketClass) :
				mCreateBucketClass(createBucketClass) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const S3Result& result,
								  const S3ParamVector& params,
								  const DataBuffer& responseData) override;
				
				//
				CreateBucketClassPtr mCreateBucketClass;
			};
			
			//
			class CreateBucketClass : public std::enable_shared_from_this<CreateBucketClass> {
				//
				static const int kMaxRetries = 5;
				
			public:
				//
				CreateBucketClass(const std::string& bucketName,
								  const std::string& region,
								  const std::string& awsPublicKey,
								  const std::string& awsPrivateKey,
								  const S3CompletionBlockPtr& completion) :
				mBucketName(bucketName),
				mRegion(region),
				mAWSPublicKey(awsPublicKey),
				mAWSPrivateKey(awsPrivateKey),
				mCompletion(completion),
				mLatestResult(S3Result::kUnknown),
				mRetries(0),
				mAccessDeniedRetries(0),
				mSleepInterval(1),
				mSleepIntervalStep(2) {
				}
				
				//
				void S3CreateBucket(const HermitPtr& h_) {
					if (CHECK_FOR_ABORT(h_)) {
						mCompletion->Call(h_, S3Result::kCanceled);
						return;
					}
					
					if (mRetries > 0) {
						S3NotificationParams params("GetObject", mRetries, mLatestResult);
						NOTIFY(h_, kS3RetryNotification, &params);
					}
					
					std::string method("PUT");
					std::string contentType;
					std::string urlToSign("/");
					urlToSign += mBucketName;
					urlToSign += "/";
					
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
						NOTIFY_ERROR(h_, "S3CreateBucket: SignAWSRequestVersion2 failed for URL:", urlToSign);
						mCompletion->Call(h_, S3Result::kError);
						return;
					}
					
					S3ParamVector params;
					params.push_back(std::make_pair("Date", authCallback.mDateString));
					params.push_back(std::make_pair("Authorization", authCallback.mAuthorizationString));
					
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
					
					auto commandCompletion = std::make_shared<CreateBucketCompletion>(shared_from_this());
					SendS3CommandWithData(h_,
										  url,
										  method,
										  params,
										  std::make_shared<SharedBuffer>(regionBody.data(), regionBody.size()),
										  commandCompletion);
				}
				
				//
				void Completion(const HermitPtr& h_,
								const S3Result& result,
								const S3ParamVector& params,
								const DataBuffer& responseData) {
					mLatestResult = result;
					
					if (!ShouldRetry(result)) {
						if (mRetries > 0) {
							S3NotificationParams params("GetObject", mRetries, result);
							NOTIFY(h_, kS3RetryCompleteNotification, &params);
						}
						ProcessResult(h_, result, params, responseData);
						return;
					}
					if (++mRetries == kMaxRetries) {
						S3NotificationParams params("GetObject", mRetries, result);
						NOTIFY(h_, kS3MaxRetriesExceededNotification, &params);
						
						NOTIFY_ERROR(h_, "Maximum retries exceeded, most recent result:", (int)result);
						mCompletion->Call(h_, result);
						return;
					}
					
					int fifthSecondIntervals = mSleepInterval * 5;
					for (int i = 0; i < fifthSecondIntervals; ++i) {
						if (CHECK_FOR_ABORT(h_)) {
							mCompletion->Call(h_, S3Result::kCanceled);
							return;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(200));
					}
					mSleepInterval += mSleepIntervalStep;
					mSleepIntervalStep += 2;
					
					S3CreateBucket(h_);
				}
				
				//
				bool ShouldRetry(const S3Result& result) {
					if ((result == S3Result::kTimedOut) ||
						(result == S3Result::kNetworkConnectionLost) ||
						(result == S3Result::k500InternalServerError) ||
						(result == S3Result::k503ServiceUnavailable) ||
						(result == S3Result::kS3InternalError) ||
						// borderline candidate for retry, but I've seen it recover "in the wild":
						(result == S3Result::kHostNotFound)) {
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
				void ProcessResult(const HermitPtr& h_,
								   const S3Result& result,
								   const S3ParamVector& params,
								   const DataBuffer& responseData) {
					if (result == S3Result::kCanceled) {
						mCompletion->Call(h_, result);
						return;
					}
					if (result == S3Result::kSuccess) {
						ProcessS3CreateBucketXMLClass pc(h_);
						pc.Process(std::string(responseData.first, responseData.second));
						if (pc.GetIsError()) {
							NOTIFY_ERROR(h_, "S3CreateBucket: Result indicated an error.");
							mCompletion->Call(h_, S3Result::kError);
							return;
						}
					}
					else {
						NOTIFY_ERROR(h_, "Error result encountered:", (int)result);
					}
					mCompletion->Call(h_, result);
				}
				
				//
				std::string mBucketName;
				std::string mRegion;
				std::string mAWSPublicKey;
				std::string mAWSPrivateKey;
				S3CompletionBlockPtr mCompletion;
				S3Result mLatestResult;
				int mRetries;
				int mAccessDeniedRetries;
				int mSleepInterval;
				int mSleepIntervalStep;
			};
			
			//
			void CreateBucketCompletion::Call(const HermitPtr& h_,
											  const S3Result& result,
											  const S3ParamVector& params,
											  const DataBuffer& responseData) {
				mCreateBucketClass->Completion(h_, result, params, responseData);
			}
			
		} // namespace S3CreateBucket_Impl
		using namespace S3CreateBucket_Impl;
		
		//
		void S3CreateBucket(const HermitPtr& h_,
							const std::string& bucketName,
							const std::string& region,
							const std::string& awsPublicKey,
							const std::string& awsPrivateKey,
							const S3CompletionBlockPtr& completion) {
			auto creator = std::make_shared<CreateBucketClass>(bucketName, region, awsPublicKey, awsPrivateKey, completion);
			creator->S3CreateBucket(h_);
		}
		
	} // namespace s3
} // namespace hermit
