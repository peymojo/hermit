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

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Foundation/FoundationErrors.h>
#import <string>
#import <vector>
#import "Hermit/Foundation/Notification.h"
#import "StreamInHTTPRequestWithBody.h"

// NSURLConnection deprecated in favor of NSUrlSession, which is a bigger change than
// I can justify diving into at this time. Defect added in cloudforge in the Vault Browser
// project.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#if !__has_feature(objc_arc)

#define AUTORELEASE(x) [x autorelease]
#define RETAIN(x) [x retain]
#define RELEASE(x) [x release]

#else

#define AUTORELEASE(x) x
#define RETAIN(x)
#define RELEASE(x)

#endif


namespace hermit {
	namespace http {
		
		namespace {
			
			//
			typedef std::pair<std::string, std::string> HTTPParam;
			typedef std::vector<HTTPParam> HTTPParamVector;
			
			//
			class ResponseParams : public EnumerateStringValuesFunction {
			public:
				//
				ResponseParams(NSDictionary* inParams)
				:
				mParams(inParams) {
				}
				
				//
				bool Function(const EnumerateStringValuesCallbackRef& inCallback) {
					NSEnumerator* enumerator = [mParams keyEnumerator];
					while (true) {
						NSString* key = [enumerator nextObject];
						if (key == nil) {
							break;
						}
						NSString* value = [mParams objectForKey:key];
						if (value == nil) {
							break;
						}
						if (!inCallback.Call([key UTF8String], [value UTF8String])) {
							return false;
						}
					}
					return true;
				}
				
			private:
				//
				NSDictionary* mParams;
			};
			
		} // private namespace
		
	} // namespace http
} // namespace hermit


@interface StreamInHTTPRequestWithBody_Cocoa : NSObject<NSURLConnectionDelegate> {
	hermit::HermitPtr _hermit;
	hermit::http::StreamInHTTPRequestDataHandlerFunctionRef _dataHandlerFunction;
}

@property (nonatomic, copy) NSString* address;
@property (nonatomic, copy) NSString* method;
@property (nonatomic, retain) NSMutableDictionary* params;
@property (nonatomic, retain) NSData* body;
@property (nonatomic, retain) NSError* error;
@property (nonatomic, retain) NSURLResponse* response;
@property (nonatomic) uint64_t expectedContentLength;
@property (nonatomic) BOOL done;
@property (nonatomic) BOOL canceled;

@end

@implementation StreamInHTTPRequestWithBody_Cocoa

@synthesize address = _address;
@synthesize method = _method;
@synthesize params = _params;
@synthesize body = _body;
@synthesize error = _error;
@synthesize response = _response;
@synthesize expectedContentLength = _expectedContentLength;
@synthesize done = _done;
@synthesize canceled = _canceled;

- (id)initWithHermit:(hermit::HermitPtr)hermit
			 address:(NSString*)address
			   method:(NSString*)method
				 body:(NSData*)body
  dataHandlerFunction:(const hermit::http::StreamInHTTPRequestDataHandlerFunctionRef&)dataHandlerFunction {
	
	self = [super init];
	if (self != nil) {
		_hermit = hermit;
		_dataHandlerFunction = dataHandlerFunction;
		
		self.address = address;
		self.method = method;
		NSMutableDictionary* dict = AUTORELEASE([[NSMutableDictionary alloc] init]);
		self.params = dict;
		self.body = body;
		self.expectedContentLength = 0;
	}
	return self;
}

#if !__has_feature(objc_arc)
- (void)dealloc {
	[_address release];
	[_method release];
	[_params release];
	[_body release];
	[_error release];
	[_response release];
	
	_hermit = nullptr;
	
	[super dealloc];
}
#endif

- (void)addParam:(const char*)name value:(const char*)value {
	NSString* nameString = [NSString stringWithUTF8String:name];
	NSString* valueString = [NSString stringWithUTF8String:value];
	[self.params setValue:valueString forKey:nameString];
}

- (BOOL)start {
	NSMutableURLRequest* request = [[NSMutableURLRequest alloc] init];
	[request setCachePolicy:NSURLRequestReloadIgnoringCacheData];
	[request setURL:[NSURL URLWithString:[self address]]];
	[request setHTTPMethod:[self method]];
	[request setHTTPBody:[self body]];
	
	//	NSString* bodyText = nil;
	//	if ([self body] != nil) {
	//		bodyText = [NSString stringWithUTF8String:(const char*)[[self body] bytes]];
	//	}
	//	NSLog(@"--> method: %@\n--> url: %@\n--> params: %@\n--> body: %@", [self method], [self address], [self params], bodyText);
	for (id key in [self params]) {
		[request addValue:[[self params] objectForKey:key] forHTTPHeaderField:key];
	}
	
	self.done = NO;
	self.canceled = NO;
	self.error = nil;
	NSURLConnection* urlConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self];
	if (urlConnection == nil) {
		RELEASE(request);
		return NO;
	}
	
	while (!_done) {
		if (CHECK_FOR_ABORT(_hermit)) {
			[urlConnection cancel];
			self.canceled = YES;
			break;
		}
		
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
	}
	
	RELEASE(urlConnection);
	RELEASE(request);
	
	return (_error == nil);
}

- (void)connection:(NSURLConnection*)connection willSendRequestForAuthenticationChallenge:(NSURLAuthenticationChallenge*)challenge {
	[challenge.sender performDefaultHandlingForAuthenticationChallenge:challenge];
}

- (NSURLRequest*)connection:(NSURLConnection*)connection willSendRequest:(NSURLRequest*)request redirectResponse:(NSURLResponse*)response {
	if (response != nil) {
		//		NSLog(@"willSendRequest: %@; redirectResponse:%@", request, response);
		
		self.response = response;
		self.expectedContentLength = [response expectedContentLength];
		return nil;
	}
	return request;
}

- (void)connection:(NSURLConnection*)connection didReceiveResponse:(NSURLResponse*)response {
	self.response = response;
	self.expectedContentLength = [response expectedContentLength];
}

- (void)connection:(NSURLConnection*)connection didReceiveData:(NSData*)data {
	[data enumerateByteRangesUsingBlock:^(const void *bytes, NSRange byteRange, BOOL *stop) {
		_dataHandlerFunction.Call(_expectedContentLength, hermit::DataBuffer((const char*)bytes, byteRange.length), false);
	}];
}

- (void)connectionDidFinishLoading:(NSURLConnection*)connection {
	_dataHandlerFunction.Call(_expectedContentLength, hermit::DataBuffer("", 0), true);
	self.done = YES;
}

- (void)connection:(NSURLConnection*)connection didFailWithError:(NSError*)error {
	self.error = error;
	self.done = YES;
}

- (NSCachedURLResponse*)connection:(NSURLConnection*)connection willCacheResponse:(NSCachedURLResponse*)cachedResponse {
	return nil;
}


@end

namespace hermit {
	namespace http {
		
		//
		void StreamInHTTPRequestWithBody(const HermitPtr& h_,
										 const std::string& inURL,
										 const std::string& inMethod,
										 const EnumerateStringValuesFunctionRef& inHeaderParamsFunction,
										 const DataBuffer& inBodyData,
										 const StreamInHTTPRequestDataHandlerFunctionRef& inDataHandlerFunction,
										 const StreamInHTTPRequestCallbackRef& inCallback) {
			@autoreleasepool {
				NSData* body = nil;
				if ((inBodyData.first != nullptr) && (inBodyData.second > 0)) {
					body = [NSData dataWithBytesNoCopy:(void*)inBodyData.first length:(size_t)inBodyData.second freeWhenDone:NO];
				}
				
				StreamInHTTPRequestWithBody_Cocoa* impl = [[StreamInHTTPRequestWithBody_Cocoa alloc]
														   initWithHermit:h_
														   address:[NSString stringWithUTF8String:inURL.c_str()]
														   method:[NSString stringWithUTF8String:inMethod.c_str()]
														   body:body
														   dataHandlerFunction:inDataHandlerFunction];
				
				EnumerateStringValuesCallbackClass headerParams;
				if (!inHeaderParamsFunction.Call(headerParams)) {
					NOTIFY_ERROR(h_, "StreamInHTTPRequestWithBody: inHeaderParamsFunction returned false.");
					inCallback.Call(HTTPRequestResult::kError, 0, nullptr);
				}
				else {
					auto end = headerParams.mValues.end();
					for (auto it = headerParams.mValues.begin(); it != end; ++it) {
						[impl addParam:(*it).first.c_str() value:(*it).second.c_str()];
					}
					bool success = [impl start];
					if (!success) {
						NSString* errorDomain = [impl.error domain];
						NSInteger errorCode = [impl.error code];
						
						if ([errorDomain isEqualToString:NSURLErrorDomain] &&
							((errorCode == NSURLErrorNetworkConnectionLost) ||
							 // in practice the following errors seems to occur under similar circumstances
							 // and we'd like to treat them as a retry
							 (errorCode == NSURLErrorSecureConnectionFailed) ||
							 (errorCode == NSURLErrorCannotParseResponse))) {
								
								if (errorCode == NSURLErrorSecureConnectionFailed) {
									NOTIFY_WARNING(h_, "Treating NSURLErrorSecureConnectionFailed as NetworkConnectionLost");
								}
								else if (errorCode == NSURLErrorCannotParseResponse) {
									NOTIFY_WARNING(h_, "Treating NSURLErrorCannotParseResponse as NetworkConnectionLost");
								}
								inCallback.Call(HTTPRequestResult::kNetworkConnectionLost, 0, nullptr);
							}
						else if ([errorDomain isEqualToString:NSURLErrorDomain] && (errorCode == NSURLErrorCannotFindHost)) {
							inCallback.Call(HTTPRequestResult::kHostNotFound, 0, nullptr);
						}
						else if ([errorDomain isEqualToString:NSURLErrorDomain] && (errorCode == NSURLErrorNotConnectedToInternet)) {
							inCallback.Call(HTTPRequestResult::kNoNetworkConnection, 0, nullptr);
						}
						else if ([errorDomain isEqualToString:NSURLErrorDomain] && (errorCode == NSURLErrorTimedOut)) {
							inCallback.Call(HTTPRequestResult::kTimedOut, 0, nullptr);
						}
						else {
							NOTIFY_ERROR(h_, "StreamInHTTPRequestWithBody: [impl start] returned false.");
							NOTIFY_ERROR(h_, "\terror:", [[impl.error localizedDescription] UTF8String]);
							NOTIFY_ERROR(h_, "\terror domain:", [errorDomain UTF8String]);
							NOTIFY_ERROR(h_, "\terror code:", (int32_t)errorCode);
							
							inCallback.Call(HTTPRequestResult::kError, 0, nullptr);
						}
					}
					else if (impl.canceled) {
						inCallback.Call(HTTPRequestResult::kCanceled, 0, nullptr);
					}
					else {
						int responseStatusCode = (int)[(NSHTTPURLResponse*)impl.response statusCode];
						NSDictionary* responseHeaderParams = [(NSHTTPURLResponse*)impl.response allHeaderFields];
						
						//				NSLog(@"<-- code: %d\n<-- params: %@", responseStatusCode, responseHeaderParams);
						
						ResponseParams responseParams(responseHeaderParams);
						inCallback.Call(HTTPRequestResult::kSuccess, responseStatusCode, responseParams);
					}
				}
				RELEASE(impl);
			}
		}
		
	} // namespace http
} // namespace hermit

#pragma GCC diagnostic pop
