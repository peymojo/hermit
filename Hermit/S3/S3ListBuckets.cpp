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
		namespace S3ListBuckets_Impl {
			
			//
			class ProcessS3ListBucketsXMLClass : xml::ParseXMLClient {
			private:
				//
				enum class ParseState {
					kNew,
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
				ProcessS3ListBucketsXMLClass(const HermitPtr& h_, const BucketNameReceiverPtr& receiver) :
				mH_(h_),
				mReceiver(receiver),
				mParseState(ParseState::kNew),
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
					if (mParseState == ParseState::kNew) {
						if (inStartTag == "ListAllMyBucketsResult") {
							PushState(ParseState::kListAllMyBucketsResult);
						}
						else if (inStartTag != "?xml") {
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
					if (mParseState == ParseState::kName) {
						mProcessedAtLeastOneBucket = true;
						if (!mReceiver->OnOneBucket(mH_, inContent)) {
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
				BucketNameReceiverPtr mReceiver;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				bool mProcessedAtLeastOneBucket;
			};
			
			//
			class ListBucketsClass;
			typedef std::shared_ptr<ListBucketsClass> ListBucketsClassPtr;
			
			//
			class ListBucketsCompletion : public SendS3CommandCompletion {
			public:
				//
				ListBucketsCompletion(const ListBucketsClassPtr& ListBucketsClass) :
				mListBucketsClass(ListBucketsClass) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const S3Result& result,
								  const S3ParamVector& params,
								  const DataBuffer& responseData) override;
				
				//
				ListBucketsClassPtr mListBucketsClass;
			};
			
			//
			class ListBucketsClass : public std::enable_shared_from_this<ListBucketsClass> {
				//
				static const int kMaxRetries = 5;
				
			public:
				//
				ListBucketsClass(const std::string& awsPublicKey,
								 const std::string& awsPrivateKey,
								 const BucketNameReceiverPtr& receiver,
								 const S3CompletionBlockPtr& completion) :
				mAWSPublicKey(awsPublicKey),
				mAWSPrivateKey(awsPrivateKey),
				mReceiver(receiver),
				mCompletion(completion),
				mLatestResult(S3Result::kUnknown),
				mRetries(0),
				mAccessDeniedRetries(0),
				mSleepInterval(1),
				mSleepIntervalStep(2) {
				}
				
				//
				void S3ListBuckets(const HermitPtr& h_) {
					if (CHECK_FOR_ABORT(h_)) {
						mCompletion->Call(h_, S3Result::kCanceled);
						return;
					}
					
					if (mRetries > 0) {
						S3NotificationParams params("GetObject", mRetries, mLatestResult);
						NOTIFY(h_, kS3RetryNotification, &params);
					}
					
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
						mCompletion->Call(h_, S3Result::kError);
						return;
					}
					
					S3ParamVector params;
					params.push_back(std::make_pair("Date", authCallback.mDateString));
					params.push_back(std::make_pair("Authorization", authCallback.mAuthorizationString));
					
					std::string url("https://s3.amazonaws.com/");
					
					auto commandCompletion = std::make_shared<ListBucketsCompletion>(shared_from_this());
					SendS3Command(h_, url, method, params, commandCompletion);
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
					
					S3ListBuckets(h_);
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
						ProcessS3ListBucketsXMLClass pc(h_, mReceiver);
						pc.Process(std::string(responseData.first, responseData.second));
					}
					else {
						NOTIFY_ERROR(h_, "Error result encountered:", (int)result);
					}
					mCompletion->Call(h_, result);
				}
				
				//
				std::string mAWSPublicKey;
				std::string mAWSPrivateKey;
				BucketNameReceiverPtr mReceiver;
				S3CompletionBlockPtr mCompletion;
				S3Result mLatestResult;
				int mRetries;
				int mAccessDeniedRetries;
				int mSleepInterval;
				int mSleepIntervalStep;
			};
			
			//
			void ListBucketsCompletion::Call(const HermitPtr& h_,
											  const S3Result& result,
											  const S3ParamVector& params,
											  const DataBuffer& responseData) {
				mListBucketsClass->Completion(h_, result, params, responseData);
			}
			
		} // namespace S3ListBuckets_Impl
		using namespace S3ListBuckets_Impl;
		
		//
		void S3ListBuckets(const HermitPtr& h_,
						   const std::string& awsPublicKey,
						   const std::string& awsPrivateKey,
						   const BucketNameReceiverPtr& receiver,
						   const S3CompletionBlockPtr& completion) {
			auto lister = std::make_shared<ListBucketsClass>(awsPublicKey, awsPrivateKey, receiver, completion);
			lister->S3ListBuckets(h_);
		}
		
	} // namespace s3
} // namespace hermit
