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

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>
#include <string>
#include "Notification.h"
#include "GetPrimaryMACAddress.h"

namespace hermit {
	
	namespace {
		
		// The following is derived from Apple Developer sample code:
		//
		// Given an iterator across a set of Ethernet interfaces, return the MAC address of the last one.
		// If no interfaces are found the MAC address is set to an empty string.
		// In this sample the iterator should contain just the primary interface.
		static kern_return_t GetMACAddress(const HermitPtr& h_, io_iterator_t intfIterator, std::string& outMACAddress) {
			kern_return_t kernResult = KERN_FAILURE;
			io_object_t intfService;
			while ((intfService = IOIteratorNext(intfIterator))) {
				// IONetworkControllers can't be found directly by the IOServiceGetMatchingServices call,
				// since they are hardware nubs and do not participate in driver matching. In other words,
				// registerService() is never called on them. So we've found the IONetworkInterface and will
				// get its parent controller by asking for it specifically.
				io_object_t controllerService;
				kernResult = IORegistryEntryGetParentEntry(intfService, kIOServicePlane, &controllerService);
				if (kernResult != KERN_SUCCESS) {
					NOTIFY_ERROR(h_, "GetMACAddress: IORegistryEntryGetParentEntry failed, kernResult:", kernResult);
				}
				else {
					CFTypeRef MACAddressAsCFData = IORegistryEntryCreateCFProperty(controllerService,
																				   CFSTR(kIOMACAddress),
																				   kCFAllocatorDefault,
																				   0);
					
					if (MACAddressAsCFData) {
						UInt8 MACAddress[kIOEthernetAddressSize];
						CFDataGetBytes((CFDataRef)MACAddressAsCFData, CFRangeMake(0, kIOEthernetAddressSize), MACAddress);
						CFRelease(MACAddressAsCFData);
						
						char addr[kIOEthernetAddressSize + 1];
						sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
								MACAddress[0],
								MACAddress[1],
								MACAddress[2],
								MACAddress[3],
								MACAddress[4],
								MACAddress[5]);
						
						outMACAddress = addr;
					}
					
					IOObjectRelease(controllerService);
				}
				IOObjectRelease(intfService);
			}
			return kernResult;
		}
		
	} // namespace
	
	//
	void GetPrimaryMACAddress(const HermitPtr& h_, const GetPrimareyMACAddressCallbackRef& callback) {
		
		CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOEthernetInterfaceClass);
		if (matchingDict == nullptr) {
			NOTIFY_ERROR(h_, "getPrimaryMACAddress: IOServiceMatching failed");
			callback.Call(kGetPrimaryMacAddressResult_Error, "");
			return;
		}
		
		CFMutableDictionaryRef propertyMatchDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
																			 &kCFTypeDictionaryKeyCallBacks,
																			 &kCFTypeDictionaryValueCallBacks);
		if (propertyMatchDict == nullptr) {
			NOTIFY_ERROR(h_, "getPrimaryMACAddress: CFDictionaryCreateMutable failed");
			callback.Call(kGetPrimaryMacAddressResult_Error, "");
			CFRelease(matchingDict);
			return;
		}
		
		CFDictionarySetValue(propertyMatchDict, CFSTR(kIOPrimaryInterface), kCFBooleanTrue);
		CFDictionarySetValue(matchingDict, CFSTR(kIOPropertyMatchKey), propertyMatchDict);
		CFRelease(propertyMatchDict);
		propertyMatchDict = nullptr;
		
		io_iterator_t matchingServices;
		kern_return_t kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &matchingServices);
		// ^^ takes ownership of matchingDict
		matchingDict = nullptr;
		
		if (kernResult != KERN_SUCCESS) {
			NOTIFY_ERROR(h_, "getPrimaryMACAddress: IOServiceGetMatchingServices failed, kernResult:", kernResult);
			callback.Call(kGetPrimaryMacAddressResult_Error, "");
			return;
		}
		
		std::string macAddress;
		kernResult = GetMACAddress(h_, matchingServices, macAddress);
		if (kernResult != KERN_SUCCESS) {
			NOTIFY_ERROR(h_, "getPrimaryMACAddress: GetMACAddress failed, kernResult:", kernResult);
			callback.Call(kGetPrimaryMacAddressResult_Error, "");
			return;
		}
		
		callback.Call(kGetPrimaryMacAddressResult_Success, macAddress.c_str());
	}
	
} // namespace hermit

