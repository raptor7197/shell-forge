#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <filesystem>

using namespace std;

int echo(int argc, char* argv[]);
int type(int argc, char* argv[]);
string find_executable(const string& command);

int pwd(int argc, char* argv[]) {
  char cwd[2048];
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    cout << cwd << endl;
    return 0;
  } else {
    cerr << "Error getting current working directory" << endl;
    return 1;
  }
}

int change_directory(int argc, char* argv[]) {
  string target;
  if (argc == 1) {
    target = getenv("HOME");
  }else{
    target = argv[1];
  }

  target.find("~") == 0 ? target.replace(0, 1, getenv("HOME")) : target;
  target = filesystem::absolute(target).string();
  if (chdir(target.c_str()) != 0) {
    cerr << "cd: " << target << ": No such file or directory" << endl;
    return 1;
  }
  return 0;
}

const static map<string, function<int(int argc, char* argv[])>> COMMANDS = {
    {"echo", echo},
    {"type", type},
    {"exit", nullptr},
    {"pwd", pwd},
    {"cd", change_directory},
};

int echo(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: echo <message>" << endl;
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    cout << argv[i] << " ";
  }
  cout << endl;
  return 0;
}

int type(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: type <filename>" << endl;
    return 1;
  }
  string command(argv[1]);

  if (COMMANDS.find(command) != COMMANDS.end()) {
    cout << command << " is a shell builtin" << endl;
    return 0; 
  }else{
    string executable_path = find_executable(command);
    if (!executable_path.empty()) {
      cout << command << " is " << executable_path << endl;
      return 0;
    }
  }
  cout << command << ": not found" << endl;
  return 0;
}

string find_executable(const string& command) {
  string path = getenv("PATH");
  size_t pos = 0;
  while ((pos = path.find(':')) != string::npos) {
    string dir = path.substr(0, pos);
    string full_path = dir + "/" + command;
    if (access(full_path.c_str(), X_OK) == 0) {
      return full_path;
    }
    path.erase(0, pos + 1);
  }
  return "";
}


vector<string> split(const string& s, char delimiter = ' ')
{
  // Stable, simple splitter that skips empty tokens (consecutive delimiters are ignored)
  vector<string> result;
  string token;
  for (char c : s) {
    if (c == delimiter) {
      if (!token.empty()) {
        result.emplace_back(move(token));
        token.clear();
      }
    } else {
      token.push_back(c);
    }
  }
  if (!token.empty()) result.emplace_back(move(token));
  return result;
}

int main(int argc, char* argv[]) {
  // Flush after every cout / std:cerrr
  cout << unitbuf;
  cerr << unitbuf;

  while (true)
  {

    cout << "$ ";

    vector<string> tokens;
    bool in_quotes = false;
    while (true)
    {
      string input;
      getline(cin, input);
      string this_token;
      for (char& c : input) 
      {
        if (c == '\''){
          in_quotes = !in_quotes; // Toggle in_quotes on single quote
          continue;
        } 
        if (in_quotes || c != ' ') {
          this_token += c;
        } else {
          if (!this_token.empty()) {
            tokens.push_back(this_token);
            this_token.clear();
          }
        }
      }
      if (!this_token.empty()) {
        tokens.push_back(this_token);
        this_token.clear();
      }
      if (!in_quotes) { break; } // Break if not in quotes
    }


    
    

    vector<string> string_args = tokens;
    
    if (string_args.empty()) { continue; }

    char* arguments[1024];
    for (size_t i = 0; i < string_args.size(); ++i) { arguments[i] = const_cast<char*>(string_args[i].c_str()); }
    arguments[string_args.size()] = nullptr; // Null-terminate the array of arguments
    int arg_count = string_args.size();
    
    string command = string_args[0];
    if (COMMANDS.find(command) != COMMANDS.end())
    {
      if (command == "exit") {
        int exit_code = 0;
        if (arg_count > 1) {
          exit_code = stoi(arguments[1]);
        }
        return exit_code; // Exit the shell
      }
      auto fun = COMMANDS.at(command);
      if (fun) {fun(arg_count, arguments);}
    }else if (find_executable(command) != "")
    {
      string executable_path = find_executable(command);
      if (fork() == 0) {
        execv(executable_path.c_str(), arguments);
        cerr << "Failed to execute " << command << endl;
        return 1; // Exit child process if exec fails
      } else {
        wait(nullptr); // Wait for the child process to finish
      }
    
    }
    else { cout << command << ": command not found" << endl; }
  }
}
