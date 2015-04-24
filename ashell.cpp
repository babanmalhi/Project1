// C headers
#include <stdio.h>      // for stdin, stout & stderr defs
#include <stdlib.h>     // for getenv
#include <unistd.h>     // for read/write sys calls
#include <termios.h>    // for struct termios
#include <dirent.h>     // for DIR struct
#include <sys/stat.h>
#include <sys/wait.h>   // for waitpid

// C++/STL headers
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <map>
#include <list>

#include "keytype.h"   // for key types

using namespace std;

static inline void owrite(const char *str, size_t len, FILE *fp=stdout)
{
    int fd = fileno(fp);
    write(fd, str, len);
    //fync(fp);
}

static inline void ewrite(const char *str, size_t len, FILE *fp=stderr)
{
    int fd = fileno(fp);
    write(fd, str, len);
    //fsync(fp);
}

const int maxCmdHistory = 10;

map<string, int(*)(const string &, FILE*)> supportedCmds;
vector<string> cmdHistory;
string pwd;

int updatePwd()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return -1;
    }
    pwd = string(cwd);
    return 0;
}

void addCmdToHistory(const string &cmd)
{
    if (cmdHistory.size() >= maxCmdHistory) {
        // remove 1st element from list
        cmdHistory.erase(cmdHistory.begin());
    }

    // Add new command to end of list
    cmdHistory.push_back(cmd);
}

int ashLs(const string &arg, FILE* fp)
{
    string dirstr = arg;
    if (arg.empty()) {
        dirstr = pwd;
    }

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirstr.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            // Get file attributes
            struct stat fs;
            if (stat(ent->d_name, &fs) < 0) {
                string errstr = "Error getting file perms!";
                ewrite(errstr.c_str(), errstr.size());
            }
            ostringstream ss;
            ss <<((S_ISDIR(fs.st_mode)) ? "d" : "-");
            ss <<((fs.st_mode & S_IRUSR) ? "r" : "-");
            ss <<((fs.st_mode & S_IWUSR) ? "w" : "-");
            ss <<((fs.st_mode & S_IXUSR) ? "x" : "-");
            ss <<((fs.st_mode & S_IRGRP) ? "r" : "-");
            ss <<((fs.st_mode & S_IWGRP) ? "w" : "-");
            ss <<((fs.st_mode & S_IXGRP) ? "x" : "-");
            ss <<((fs.st_mode & S_IROTH) ? "r" : "-");
            ss <<((fs.st_mode & S_IWOTH) ? "w" : "-");
            ss <<((fs.st_mode & S_IXOTH) ? "x" : "-");
            ss <<"\t";
            ss <<("%s\n", ent->d_name);
            ss <<"\n";
            string outs = ss.str();
            owrite(outs.c_str(), outs.size(), fp);
        }
        closedir(dir);
    } else {
        /* could not open directory */
        ostringstream ss;
        ss << arg << " : " << "No such file or directory\n";
        string errstr = ss.str();
        ewrite(errstr.c_str(), errstr.size());
        return -1;
    }
    return 0;
}

int ashCd(const string &arg, FILE* fp)
{
    string dirstr = arg;
    if (dirstr.empty()) {
        const char *home = getenv("HOME");
        if (home == NULL) {
            string errstr = "Could not get HOME environment variable\n";
            ewrite(errstr.c_str(), errstr.size());
            return -1;
        }
        dirstr = string(home);
    }

    if (chdir(dirstr.c_str())) {
        ostringstream ss;
        ss << "cd: " << arg << ": No such file or directory\n";
        string errstr = ss.str();
        ewrite(errstr.c_str(), errstr.size());
        return -1;
    }
    updatePwd();
    return 0;
}

int ashPwd(const string &arg, FILE* fp)
{
    owrite(pwd.c_str(), pwd.size(), fp);
    owrite("\n", 1, fp);
    return 0;
}

int ashHistory(const string &arg, FILE* fp)
{
    int count = 0;
    for (string s : cmdHistory) {
        ostringstream ss;
        ss << count++ << " " << s;
        string hs = ss.str();
        owrite(hs.c_str(), hs.size(), fp);
        owrite("\n", 1, fp);
    }

    return 0;
}

void printPrompt(string pwd)
{
    // check if length of string is > 16
    if (pwd.size() > 16) {
        // elide string
        unsigned lastpos = pwd.find_last_of('/');
        string elided = "/...";
        string sub = pwd.substr(lastpos, pwd.size());
        pwd = elided + sub;
    }
    pwd = pwd + ">";
    const char* cstr = pwd.c_str();
    owrite(cstr, pwd.size());
}

KeyType::TypeCode getKeyType(char c)
{
    switch(c) {
        case 0x7F: // Del
            return KeyType::DEL;
        case 0x08: // Backspace
            return KeyType::BACKSPACE;
        case 0x41: // Up
            return KeyType::UP_ARROW;
        case 0x42: // Down
            return KeyType::DOWN_ARROW;
        default:
            return KeyType::REGULAR;
    }
}

void handlebackAndDelKey(vector<char> &chars)
{
    chars.pop_back();
}

void handleUpAndDownArrow(vector<char> &chars, int index)
{
    if (cmdHistory.size() == 0 || index < 0 || index >= cmdHistory.size()) {
        return;
    }

    string histCmd = cmdHistory[index];
    for(char c: chars) {
        write(STDOUT_FILENO, "\b \b", 3);
    }
    chars.clear();

    for(char c: histCmd) {
        write(STDOUT_FILENO, &c, 1);
        chars.push_back(c);
    }
}

void executeCmd(string cmd, vector<string> args, const string& rfile)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Check if we need to redirect output
        if (!rfile.empty()) {
            FILE *fp = fopen(rfile.c_str(), "w");
            int fd = fileno(fp);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // Child process
        char **al;
        al = (char **) malloc(sizeof(char *) * (args.size()+1));
        for (int i = 0; i < args.size(); i++) {
            al[i] = (char *) args[i].c_str();
        }
        al[args.size()] = (char *) 0; // end marker

        if (execvp(cmd.c_str(), al) == -1) {
            ostringstream ss;
            ss << cmd << ": command not found\n";
            string errstr = ss.str();
            ewrite(errstr.c_str(), errstr.size());
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        string errstr = "Error forking\n";
        ewrite(errstr.c_str(), errstr.size());
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

string ashReadLine()
{
    // Setup unbuffered mode on stdin
    struct termios setting;
    if (tcgetattr (STDIN_FILENO, &setting)) {
        ewrite("error 1\n", 8);
        return string();
    }

    setting.c_lflag &= ~ICANON;
    setting.c_lflag &= ~ECHO;
    setting.c_cc[VMIN] = 1;
    setting.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSANOW, &setting) < 0)
        ewrite("error 2\n", 8);

    vector<char> chars; 
    int index = cmdHistory.size();

    while (true) {
        char c;
        read(STDIN_FILENO, &c, 1);

        if (c == '\n') {
            index = cmdHistory.size();
            break;
        } else if (c == 0x1B) { // spl key. read again
            read(STDIN_FILENO, &c, 1); // consume [ or 0x5B
            read(STDIN_FILENO, &c, 1);
        }

        switch (getKeyType(c)) {
            case KeyType::DEL:
            case KeyType::BACKSPACE:
                if (chars.size() > 0) {
                    handlebackAndDelKey(chars);
                    write(STDOUT_FILENO, "\b \b", 3);
                } else {
                    write(STDOUT_FILENO, "\a", 1);
                }
                break;
            case KeyType::UP_ARROW:
                if ((index - 1) >= 0) {
                    index--;
                    handleUpAndDownArrow(chars, index);
                } else {
                    // ring the bell
                    write(STDOUT_FILENO, "\a", 1);
                }
                break;
            case KeyType::DOWN_ARROW:
                if ((index +1) < (int) cmdHistory.size()) {
                    index++;
                    handleUpAndDownArrow(chars, index);
                } else {
                    // ring the bell
                    write(STDOUT_FILENO, "\a", 1);
                }
                break;
            case KeyType::REGULAR:
                write(STDOUT_FILENO, &c, 1);
                chars.push_back(c);
            default:
                break;
        }
    }

    // restore setting
    setting.c_lflag |= ICANON;
    setting.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &setting) < 0)
        ewrite("error 3\n", 8);

    // create a string from the list of chars
    string cmdline;
    for (char c: chars) {
        cmdline += c;
    }
    return cmdline;
}

vector<string> &split(const string &s, char delim, vector<string> &elems)
{
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

vector<string> ashParseCmdLine(const string &cmdline, char delim)
{
    vector<string> args;
    split(cmdline, delim, args);
    return args;
}

void ashMainLoop()
{
    // Before starting mainloop store the CWD
    if (updatePwd()) {
        string errstr = "Error getting CWD! Exiting.";
        ewrite(errstr.c_str(), errstr.size());
        return;
    }

    do {
        printPrompt(pwd);
        
        string cmdline = ashReadLine();
        owrite("\n", 1);
        
        if (cmdline.empty()) {
            continue;
        }

        // add cmd to history
        addCmdToHistory(cmdline);

        vector<string> args = ashParseCmdLine(cmdline, ' ');

        bool redirect = false;
        string rfile;
        for (int i = 0; i < args.size(); i++) {
            // Look for redirection operator '>'
            if (args[i] == ">") {
                redirect = true;
                if (i == (args.size() - 1)) {
                    args.erase(args.begin() + i, args.begin() + args.size());
                    break;
                }
                rfile = args[i+1];
                args.erase(args.begin() + i, args.begin() + args.size());
                break;
            }
        }

        // By default all output goes to STDOUT
        FILE *fp = stdout;

        if (redirect) {
            if (rfile.empty()) {
                // We need a filename after redirect operator
                string errstr  = "Syntax error. Specify filename after >\n";
                ewrite(errstr.c_str(), errstr.size());
                continue;
            }
            // open file for writing only
            fp = fopen(rfile.c_str(), "w");
        }

        map<string, int(*)(const string&, FILE*)>::const_iterator it = supportedCmds.find(args[0]);
        if (it == supportedCmds.end()) {
            // execute external application
            executeCmd(args[0], args, rfile);
        } else {
            if (args[0] == "exit") {
                return;
            }
            int(*cmdfxn)(const string&, FILE*) = it->second;

            string arg;
            if (args.size() > 1) {
                // none of the internal cmds take more than 1 arg
                arg = args[1];
            }
            // execute internal application
            (*cmdfxn)(arg, fp);
            if (fp != stdout) {
                fclose(fp);
            }
        }
    } while (true); // loop forever until "exit"
}

int main()
{
    // Fill up list of supported commands
    supportedCmds["cd"]         = &ashCd;
    supportedCmds["ls"]         = &ashLs;
    supportedCmds["pwd"]        = &ashPwd;
    supportedCmds["history"]    = &ashHistory;
    supportedCmds["exit"]       = NULL;

    // Run main loop
    ashMainLoop();
 
    return 0;
}
