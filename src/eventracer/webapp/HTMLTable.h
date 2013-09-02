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

#ifndef HTMLTABLE_H_
#define HTMLTABLE_H_

#include <string>
#include <vector>

class HTMLTable {
public:
	HTMLTable(int num_columns, std::string* out);

	void setColumn(int index, const std::string& s);
	void setColumnF(int index, const char* format, ...)
	  __attribute__((__format__ (__printf__, 3, 4)));

	void clearColumnData();

	void writeHeader();
	void writeRow(const char* row_color);
	void writeExpandableRow(const char* row_color, const std::string& expanded_text);

	void writeFooter(bool write_num_rows);

	static void addJavaScript(std::string* out);
private:

	std::vector<std::string> m_columns;
	std::string* m_out;
	int m_numRows;
};


#endif /* HTMLTABLE_H_ */
