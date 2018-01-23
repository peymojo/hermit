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

#ifndef OperationTimer_h
#define OperationTimer_h

#include <string>
#include <time.h>

namespace hermit {
    namespace utility {

        //
        template <typename Reporter> class OperationTimer {
        public:
            //
            OperationTimer(Reporter& reporter) :
            mReporter(reporter),
            mStart(0),
            mSuppressOutput(false) {
                time(&mStart);
            }
            
            //
            OperationTimer(Reporter& reporter, const std::string& tag) :
            mReporter(reporter),
            mStart(0),
            mTag(tag),
            mSuppressOutput(false) {
                time(&mStart);
            }
            
            //
            ~OperationTimer() {
                time_t end = 0;
                time(&end);
                if (!mSuppressOutput) {
                    mReporter.Report(mTag, end - mStart);
                }
            }
            
            //
            void SuppressOutput() {
                mSuppressOutput = true;
            }
            
        private:
            //
            Reporter& mReporter;
            time_t mStart;
            std::string mTag;
            bool mSuppressOutput;
        };
	
    } // namespace utility
} // namespace hermit

#endif
