#include <iostream>
#include <fstream>
#include <regex>
#include <algorithm>
#include <omp.h>
#include <dirent.h>
#include "../include/perg.h"


Perg::Perg() = default;

Perg::Perg(std::vector<std::string> &args) {
	this-> args = args;
	this->argc = args.size();
}

// checkhelpPage
bool Perg::checkHelpArgs() const {
	return args[1] == std::string("-h") || args[1] == std::string("--help") || args[1] == std::string("-help");
}

void Perg::printHelpPage() {
		std::cout << "\nPERG - Parallel grep by Ryan Chui (2017)\n" << std::endl;
		std::cout << "    perg is a custom multithreaded c++ implementation of grep to search multi gigabyte files, datasets, and\n";
		std::cout << "    directories developed at the National Center for Supercomputing Applications.\n" << std::endl;
		std::cout << "    Usage:\n";
		std::cout << "    perg [-A <#>|-f <file>|-r|-v|-V|-w] <search pattern>\n" << std::endl;
		std::cout << "    Modes:\n";
		std::cout << "    -A    After Context         perg will grab a number of lines after the line containing the\n";
		std::cout << "                                <search pattern>. If this option is set too high, repeated sequences can\n";
		std::cout << "                                appear in the output. This does not work with search inversion. \n" << std::endl;
		std::cout << "    -f    Single File Search    Signals perg to only search the <file> for the <search pattern>. If -f is not\n";
		std::cout << "                                used, perg will search the entire directory from where perg is called from.\n" << std::endl;
		std::cout << "    -i    Include Hidden        Will include hidden files in the search. Default search behavior is to\n";
		std::cout << "                                ignore hidden files.\n" << std::endl;
		std::cout << "    -r    Recursive Search      Recursively searches through the directory and all sub directories for the \n";
		std::cout << "                                given <search pattern>. Will not do anything if the [-f <file>] flag is given.\n" << std::endl;
		std::cout << "    -v    Search Inversion      Search for every line that does not include the <search pattern>.\n" << std::endl;
		std::cout << "    -V    Enable Verbose        The file path to the file will be printed along with the search result.\n" << std::endl;
		std::cout << "    -w    File Parallelism      Signals perg to perform single-threaded searches of multiple files. Default\n";
		std::cout << "                                search behavior is to search files one at a time with mulitple threads.\n";
		std::cout << "                                This is optimal when the files are small, similar size, or there are many.\n" << std::endl;
}

void Perg::checkHelpPage() const {
	if (checkHelpArgs()){
		printHelpPage();
		exit(0);
	}
}


// parseCommandLine
std::queue<std::string> Perg::serializeSettings() const {
	std::queue<std::string> settings;
	for (int i = 1; i < argc; i++) {
		settings.push(args[i]);
	}
	return std::move(settings);
}

void Perg::handleFileFlag(std::queue<std::string>& settings) {
	flags.isFile = true;
	if (settings.empty() || isNextArgOption(settings.front())) {
		reportError("The path to the file was not given.");
	}
	flags.file = settings.front();
	settings.pop();
}

void Perg::handleAfterContextFlag(std::queue<std::string>& settings) {
	flags.extra = true;
	if (settings.empty() || isNextArgOption(settings.front())) {
		reportError("The number of after context lines was not given.");
	}
	flags.numExtra = std::stoi(settings.front());
	settings.pop();
}

void Perg::handlePattern(const std::string& arg, const std::queue<std::string>& settings) {
	if (!settings.empty()) {
		reportError("perg was called incorrectly.");
	}
	flags.pattern = arg;
}

void Perg::validatePattern() const {
	if (flags.pattern.empty()) {
		reportError("Search pattern not given.");
	}
}

bool Perg::isNextArgOption(const std::string& arg) {
	return arg.compare(0, 1, "-") == 0;
}

void Perg::reportError(const std::string& message) {
	std::cout << "ERROR: " << message << " \"perg -h\" for help." << std::endl;
	exit(0);
}

bool Perg::isFlag(const std::string& arg) {
	return arg.size() > 1 && arg[0] == '-';
}

void Perg::parseCommandLine() {
	auto settings = serializeSettings();
	while (!settings.empty()) {
		std::string arg = settings.front();
		settings.pop();

		if (isFlag(arg)) {
			if (arg == "-r") {
				flags.recursive = true;
			} else if (arg == "-v") {
				flags.invert = true;
			} else if (arg == "-V") {
				flags.verbose = true;
			} else if (arg == "-f") {
				handleFileFlag(settings);
			} else if (arg == "-w") {
				flags.fileWise = true;
			} else if (arg == "-i") {
				flags.checkHidden = true;
			} else if (arg == "-A") {
				handleAfterContextFlag(settings);
			} else if (arg == "-h" || arg == "--help") {
				checkHelpPage();
				exit(0);
			}
		} else {
			if (settings.empty() && flags.pattern.empty()) {
				flags.pattern = arg;
			} else {
				filePaths.emplace(arg);
			}
		}
	}
	validatePattern();
}




// printMultiple
bool Perg::shouldOutputLine(Info& info) const {
	bool hasMatch = std::regex_search(info.line.begin(), info.line.end(), info.pattern);
	return (hasMatch && !flags.invert) || (!hasMatch && flags.invert);
}

std::string Perg::formatOutput(Info& info) const{
	return (flags.verbose)? info.fileName + ": " + info.line + "\n" :  info.line + "\n";
}

std::string Perg::getExtraLinesIfNeeded(Info& info) const {
	if (!flags.extra) return "";

	std::string output;
	int linesGrabbed = 0;
	std::string extraLine;  

	while (linesGrabbed < flags.extra && std::getline(info.file, extraLine)) {
		output += extraLine + "\n";
		linesGrabbed++;

		if (std::regex_search(extraLine, info.pattern)) {
			linesGrabbed = 0;  // Reset on match
		}
	}
	return output;
}

void Perg::findPatternMatches(Info& info , std::string & output) const{
	if (shouldOutputLine(info)) {
		output += formatOutput(info);
		output += getExtraLinesIfNeeded(info);
	}
}

std::string Perg::getFilePathsFront() {
	std::string fileName;
	#pragma omp critical
	{
		fileName = filePaths.front();
		filePaths.pop();
	}
	return fileName;
}

std::optional<std::ifstream> Perg::openFile(const std::string &fileName) {
	std::ifstream file(fileName);
	if (!file.is_open())
		return std::nullopt;
	return file;
}

void Perg::printFormatedOutput(const std::string &output) const {
	if (!output.empty()) {
		#pragma omp critical
		std::cout << output << (flags.extra ? "--\n" : "");
	}
}


std::vector<std::string> Perg::getFiles() {
	std::vector<std::string> filesToProcess;
	#pragma omp critical
	{
		while (!filePaths.empty()) {
			filesToProcess.push_back(filePaths.front());
			filePaths.pop();
		}
	}
	return filesToProcess;
}


void Perg::printMultiple() {
	std::regex rgx(flags.pattern);
	auto filesToProcess = getFiles();

	#pragma omp parallel for schedule(dynamic)
	for (size_t i = 0; i < filesToProcess.size(); ++i) {
		const std::string& fileName = filesToProcess[i];
		std::string accumulatedOutput;

		auto fileOpt = openFile(fileName);
		if (!fileOpt) continue;

		// Create Info once per file, not per line
		Info info{"", rgx, fileName, std::move(fileOpt.value())};

		while (std::getline(info.file, info.line)) {
			std::string lineOutput;
			findPatternMatches(info, lineOutput);
			accumulatedOutput += lineOutput;
		}

		printFormatedOutput(accumulatedOutput);
	}
}




std::string Perg::getFront() {
	std::string front = filePaths.front().string();
	filePaths.pop();
	return front;
}

std::vector<std::string> getLines(std::ifstream& file) {
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(file, line)) {
		lines.push_back(line);
	}
	return lines;
}

void Perg::printSingle() {
	while (!filePaths.empty()) {
		std::string currentFile = getFront();
		auto file = openFile(currentFile);
		if (!file) return;

		std::regex rgx(flags.pattern);
		std::string line;

		auto lines = getLines(file.value());

		#pragma omp parallel for
		for (size_t i = 0; i < lines.size(); ++i) {
			if (std::regex_search(lines[i], rgx)) {
				#pragma omp critical
				{
					std::cout << (flags.verbose ? currentFile + ": " : "")
							  << lines[i] << "\n";
				}
			}
		}
	}
}



void Perg::findAll(std::filesystem::path& path) {
	if (flags.recursive) {
		// Recursive search (-r flag)
		for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
			if (!std::filesystem::is_directory(entry) &&
				(flags.checkHidden || entry.path().filename().string()[0] != '.')) {
					filePaths.push(entry.path());
				}
		}
	} else {
		// Non-recursive search (only current directory)
		for (auto& entry : std::filesystem::directory_iterator(path)) {  // Note: NOT recursive
			if (!std::filesystem::is_directory(entry) &&
				(flags.checkHidden || entry.path().filename().string()[0] != '.')) {
				filePaths.push(entry.path());
				}
		}
	}

	if (flags.fileWise) {
		printMultiple();
	} else {
		printSingle();
	}
}
