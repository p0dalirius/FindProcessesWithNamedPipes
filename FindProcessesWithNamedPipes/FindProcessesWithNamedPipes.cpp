#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <algorithm>
#include <tlhelp32.h>
#include "ArgumentsParser/ArgumentsParser.h"

/**
 * Gets the executable name of a process given its process ID.
 * 
 * @param processId The process ID to look up
 * @return The executable name of the process as a wide string. Returns "Unknown" if the process cannot be found
 *         or if there was an error accessing process information.
 * 
 * This function uses the Windows Tool Help library to take a snapshot of currently running processes
 * and find the one matching the given process ID. It requires the TH32CS_SNAPPROCESS access right.
 */
std::wstring GetProcessName(DWORD processId) {
    std::wstring UNKNOWN = L"- (Unknown)";

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return UNKNOWN;
    }

    PROCESSENTRY32W processEntry = { sizeof(PROCESSENTRY32W) };
    if (Process32FirstW(snapshot, &processEntry)) {
        do {
            if (processEntry.th32ProcessID == processId) {
                CloseHandle(snapshot);
                return processEntry.szExeFile;
            }
        } while (Process32NextW(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    return UNKNOWN;
}

/**
 * Gets the full path of a process's executable given its process ID.
 * 
 * @param processId The process ID to look up
 * @return The full path of the process executable as a wide string. Returns "Access denied" if the process
 *         cannot be accessed or if there was an error retrieving the path.
 * 
 * This function opens a handle to the process with PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access rights
 * to query its full image path name. The path is returned as an absolute path to the executable.
 */
std::wstring GetProcessPath(DWORD processId) {
    std::wstring ACCESS_DENIED = L"- (Access Denied)";

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (processHandle == NULL) {
        return ACCESS_DENIED;
    }

    wchar_t buffer[MAX_PATH];
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(processHandle, 0, buffer, &size)) {
        CloseHandle(processHandle);
        return buffer;
    }

    CloseHandle(processHandle);
    return ACCESS_DENIED;
}

/**
 * Structure to hold process information including ID, name, path and associated pipes.
 */
struct ProcessInfo {
    DWORD processId;                          // Process ID
    std::wstring processName;                 // Process executable name
    std::wstring processPath;                 // Full path to process executable
    std::vector<std::wstring> namedPipes;     // List of named pipes owned by the process
};


/**
 * Parses command line arguments and configures the argument parser.
 * 
 * @param argc The number of command line arguments
 * @param argv Array of command line argument strings
 * @return Configured ArgumentsParser object containing parsed arguments
 * 
 * This function sets up the argument parser with the following options:
 * Output format options:
 *   --json   Output results in JSON format
 *   --csv    Output results in CSV format 
 *   --text   Output results in plain text format (default)
 * 
 * Debug options:
 *   --debug    Enable debug logging
 *   -v,--verbose  Enable verbose output
 * 
 * The function also displays the program banner before parsing arguments.
 */
ArgumentsParser parseArgs(int argc, char* argv[]) {
	printf("Find Processes With Named Pipes - by Remi GASCOU (Podalirius)\n\n");

	ArgumentsParser parser = ArgumentsParser();
	
	// Output format options
	parser.add_string_argument("json", "-j", "--json", "", false, "Output results in JSON format");
	parser.add_string_argument("csv", "-c", "--csv", "", false, "Output results in CSV format");
    parser.add_string_argument("text", "-t", "--text", "", false, "Output results in plain text format (default)");

	parser.add_boolean_switch_argument("show", "-s", "--show", false, false, "Print the results.");
	
	// Debug and verbosity options
	parser.add_boolean_switch_argument("debug", "-d", "--debug", false, false, "Enable debug logging");

	parser.parse_args(argc, argv);

	return parser;
}

std::vector<ProcessInfo> getProcessesWithNamedPipes() {
    const std::wstring pipePath = L"\\\\.\\pipe\\";
    std::vector<ProcessInfo> processes;

    // Get all named pipes
    std::vector<std::wstring> namedPipes;
    for (const auto& entry : std::filesystem::directory_iterator(pipePath)) {
        namedPipes.push_back(entry.path().filename().wstring());
    }

    // Group pipes by process ID and build ProcessInfo objects
    std::map<DWORD, ProcessInfo> processMap;
    for (const auto& pipeName : namedPipes) {
        std::wstring fullPipePath = pipePath + pipeName;
        
        HANDLE pipeHandle = CreateFileW(
            fullPipePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );

        if (pipeHandle != INVALID_HANDLE_VALUE) {
            DWORD processId;
            if (GetNamedPipeServerProcessId(pipeHandle, &processId)) {
                // Create ProcessInfo if it doesn't exist
                if (processMap.find(processId) == processMap.end()) {
                    ProcessInfo info;
                    info.processId = processId;
                    info.processName = GetProcessName(processId);
                    info.processPath = GetProcessPath(processId);
                    processMap[processId] = info;
                }
                processMap[processId].namedPipes.push_back(pipeName);
            }
            CloseHandle(pipeHandle);
        }
    }

    // Convert map to vector
    for (const auto& [pid, info] : processMap) {
        processes.push_back(info);
    }

    return processes;
}


int main(int argc, char * argv[]) {
    std::setlocale(LC_ALL, "en_US.UTF-8");
    ArgumentsParser parser = parseArgs(argc, argv);

    bool show = std::get<bool>(parser.get_value("show"));
    std::string csv = std::get<std::string>(parser.get_value("csv"));
    std::string json = std::get<std::string>(parser.get_value("json"));
    std::string text = std::get<std::string>(parser.get_value("text"));

    if (csv.empty() && json.empty() && text.empty()) {
        show = true;
    }

    std::vector<ProcessInfo> processes = getProcessesWithNamedPipes();

    // Print the results in the console
    if (show) {
        for (const auto& process : processes) {
            wprintf(L"[+] PID %d:\n", process.processId);
            wprintf(L"  ├── ProcessName: ");
            std::wcout << process.processName << L"\n";
            wprintf(L"  ├── Path: ");
            std::wcout << process.processPath << L"\n";
            wprintf(L"  ├── Named pipes:\n");
            for (const auto& pipe : process.namedPipes) {
                wprintf(L"  │  ├──  \\\\PIPE\\");
                std::wcout << pipe << L"\n";
            }
            wprintf(L"  │  └────\n");
            wprintf(L"  └────\n");
        }
    }

    // Write the textual output
    if (!text.empty()) {
        std::wofstream textFile(text, std::ios::out);
        if (textFile.is_open()) {
            for (const auto& process : processes) {
                textFile << L"[+] PID " << process.processId << L":\n";
                textFile << L"  ├── ProcessName: " << process.processName << L"\n";
                textFile << L"  ├── Path: " << process.processPath << L"\n";
                textFile << L"  ├── Named pipes:\n";
                for (const auto& pipe : process.namedPipes) {
                    textFile << L"  │  ├──  \\\\PIPE\\" << pipe << L"\n";
                }
                textFile << L"  │  └────\n";
                textFile << L"  └────\n";
            }
            textFile.close();
            printf("[+] Results written to text file: %s\n", text.c_str());
        } else {
            printf("[!] Error: Could not open file %s for writing\n", text.c_str());
        }
    }

    // Write the JSON output
    if (!json.empty()) {
        std::ofstream jsonFile(json);
        if (jsonFile.is_open()) {
            jsonFile << "{\n  \"processes\": [\n";
            for (size_t i = 0; i < processes.size(); i++) {
                const auto& process = processes[i];
                jsonFile << "    {\n";
                jsonFile << "      \"pid\": " << process.processId << ",\n";
                jsonFile << "      \"name\": \"" << std::string(process.processName.begin(), process.processName.end()) << "\",\n";
                std::string escapedProcessPath = std::string(process.processPath.begin(), process.processPath.end());
                // Replace backslashes with double backslashes in process path
                size_t pos = 0;
                while ((pos = escapedProcessPath.find('\\', pos)) != std::string::npos) {
                    escapedProcessPath.replace(pos, 1, "\\\\");
                    pos += 2;
                }
                jsonFile << "      \"path\": \"" << escapedProcessPath << "\",\n";
                jsonFile << "      \"namedPipes\": [\n";
                for (size_t j = 0; j < process.namedPipes.size(); j++) {
                    jsonFile << "        \"\\\\\\\\PIPE\\\\" << std::string(process.namedPipes[j].begin(), process.namedPipes[j].end()) << "\"";
                    if (j < process.namedPipes.size() - 1) {
                        jsonFile << ",";
                    }
                    jsonFile << "\n";
                }
                jsonFile << "      ]\n";
                jsonFile << "    }";
                if (i < processes.size() - 1) {
                    jsonFile << ",";
                }
                jsonFile << "\n";
            }
            jsonFile << "  ]\n}\n";
            jsonFile.close();
            printf("[+] Results written to JSON file: %s\n", json.c_str());
        } else {
            printf("[!] Error: Could not open file %s for writing\n", json.c_str());
        }
    }

    // Write the CSV output
    if (!csv.empty()) {
        std::wofstream csvFile(L"output.txt");
        if (csvFile.is_open()) {
            // Write CSV header
            csvFile << "PID,ProcessName,ProcessPath,NamedPipes\n";
            
            for (const auto& process : processes) {
                csvFile << process.processId << ",";
                csvFile << L"\"" << std::wstring(process.processName.begin(), process.processName.end()) << "\",";
                csvFile << L"\"" << std::wstring(process.processPath.begin(), process.processPath.end()) << "\",";
                
                // Combine all pipes into one cell with semicolon separator
                csvFile << "\"";
                for (size_t i = 0; i < process.namedPipes.size(); i++) {
                    csvFile << std::wstring(process.namedPipes[i].begin(), process.namedPipes[i].end());
                    if (i < process.namedPipes.size() - 1) {
                        csvFile << ";";
                    }
                }
                csvFile << "\"\n";
            }
            csvFile.close();
            printf("[+] Results written to CSV file: %s\n", csv.c_str());
        } else {
            printf("[!] Error: Could not open file %s for writing\n", csv.c_str());
        }
    }

    return 0;
}