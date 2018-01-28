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

#include <list>
#include <string>
#include "Hermit/Foundation/Notification.h"
#include "CreateFilePathFromCanonicalString.h"
#include "CreateFilePathFromComponents.h"
#include "CreateFilePathFromUTF8String.h"
#include "FilePath.h"
#include "FilePathImpl.h"
#include "GetCanonicalFilePathString.h"
#include "GetFilePathComponents.h"
#include "GetFilePathLeaf.h"
#include "GetFilePathUTF8String.h"

#define E38_LOG_FILEPATH_COUNT 0

#if E38_LOG_FILEPATH_COUNT
#include <iostream>
#endif

namespace hermit {
	namespace file {
		
		namespace
		{
			
#if E38_LOG_FILEPATH_COUNT
			//
			//
			static uint64_t sFilePathCount = 0;
#endif
			
			//
			//
			static const char kSeparator = GetOSPathSeparator();
			
			//
			//
			std::string EscapePath(const std::string& inString)
			{
				std::string result;
				const char* p = inString.c_str();
				while (*p != 0)
				{
					char ch = *p++;
					
					if (ch == '&')
					{
						result += "&amp;";
					}
					else if (ch == '/')
					{
						result += "&#47;";
					}
					else
					{
						result += ch;
					}
				}
				return result;
			}
			
			//
			//
			bool IsPotentialMatch(const std::string& inCandidateString,
								  const std::string& inTargetString,
								  bool& outComplete)
			{
				std::string::size_type index = 0;
				while (true)
				{
					if ((index >= inCandidateString.size()) || (index >= inTargetString.size()))
					{
						break;
					}
					if (inCandidateString[index] != inTargetString[index])
					{
						return false;
					}
					++index;
				}
				outComplete = (index == inTargetString.size());
				return true;
			}
			
			//
			//
			bool IsEscapeSequenceValid(const std::string& inEscapeSequence,
									   bool& outIsAmp,
									   bool& outIsSlash)
			{
				if (inEscapeSequence.empty())
				{
					return false;
				}
				if (inEscapeSequence[0] != '&')
				{
					return false;
				}
				if (inEscapeSequence.size() == 1)
				{
					outIsAmp = false;
					outIsSlash = false;
					return true;
				}
				if (inEscapeSequence[1] == 'a')
				{
					return IsPotentialMatch(inEscapeSequence, "&amp;", outIsAmp);
				}
				if (inEscapeSequence[1] == '#')
				{
					return IsPotentialMatch(inEscapeSequence, "&#47;", outIsSlash);
				}
				//	older versions used non-standard &slash; for the / character, we
				//	still support it during unescape.
				if (inEscapeSequence[1] == 's')
				{
					return IsPotentialMatch(inEscapeSequence, "&slash;", outIsSlash);
				}
				return false;
			}
			
			//
			//
			std::string UnescapeSpecialCharacters(const HermitPtr& h_, const std::string& inString)
			{
				std::string result;
				std::string currentEscapeSequence;
				const char* p = inString.c_str();
				while (*p != 0)
				{
					char ch = *p++;
					if (ch == '&')
					{
						if (!currentEscapeSequence.empty())
						{
							NOTIFY_ERROR(h_, "UnescapePath: unexpected sequence in string:", inString);
							return std::string();
						}
						currentEscapeSequence += ch;
					}
					else if (!currentEscapeSequence.empty())
					{
						currentEscapeSequence += ch;
						bool isAmp = false;
						bool isSlash = false;
						if (!IsEscapeSequenceValid(currentEscapeSequence, isAmp, isSlash))
						{
							NOTIFY_ERROR(h_, "UnescapePath: invalid escape sequence in string:", inString);
							return std::string();
						}
						if (isAmp)
						{
							result += '&';
							currentEscapeSequence.clear();
						}
						else if (isSlash)
						{
							result += '/';
							currentEscapeSequence.clear();
						}
					}
					else
					{
						result += ch;
					}
				}
				return result;
			}
			
			//
			//
			class FilePathImpl;
			
			//
			//
			typedef std::shared_ptr<FilePathImpl> ManagedFilePathPtr;
			
			//
			//
			class FilePathImpl
			:
			public FilePath
			{
			public:
				//
				//
				FilePathImpl()
				{
#if E38_LOG_FILEPATH_COUNT
					++sFilePathCount;
					std::cout << "FilePathCount:" << sFilePathCount << "\n";
#endif
				}
				
				//
				//
				FilePathImpl(const std::string& inPath)
				{
#if E38_LOG_FILEPATH_COUNT
					++sFilePathCount;
					std::cout << "FilePathCount:" << sFilePathCount << "\n";
#endif
					
					SplitPath(inPath);
				}
				
				//
				//
				FilePathImpl(const FilePathImpl& inOther)
				:
				mNodes(inOther.mNodes)
				{
#if E38_LOG_FILEPATH_COUNT
					++sFilePathCount;
					std::cout << "FilePathCount:" << sFilePathCount << "\n";
#endif
				}
				
				//
				//
				FilePathImpl(const FilePathImpl& inOther, const std::string& inNodeToAppend)
				:
				mNodes(inOther.mNodes)
				{
#if E38_LOG_FILEPATH_COUNT
					++sFilePathCount;
					std::cout << "FilePathCount:" << sFilePathCount << "\n";
#endif
					
					Append(inNodeToAppend);
				}
				
				//
				//
				FilePathImpl(const std::list<std::string>& inNodes)
				:
				mNodes(inNodes)
				{
#if E38_LOG_FILEPATH_COUNT
					++sFilePathCount;
					std::cout << "FilePathCount:" << sFilePathCount << "\n";
#endif
				}
				
				//
				//
				virtual ~FilePathImpl()
				{
#if E38_LOG_FILEPATH_COUNT
					--sFilePathCount;
					std::cout << "FilePathCount:" << sFilePathCount << "\n";
#endif
				}
				
				//
				void GetComponents(std::vector<std::string>& outComponents) {
					std::vector<std::string> components;
					auto end = mNodes.end();
					for (auto it = mNodes.begin(); it != end; ++it) {
						components.push_back(*it);
					}
					outComponents = components;
				}
				
				//
				void BuildFromComponents(const std::vector<std::string>& components) {
					std::list<std::string> nodes;
					auto end = components.end();
					for (auto it = components.begin(); it != end; ++it) {
						nodes.push_back(*it);
					}
					mNodes = nodes;
				}
				
				//
				//
				FilePathPtr GetParent() const
				{
					if (mNodes.size() <= 1)
					{
						return 0;
					}
					std::list<std::string> nodes(mNodes);
					nodes.pop_back();
					return std::make_shared<FilePathImpl>(nodes);
				}
				
				//
				//
				std::string GetUTF8String() const
				{
					if (mNodes.empty())
					{
						return std::string();
					}
					std::string result = mNodes.front();
					if (mNodes.size() > 1)
					{
						if (result != "/")
						{
							result += kSeparator;
						}
						auto end = mNodes.end();
						auto it = mNodes.begin();
						++it;
						while (it != end)
						{
							result += *it;
							++it;
							if (it != end)
							{
								result += kSeparator;
							}
						}
					}
					return result;
				}
				
				//
				//
				std::string GetLeaf() const
				{
					if (mNodes.empty())
					{
						return std::string();
					}
					return mNodes.back();
				}
				
				//
				//
				std::string GetCanonicalString() const
				{
					std::string result;
					auto end = mNodes.end();
					for (auto it = mNodes.begin(); it != end; ++it)
					{
						if ((it == mNodes.begin()) && (*it == "/"))
						{
							result += "/";
						}
						else
						{
							if (!result.empty() && (result != "/"))
							{
								result += "/";
							}
							result += EscapePath(*it);
						}
					}
					return result;
				}
				
			private:
				//
				//
				void SplitPath(const std::string& inPath)
				{
					std::string path(inPath);
					while (true)
					{
						if (path.empty())
						{
							break;
						}
						std::string::size_type sepPos = path.find(kSeparator);
						if (sepPos == std::string::npos)
						{
							mNodes.push_back(path);
							break;
						}
						if (sepPos == 0)
						{
							if (mNodes.empty())
							{
								mNodes.push_back(std::string("/"));
							}
						}
						else
						{
							mNodes.push_back(path.substr(0, sepPos));
						}
						path = path.substr(sepPos + 1);
					}
				}
				
				//
				void Append(const std::string& inNodeToAppend) {
					if (!inNodeToAppend.empty()) {
						mNodes.push_back(inNodeToAppend);
					}
				}
				
				//
				std::list<std::string> mNodes;
			};
			
		} // private namespace
		
		//
		void CreateFilePathFromUTF8String(const HermitPtr& h_, const std::string& pathString, FilePathPtr& outFilePath) {
			outFilePath = std::make_shared<FilePathImpl>(pathString);
		}
		
		//
		void CreateFilePathFromCanonicalString(const HermitPtr& h_, const std::string& canonicalPath, FilePathPtr& outFilePath) {
			std::vector<std::string> components;
			std::string path(canonicalPath);
			if (!path.empty() && path[0] == '/') {
				components.push_back("/");
				path = path.substr(1);
			}
			while (!path.empty()) {
				std::string::size_type slashPos = path.find('/');
				std::string component;
				if (slashPos == std::string::npos) {
					component = path;
					path.clear();
				}
				else {
					component = path.substr(0, slashPos);
					path = path.substr(slashPos + 1);
				}
				component = UnescapeSpecialCharacters(h_, component);
				if (component.empty()) {
					NOTIFY_ERROR(h_, "UnescapeSpecialCharacters failed for string:", component);
					outFilePath = nullptr;
					return;
				}
				components.push_back(component);
			}
			FilePathPtr filePath;
			CreateFilePathFromComponents(components, filePath);
			if (filePath == nullptr) {
				outFilePath = nullptr;
				return;
			}
			outFilePath = filePath;
		}
		
		//
		void AppendToFilePath(const HermitPtr& h_,
							  const FilePathPtr& inFilePath,
							  const std::string& inNodeToAppend,
							  FilePathPtr& outFilePath) {
			if (inFilePath == nullptr) {
				NOTIFY_ERROR(h_, "AppendToFilePath: inFilePath is null.");
				outFilePath = nullptr;
				return;
			}
			
			FilePathImpl* filePath = (FilePathImpl*)inFilePath.get();
			outFilePath = std::make_shared<FilePathImpl>(*filePath, inNodeToAppend);
		}
		
		//
		void GetFilePathParent(const HermitPtr& h_, const FilePathPtr& inFilePath, FilePathPtr& outParentPath) {
			if (inFilePath == nullptr) {
				NOTIFY_ERROR(h_, "GetFilePathParent: inFilePath is null.");
				outParentPath = nullptr;
				return;
			}
			
			FilePathImpl* filePath = (FilePathImpl*)inFilePath.get();
			outParentPath = filePath->GetParent();
		}
		
		//
		void GetFilePathLeaf(const HermitPtr& h_, const FilePathPtr& filePath, std::string& outLeaf) {
			if (filePath == nullptr) {
				NOTIFY_ERROR(h_, "GetFilePathLeaf: inFilePath is null.");
				outLeaf = "";
				return;
			}
			FilePathImpl* impl = (FilePathImpl*)filePath.get();
			outLeaf = impl->GetLeaf();
		}
		
		//
		void GetCanonicalFilePathString(const HermitPtr& h_, const FilePathPtr& filePath, std::string& outCanonicalPath) {
			if (filePath == nullptr) {
				NOTIFY_ERROR(h_, "GetCanonicalFilePathString: inFilePath is null.");
				outCanonicalPath = "";
				return;
			}
			FilePathImpl* impl = (FilePathImpl*)filePath.get();
			outCanonicalPath = impl->GetCanonicalString();
		}
		
		//
		void GetFilePathComponents(const HermitPtr& h_, const FilePathPtr& filePath, std::vector<std::string>& outComponents) {
			if (filePath == nullptr) {
				NOTIFY_ERROR(h_, "GetFilePathComponents: filePath is null.");
				outComponents.clear();
				return;
			}
			
			FilePathImpl* impl = (FilePathImpl*)filePath.get();
			impl->GetComponents(outComponents);
		}
		
		//
		//
		void CreateFilePathFromComponents(const std::vector<std::string>& components, FilePathPtr& outFilePath) {
			auto p = std::make_shared<FilePathImpl>();
			p->BuildFromComponents(components);
			outFilePath = p;
		}
		
		//
		void GetFilePathUTF8String(const HermitPtr& h_, const FilePathPtr& filePath, std::string& outUTF8PathString) {
			if (filePath == nullptr) {
				NOTIFY_ERROR(h_, "GetFilePathUTF8String: inFilePath is null.");
				outUTF8PathString = "";
				return;
			}
			FilePathImpl* impl = (FilePathImpl*)filePath.get();
			outUTF8PathString = impl->GetUTF8String();
		}
		
		//
		void StreamOut(const HermitPtr& h_, std::ostream& strm, const FilePathPtr& filePath) {
			// can't call GetFilePathUTF8String here since it may end up trying to call NOTIFY again
			FilePathImpl* impl = (FilePathImpl*)filePath.get();
			if (impl == nullptr) {
				strm << "<null>";
			}
			else {
				std::string pathUTF8(impl->GetUTF8String());
				if (pathUTF8.empty()) {
					strm << "<empty path>";
				}
				else {
					strm << pathUTF8;
				}
			}
		}
		
	} // namespace file
} // namespace hermit


