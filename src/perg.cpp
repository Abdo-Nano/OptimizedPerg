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



bool Perg::needHelp() const {
	return args[1] == std::string("-h") || args[1] == std::string("--help") || args[1] == std::string("-help");
}

bool Perg::helpCheck() const {
	if (needHelp()){
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
		return true;
	}
	return false;
}

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

void Perg::handlePattern(const std::string& arg, std::queue<std::string>& settings) {
	if (!settings.empty()) {
		reportError("perg was called incorrectly.");
	}
	flags.pattern = arg;
}

void Perg::validatePattern() {
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
				helpCheck();
				exit(0);
			}
		} else {
			if (settings.empty() && flags.pattern.empty()) {
				flags.pattern = arg;
			} else {
				filePaths.push(arg);
			}
		}
	}

	validatePattern();
}

bool Perg::isFlag(const std::string& arg) {
	return arg.size() > 1 && arg[0] == '-';
}

bool Perg::needsParameter(const std::string& flag) {
	return flag == "-f" || flag == "-A";
}

void Perg::findPatternMatches(std::string& line, std::regex& rgx , std::string & output , std::string& fileName , std::ifstream& file) {
	if (!std::regex_search(line.begin(), line.end(), rgx) && flags.invert) {
		output += (flags.verbose)? fileName + ": " + line + "\n" :  line + "\n";
	} else if (std::regex_search(line.begin(), line.end(), rgx) && !flags.invert) {
		output += (flags.verbose)? fileName + ": " + line + "\n" :  line + "\n";

		if (flags.extra) {
			try {
				for (int j = 0; j < flags.numExtra; ++j) {
					std::getline(file, line);
					output += line + "\n";
					if (std::regex_search(line.begin(), line.end(), rgx)) {
						j = 0;
					}
				}
			} catch (...) {
				std::cout << "ERROR: Could not grab line because it did not exist.\n";
			}
		}
	}
}

void Perg::printMultiple() {
	std::regex rgx(flags.pattern);

	#pragma omp parallel for schedule(dynamic)
	for (unsigned long long i = 0; i < static_cast<unsigned long long>(filePaths.size()); ++i) {
		std::string fileName;
		std::string output;

		#pragma omp critical
		{
			fileName = filePaths.front();
			filePaths.pop();
		}

		std::ifstream file(fileName);
		if (!file.is_open()) {
			#pragma omp critical
			std::cerr << "ERROR: Could not open file: " << fileName << std::endl;
			continue;
		}

		std::string line;
		while (std::getline(file, line)) {
			output.clear();
			findPatternMatches(line, rgx, output, fileName, file);

			if (!output.empty()) {
				#pragma omp critical
				std::cout << output << (flags.extra ? "--\n" : "");
			}
		}
	}
}

unsigned long long Perg::getTotalLines(std::ifstream &file) {
	unsigned long long counter = 0;
	std::string line;
	while (std::getline(file , line)) {
		counter++;
	}
	return counter;
}


void Perg::getExtraContext(std::ifstream& file , std::string line , std::string& output ,
	unsigned long long lineNum , unsigned long long maxLines , std::regex& rgx) {
	try {
		for (int k = 0; k < flags.numExtra && lineNum < maxLines; ++k) {
			std::getline(file, line);
			lineNum++;
			output += line + "\n";
			if (std::regex_search(line.begin(), line.end(), rgx)) {
				k = 0; // Reset counter but watch for infinite loops
			}
		}
	} catch (...) {
		std::cout << "ERROR: Could not grab line because it did not exist.\n";
	}
}

std::string Perg::findPatternMatches(std::string &line, std::regex& rgx, std::ifstream& file,
                                   unsigned long long& lineNum, unsigned long long maxLines) {
    std::string output;
    if (!std::regex_search(line.begin(), line.end(), rgx) && flags.invert) {
        output += flags.verbose ? filePaths.front().string() + ": " + line + "\n" : line + "\n";
    } else if (std::regex_search(line.begin(), line.end(), rgx) && !flags.invert) {
        output += flags.verbose ? filePaths.front().string() + ": " + line + "\n" : line + "\n";
        if (flags.extra) {
        	getExtraContext(file , line , output , lineNum , maxLines , rgx);
        }
    }
    return output;
}

void Perg::printSingle() {
    while (!filePaths.empty()) {
        std::string currentFile = filePaths.front().string();
        std::ifstream file1(currentFile);
        unsigned long long linesNum= getTotalLines(file1);
    	std::regex rgx(flags.pattern);
        file1.close();

    	// don't forget to adjust this
        int threadsNum = 10;
        unsigned long long blockSize = linesNum/ threadsNum + 1;

        #pragma omp parallel for schedule(static)
        for (int i = 0; i < threadsNum; ++i) {
            std::ifstream file2(currentFile);
            std::string line;
            unsigned long long start = i * blockSize;
            unsigned long long end = std::min(linesNum, start + blockSize);

            // Seek to starting position
            for (unsigned long long j = 0; j < start; ++j) {
                std::getline(file2, line);
            }

            // Process assigned block
            for (unsigned long long j = start; j < end; ++j) {
                std::getline(file2, line);
                std::string output = findPatternMatches(line, rgx, file2, j, end);
                if (!output.empty()) {
                    #pragma omp critical
                    std::cout << output << (flags.extra ? "--\n" : "");
                }
            }
        }
        filePaths.pop();
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
