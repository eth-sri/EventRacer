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


#ifndef ESCAPING_H_
#define ESCAPING_H_

#include <string>

// Escaping routines. The result is appended to the out string.
void AppendHTMLEscape(const std::string& str, std::string* out);
void AppendStringInHTMLEscape(const std::string& str, std::string* out);
void AppendStringEscape(const std::string& str, std::string* out);

// Escaping routines. The result is returned as a string.
std::string HTMLEscape(const std::string& str);
std::string StringInHTMLEscape(const std::string& str);
std::string StringEscape(const std::string& str);


#endif /* ESCAPING_H_ */
