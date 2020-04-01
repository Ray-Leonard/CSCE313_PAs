#include "shell.h"
using namespace std;

int main()
{
	// stores the fd of stdin
	int origStdin = dup(0);
	// the vector to store all the bg processes
	vector<int> bgPid;
	// a stack to store the previous working directory
	stack <char*> prevDirs;
	// main body of shell
	while (true)
	{
		// restore stdin at the beginning of every shell loop, 
		// since it may get overwritten by pipings
		dup2(origStdin, 0);
		// create a vector to store all the bg processes' pid and check on them
		zombie_handler(bgPid);

		printf("Ray's Shell$ ");
		string inputline;
		getline(cin, inputline);
		// trim the inputline for any unintentional white spaces
		inputline = trim(inputline);

		if(inputline == "exit")
		{
			cout << "exit successfully" << endl;
			break;
		}
		// here may add some other special command cases
		if(inputline.substr(0, 2) == "cd")
		{
			change_directory(trim(inputline.substr(2)), prevDirs);
			continue;
		}
		// customize instruction
		if(inputline == "jojo")
		{
			jojo();
			continue;
		}
		// jobs
		if(inputline == "jobs")
		{
			print_jobs(bgPid);
			continue;
		}

		// check if the command is a background command!
		bool isBg = false;
		if(inputline[inputline.size()-1] == '&')
		{
			isBg = true;
			// get rid of the '&'
			inputline = inputline.substr(0, inputline.size()-1);
		}

		// split the inputline by '|' and stores separate commands in a vector
		vector<string> levels = split(inputline, '|');

		// execute commands of different levels inside a loop and set up pipes if necessary
		for(size_t i = 0; i < levels.size(); ++i)
		{
			// set up the pipe
			int fd[2];
			pipe(fd);	// connect the pipe

			int pid = fork();
			if (pid == 0) // child process
			{
				// redirect output to the next level unless this is the last level
				if(i < levels.size() - 1)
				{
					// redirect STDOUT to fd[1], so that it can write to the other side
					dup2(fd[1], 1);
					// STDOUT already points to fd[1], which must be closed
					close(fd[1]);
				}
				// execute the command at this level
				execute(levels[i]);
			}
			else	// parent side
			{
				// other levels does not have to wait
				// redirect input from the child process
				dup2(fd[0], 0);
				close(fd[1]);
				// only waits if it is the last level
				if(i == levels.size()-1 && !isBg)
				{
					waitpid(pid, NULL, 0);
				}
				// when it is a background process, push the pid into the vector.
				else if(isBg)
				{
					bgPid.push_back(pid);
				}
			}
		}


	}

	return 0;
}
