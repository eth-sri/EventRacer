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

#include "Escaping.h"

void AppendHTMLEscape(const std::string& str, std::string* out) {
	bool in_utf8_char = false;
	for (size_t pos = 0; pos < str.size(); ++pos) {
		char c = str[pos];
		if (c >= 128) {
			in_utf8_char = true;
			out->push_back(c);
			continue;
		}
		if (in_utf8_char) {
			in_utf8_char = false;
			out->push_back(c);
			continue;
		}
		switch(c) {
			case '&':  out->append("&amp;"); break;
			case '\"': out->append("&quot;");  break;
			case '\'': out->append("&apos;"); break;
			case '<':  out->append("&lt;"); break;
			case '>':  out->append("&gt;"); break;
			default:   out->push_back(c); break;
		}
	}
}

void AppendStringInHTMLEscape(const std::string& str, std::string* out) {
	bool in_utf8_char = false;
	for (size_t pos = 0; pos < str.size(); ++pos) {
		char c = str[pos];
		if (c >= 128) {
			in_utf8_char = true;
			out->push_back(c);
			continue;
		}
		if (in_utf8_char) {
			in_utf8_char = false;
			out->push_back(c);
			continue;
		}
		switch(c) {
			case '\"': out->append("\\\""); break;
			case '\\': out->append("\\\\");  break;
			case '<':  out->append("<\"+\""); break;  // To avoid tricking the parser this is an HTML tag.
			default:   out->push_back(c); break;
		}
	}
}

void AppendStringEscape(const std::string& str, std::string* out) {
	bool in_utf8_char = false;
	for (size_t pos = 0; pos < str.size(); ++pos) {
		char c = str[pos];
		if (c >= 128) {
			in_utf8_char = true;
			out->push_back(c);
			continue;
		}
		if (in_utf8_char) {
			in_utf8_char = false;
			out->push_back(c);
			continue;
		}
		switch(c) {
			case '\"': out->append("\\\""); break;
			case '\\': out->append("\\\\");  break;
			default:   out->push_back(c); break;
		}
	}
}


std::string HTMLEscape(const std::string& str) {
	std::string r;
	AppendHTMLEscape(str, &r);
	return r;
}

std::string StringInHTMLEscape(const std::string& str) {
	std::string r;
	AppendStringInHTMLEscape(str, &r);
	return r;
}

std::string StringEscape(const std::string& str) {
	std::string r;
	AppendStringEscape(str, &r);
	return r;
}



