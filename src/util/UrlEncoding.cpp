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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "UrlEncoding.h"
#include "stringprintf.h"


namespace {
int HexToI(char c) {
	return isdigit(c) ? c - '0' : c - 'W';
}
}

std::string URLDecode(const std::string& url_str, bool is_form_url_encoded) {
	std::string dst;

	const char* src = url_str.c_str();
	int src_len = url_str.size();
	for (int i = 0; i < src_len; ++i) {
		if (src[i] == '%' && i < src_len - 2 &&
				isxdigit(* (src + i + 1)) &&
				isxdigit(* (src + i + 2))) {
			char a = tolower(* (src + i + 1));
			char b = tolower(* (src + i + 2));
			dst.push_back( static_cast<char>((HexToI(a) << 4) | HexToI(b)) );
			i += 2;
		} else if (is_form_url_encoded && src[i] == '+') {
			dst.push_back(' ');
		} else {
			dst.push_back(src[i]);
		}
	}

	return dst;
}

std::string URLEncode(const std::string& enc_str) {
	std::string dst;
	static const char *dont_escape = "._-$,;~()";
	static const char *hex = "0123456789abcdef";

	const char* src = enc_str.c_str();
	for (; *src != '\0'; ++src) {
		if (isalnum(*src) ||
			strchr(dont_escape, *src) != NULL) {
			dst.push_back(*src);
		} else {
			dst.push_back('%');
			unsigned char ch = *src;
			dst.push_back(hex[ch >> 4]);
			dst.push_back(hex[ch & 0xf]);
		}
	}
	return dst;
}

std::string serializeIntVector(const std::vector<int>& values) {
	std::string result;
	for (size_t i = 0; i < values.size(); ++i) {
		if (!result.empty()) result.append(":");
		StringAppendF(&result, "%d", values[i]);
	}
	return result;
}

void parseIntVector(const std::string& s, std::vector<int>* result) {
	size_t start_pos = 0;
	for (;;) {
		size_t end_pos = s.find(":", start_pos);
		std::string sub = s.substr(
				start_pos,
				(end_pos == std::string::npos) ? std::string::npos : (end_pos - start_pos));
		int tmp;
		if (sscanf(sub.c_str(), "%d", &tmp) == 1) result->push_back(tmp);
		if (end_pos == std::string::npos) break;
		start_pos = end_pos + 1;
	}
}

void URLParams::parse(const std::string& s) {
	size_t start_pos = 0;
	for (;;) {
		size_t end_pos = s.find("&", start_pos);
		std::string sub = s.substr(
				start_pos,
				(end_pos == std::string::npos) ? std::string::npos : (end_pos - start_pos));
		size_t eq_pos = sub.find("=");
		if (eq_pos != std::string::npos) {
			m_decodedMap[sub.substr(0, eq_pos)] = URLDecode(sub.substr(eq_pos + 1), true);
		}
		if (end_pos == std::string::npos) break;
		start_pos = end_pos + 1;
	}
}

std::string URLParams::toString() const {
	std::string result;
	for (DecodedMap::const_iterator it = m_decodedMap.begin(); it != m_decodedMap.end(); ++it) {
		if (!result.empty()) result += "&";
		StringAppendF(&result, "%s=%s", it->first.c_str(), URLEncode(it->second.c_str()).c_str());
	}
	return result;
}

void URLParams::setInt(const std::string& key, int value) {
	m_decodedMap[key] = StringPrintf("%d", value);
}

int URLParams::getIntDefault(const std::string& key, int d) const {
	DecodedMap::const_iterator it = m_decodedMap.find(key);
	if (it == m_decodedMap.end()) return d;
	int result = d;
	sscanf(it->second.c_str(), "%d", &result);
	return result;
}

std::string URLParams::getString(const std::string& key) const {
	DecodedMap::const_iterator it = m_decodedMap.find(key);
	if (it == m_decodedMap.end()) return "";
	return it->second.c_str();
}
