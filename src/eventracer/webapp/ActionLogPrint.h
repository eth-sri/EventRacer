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

#ifndef ACTIONLOGPRINT_H_
#define ACTIONLOGPRINT_H_

#include <map>
#include <string>

#include "ActionLog.h"
#include "StringSet.h"

// Class to discovers the names of the functions in an ActionLog.
// All JavaScript functions are anonymous, but are often assigned to
// good variable names. This class scans the log to link every function
// object with a name of a variables it is assigned to.
class FunctionNamePrinter {
public:
	explicit FunctionNamePrinter(
			const ActionLog* actions, StringSet* variables, StringSet* mem_values);

	const char* getFunctionName(int function_id) const;

private:
	typedef std::map<int, std::string> FunctionMap;

	FunctionMap m_fMap;
};

class CodeOutput {
public:
	CodeOutput(const FunctionNamePrinter& fn_name_printer, std::string* out);
	~CodeOutput();

	void outputScopeEnter(const std::string& text);
	void outputStatement(const std::string& text);
	void outputScopeExit();

private:
	const FunctionNamePrinter& m_FnNamePrinter;
	std::string* m_out;
	int m_scopeDepth;
};

class ActionLogPrinter {
public:
	explicit ActionLogPrinter(const ActionLog* actions, StringSet* variables, StringSet* scopes, StringSet* mem_values);

	void printEventActionDetails(int event_action_id, std::string* out) const;

	// Outputs a list of summary actions for action log operation with the id. The actions are separated by
	// the given separator.
	void getEventActionSummary(int event_action_id, const char* separator, std::string* out) const;

	// Returns a one-line summary of an operation to be used in a link.
	std::string getEventActionSummaryForLink(int event_action_id) const;

	const FunctionNamePrinter& function_name_printer() const { return m_fnNamePrinter; }

private:
	const ActionLog* m_actions;
	const StringSet* m_variables;
	const StringSet* m_scopes;
	const StringSet* m_memValues;
	FunctionNamePrinter m_fnNamePrinter;
};

#endif /* ACTIONLOGPRINT_H_ */
