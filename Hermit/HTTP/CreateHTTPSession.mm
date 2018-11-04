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
#import "Hermit/Foundation/Notification.h"
#import "CreateHTTPSession.h"

// magic key for objc_setAssociatedObject
static void* const TASK_PARAMS_KEY = (void*)&TASK_PARAMS_KEY;

@interface QueueEntry : NSObject {
	const void * _bytes;
	NSRange _byteRange;
}

@property (retain) NSData* data;
@property const void* bytes;
@property NSRange byteRange;

@end

@implementation QueueEntry

-(id) initWith:(NSData*)data bytes:(const void*)bytes byteRange:(NSRange)byteRange {
	self = [super init];
	if (self != nil) {
		self.data = data;
		_bytes = bytes;
		_byteRange = byteRange;
	}
	return self;
}

@end

@interface TaskParams : NSObject {
	hermit::HermitPtr _h_;
	hermit::DataReceiverPtr _dataReceiver;
	hermit::http::HTTPRequestStatusBlockPtr _status;
	hermit::http::HTTPRequestCompletionBlockPtr _completion;
	NSMutableArray* _queue;
	NSLock* _lock;
}

- (id)initWith:(hermit::HermitPtr)h_
  dataReceiver:(hermit::DataReceiverPtr)dataReceiver
		status:(hermit::http::HTTPRequestStatusBlockPtr)status
	completion:(hermit::http::HTTPRequestCompletionBlockPtr)completion;

- (void)didReceiveData:(NSData*)data;
- (void)completion:(NSError*)error;
- (void)handleDataResult:(const hermit::StreamDataResult&)result;

@end

namespace CreateHTTPSession_Impl {
	
	//
	class RecieveCompletion : public hermit::DataCompletion {
	public:
		//
		RecieveCompletion(TaskParams* params) : mParams(params) {
		}
		
		//
		virtual void Call(const hermit::HermitPtr& h_, const hermit::StreamDataResult& result) override {
			[mParams handleDataResult:result];
		}
		
		//
		TaskParams* mParams;
	};
	
} // namespace CreateHTTPSession_Impl;
using namespace CreateHTTPSession_Impl;

@implementation TaskParams

- (id)initWith:(hermit::HermitPtr)h_
  dataReceiver:(hermit::DataReceiverPtr)dataReceiver
		status:(hermit::http::HTTPRequestStatusBlockPtr)status
	completion:(hermit::http::HTTPRequestCompletionBlockPtr)completion {
	self = [super init];
	if (self != nil) {
		_h_ = h_;
		_dataReceiver = dataReceiver;
		_status = status;
		_completion = completion;
		_lock = [[NSLock alloc] init];
		_queue = [NSMutableArray array];
	}
	return self;
}

- (void)status:(NSURLSessionTask*)task {
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

- (void)didReceiveData:(NSData*)data {
	[data enumerateByteRangesUsingBlock:^(const void *bytes, NSRange byteRange, BOOL *stop) {
		[self addItemToQueue:data bytes:bytes byteRange:byteRange];
	}];
}

- (void)addItemToQueue:(NSData*)data bytes:(const void*)bytes byteRange:(NSRange)byteRange {
	[_lock lock];
	Boolean queueWasEmpty = [_queue count] == 0;
	QueueEntry* entry = [[QueueEntry alloc] initWith:data bytes:bytes byteRange:byteRange];
	[_queue addObject:entry];
	[_lock unlock];
	
	if (queueWasEmpty) {
		[self processQueue];
	}
}

- (void)processQueue {
	QueueEntry* entry = nil;
	[_lock lock];
	if ([_queue count] > 0) {
		entry = [_queue objectAtIndex:0];
		[_queue removeObjectAtIndex:0];
	}
	[_lock unlock];
	
	if (entry != nil) {
		auto receiveCompletion = std::make_shared<RecieveCompletion>(self);
		auto buffer = hermit::DataBuffer((const char*)entry.bytes, entry.byteRange.length);
		_dataReceiver->Call(_h_, buffer, false, receiveCompletion);
	}
}

- (void)handleDataResult:(const hermit::StreamDataResult&)result {
	if (result != hermit::StreamDataResult::kSuccess) {
		NOTIFY_ERROR(self->_h_, "result != StreamDataResult::kSuccess");
	}
	[self processQueue];
}

- (void)completion:(NSError*)error {
	hermit::http::HTTPRequestResult result = hermit::http::HTTPRequestResult::kSuccess;
	if (error != nil) {
		bool isNSURLError = [[error domain] isEqualToString:NSURLErrorDomain];
		NSInteger errorCode = [error code];
		if (isNSURLError && (errorCode == kCFURLErrorNetworkConnectionLost)) {
			result = hermit::http::HTTPRequestResult::kNetworkConnectionLost;
		}
		else if (isNSURLError && (errorCode == kCFURLErrorTimedOut)) {
			result = hermit::http::HTTPRequestResult::kTimedOut;
		}
		else if (isNSURLError && (errorCode == kCFURLErrorNotConnectedToInternet)) {
			result = hermit::http::HTTPRequestResult::kNoNetworkConnection;
		}
		else {
			NSLog(@"error: %@", error);
			NOTIFY_ERROR(_h_, "DataSessionTask failed, error:", [error localizedDescription]);
			result = hermit::http::HTTPRequestResult::kError;
		}
	}
	_completion->Call(_h_, result);
	
	// We must clear this to resolve a potential circular reference.
	_completion.reset();
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
	completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task willPerformHTTPRedirection:(NSHTTPURLResponse *)response newRequest:(NSURLRequest *)request completionHandler:(void (^)(NSURLRequest *))completionHandler {
	// Don't do redirects automatically, let the client decide what to do. Passing nil here
	// will cause the response to bubble up back to the caller.
	completionHandler(nil);
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
	TaskParams* params = objc_getAssociatedObject(dataTask, TASK_PARAMS_KEY);
	if (params != nil) {
		[params didReceiveData:data];
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
			class Receiver : public DataReceiver {
			public:
				//
				virtual void Call(const HermitPtr& h_,
								  const DataBuffer& data,
								  const bool& isEndOfData,
								  const DataCompletionPtr& completion) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					completion->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<Receiver> ReceiverPtr;

			//
			class StreamCompletion : public HTTPRequestCompletionBlock {
			public:
				//
				StreamCompletion(const ReceiverPtr& dataReceiver,
								 const HTTPRequestStatusPtr& status,
								 const HTTPRequestResponseBlockPtr& response,
								 const HTTPRequestCompletionBlockPtr& completion) :
				mDataReceiver(dataReceiver),
				mStatus(status),
				mResponse(response),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const HTTPRequestResult& result) {
					if (result == HTTPRequestResult::kSuccess) {
						auto buffer = DataBuffer(mDataReceiver->mData.data(), mDataReceiver->mData.size());
						mResponse->Call(h_,
										mStatus->mStatusCode,
										mStatus->mHeaderParams,
										buffer);
					}
					mCompletion->Call(h_, result);
				}
				
				//
				ReceiverPtr mDataReceiver;
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
					auto dataReceiver = std::make_shared<Receiver>();
					auto status = std::make_shared<HTTPRequestStatus>();
					auto streamCompletion = std::make_shared<StreamCompletion>(dataReceiver, status, response, completion);
					StreamInRequestWithBody(h_,
											url,
											method,
											headerParams,
											body,
											dataReceiver,
											status,
											streamCompletion);
				}
				
				//
				virtual void StreamInRequest(const HermitPtr& h_,
											 const std::string& url,
											 const std::string& method,
											 const HTTPParamVector& headerParams,
											 const DataReceiverPtr& dataHandler,
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
													 const DataReceiverPtr& dataReceiver,
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
					NSURLSessionDataTask* task = nil;
					if ((body != nullptr) && (body->Size() > 0)) {
						NSData* data = [NSData dataWithBytes:body->Data() length:body->Size()];
						task = [mSession uploadTaskWithRequest:request fromData:data];
					}
					else {
						task = [mSession dataTaskWithRequest:request];
					}
					TaskParams* params = [[TaskParams alloc] initWith:h_ dataReceiver:dataReceiver status:status completion:completion];
					objc_setAssociatedObject(task, TASK_PARAMS_KEY, params, OBJC_ASSOCIATION_RETAIN);
					[task resume];
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
