/*
   Copyright 2013 Software Reliability Lab, ETH Zurich

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef JSVIEWER_H_
#define JSVIEWER_H_

#include <string>

// Viewer for JavaScript code.
class JsViewer {
public:
	JsViewer();

	void jsToHTML(const std::string& js, std::string* out);

private:
	void jsLineToHTML(int line_number, const std::string& line, std::string* out);
	void jsEscapeToHTML(const std::string& line, std::string* out);


	void jsLineContinuation(int line_number, int scope_depth, std::string* out);

	int m_numLinesNoNumber;
};

#endif /* JSVIEWER_H_ */
