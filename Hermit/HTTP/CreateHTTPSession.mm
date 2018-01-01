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

#import <objc/runtime.h>
#import <Foundation/Foundation.h>
#import "CreateHTTPSession.h"

// magic key for objc_setAssociatedObject
static void* const TASK_PARAMS_KEY = (void*)&TASK_PARAMS_KEY;

@interface TaskParams : NSObject {
	hermit::DataHandlerBlockPtr _dataHandler;
	hermit::http::HTTPRequestStatusBlockPtr _status;
	hermit::http::HTTPRequestCompletionBlockPtr _completion;
}

- (id)initWith:(hermit::DataHandlerBlockPtr)dataHandler
		status:(hermit::http::HTTPRequestStatusBlockPtr)status
	completion:(hermit::http::HTTPRequestCompletionBlockPtr)completion;

@end

@implementation TaskParams

- (id)initWith:(hermit::DataHandlerBlockPtr)dataHandler
		status:(hermit::http::HTTPRequestStatusBlockPtr)status
	completion:(hermit::http::HTTPRequestCompletionBlockPtr)completion {
	self = [super init];
	if (self != nil) {
		_dataHandler = dataHandler;
		_status = status;
		_completion = completion;
	}
	return self;
}

@end


@interface SessionDelegate : NSObject<NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLSessionDataDelegate, NSURLSessionDownloadDelegate, NSURLSessionStreamDelegate>

@end

@implementation SessionDelegate

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task willPerformHTTPRedirection:(NSHTTPURLResponse *)response newRequest:(NSURLRequest *)request completionHandler:(void (^)(NSURLRequest *))completionHandler {
	TaskParams* params = objc_getAssociatedObject(task, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential * _Nullable credential))completionHandler {
	TaskParams* params = objc_getAssociatedObject(task, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)totalBytesWritten totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
	TaskParams* params = objc_getAssociatedObject(downloadTask, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(NSURL *)location {
	TaskParams* params = objc_getAssociatedObject(downloadTask, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveResponse:(NSURLResponse *)response completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
	TaskParams* params = objc_getAssociatedObject(dataTask, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(nullable NSError *)error {
	TaskParams* params = objc_getAssociatedObject(task, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

@end


namespace hermit {
	namespace http {
		
		namespace CreateHTTPSession_Impl {
			
			//
			class DataHandler : public DataHandlerBlock {
			public:
				//
				virtual void HandleData(const HermitPtr& h_,
										const DataBuffer& data,
										bool isEndOfData,
										const StreamResultBlockPtr& resultBlock) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					resultBlock->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<DataHandler> DataHandlerPtr;

			//
			class StreamCompletion : public HTTPRequestCompletionBlock {
			public:
				//
				StreamCompletion(const DataHandlerPtr& dataHandler,
								 const HTTPRequestStatusPtr& status,
								 const HTTPRequestResponseBlockPtr& response,
								 const HTTPRequestCompletionBlockPtr& completion) :
				mDataHandler(dataHandler),
				mStatus(status),
				mResponse(response),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const HTTPRequestResult& result) {
					if (result == HTTPRequestResult::kSuccess) {
						mResponse->Call(h_,
										mStatus->mStatusCode,
										mStatus->mHeaderParams,
										DataBuffer(mDataHandler->mData.data(), mDataHandler->mData.size()));
					}
					mCompletion->Call(h_, result);
				}
				
				//
				DataHandlerPtr mDataHandler;
				HTTPRequestStatusPtr mStatus;
				HTTPRequestResponseBlockPtr mResponse;
				HTTPRequestCompletionBlockPtr mCompletion;
			};
			
			//
			class URLSessionHTTPSession : public HTTPSession {
			public:
				//
				URLSessionHTTPSession(NSURLSession* session) : mSession(session) {
				}
				
				//
				~URLSessionHTTPSession() {
				}
				
				//
				virtual void SendRequest(const HermitPtr& h_,
										 const std::string& url,
										 const std::string& method,
										 const HTTPParamVector& headerParams,
										 const HTTPRequestResponseBlockPtr& response,
										 const HTTPRequestCompletionBlockPtr& completion) override {
					SendRequestWithBody(h_,
										url,
										method,
										headerParams,
										SharedBufferPtr(),
										response,
										completion);
				}
				
				//
				virtual void SendRequestWithBody(const HermitPtr& h_,
												 const std::string& url,
												 const std::string& method,
												 const HTTPParamVector& headerParams,
												 const SharedBufferPtr& body,
												 const HTTPRequestResponseBlockPtr& response,
												 const HTTPRequestCompletionBlockPtr& completion) override {
					auto dataHandler = std::make_shared<DataHandler>();
					auto status = std::make_shared<HTTPRequestStatus>();
					auto streamCompletion = std::make_shared<StreamCompletion>(dataHandler, status, response, completion);
					StreamInRequestWithBody(h_,
											url,
											method,
											headerParams,
											body,
											dataHandler,
											status,
											streamCompletion);
				}
				
				//
				virtual void StreamInRequest(const HermitPtr& h_,
											 const std::string& url,
											 const std::string& method,
											 const HTTPParamVector& headerParams,
											 const DataHandlerBlockPtr& dataHandler,
											 const HTTPRequestStatusBlockPtr& status,
											 const HTTPRequestCompletionBlockPtr& completion) override {
					StreamInRequestWithBody(h_,
											url,
											method,
											headerParams,
											SharedBufferPtr(),
											dataHandler,
											status,
											completion);
				}
				
				//
				virtual void StreamInRequestWithBody(const HermitPtr& h_,
													 const std::string& url,
													 const std::string& method,
													 const HTTPParamVector& headerParams,
													 const SharedBufferPtr& body,
													 const DataHandlerBlockPtr& dataHandler,
													 const HTTPRequestStatusBlockPtr& status,
													 const HTTPRequestCompletionBlockPtr& completion) override {
					NSString* urlString = [NSString stringWithUTF8String:url.c_str()];
					NSURL* requestURL = [NSURL URLWithString: urlString];
					NSURLSessionDownloadTask* downloadTask = [mSession downloadTaskWithURL:requestURL];
					TaskParams* params = [[TaskParams alloc] initWith:dataHandler status:status completion:completion];
					objc_setAssociatedObject(downloadTask, TASK_PARAMS_KEY, params, OBJC_ASSOCIATION_RETAIN);
					[downloadTask resume];
				}
				
			private:
				//
				NSURLSession* mSession;
			};
			
		} // namespace CreateHTTPSession_Impl
		using namespace CreateHTTPSession_Impl;
		
		//
		HTTPSessionPtr CreateHTTPSession() {
			NSURLSessionConfiguration *defaultConfiguration = [NSURLSessionConfiguration defaultSessionConfiguration];
			id <NSURLSessionDelegate> delegate = [[SessionDelegate alloc] init];
			NSOperationQueue *operationQueue = [NSOperationQueue mainQueue];
			NSURLSession *defaultSession = [NSURLSession sessionWithConfiguration:defaultConfiguration
																		 delegate:delegate
																	delegateQueue:operationQueue];

			return std::make_shared<URLSessionHTTPSession>(defaultSession);
		}
		
	} // namespace http
} // namespace hermit
