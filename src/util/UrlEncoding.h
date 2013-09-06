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

#ifndef URLENCODING_H_
#define URLENCODING_H_

#include <map>
#include <string>
#include <vector>

std::string serializeIntVector(const std::vector<int>& values);
void parseIntVector(const std::string& s, std::vector<int>* result);

std::string URLDecode(const std::string& url_str, bool is_form_url_encoded);
std::string URLEncode(const std::string& url_str);

class URLParams {
public:
	void parse(const std::string& s);

	std::string toString() const;

	typedef std::map<std::string, std::string> DecodedMap;

	const DecodedMap& values() const { return m_decodedMap; }

	void setString(const std::string& key, const std::string& value) {
		m_decodedMap[key] = value;
	}

	void setInt(const std::string& key, int value);

	int getIntDefault(const std::string& key, int d) const;

	std::string getString(const std::string& key) const;

private:
	DecodedMap m_decodedMap;
};



#endif /* URLENCODING_H_ */
