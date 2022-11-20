
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string.h>
#include "inifile.h"

using std::string;
//using namespace std;

/* Strip whitespace chars off end of given string, in place. Return s. */
static char* rstrip(char* s)
{
	char* p = s + strlen(s);
	while (p > s && isspace((unsigned char)(*--p)))
		*p = '\0';
	return s;
}

/* Return pointer to first non-whitespace char in given string. */
static char* lskip(const char* s)
{
	while (*s && isspace((unsigned char)(*s)))
		s++;
	return (char*)s;
}

/* Return pointer to first char (of chars) or inline comment in given string,
   or pointer to NUL at end of string if neither found. Inline comment must
   be prefixed by a whitespace character to register as a comment. */
static char* find_chars_or_comment(const char* s, const char* chars)
{
#if INI_ALLOW_INLINE_COMMENTS
	int was_space = 0;
	while (*s && (!chars || !strchr(chars, *s)) &&
		!(was_space && strchr(INI_INLINE_COMMENT_PREFIXES, *s))) {
		was_space = isspace((unsigned char)(*s));
		s++;
	}
#else
	while (*s && (!chars || !strchr(chars, *s))) {
		s++;
	}
#endif
	return (char*)s;
}

/* Similar to strncpy, but ensures dest (size bytes) is
   NUL-terminated, and doesn't pad with NULs. */
static char* strncpy0(char* dest, const char* src, size_t size)
{
	/* Could use strncpy internally, but it causes gcc warnings (see issue #91) */
	size_t i;
	for (i = 0; i < size - 1 && src[i]; i++)
		dest[i] = src[i];
	dest[i] = '\0';
	return dest;
}

int CIniFile::ini_parse_file()
{
	char* line;
	size_t max_line = MAX_INI_STR;

	char section[MAX_SECTION] = "";
	char prev_name[MAX_NAME] = "";

	char* start; char* end;
	char* name; char* value;
	int lineno = 0;
	int error = 0;

	line = (char*)malloc(MAX_INI_STR);
	if (!line) {
		return -2;
	}

	/* Scan through file line by line */
	while (fgets(line, (int)max_line, m_hfile) != NULL)
	{
		lineno++;

		start = line;
		start = lskip(rstrip(start));

		if (strchr(INI_START_COMMENT_PREFIXES, *start)) {
			/* Start-of-line comment */
		}
		else if (*start == '[') {
			/* A "[section]" line */
			end = find_chars_or_comment(start + 1, "]");
			if (*end == ']') {
				*end = '\0';
				strncpy0(section, start + 1, sizeof(section));
				*prev_name = '\0';
			}
			else if (!error) {
				/* No ']' found on section line */
				error = lineno;
			}
		}
		else if (*start) {
			/* Not a comment, must be a name[=:]value pair */
			end = find_chars_or_comment(start, "=:");
			if (*end == '=' || *end == ':') {
				*end = '\0';
				name = rstrip(start);
				value = end + 1;
#if INI_ALLOW_INLINE_COMMENTS
				end = find_chars_or_comment(value, NULL);
				if (*end)
					*end = '\0';
#endif
				value = lskip(value);
				rstrip(value);

				/* Valid name[=:]value pair found, call handler */
				strncpy0(prev_name, name, sizeof(prev_name));
				if (!GetValue(section, name, value) && !error)
					error = lineno;
			}
			else if (!error) {
				/* No '=' or ':' found on name[=:]value line */
//#if INI_ALLOW_NO_VALUE
//				*end = '\0';
//				name = rstrip(start);
//				if (!GetValue(section, name, NULL) && !error)
//					error = lineno;
//#else
				error = lineno;
//#endif
			}
		}

#if INI_STOP_ON_FIRST_ERROR
		if (error)
			break;
#endif
	}

	free(line);

	return error;
}

string CIniFile::Get(const string& section, const string& name, const string& default_value) const
{
	string key = MakeKey(section, name);
	// Use _values.find() here instead of _values.at() to support pre C++11 compilers
	return m_values.count(key) ? m_values.find(key)->second : default_value;
}

string CIniFile::GetString(const string& section, const string& name, const string& default_value) const
{
	const string str = Get(section, name, "");
	return str.empty() ? default_value : str;
}

long CIniFile::GetInt(const string& section, const string& name, long default_value) const
{
	string valstr = Get(section, name, "");
	const char* value = valstr.c_str();
	char* end;
	// This parses "1234" (decimal) and also "0x4D2" (hex)
	long n = strtol(value, &end, 0);
	return end > value ? n : default_value;
}

double CIniFile::GetDouble(const string& section, const string& name, double default_value) const
{
	string valstr = Get(section, name, "");
	const char* value = valstr.c_str();
	char* end;
	double n = strtod(value, &end);
	return end > value ? n : default_value;
}

bool CIniFile::GetBool(const string& section, const string& name, bool default_value) const
{
	string valstr = Get(section, name, "");
	// Convert to lower case to make string comparisons case-insensitive
	std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
	if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
		return true;
	else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
		return false;
	else
		return default_value;
}

bool CIniFile::HasSection(const string& section) const
{
	const string key = MakeKey(section, "");
	std::map<string, string>::const_iterator pos = m_values.lower_bound(key);
	if (pos == m_values.end())
		return false;
	// Does the key at the lower_bound pos start with "section"?
	return pos->first.compare(0, key.length(), key) == 0;
}

bool CIniFile::HasValue(const string& section, const string& name) const
{
	string key = MakeKey(section, name);
	return m_values.count(key);
}

string CIniFile::MakeKey(const string& section, const string& name)
{
	string key = section + "=" + name;
	// Convert to lower case to make section/name lookups case-insensitive
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	return key;
}

int CIniFile::GetValue(const char* section, const char* name, const char* value)
{
	if (!name)  // Happens when INI_CALL_HANDLER_ON_NEW_SECTION enabled
		return 1;
	string key = MakeKey(section, name);
	if (m_values[key].size() > 0)
		m_values[key] += "\n";
	m_values[key] += value ? value : "";
	return 1;
}

CIniFile::CIniFile(const string& filename)
{
	m_hfile = fopen(filename.c_str(), "r+");
	if (!m_hfile)
		m_error = -1;
	else
	{
		m_error = ini_parse_file();
		fclose(m_hfile);
	}
}

int CIniFile::GetError() const
{
	return m_error;
}

