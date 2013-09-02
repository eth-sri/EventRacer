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

#include "HTMLTable.h"

#include "stringprintf.h"


HTMLTable::HTMLTable(int num_columns, std::string* out)
    : m_columns(num_columns), m_out(out), m_numRows(0) {
}

void HTMLTable::setColumn(int index, const std::string& s) {
	m_columns[index] = s;
}

void HTMLTable::setColumnF(int index, const char* format, ...) {
	va_list ap;
	va_start(ap, format);
	std::string s;
	StringAppendV(&s, format, ap);
	va_end(ap);
	m_columns[index] = s;
}

void HTMLTable::clearColumnData() {
	m_columns.assign(m_columns.size(), "");
}

void HTMLTable::writeHeader() {
	m_out->append("<table width=\"100%\">\n<tr>");
	for (size_t i = 0; i < m_columns.size(); ++i) {
		m_out->append("<td><b>");
		m_out->append(m_columns[i]);
		m_out->append("</b></td>");
	}
	m_out->append("</tr>\n");

	clearColumnData();
}

void HTMLTable::writeRow(const char* row_color) {
	StringAppendF(m_out, "<tr class=\"r%s%d\">", row_color, m_numRows % 2);
	for (size_t i = 0; i < m_columns.size(); ++i) {
		m_out->append("<td>");
		m_out->append(m_columns[i]);
		m_out->append("</td>");
	}
	m_out->append("</tr>\n");

	++m_numRows;
	clearColumnData();
}

void HTMLTable::writeExpandableRow(const char* row_color, const std::string& expanded_text) {
	StringAppendF(m_out, "<tr class=\"r%s%d clickable\" onclick=\"javascript:toggle('row%d')\">",
			row_color, m_numRows % 2, m_numRows);
	for (size_t i = 0; i < m_columns.size(); ++i) {
		m_out->append("<td>");
		m_out->append(m_columns[i]);
		m_out->append("</td>");
	}
	m_out->append("</tr>\n");

	// Write the expanded text.
	StringAppendF(m_out,
			"<tr id=\"row%d\" class=\"hiddenrow\">"
			"<td colspan=\"%d\">"
			"<div id=\"row%d_d\" class=\"hiddend\">"
			"<div class=\"padparagraph\"><br>",
			m_numRows, static_cast<int>(m_columns.size()), m_numRows);
	m_out->append(expanded_text);
	m_out->append("</div></div><br></td></tr>\n");

	++m_numRows;
	clearColumnData();
}

void HTMLTable::writeFooter(bool write_num_rows) {
	m_out->append("</table>");
	if (write_num_rows) {
		StringAppendF(m_out, "<p>%d rows</p>", m_numRows);
	}
}

void HTMLTable::addJavaScript(std::string* out) {
	out->append("<script>\n"
			"var a_id = 0;  // Synchronization.\n"
			"function setc(action_id, t, el1, c) {\n"
			"  setTimeout(function() { if (a_id == action_id) el1.className = c; }, t);\n"
			"}\n"
			"\n"
			"function toggle(id) {\n"
			"  var el = document.getElementById(id);\n"
			"  var el1 = document.getElementById(id + \"_d\");\n"
			"  if (el && el1) {\n"
			"    if (el.className == \"hiddenrow\") { el.className = \"visiblerow\"; setc(++a_id, 20, el1, \"visibled\"); }\n"
			"                              else { el1.className = \"hiddend\"; setc(++a_id, 200, el, \"hiddenrow\"); }\n"
			"  }\n"
			"}"
			"</script>\n");
}

