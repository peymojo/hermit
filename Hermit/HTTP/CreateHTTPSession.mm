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

namespace hermit {
	namespace http {
		namespace CreateHTTPSession_Impl {
			
			//
			class StreamResult : public StreamResultBlock {
			public:
				//
				StreamResult() : mResult(StreamDataResult::kUnknown) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, StreamDataResult result) override {
					mResult = result;
				}
				
				//
				StreamDataResult mResult;
			};

		} // namespace CreateHTTPSession_Impl
		using namespace CreateHTTPSession_Impl;
	} // namespace http
} // namespace hermit


// magic key for objc_setAssociatedObject
static void* const TASK_PARAMS_KEY = (void*)&TASK_PARAMS_KEY;

@interface TaskParams : NSObject {
	hermit::HermitPtr _h_;
	hermit::DataHandlerBlockPtr _dataHandler;
	hermit::http::HTTPRequestStatusBlockPtr _status;
	hermit::http::HTTPRequestCompletionBlockPtr _completion;
}

- (id)initWith:(hermit::HermitPtr)h_
   dataHandler:(hermit::DataHandlerBlockPtr)dataHandler
		status:(hermit::http::HTTPRequestStatusBlockPtr)status
	completion:(hermit::http::HTTPRequestCompletionBlockPtr)completion;

- (void)sendData:(NSData*)data;
- (void)completion:(NSError*)error;

@end

@implementation TaskParams

- (id)initWith:(hermit::HermitPtr)h_
   dataHandler:(hermit::DataHandlerBlockPtr)dataHandler
		status:(hermit::http::HTTPRequestStatusBlockPtr)status
	completion:(hermit::http::HTTPRequestCompletionBlockPtr)completion {
	self = [super init];
	if (self != nil) {
		_h_ = h_;
		_dataHandler = dataHandler;
		_status = status;
		_completion = completion;
	}
	return self;
}

- (void)status:(NSURLSessionTask*) task {
	NSURLResponse* response = task.response;
	if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
		NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
		NSDictionary* headerParams = httpResponse.allHeaderFields;
		__block hermit::http::HTTPParamVector params;
		[headerParams enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop) {
			std::string name([key UTF8String]);
			std::string value([obj UTF8String]);
			params.push_back(std::make_pair(name, value));
		}];
		
		_status->Call(_h_, (int)httpResponse.statusCode, params);
	}
}

- (void)sendData:(NSData*)data {
	[data enumerateByteRangesUsingBlock:^(const void *bytes, NSRange byteRange, BOOL *stop) {
		auto completion = std::make_shared<hermit::http::StreamResult>();
		_dataHandler->HandleData(_h_, hermit::DataBuffer((const char*)bytes, byteRange.length), false, completion);
	}];
}

- (void)completion:(NSError*)error {
	hermit::http::HTTPRequestResult result = hermit::http::HTTPRequestResult::kSuccess;
	if (error != nil) {
		NSLog(@"error: %@", error);
		result = hermit::http::HTTPRequestResult::kError;
	}
	_completion->Call(_h_, result);
}

@end


@interface SessionDelegate : NSObject<NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLSessionDataDelegate, NSURLSessionDownloadDelegate, NSURLSessionStreamDelegate>

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(nullable NSError *)error;

@end

@implementation SessionDelegate

- (void)URLSession:(NSURLSession *)session didBecomeInvalidWithError:(nullable NSError *)error {
	NSLog(@"didBecomeInvalidWithError");
}

- (void)URLSession:(NSURLSession *)session didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential * _Nullable credential))completionHandler {
	NSLog(@"session didReceiveChallenge");
	completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task willPerformHTTPRedirection:(NSHTTPURLResponse *)response newRequest:(NSURLRequest *)request completionHandler:(void (^)(NSURLRequest *))completionHandler {
	NSLog(@"willPerformHTTPRedirection");
	completionHandler(request);
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential * _Nullable credential))completionHandler {
	NSLog(@"task didReceiveChallenge");
	TaskParams* params = objc_getAssociatedObject(task, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task needNewBodyStream:(void (^)(NSInputStream * _Nullable bodyStream))completionHandler {
	NSLog(@"needNewBodyStream");
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(nullable NSError *)error {
	NSLog(@"didCompleteWithError");
	TaskParams* params = objc_getAssociatedObject(task, TASK_PARAMS_KEY);
	if (params != nil) {
		[params status:task];
		[params completion:error];
	}
}

//- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveResponse:(NSURLResponse *)response completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
//	NSLog(@"didReceiveResponse");
//	completionHandler(NSURLSessionResponseAllow);
//}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveData:(NSData *)data {
	NSLog(@"didReceiveData");
	TaskParams* params = objc_getAssociatedObject(dataTask, TASK_PARAMS_KEY);
	if (params != nil) {
		[params sendData:data];
	}
}

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)totalBytesWritten totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
	NSLog(@"didWriteData");
	TaskParams* params = objc_getAssociatedObject(downloadTask, TASK_PARAMS_KEY);
	if (params != nil) {
	}
}

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(NSURL *)location {
	NSLog(@"didFinishDownloadingToURL");
	TaskParams* params = objc_getAssociatedObject(downloadTask, TASK_PARAMS_KEY);
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
					NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:requestURL];
					request.HTTPMethod = [NSString stringWithUTF8String:method.c_str()];
					auto end = headerParams.end();
					for (auto it = headerParams.begin(); it != end; ++it) {
						NSString* value = [NSString stringWithUTF8String:(*it).second.c_str()];
						NSString* field = [NSString stringWithUTF8String:(*it).first.c_str()];
						[request setValue:value forHTTPHeaderField:field];
					}
					NSURLSessionDataTask* dataTask = [mSession dataTaskWithRequest:request];
					TaskParams* params = [[TaskParams alloc] initWith:h_
														  dataHandler:dataHandler
															   status:status
														   completion:completion];
					objc_setAssociatedObject(dataTask, TASK_PARAMS_KEY, params, OBJC_ASSOCIATION_RETAIN);
					[dataTask resume];
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
			NSURLSession *defaultSession = [NSURLSession sessionWithConfiguration:defaultConfiguration
																		 delegate:delegate
																	delegateQueue:nil];

			return std::make_shared<URLSessionHTTPSession>(defaultSession);
		}
		
	} // namespace http
} // namespace hermit
