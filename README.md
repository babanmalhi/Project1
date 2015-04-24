ECS150 SQ15
March 28, 2015
Due April 12, 2015 at 11:55 PM
Project 1
You will be working with a partner for this project, you specify your partner in your readme file. This specification is subject to change at anytime for additional clarification. For this project, you and a partner will be implementing a fairly simple shell called ashell. The shell must be able to execute applications, be able to setup pipes between executed applications, redirect standard input/output from/to files for the executed applications, and perform simple commands internally. You must provide a make file (or cmake file) to compile your shell. Your program must be written in C or C++. You may not use scanf, fscanf, printf, fprintf, cin, cout, or their derivatives; you must use read and write system calls for all I/O. You may use the C++ containers. If you have any concerns if a standard library function is allowed, ask before using it. Submit only one copy of your project per pair on smartsite as either a zip or tar.gz file.
The shell must handle commands cd, ls, pwd, history, and exit internally. Below are descriptions of the commands:
cd [directory] – Changes the current working directory to specified directory. If the optional directory is not specified, then the current working directory is changed to the directory specified by the HOME environmental variable.
ls [directory] – Lists the files/directories in the directory specified, if no directory is specified, it lists the contents of the current working directory. The order of files listed does not matter, but the type and permissions must precede the file/directory name with one entry per line. Below is an example listing
drwxr-xr-x .
drwxr-xr-x ..
-rwxr-xr-x ashell
-rwxr-xr-x main.cpp
pwd – Prints the current working directory name.
history – Prints the history of the last 10 commands entered into the shell. Below is an example where the commands entered were ls, cd .., pwd, and then history:
0 ls
1 cd ..
2 pwd
3 history
exit – Exits the shell.
The shell needs to be able to handle up and down arrows to select through the history of
commands. Only the most recent 10 commands need to be stored in the history. The up
Project 1 1 of 2
ECS150 SQ15 March 28, 2015
arrow must toggle to the previous command, down arrow to the next command. If the user attempts to navigate beyond the end of the list the audible bell character '\a' (ASCII 0x07) must be output. In addition, if a user enters a backspace or delete and no characters exist on the command to delete, the audible bell must be output.
The shell prompt must have the current working directory followed by the greater than symbol. If the full current working directory path is longer than 16 characters, then "/..." should be prepended to the last level directory name. Below are examples: /home/cjnitta>
/.../Project01>
A working example can be found on the CSIF at /home/cjnitta/ecs150/ashell. This version has been compiled to work on the 32-bit machines.
You should avoid using existing source code as a primer that is currently available on the Internet. You must specify in your readme file any sources of code that you or your partner have viewed to help you complete this project. All class projects will be submitted to MOSS to determine if pairs of students have excessively collaborated with other pairs. Excessive collaboration, or failure to list external code sources will result in the matter being transferred to Student Judicial Affairs.
Project 1 2 of 2
