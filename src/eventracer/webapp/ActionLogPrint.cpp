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

#include "ActionLogPrint.h"
#include "stringprintf.h"

#include "Escaping.h"

using std::string;

FunctionNamePrinter::FunctionNamePrinter(
		const ActionLog* actions, StringSet* variables, StringSet* mem_values) {
	for (int event_action_id = 0; event_action_id <= actions->maxEventActionId();
			++event_action_id) {
		const ActionLog::EventAction& event = actions->event_action(event_action_id);
		for (size_t i = 1; i < event.m_commands.size(); ++i) {
			if ((event.m_commands[i - 1].m_cmdType == ActionLog::WRITE_MEMORY || event.m_commands[i - 1].m_cmdType == ActionLog::READ_MEMORY) &&
					event.m_commands[i].m_cmdType == ActionLog::MEMORY_VALUE) {
				int fn_id = 0;
				if (sscanf(mem_values->getString(event.m_commands[i].m_location),
						"Function[%d]", &fn_id) == 1 &&
						m_fMap.count(fn_id) == 0) {
					// A function was written to a variable for the first time.
					std::string fn_name = variables->getString(event.m_commands[i - 1].m_location);
					std::string::size_type dot_pos = fn_name.find('.');
					if (dot_pos != string::npos) {
						// Take only the name after the dot.
						fn_name = fn_name.substr(dot_pos, fn_name.size() - dot_pos);
					}
					m_fMap[fn_id] = fn_name;
				}
			}
		}
	}
}

const char* FunctionNamePrinter::getFunctionName(int function_id) const {
	FunctionMap::const_iterator it = m_fMap.find(function_id);
	if (it == m_fMap.end())
		return "___";
	return it->second.c_str();
}


CodeOutput::CodeOutput(const FunctionNamePrinter& fn_name_printer, std::string* out)
	: m_FnNamePrinter(fn_name_printer), m_out(out), m_scopeDepth(0) {
	m_out->append("<pre>");
}

CodeOutput::~CodeOutput() {
	m_out->append("</pre>");
}

void CodeOutput::outputScopeEnter(const std::string& text) {
	outputStatement(text);
	++m_scopeDepth;
}

void CodeOutput::outputStatement(const std::string& text) {
	for (int i = 0; i < m_scopeDepth; ++i) {
		m_out->append("  ");
	}

	int fnid, jsid, line1, line2;
	if (sscanf(text.c_str(), "Call (#%d) line %d-%d", &jsid, &line1, &line2) == 3 ||
		sscanf(text.c_str(), "Exec (#%d) line %d-%d", &jsid, &line1, &line2) == 3) {
		StringAppendF(m_out,
				"<a href=\"js?jsid=%d#l%d\">JS</a> ",
				jsid, line1);
	} else if (
			sscanf(text.c_str(), "Call (fn=%d #%d) line %d-%d", &fnid, &jsid, &line1, &line2) == 4 ||
			sscanf(text.c_str(), "Exec (fn=%d #%d) line %d-%d", &fnid, &jsid, &line1, &line2) == 4) {
		StringAppendF(m_out,
				"<b>%s</b> <a href=\"js?jsid=%d#l%d\">[link to JS]</a> ",
				HTMLEscape(m_FnNamePrinter.getFunctionName(fnid)).c_str(), jsid, line1);
	}
	m_out->append(text);

	m_out->append("\n");
}

void CodeOutput::outputScopeExit() {
	--m_scopeDepth;
}

ActionLogPrinter::ActionLogPrinter(
		const ActionLog* actions, StringSet* variables, StringSet* scopes, StringSet* mem_values)
	: m_actions(actions),
	  m_variables(variables),
	  m_scopes(scopes),
	  m_memValues(mem_values),
	  m_fnNamePrinter(actions, variables, mem_values) {
}

void ActionLogPrinter::printEventActionDetails(int event_action_id, std::string* out) const {
	const ActionLog::EventAction& event = m_actions->event_action(event_action_id);

	CodeOutput code(m_fnNamePrinter, out);
	for (size_t i = 0; i < event.m_commands.size(); ++i) {
		const ActionLog::Command& cmd = event.m_commands[i];
		if (cmd.m_cmdType == ActionLog::ENTER_SCOPE) {
			code.outputScopeEnter(
					HTMLEscape(m_scopes->getString(cmd.m_location)).c_str());
		} else if (cmd.m_cmdType == ActionLog::EXIT_SCOPE) {
			code.outputScopeExit();
		} else if (cmd.m_cmdType == ActionLog::READ_MEMORY) {
			code.outputStatement(StringPrintf("read <b>%s</b>",
					HTMLEscape(m_variables->getString(cmd.m_location)).c_str()));
		} else if (cmd.m_cmdType == ActionLog::WRITE_MEMORY) {
			code.outputStatement(StringPrintf("write <b>%s</b>",
					HTMLEscape(m_variables->getString(cmd.m_location)).c_str()));
		} else if (cmd.m_cmdType == ActionLog::TRIGGER_ARC) {
			code.outputStatement(StringPrintf("start <a href=\"code?focus=%d\">%s</a>",
					cmd.m_location,
					getEventActionSummaryForLink(cmd.m_location).c_str()));
		} else if (cmd.m_cmdType == ActionLog::MEMORY_VALUE) {
			code.outputStatement(StringPrintf("value <b>%s</b>",
					HTMLEscape(m_memValues->getString(cmd.m_location)).c_str()));
		} else {
			code.outputStatement("Unknown");
		}
	}
}

namespace {

void addFirstCharsEscaped(const string& input, string* out) {
	if (input.size() > 28) {
		size_t open_bracket = input.find('[');
		size_t close_bracket = input.find(']');
		if (open_bracket != string::npos &&
				close_bracket != string::npos &&
				close_bracket > open_bracket) {
			std::string input_copy(input);
			input_copy.erase(open_bracket, close_bracket - open_bracket + 1);
			if (input_copy.size() > 29) {
				input_copy = input_copy.substr(0, 27);
				input_copy.append("..");
			}
			AppendStringEscape(input_copy, out);
		} else {
			AppendStringEscape(input.substr(0, 26), out);
			out->append("...");
		}
	} else {
		AppendStringEscape(input, out);
	}
}
}  // namespace

void ActionLogPrinter::getEventActionSummary(int event_action_id, const char* separator, std::string* out) const {
	int num_outs = 0;

	const ActionLog::EventAction& event = m_actions->event_action(event_action_id);
	for (size_t i = 0; i < event.m_commands.size(); ++i) {
		const ActionLog::Command& cmd = event.m_commands[i];
		if (cmd.m_cmdType == ActionLog::ENTER_SCOPE) {
			if (num_outs != 0) out->append(separator);
			addFirstCharsEscaped(m_scopes->getString(cmd.m_location), out);
			if (++num_outs == 3) break;
		}
	}
	for (size_t i = 0; i < event.m_commands.size(); ++i) {
		const ActionLog::Command& cmd = event.m_commands[i];
		if (cmd.m_cmdType == ActionLog::WRITE_MEMORY) {
			if (num_outs != 0) out->append(separator);
			addFirstCharsEscaped(StringPrintf("write %s", m_variables->getString(cmd.m_location)), out);
			if (++num_outs == 4) return;
		}
	}
	for (size_t i = 0; i < event.m_commands.size(); ++i) {
		const ActionLog::Command& cmd = event.m_commands[i];
		if (cmd.m_cmdType == ActionLog::READ_MEMORY) {
			if (num_outs != 0) out->append(separator);
			addFirstCharsEscaped(StringPrintf("read %s",  m_variables->getString(cmd.m_location)), out);
			if (++num_outs == 4) return;
		}
	}
}

std::string ActionLogPrinter::getEventActionSummaryForLink(int event_action_id) const {
	const ActionLog::EventAction& event = m_actions->event_action(event_action_id);
	for (size_t i = 0; i < event.m_commands.size(); ++i) {
		const ActionLog::Command& cmd = event.m_commands[i];
		if (cmd.m_cmdType == ActionLog::ENTER_SCOPE) {
			return StringPrintf("event action %d : %s", event_action_id,
					HTMLEscape(m_scopes->getString(cmd.m_location)).c_str());
		}
	}
	return StringPrintf("event action %d (not instrumented, e.g. rendering)", event_action_id);
}

