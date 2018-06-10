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
#include "Hermit/String/HexStringToBinary.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SendS3CommandWithData.h"
#include "SignAWSRequestVersion2.h"
#include "S3SetBucketVersioning.h"

namespace hermit {
	namespace s3 {
		namespace S3SetBucketVersioning_Impl {

			//
			class ProcessS3SetBucketVersioningXMLClass : xml::ParseXMLClient  {
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
				ProcessS3SetBucketVersioningXMLClass() : mParseState(ParseState::kNew), mIsError(false) {
				}
				
				//
				bool GetIsError() const {
					return mIsError;
				}
				
				//
				xml::ParseXMLStatus Process(const HermitPtr& h_, const std::string& inXMLData) {
					return xml::ParseXMLData(h_, inXMLData, *this);
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
					if (mParseState == ParseState::kName) {
						//std::cout << inContent << "\n";
					}
					else if (mParseState == ParseState::kError_Code) {
						//std::cout << "Error Code: " << inContent << "\n";
					}
					else if (mParseState == ParseState::kError_Message) {
						//std::cout << "Error Message: " << inContent << "\n";
					}
					else if (mParseState == ParseState::kError_StringToSignBytes) {
						//std::cout << "Error String To Sign Bytes: " << inContent << "\n";
						//std::cout << "Error String To Sign: " << BytesToString(inHub, inContent) << "\n";
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
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				bool mIsError;
			};
			
			//
			class CommandCompletion : public SendS3CommandCompletion {
			public:
				//
				CommandCompletion(const S3CompletionBlockPtr& completion) : mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const S3Result& result,
								  const S3ParamVector& params,
								  const DataBuffer& data) override {
					if (result == S3Result::kCanceled) {
						mCompletion->Call(h_, result);
						return;
					}
					if (result != S3Result::kSuccess) {
						NOTIFY_ERROR(h_, "S3SetBucketVersioning: SendS3CommandWithData failed.");
						mCompletion->Call(h_, result);
						return;
					}
					
					ProcessS3SetBucketVersioningXMLClass pc;
					pc.Process(h_, std::string(data.first, data.second));
					if (pc.GetIsError()) {
						NOTIFY_ERROR(h_, "S3SetBucketVersioning: Response indicated an error.");
						mCompletion->Call(h_, S3Result::kError);
						return;
					}
					mCompletion->Call(h_, S3Result::kSuccess);
				}
				
				//
				S3CompletionBlockPtr mCompletion;
			};
			
		} // namespace S3SetBucketVersioning_Impl
		using namespace S3SetBucketVersioning_Impl;
		
		//
		void S3SetBucketVersioning(const HermitPtr& h_,
								   const http::HTTPSessionPtr& session,
								   const std::string& s3PublicKey,
								   const std::string& s3PrivateKey,
								   const std::string& bucketName,
								   const bool& versioningEnabled,
								   const S3CompletionBlockPtr& completion) {

			std::string method("PUT");
			
			std::string urlToSign("/");
			urlToSign += bucketName;
			urlToSign += "/?versioning";
			
			SignAWSRequestVersion2CallbackClass authCallback;
			SignAWSRequestVersion2(s3PublicKey,
								   s3PrivateKey,
								   method,
								   "",
								   "",
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess) {
				NOTIFY_ERROR(h_, "S3SetBucketVersioning: SignAWSRequestVersion2 failed for URL:", urlToSign);
				completion->Call(h_, S3Result::kError);
				return;
			}
			
			std::string url("https://");
			url += bucketName;
			url += ".s3.amazonaws.com/?versioning";
			
			std::string body;
			body += "<VersioningConfiguration xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n";
			body += "<Status>";
			if (versioningEnabled) {
				body += "Enabled";
			}
			else {
				body += "Suspended";
			}
			body += "</Status>\n";
			body += "</VersioningConfiguration>\n";
			
			S3ParamVector params;
			params.push_back(std::make_pair("Date", authCallback.mDateString));
			params.push_back(std::make_pair("Authorization", authCallback.mAuthorizationString));
			
			auto commandCompletion = std::make_shared<CommandCompletion>(completion);
			SendS3CommandWithData(h_,
								  session,
								  url,
								  method,
								  params,
								  std::make_shared<SharedBuffer>(body),
								  commandCompletion);
		}
		
	} // namespace s3
} // namespace hermit
