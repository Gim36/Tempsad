#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

// ========================================================
// GLOBALS & LOGGING
// ========================================================

ofstream logFile;

void log(const string &msg, bool newline = true) {
  cout << msg;
  if (logFile.is_open())
    logFile << msg;

  if (newline) {
    cout << endl;
    if (logFile.is_open())
      logFile << endl;
  }
}

// ========================================================
// DATA STRUCTURES
// ========================================================

struct TestCase {
  string input;
  string output;
};

struct FileResult {
  string fileName;
  int passed;
  int total;
  string statusType; // "Error", "NoTests", "Results"
  string statusMsg;
};

// ========================================================
// STRING HELPERS
// ========================================================

string trim(const string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (string::npos == first)
    return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

string normalize(string str) {
  str = trim(str);
  str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
  return str;
}

string cleanValue(string raw) {
  raw = trim(raw);
  if (raw.size() >= 2) {
    if ((raw.front() == '"' && raw.back() == '"') ||
        (raw.front() == '\'' && raw.back() == '\'')) {
      return raw.substr(1, raw.size() - 2);
    }
  }
  return raw;
}

string cleanKey(string raw) {
  if (!raw.empty() && raw.back() == ':')
    raw.pop_back();
  return cleanValue(raw);
}

// ========================================================
// YAML PARSER
// ========================================================

void saveTest(map<string, vector<TestCase>> &data, const string &key,
              TestCase &test) {
  if (key.empty())
    return;
  if (test.input.empty() && test.output.empty())
    return;
  data[key].push_back(test);
  test = TestCase();
}

map<string, vector<TestCase>> parseYaml(const string &configPath) {
  map<string, vector<TestCase>> data;
  ifstream file(configPath);

  if (!file.is_open()) {
    log("Error: Could not open config file: " + configPath);
    exit(1);
  }

  string line;
  string currentKey = "";
  string currentBlock = "";
  TestCase currentTest;
  bool inBlock = false;

  while (getline(file, line)) {
    string trimmed = trim(line);
    if (trimmed.empty())
      continue;

    // FIXED: Check indentation to distinguish comments from content
    bool isIndented = (line.size() > 0 && (line[0] == ' ' || line[0] == '\t'));

    // Comment handling:
    // If it starts with #, it is a comment UNLESS we are in a block and it's
    // indented.
    if (trimmed[0] == '#') {
      if (inBlock && isIndented) {
        // It is content (e.g., #...#+.), fall through to content handler
      } else {
        continue; // It is a real comment, skip
      }
    }

    // 1. Detect Key (e.g., "04":)
    bool isKey = trimmed.back() == ':' &&
                 trimmed.find("input:") == string::npos &&
                 trimmed.find("output:") == string::npos;

    if (isKey) {
      saveTest(data, currentKey, currentTest);
      currentKey = cleanKey(trimmed);
      inBlock = false;
      continue;
    }

    // 2. Detect Block Start (- input:)
    if (trimmed.find("- input:") == 0) {
      saveTest(data, currentKey, currentTest);

      string val = trimmed.substr(8);
      val = trim(val);

      if (val == "|" || val.empty()) {
        inBlock = true;
        currentBlock = "input";
      } else {
        inBlock = false;
        currentTest.input = cleanValue(val);
      }
      continue;
    }

    // 3. Detect Output
    if (trimmed.find("output:") == 0) {
      string val = trimmed.substr(7);
      val = trim(val);

      if (val == "|" || val.empty()) {
        inBlock = true;
        currentBlock = "output";
      } else {
        inBlock = false;
        currentTest.output = cleanValue(val);
      }
      continue;
    }

    // 4. Handle Block Content
    if (inBlock && isIndented) {
      string content = trimmed;
      if (content == "|")
        continue;

      if (currentBlock == "input") {
        if (!currentTest.input.empty())
          currentTest.input += "\n";
        currentTest.input += content;
      } else if (currentBlock == "output") {
        if (!currentTest.output.empty())
          currentTest.output += "\n";
        currentTest.output += content;
      }
    }
  }
  saveTest(data, currentKey, currentTest);

  return data;
}

// ========================================================
// MAIN LOGIC
// ========================================================

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " <file_path_or_dir>" << endl;
    return 1;
  }

  fs::path inputPath(argv[1]);
  fs::path rootDir;
  vector<fs::path> filesToTest;
  string configFileName = "test.yml";

  fs::create_directories("Out/Bin");
  fs::create_directories("Out/Logs");

  logFile.open("Out/Logs/test.log");
  log("Log file created at: Out\\Logs\\test.log");

  if (fs::is_regular_file(inputPath)) {
    filesToTest.push_back(inputPath);
    rootDir = inputPath.parent_path();
  } else if (fs::is_directory(inputPath)) {
    rootDir = inputPath;
    for (const auto &entry : fs::directory_iterator(inputPath)) {
      if (entry.path().extension() == ".cpp") {
        filesToTest.push_back(entry.path());
      }
    }
  } else {
    log("Error: That is not a valid path!");
    return 1;
  }

  // Config Load
  fs::path configPath = rootDir / configFileName;
  log("Loading config from: " + configPath.string() + "\n");
  map<string, vector<TestCase>> testData = parseYaml(configPath.string());

  vector<FileResult> summaryResults;

  // --- TESTING LOOP ---
  for (const auto &filePath : filesToTest) {
    string fileName = filePath.filename().string();
    string stem = filePath.stem().string();

    FileResult result;
    result.fileName = fileName;
    result.passed = 0;
    result.total = 0;

    log("--------------------------------------------------------");
    log("\n-> Testing file: " + fileName + "\n");
    log("--------------------------------------------------------\n");

    fs::path exePathRel = "Out/Bin/" + stem + ".exe";
    fs::path exePathAbs = fs::absolute(exePathRel);

    // Define temps OUTSIDE loop
    fs::path tempIn = fs::absolute("Out/temp.in");
    fs::path tempOut = fs::absolute("Out/temp.out");

    string compileCmd =
        "g++ \"" + filePath.string() + "\" -o \"" + exePathAbs.string() + "\"";

    if (system(compileCmd.c_str()) != 0) {
      log("\n [!] Compilation Failed!\n");
      log("\n--------------------------------------------------------\n");
      result.statusType = "Error";
      result.statusMsg = "COMPILATION ERROR";
      summaryResults.push_back(result);
      continue;
    }

    vector<TestCase> *casesPtr = nullptr;
    if (testData.find(stem) != testData.end())
      casesPtr = &testData[stem];
    else if (testData.find(fileName) != testData.end())
      casesPtr = &testData[fileName];

    if (casesPtr == nullptr || casesPtr->empty()) {
      log("\n [!] No test cases found in yaml for this file.\n");
      log("\n--------------------------------------------------------\n\n");
      result.statusType = "NoTests";
      result.statusMsg = "NO TESTS FOUND";
      summaryResults.push_back(result);
      continue;
    }

    vector<TestCase> &cases = *casesPtr;
    result.total = cases.size();

    for (const auto &tc : cases) {
      string cleanInput = trim(tc.input);
      string cleanExpected = normalize(tc.output);

      // FIXED: Added newline here to match spacing
      log("Input:\n" + cleanInput + "\n");

      ofstream inFile(tempIn);
      inFile << cleanInput;
      inFile.close();

      string runCmd = "cmd /c \"\"" + exePathAbs.string() + "\" < \"" +
                      tempIn.string() + "\" > \"" + tempOut.string() + "\"\"";

      int ret = system(runCmd.c_str());

      ifstream outFile(tempOut);
      stringstream buffer;
      buffer << outFile.rdbuf();
      string rawOutput = buffer.str();
      outFile.close();

      string cleanActual = normalize(rawOutput);

      log("Your output:\n" + cleanActual + "\n");
      log("Expected output:\n" + cleanExpected + "\n");

      if (cleanActual == cleanExpected) {
        log("Test PASSED!\n");
        result.passed++;
      } else {
        log("Test FAILED!\n");
      }
      log("--------------------------------------------------------\n");
    }

    result.statusType = "Results";
    result.statusMsg =
        to_string(result.passed) + "/" + to_string(result.total) + " Passed";

    log("Test cases passed: " + to_string(result.passed));
    log("Total test cases: " + to_string(result.total) + "\n");
    log("--------------------------------------------------------\n\n");

    summaryResults.push_back(result);

    // Clean temps
    try {
      fs::remove(tempIn);
      fs::remove(tempOut);
    } catch (...) {
    }
  }

  // --- FINAL SUMMARY ---
  log("========================================================");
  log("                     FINAL SUMMARY                      ");
  log("========================================================");

  stringstream header;
  header << left << setw(30) << "File Name" << "| Result";
  log(header.str());
  log("------------------------------|-------------------------");

  for (const auto &res : summaryResults) {
    stringstream ss;
    ss << left << setw(30) << res.fileName << "| ";

    ss << left << setw(17) << res.statusMsg;

    if (res.statusType == "Error")
      ss << " [X]";
    else if (res.statusType == "NoTests")
      ss << " [?]";
    else if (res.passed == res.total && res.total > 0)
      ss << " [O]";
    else
      ss << " [X]";

    log(ss.str());
  }
  log("========================================================");

  if (logFile.is_open())
    logFile.close();

  return 0;
}
