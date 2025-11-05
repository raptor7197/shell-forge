#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>

using namespace std;


// Windows & Linux/Unix differences in file paths
#ifdef _WIN32
  const char PATH_SEPARATOR = ';';
  const char PATH_SLASH = '\\';
#else  
  const char PATH_SEPARATOR = ':'; 
  const char PATH_SLASH = '/';
#endif

filesystem::path current_working_directory;

enum CommandType {
  Cd,     // change directory
  Pwd,    // print working directory  
  Echo, 
  Exit0,
  Type,
  Path,
  Invalid
};

vector<string> parseInputIntoTokens(string input) {
  const char kSingleQuote = '\'';
  const char kDoubleQuote = '"';
  bool inSingleQuotes = false;
  bool inDoubleQuotes = false;
  bool escapeLiteral = false;

  vector<string> tokens;
  string nextToken = "";
  input.push_back(' ');  // we do this so our for loop properly pushs back the last token to the token list
  for(auto c : input) {
    if(c == kSingleQuote && !escapeLiteral && !inDoubleQuotes) {
      inSingleQuotes = !inSingleQuotes;      
    }
    else if(c == kDoubleQuote && !escapeLiteral && !inSingleQuotes) {
      inDoubleQuotes = !inDoubleQuotes;      
    }
    else if(c == '\\' && !escapeLiteral && !inSingleQuotes) {
      escapeLiteral = true;
    }
    else if(c == ' ' && !escapeLiteral && !inSingleQuotes && !inDoubleQuotes) {
      if(nextToken != "") // skip on spaces
        tokens.push_back(nextToken);
      nextToken = "";
    }
    else {
      nextToken.push_back(c);
      if (escapeLiteral)
        escapeLiteral = false;
    }
  }
  return tokens;
}


/// @brief Looks for the path of a command found in PATH.
/// @param command Command to look for e.g. "git" or "python3"
/// @return path to the executable of a command. Return empty string if command is not found in PATH.
string getPath(string command) {
  string env_path = getenv("PATH");
  
  stringstream ss(env_path);
  string path;
  while(!ss.eof()){
    getline(ss, path, PATH_SEPARATOR);
    path += PATH_SLASH + command;

    #ifdef _WIN32
    path += ".exe";
    #endif

    // cout << "   next check " << path << endl;
    if(filesystem::exists(path)) {
      return path;
    }
  }

  return "";
}

CommandType interpreteInputAsCommand(string command) {
  // TODO: make it ignore upper case
  if(command == "cd") return CommandType::Cd;
  if(command == "pwd") return CommandType::Pwd;
  if(command == "echo") return CommandType::Echo;
  if(command == "exit") return CommandType::Exit0;
  if(command == "type") return CommandType::Type;
  if(!getPath(command).empty()) return CommandType::Path;

  return CommandType::Invalid;
}

/// @brief Performs the Shell builtin 'echo' command
/// @param input user input
void RunBuiltinEcho(vector<string> const &tokens) {
  if(tokens.begin() == tokens.end()) return;  // safety
  for(auto it = tokens.begin() + 1; it != tokens.end(); ++it) {
    cout << *it << " ";// << endl;
  }
  cout << endl;
}

/// @brief Performs the Shell builtin 'type' command
/// @param input user input
void RunBuiltinType(vector<string> const &tokens) {
  string const &command = tokens[1];
  switch(interpreteInputAsCommand(command)) {
    case CommandType::Invalid:
      cout << command << ": not found" << endl; 
      break;
    case CommandType::Path:
      cout << command << " is " << getPath(command) <<endl; 
      break;
    default: 
      cout << command << " is a shell builtin" << endl; 
  }     
}

/// @brief Performs the command
/// @param input user input
void RunPathCommand(string input) {
  const char* c_ptr_input = input.c_str();
  system(c_ptr_input);
}

void RunBuiltinPwd() {
  cout << current_working_directory.string() << endl;
}

void RunBuiltinCd(string new_path) {  
  // ~ relates to %HOME
  if(new_path == "~") {
    #ifdef _WIN32
    new_path = getenv("USERPROFILE");
    #else
    new_path = getenv("HOME");
    #endif
  }

  // glue relative part to current working dir
  if(new_path.front() == '.') {
    new_path = current_working_directory.string() + PATH_SLASH + new_path; 
  }
  
  filesystem::path path = filesystem::absolute(new_path);
  if(filesystem::exists(path)) {
    #ifndef _WIN32
    path = filesystem::canonical(new_path);
    #endif
    current_working_directory = path;
  }
  else {
    cout << "cd: " << path.string() << ": No such file or directory" << endl;
  }
}


void printNextInputLine() {
  // for better visual - will fail Code Crafter tests
  // cout << "$ " << current_working_directory.string() << "> ";
  
  cout << "$ "; // comment this if you want to use above
}

int main() {  
  current_working_directory = filesystem::current_path();
  string input;
  printNextInputLine();

  while(getline(cin, input)) {
    // Flush after every cout / std:cerr
    cout << unitbuf;
    cerr << unitbuf;

    // input = trim(input);
    vector<string> tokens = parseInputIntoTokens(input);
    string *command = &tokens[0];
    switch(interpreteInputAsCommand(*command)){
      case CommandType::Cd:
        RunBuiltinCd(tokens[1]);
        break;
      case CommandType::Pwd:
        RunBuiltinPwd();
        break;
      case CommandType::Exit0:
        return 0;
      case CommandType::Echo:
        RunBuiltinEcho(tokens);
        break;
      case CommandType::Type:
        RunBuiltinType(tokens);       
        break;
      case CommandType::Path:
        RunPathCommand(input);
        break;
        
      default:
        cout << *command << ": command not found" << endl;  
    }

    printNextInputLine();
    
  }
}