#ifndef SHELL_H
#define SHELL_H
#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <sys/wait.h>
#include <sys/types.h>
#include <stack>
#include <ctime>
#include <chrono>
using namespace std;

void print_jobs(vector<int> & jobs)
{
	if(jobs.size() == 0)
	{
		cout << "There is no background processes running now!" << endl;
		return;
	}
	for(size_t i = 0; i < jobs.size(); ++i)
	{
		cout << "Background Job " << i+1 << ": " << jobs[i] << endl;
	}
}

void jojo()
{
	char s[200];
	cout << endl << "WHAT IS MY DIRECTORY?" << endl;
	getcwd(s, 200);
	cout << s << endl << endl;

	cout << "WHO AM I?" << endl;
	cout << "ray" << endl << endl;

	cout << "WHEN IS NOW?" << endl;
	auto start = chrono::system_clock::now();
	time_t time = chrono::system_clock::to_time_t(start);
	cout << ctime(&time) << endl;

}

void change_directory(string dir, stack<char*> & prevDirs)
{
	char* currDir_ptr = new char[200];
	char currDir[200];
	getcwd(currDir, 200);
	getcwd(currDir_ptr, 200);
	cout << "Directory before change: " << currDir << endl;

	// used to record the previous directory
	if(dir == "-" && prevDirs.size() > 0)
	{
		chdir(prevDirs.top());
		delete prevDirs.top();
		prevDirs.pop();
	}
	else
	{
		char* d = (char*) dir.c_str();
		chdir(d);
	}

	//push the current working dir into the stack
	prevDirs.push(currDir_ptr);

	// print the current working dir
	getcwd(currDir, 200);
	cout << "Current directory: " << currDir << endl;

}

/* 
	trim off any white spaces in the beginning and at the end of a string(command)
	But if the second argument is set to true, remove all whitespaces!
 */
string trim(string s, bool power = false)
{
	if(power)
	{
		string ret = "";
		for(size_t i = 0; i < s.size(); ++i)
		{
			// anything other than whitespaces are saved.
			if(s[i] != ' ')
			{
				ret += s[i];
			}
		}
		return ret;
	}

	// integers that indicates the start pos of the substring
	int beginning = 0;
	// start from the beginning of s
	while(s[beginning] == ' ')
	{
		++beginning;
	}
	// trim once to get rid of any pre-whitespaces
	s = s.substr(beginning);

	int len = s.length();
	// then start from the end
	while(s[len-1] == ' ')
	{
		--len;
	}
	// trim the second time to get red of any post-whitespaces
	s = s.substr(0, len);

	return s;
}

/* handles I/O redirection */
void redirect(char direction, string filename)
{
	// determine the direction of redirection
	// '>' means to direct the stdout to file
	if(direction == '>')
	{
		int fd = open((char*)filename.c_str(), O_CREAT|O_WRONLY|O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		dup2(fd, 1);
		close(fd);
	}
	// otherwise, '<' means to direct std in from file
	else
	{
		int fd = open((char*)filename.c_str(), O_RDONLY);
		dup2(fd, 0);
		close(fd);
	}
}

/* 
	redirection_handler(string s)
	Be called in command_parser(string), when redirection symbol appears.
	This function extract the filename from the given string and 
	handles the case where both < and > appear in the given string s
	This function calls redirect() to perform the actual redirection.
 */
void redirection_handler(string s)
{
	// record the direction(first symbol)
	// direction will always be the first symbol of the input string
	char direction = s[0];

	// get the file name that comes after the redirection symbol
	// in case both < and > appears (< always appears before '>'), 
	// the first file name should be before the '>'
	// but it is also possible that only one of the symbols appear.
	string filename1 = "";
	string filename2 = "";

	// get rid of the first direction and start checking for filenames and the second symbol(if any)
	s = s.substr(1);
	size_t index = s.find(">");
	// if the second symbol exists, filename1 should be everything before the index
	// and filename 2 should be everything after the index
	if(index != string::npos)
	{
		filename1 = s.substr(0, index);
		filename2 = s.substr(index+1);
	}
	// if the second symbol does not exist, then filename1 is s itself
	else
	{
		filename1 = s;
	}

	//cout << filename1 << "&" << filename2 << endl;

	// if filename2 is not empty, then call redirect() to handle it.
	if(filename2 != "")
	{
		redirect('>', filename2);
	}
	// call the function to complete the redirection
	redirect(direction, filename1);
}


/* 
   given a command, split the command by spaces to find out 
   all the arguments 
*/
vector<string> command_parser(string s)
{
	vector<string> args;
	string arg = "";
	// iterating through s and split by ' '
	// but if encounters \' or \", dont split until the next quotation
	for(unsigned int i = 0; i < s.length(); ++i)
	{
		// quotation encountered, go until the next quotation
		// this check has the highest priority!
		if (s[i] == '\'' || s[i] == '\"')
		{
			// starting from the next to quotation, add everything 
			// to arg until the next quotation is encountered.
			++i;
			while (s[i] != '\'' && s[i] != '\"')
			{
				arg += s[i];
				++i;
			}
			++i;
		}

		// space or end of string is encountered, push the arg into args and clear arg
		if (s[i] == ' ' || i >= s.length()-1)
		{
			// make sure to add the last character of command to arg before pushing
			if(i == s.length()-1 && (s[i] != '\'' && s[i] != '\"'))
			{
				arg += s[i];
			}

			args.push_back(arg);
			arg = "";
		}
		// encounters '<' or '>', meaning I/O redirection
		else if(s[i] == '>' || s[i] == '<')
		{
			// immediatelly push the arg back to args
			if(arg != "")
			{
				// but only push when arg is not empty
				// (meaning there was a space before redirection symbol and arg already got pushed)
				args.push_back(arg);
			}

			// call the redirection_handler() to handle redirection. 
			// passing in the redirection symbol and everything after the symbol
			redirection_handler(trim(s.substr(i), true));

			// break out of the loop
			break;
		}
		// safe to append to arg
		else
		{
			arg += s[i];
		}
	}

	return args;
}

/* split function takes in an inputline and split them into individual commands */
vector<string> split(string s, char token)
{
	vector<string> levels;

	string level = "";
	for(size_t i = 0; i < s.size(); ++i)
	{
		// check for quotation, this check has the highest priority!
		if(s[i] == '\'' || s[i] == '\"')
		{
			// add everything between quotation marks to level string
			// regardless of the content
			level += s[i++];
			while(s[i] != '\'' && s[i] != '\"')
			{
				level += s[i];
				++i;
			}
		}

		// then check for the apperance of token
		// if token encountered, add level to levels and clear level
		if(s[i] == token)
		{
			levels.push_back(trim(level));
			level = "";
		}
		// otherwise, regular command character is safe to add to level
		else
		{
			level += s[i];
		}
	}
	// at last, make sure the last level is added to levels
	levels.push_back(trim(level));

	return levels;
}

/* called in main to execute a level of command */
void execute(string command)
{
	// parse the command and get a vector of strings
	vector<string> args = command_parser(trim(command));

	// put the args into char pointer array for execvp() to use
	char** ARGS = new char*[args.size() + 1];
	ARGS[args.size()] = NULL;	// make sure it is NULL terminated
	for(unsigned int i = 0; i < args.size(); ++i)
	{
		ARGS[i] = (char*) args[i].c_str();
	}

	// execute the command
	execvp(ARGS[0], ARGS);
}

/* 
check on each background pid in the vector and call waitpid() function in a 
non-blocking manner. 
if waitpid() returns 0, process is still running
if waitpid() returns the pid of this process, the process terminates.
if returns -1, error.
 */
void zombie_handler(vector<int> &bgPid)
{
	for(size_t i = 0; i < bgPid.size(); ++i)
	{
		// call non-blocking wait on the process
		int status = waitpid(bgPid[i], 0, WNOHANG);
		// if the process terminates, remove this from the bgPid vector
		if(status == bgPid[i])
		{
			bgPid.erase(bgPid.begin() + i);
		}
	}
}

#endif
