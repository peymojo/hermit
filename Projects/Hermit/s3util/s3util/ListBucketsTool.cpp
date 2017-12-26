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

#include <iostream>
#include "Hermit/S3/S3ListBuckets.h"
#include "ListBucketsTool.h"
#include "ReadKeyFile.h"

namespace hermit {

	namespace ListBucketsTool_Impl {

		//
		void usage(bool local) {
			if (local) {
				std::cout << "usage:\n";
			}
			std::cout << "\tlist_buckets <aws_public_key> <path_to_file_containing_aws_private_key>\n";
		}
		
		//
		class BucketNameReceiver : public s3::BucketNameReceiver {
		public:
			//
			BucketNameReceiver() {
			}
			
			//
			virtual bool operator()(const std::string& bucketName) {
				std::cout << bucketName << "\n";
				return true;
			}
		};
		
		//
		int list_buckets(const HermitPtr& h_, const std::string& s3PublicKey, const std::string& pathToS3PrivateKeyFileUTF8) {
			std::string s3PrivateKey;
			if (ReadKeyFile(h_, pathToS3PrivateKeyFileUTF8.c_str(), s3PrivateKey)) {
				BucketNameReceiver bucketNameReceiver;
				auto result = s3::S3ListBuckets(h_, s3PublicKey, s3PrivateKey, bucketNameReceiver);
				if (result != s3::S3Result::kSuccess) {
					std::cout << "list_buckets(): S3ListBuckets() failed.\n";
					return EXIT_FAILURE;
				}
			}
			return 0;
		}
		
		//
		int list_buckets(const HermitPtr& h_, const std::list<std::string>& inArgs) {
			bool gotS3PublicKey = false;
			std::string s3PublicKey;
			bool gotPathToS3PrivateKeyFileUTF8 = false;
			std::string pathToS3PrivateKeyFileUTF8;
			
			std::list<std::string> args(inArgs);
			while (!args.empty()) {
				std::string arg(args.front());
				args.pop_front();
				
				if (!gotS3PublicKey) {
					s3PublicKey = arg;
					gotS3PublicKey = true;
				}
				else if (!gotPathToS3PrivateKeyFileUTF8) {
					pathToS3PrivateKeyFileUTF8 = arg;
					gotPathToS3PrivateKeyFileUTF8 = true;
				}
				else {
					usage(true);
					return -1;
				}
			}
			
			if (!gotS3PublicKey || !gotPathToS3PrivateKeyFileUTF8) {
				usage(true);
				return -1;
			}
			
			return list_buckets(h_, s3PublicKey, pathToS3PrivateKeyFileUTF8);
		}
		
	} // namespace ListBucketsTool_Impl
	using namespace ListBucketsTool_Impl;
	
	//
	void ListBucketsTool::Usage() const {
		usage(false);
	}
	
	//
	int ListBucketsTool::Run(const HermitPtr& h_, const std::list<std::string>& args) {
		return list_buckets(h_, args);
	}

} // namespace hermit
