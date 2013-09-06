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

#include "JsViewer.h"
#include "stringprintf.h"

#include "Escaping.h"

JsViewer::JsViewer() {}

void JsViewer::jsToHTML(const std::string& js, std::string* out) {
	bool in_utf8_char = false;

	std::string current_line;
	int line_number = 0;
	for (size_t i = 0; i < js.size(); ++i) {
		char c = js[i];
		current_line.push_back(c);
		if (c >= 128) {
			in_utf8_char = true;
			continue;
		}
		if (in_utf8_char) {
			in_utf8_char = false;
			continue;
		}
		if (c == '\n') {
			jsLineToHTML(++line_number, current_line, out);
			current_line.clear();
		}
	}
	jsLineToHTML(++line_number, current_line, out);
}

void JsViewer::jsLineToHTML(int line_number, const std::string& js, std::string* out) {
	StringAppendF(out, "<a name=\"l%d\">%6d</a> :         ", line_number, line_number);
	m_numLinesNoNumber = 0;

	int num_scopes = 0;
	for (size_t i = 0; i < js.size(); ++i) {
		num_scopes += (js[i] == '{');
	}
	if (num_scopes <= 2) {
		AppendHTMLEscape(js, out);
		return;
	}
	// Try to beautify the line:
	std::string current_line;
	int scope = 4;
	bool in_utf8_char;
	for (size_t i = 0; i < js.size(); ++i) {
		char c = js[i];
		if (c >= 128) {
			in_utf8_char = true;
			current_line.push_back(c);
			continue;
		}
		if (in_utf8_char) {
			in_utf8_char = false;
			current_line.push_back(c);
			continue;
		}
		char nextc = '\0';
		if (i + 1 < js.size()) {
			nextc = js[i + 1];
		}
		if (c == '}') {
			AppendHTMLEscape(current_line, out);
			current_line.clear();
			--scope;
			jsLineContinuation(line_number, scope, out);
			current_line.push_back(c);
			if (nextc == ',' || nextc == ';') {
				current_line.push_back(nextc);
				++i;
			}
			AppendHTMLEscape(current_line, out);
			jsLineContinuation(line_number, scope, out);
			current_line.clear();
		} else if (c == '{' || c == ';') {
			current_line.push_back(c);
			AppendHTMLEscape(current_line, out);
			if (c=='{') { ++scope; }
			jsLineContinuation(line_number, scope, out);
			current_line.clear();
		} else {
			current_line.push_back(c);
		}

	}
	AppendHTMLEscape(current_line, out);
}

void JsViewer::jsLineContinuation(int line_number, int scope_depth, std::string* out) {
	++m_numLinesNoNumber;
	if (m_numLinesNoNumber == 9) {
		out->append("\n   ... : ");
	} else if (m_numLinesNoNumber == 10) {
		StringAppendF(out, "\n%6d : ", line_number);
		m_numLinesNoNumber = 0;
	} else {
		out->append("\n       : ");
	}
	for (int i = 0; i < scope_depth; ++i) out->append("  ");
}


