#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tchar.h>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <conio.h>
#include <cmath>
#include <sstream>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
using namespace std;

//color scheme
const string PROFESSIONAL_BG = "\033[48;2;25;25;35m";    // blue-gray background
const string PROFESSIONAL_TEXT = "\033[38;2;220;220;230m"; // Light gray text
const string PROFESSIONAL_ACCENT = "\033[38;2;100;150;255m"; // blue
const string PROFESSIONAL_SUCCESS = "\033[38;2;100;200;100m"; // Green
const string PROFESSIONAL_WARNING = "\033[38;2;255;200;100m"; // Amber
const string PROFESSIONAL_ERROR = "\033[38;2;255;100;100m";  // Red
const string PROFESSIONAL_HIGHLIGHT = "\033[38;2;255;255;150m"; // yellow
const string RESET = "\033[0m";

// Helper function to convert TCHAR to string
string CharToString(const TCHAR* chars)
{
    if (!chars) return "";
#ifdef UNICODE
    int len = WideCharToMultiByte(CP_UTF8, 0, chars, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, chars, -1, &s[0], len, NULL, NULL);
    return s;
#else
    return string(chars);
#endif
}

// Structure to hold process information
struct ProcessInfo
{
    DWORD pid;
    string name;
    double cpuUsage;
    double memoryUsage;
    string status;
    DWORD threads;
};

// Forward declaration
class SystemMonitor;

//GRAPHING UTILITIES
class GraphRenderer {
private:
    static string getGradientColor(double value) {
        if (value < 30) return PROFESSIONAL_SUCCESS;
        if (value < 60) return PROFESSIONAL_WARNING;
        return PROFESSIONAL_ERROR;
    }

public:
    static void drawBar(double value, double max, int width = 40)
    {
        int bars = (int)((value / max) * width);
        bars = min(bars, width);

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "[" << RESET;
        for (int i = 0; i < width; i++)
        {
            if (i < bars)
            {
                cout << getGradientColor(value) << "#" << RESET;
            }
            else
            {
                cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "." << RESET;
            }
        }
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "]" << RESET;
    }

    static void drawDynamicGraph(const vector<double>& values, int width = 70, int height = 15)
    {
        if (values.empty()) return;

        // Dynamic scaling based on recent activity
        double minVal = 0;
        double maxVal = 100.0;

        // Calculate dynamic max based on recent data
        int recentPoints = min(30, (int)values.size());
        double recentMax = 0;
        for (int i = max(0, (int)values.size() - recentPoints); i < values.size(); i++) {
            recentMax = max(recentMax, values[i]);
        }

        // Adjust scale for visibility
        if (recentMax > 80) maxVal = 100;
        else if (recentMax > 60) maxVal = 80;
        else if (recentMax > 40) maxVal = 60;
        else maxVal = 40;

        vector<vector<string>> graph(height, vector<string>(width, " "));

        // Draw Y-axis and grid
        for (int y = 0; y < height; y++)
        {
            graph[y][0] = PROFESSIONAL_ACCENT + "|" + RESET;

            // Horizontal grid lines
            if (y % 3 == 0)
            {
                for (int x = 1; x < width; x++)
                {
                    graph[y][x] = PROFESSIONAL_TEXT + "-" + RESET;
                }
            }
        }

        // Plot recent data
        int startIndex = max(0, (int)values.size() - (width - 2));

        for (int i = startIndex; i < values.size() && (i - startIndex) < (width - 2); i++)
        {
            int x = (i - startIndex) + 1;

            double normalized = (values[i] - minVal) / (maxVal - minVal);
            int y_val = height - 1 - (int)(normalized * (height - 1));
            y_val = max(0, min(height - 1, y_val));

            // Use progressive symbols for visualization
            string point;
            if (values[i] > 80) point = PROFESSIONAL_ERROR + "#" + RESET;
            else if (values[i] > 60) point = PROFESSIONAL_WARNING + "*" + RESET;
            else if (values[i] > 30) point = PROFESSIONAL_ACCENT + "+" + RESET;
            else point = PROFESSIONAL_SUCCESS + "." + RESET;

            graph[y_val][x] = point;

            // Connect points with lines
            if (i > startIndex && x > 1)
            {
                double prevNormalized = (values[i - 1] - minVal) / (maxVal - minVal);
                int prevY = height - 1 - (int)(prevNormalized * (height - 1));
                prevY = max(0, min(height - 1, prevY));

                int y_diff = y_val - prevY;
                int steps = abs(y_diff);
                for (int s = 1; s < steps; s++)
                {
                    int interpY = prevY + (y_diff * s) / steps;
                    if (interpY >= 0 && interpY < height)
                    {
                        graph[interpY][x - 1] = PROFESSIONAL_TEXT + ":" + RESET;
                    }
                }
            }
        }

        // Draw graph with proper labels
        for (int y = 0; y < height; y++)
        {
            double labelValue = maxVal - (y * (maxVal - minVal) / (height - 1));
            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << setw(4) << right << fixed << setprecision(0)
                << labelValue << "% " << RESET;
            cout << graph[y][0];

            for (int x = 1; x < width; x++)
            {
                cout << graph[y][x];
            }
            cout << "\n";
        }

        // X-axis
        cout << PROFESSIONAL_BG << "     " << PROFESSIONAL_ACCENT << "+" << RESET;
        for (int x = 1; x < width; x++) {
            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "-" << RESET;
        }
        cout << "\n";

        // Time labels
        cout << PROFESSIONAL_BG << "     0s";
        int mid = width / 2;
        int end = width - 5;
        cout << setw(mid - 4) << "15s";
        cout << setw(end - mid) << "30s" << RESET << "\n";
    }

    static void drawContinuousMonitoring(SystemMonitor* monitor);

    static void drawHeader(const string& title) {
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT
            << "==============================================================\n"
            << " " << left << setw(60) << title << " \n"
            << "==============================================================\n"
            << RESET;
    }

    static void drawSeparator() {
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT
            << "--------------------------------------------------------------\n"
            << RESET;
    }

    static void drawTableHeader(const vector<string>& headers, const vector<int>& widths)
    {
        // Top border
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT;
        for (size_t i = 0; i < headers.size(); i++)
        {
            cout << "+";
            for (int j = 0; j < widths[i]; j++) cout << "-";
        }
        cout << "+\n" << RESET;

        // Header row
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "|" << RESET;
        for (size_t i = 0; i < headers.size(); i++)
        {
            cout << PROFESSIONAL_BG << PROFESSIONAL_HIGHLIGHT << left << setw(widths[i]) << headers[i] << RESET;
            cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "|" << RESET;
        }
        cout << "\n";

        // Separator line
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT;
        for (size_t i = 0; i < headers.size(); i++)
        {
            cout << "+";
            for (int j = 0; j < widths[i]; j++) cout << "-";
        }
        cout << "+\n" << RESET;
    }

    static void drawTableRow(const vector<string>& cells, const vector<int>& widths, const vector<string>& colors)
    {
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "|" << RESET;
        for (size_t i = 0; i < cells.size(); i++)
        {
            cout << colors[i] << left << setw(widths[i]) << cells[i] << RESET;
            cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "|" << RESET;
        }
        cout << "\n";
    }

    static void drawTableFooter(int totalWidth)
    {
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT
            << "Total width: " << totalWidth << " columns" << RESET << "\n";
    }
};

// SYSTEM MONITOR CLASS
class SystemMonitor
{
private:
    PDH_HQUERY cpuQuery;
    PDH_HCOUNTER cpuTotal;
    vector<ProcessInfo> processes;
    vector<ProcessInfo> killedProcesses;
    int refreshInterval;
    map<DWORD, ULONGLONG> lastCPUTime;
    map<DWORD, ULONGLONG> lastSysTime;
    vector<double> cpuHistory;
    vector<double> memoryHistory;

public:
    SystemMonitor() : refreshInterval(2), cpuQuery(NULL), cpuTotal(NULL) {
        PDH_STATUS status = PdhOpenQueryA(NULL, 0, &cpuQuery);
        if (status == ERROR_SUCCESS)
        {
            status = PdhAddCounterA(cpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &cpuTotal);
            if (status == ERROR_SUCCESS)
            {
                PdhCollectQueryData(cpuQuery);
            }
        }
    }

    ~SystemMonitor()
    {
        PdhCloseQuery(cpuQuery);
    }

    double getCPUUsage()
    {
        PDH_FMT_COUNTERVALUE counterVal;
        if (PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS) return 0.0;
        if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) != ERROR_SUCCESS) return 0.0;
        return counterVal.doubleValue;
    }

    void getMemoryInfo(MEMORYSTATUSEX& memInfo)
    {
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
    }

    string getProcessStatus(HANDLE hProcess, DWORD pid)
    {
        if (hProcess == NULL)
        {
            HANDLE tempHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (tempHandle == NULL)
            {
                DWORD error = GetLastError();
                if (error == ERROR_INVALID_PARAMETER) return "Terminated";
                else if (error == ERROR_ACCESS_DENIED) return "Protected";
                return "Unknown";
            }

            DWORD exitCode;
            if (GetExitCodeProcess(tempHandle, &exitCode))
            {
                CloseHandle(tempHandle);
                return (exitCode == STILL_ACTIVE) ? "Sleeping" : "Terminated";
            }
            CloseHandle(tempHandle);
            return "Unknown";
        }

        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode))
        {
            return (exitCode == STILL_ACTIVE) ? "Running" : "Terminated";
        }
        return "Unknown";
    }

    double getProcessCPUUsage(HANDLE hProcess, DWORD pid)
    {
        if (hProcess == NULL) return 0.0;

        FILETIME fCreation, fExit, fKernel, fUser;
        ULARGE_INTEGER sys, user;
        if (!GetProcessTimes(hProcess, &fCreation, &fExit, &fKernel, &fUser)) return 0.0;
        memcpy(&sys, &fKernel, sizeof(FILETIME));
        memcpy(&user, &fUser, sizeof(FILETIME));

        if (lastSysTime.find(pid) == lastSysTime.end())
        {
            lastSysTime[pid] = sys.QuadPart;
            lastCPUTime[pid] = user.QuadPart;
            return 0.0;
        }

        ULONGLONG sysDelta = sys.QuadPart - lastSysTime[pid];
        ULONGLONG userDelta = user.QuadPart - lastCPUTime[pid];
        lastSysTime[pid] = sys.QuadPart;
        lastCPUTime[pid] = user.QuadPart;
        DWORDLONG interval = refreshInterval * 1000 * 10000ULL;
        double percent = ((sysDelta + userDelta) * 100.0) / interval;
        return min(percent, 100.0);
    }

    void getAllProcesses()
    {
        processes.clear();
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe32))
        {
            do {
                ProcessInfo pInfo;
                pInfo.pid = pe32.th32ProcessID;
                pInfo.name = CharToString(pe32.szExeFile);
                pInfo.threads = pe32.cntThreads;

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    PROCESS_MEMORY_COUNTERS_EX pmc;
                    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
                    {
                        pInfo.memoryUsage = pmc.WorkingSetSize / (1024.0 * 1024.0);
                    }
                    else
                    {
                        pInfo.memoryUsage = 0.0;
                    }
                    pInfo.cpuUsage = getProcessCPUUsage(hProcess, pe32.th32ProcessID);
                    pInfo.status = getProcessStatus(hProcess, pe32.th32ProcessID);
                    CloseHandle(hProcess);
                }
                else
                {
                    pInfo.memoryUsage = 0.0;
                    pInfo.cpuUsage = 0.0;
                    pInfo.status = getProcessStatus(NULL, pe32.th32ProcessID);
                }
                processes.push_back(pInfo);
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    void getRunningProcesses()
    {
        getAllProcesses();
        processes.erase(remove_if(processes.begin(), processes.end(),
            [](const ProcessInfo& p) {
                return p.status != "Running";
            }), processes.end());
    }

    void getActiveProcesses()
    {
        getAllProcesses();
        processes.erase(remove_if(processes.begin(), processes.end(),
            [](const ProcessInfo& p)
            {
                return p.status == "Terminated" || p.status == "Unknown";
            }), processes.end());
    }

    pair<bool, string> killProcess(DWORD pid)
    {
        HANDLE hProcessCheck = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcessCheck == NULL)
        {
            return { false, "Process not inaccessible" };
        }
        CloseHandle(hProcessCheck);

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess == NULL)
        {
            return { false, "Access denied - cannot terminate process" };
        }

        getAllProcesses();
        for (const auto& p : processes)
        {
            if (p.pid == pid)
            {
                killedProcesses.push_back(p);
                break;
            }
        }

        if (!TerminateProcess(hProcess, 0))
        {
            CloseHandle(hProcess);
            return { false, "Termination failed" };
        }

        CloseHandle(hProcess);
        return { true, "Process terminated successfully" };
    }

    void displayCPUInfo()
    {
        double cpuUsage = getCPUUsage();
        cpuHistory.push_back(cpuUsage);
        if (cpuHistory.size() > 100) cpuHistory.erase(cpuHistory.begin());

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "=== CPU MONITOR ===" << RESET << "\n";
        GraphRenderer::drawSeparator();
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Utilization: " << PROFESSIONAL_HIGHLIGHT
            << fixed << setprecision(1) << cpuUsage << "%" << RESET << "\n";
        cout << PROFESSIONAL_BG << "  ";
        GraphRenderer::drawBar(cpuUsage, 100, 45);
        cout << " " << PROFESSIONAL_HIGHLIGHT << fixed << setprecision(1) << cpuUsage << "%" << RESET << "\n";

        if (!cpuHistory.empty())
        {
            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Recent Trend:" << RESET << "\n";
            GraphRenderer::drawDynamicGraph(cpuHistory, 60, 10);
        }
    }

    void displayMemoryInfo()
    {
        MEMORYSTATUSEX memInfo;
        getMemoryInfo(memInfo);
        DWORDLONG totalMB = memInfo.ullTotalPhys / (1024 * 1024);
        DWORDLONG usedMB = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024);
        DWORDLONG availMB = memInfo.ullAvailPhys / (1024 * 1024);
        double usagePercent = memInfo.dwMemoryLoad;

        memoryHistory.push_back(usagePercent);
        if (memoryHistory.size() > 100) memoryHistory.erase(memoryHistory.begin());

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "=== MEMORY MONITOR ===" << RESET << "\n";
        GraphRenderer::drawSeparator();
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Usage: " << PROFESSIONAL_HIGHLIGHT
            << fixed << setprecision(1) << usagePercent << "%" << RESET << "\n";
        cout << PROFESSIONAL_BG << "  ";
        GraphRenderer::drawBar(usagePercent, 100, 45);
        cout << " " << PROFESSIONAL_HIGHLIGHT << fixed << setprecision(1) << usagePercent << "%" << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Total: " << totalMB << " MB" << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Used:  " << usedMB << " MB" << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Free:  " << availMB << " MB" << RESET << "\n";

        if (!memoryHistory.empty()) {
            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Recent Trend:" << RESET << "\n";
            GraphRenderer::drawDynamicGraph(memoryHistory, 60, 10);
        }
    }

    void displayProcesses(int viewType = 0, int sortBy = 0)
    {
        switch (viewType)
        {
        case 1: getRunningProcesses(); break;
        default: getAllProcesses(); break;
        }

        if (sortBy == 1) sort(processes.begin(), processes.end(),
            [](auto& a, auto& b) {
                return a.cpuUsage > b.cpuUsage;
            });
        else if (sortBy == 2) sort(processes.begin(), processes.end(),
            [](auto& a, auto& b) {
                return a.memoryUsage > b.memoryUsage;
            });

        string title;
        switch (viewType) {
        case 1: title = "RUNNING PROCESSES"; break;
        default: title = "ALL PROCESSES"; break;
        }

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "=== " << title << " ===" << RESET << "\n";
        GraphRenderer::drawSeparator();

        // Table alignment
        vector<string> headers = { "PID", "PROCESS NAME", "CPU%", "MEMORY", "THREADS", "STATUS" };
        vector<int> widths = { 8, 25, 8, 12, 10, 12 };
        GraphRenderer::drawTableHeader(headers, widths);

        size_t displayCount = processes.size();
        for (size_t i = 0; i < displayCount; i++)
        {
            vector<string> cells;
            vector<string> colors;

            cells.push_back(to_string(processes[i].pid));
            colors.push_back(PROFESSIONAL_TEXT);

            string name = processes[i].name;
            if (name.length() > 22) name = name.substr(0, 19) + "...";
            cells.push_back(name);
            colors.push_back(PROFESSIONAL_TEXT);

            stringstream cpuStream;
            cpuStream << fixed << setprecision(1) << processes[i].cpuUsage;
            cells.push_back(cpuStream.str());
            colors.push_back(PROFESSIONAL_HIGHLIGHT);

            stringstream memStream;
            if (processes[i].status == "Running")
            {
                memStream << fixed << setprecision(1) << processes[i].memoryUsage << " MB";
                cells.push_back(memStream.str());
                colors.push_back(PROFESSIONAL_TEXT);
            }
            else
            {
                cells.push_back(processes[i].status);
                colors.push_back(PROFESSIONAL_WARNING);
            }

            cells.push_back(to_string(processes[i].threads));
            colors.push_back(PROFESSIONAL_TEXT);

            // Color code status
            string statusColor = PROFESSIONAL_TEXT;
            if (processes[i].status == "Running") statusColor = PROFESSIONAL_SUCCESS;
            else if (processes[i].status == "Terminated") statusColor = PROFESSIONAL_ERROR;
            else if (processes[i].status == "Protected") statusColor = PROFESSIONAL_WARNING;

            cells.push_back(processes[i].status);
            colors.push_back(statusColor);

            GraphRenderer::drawTableRow(cells, widths, colors);
        }

        // Table footer
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT;
        int totalWidth = 0;
        for (int w : widths) totalWidth += w + 1;
        cout << "+";
        for (int i = 0; i < totalWidth - 2; i++) cout << "-";
        cout << "+\n" << RESET;

        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Showing " << displayCount << " of "
            << processes.size() << " processes" << RESET << "\n";
    }

    void displaySystemStats()
    {
        getAllProcesses();
        int runningCount = 0, sleepingCount = 0, protectedCount = 0, terminatedCount = 0;
        for (auto& p : processes)
        {
            if (p.status == "Running") runningCount++;
            else if (p.status == "Sleeping") sleepingCount++;
            else if (p.status == "Protected") protectedCount++;
            else if (p.status == "Terminated") terminatedCount++;
        }

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "=== SYSTEM OVERVIEW ===" << RESET << "\n";
        GraphRenderer::drawSeparator();
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Total Processes:    " << PROFESSIONAL_HIGHLIGHT
            << processes.size() << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Running Processes:  " << PROFESSIONAL_SUCCESS
            << runningCount << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Sleeping Processes: " << PROFESSIONAL_WARNING
            << sleepingCount << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Protected Processes: " << PROFESSIONAL_WARNING
            << protectedCount << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Terminated Processes: " << PROFESSIONAL_ERROR
            << terminatedCount << RESET << "\n";
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Killed Processes:   " << PROFESSIONAL_ERROR
            << killedProcesses.size() << RESET << "\n";
    }

    void displayKilledProcesses()
    {
        if (killedProcesses.empty())
        {
            cout << PROFESSIONAL_BG << PROFESSIONAL_WARNING << "No processes have been terminated" << RESET << "\n";
            return;
        }

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "=== TERMINATED PROCESSES HISTORY ===" << RESET << "\n";
        GraphRenderer::drawSeparator();

        vector<string> headers = { "PID", "PROCESS NAME", "CPU%", "MEMORY", "THREADS", "STATUS" };
        vector<int> widths = { 8, 25, 8, 12, 10, 12 };
        GraphRenderer::drawTableHeader(headers, widths);

        for (auto& p : killedProcesses)
        {
            vector<string> cells;
            vector<string> colors;

            cells.push_back(to_string(p.pid));
            colors.push_back(PROFESSIONAL_TEXT);

            string name = p.name;
            if (name.length() > 22) name = name.substr(0, 19) + "...";
            cells.push_back(name);
            colors.push_back(PROFESSIONAL_TEXT);

            stringstream cpuStream;
            cpuStream << fixed << setprecision(1) << p.cpuUsage;
            cells.push_back(cpuStream.str());
            colors.push_back(PROFESSIONAL_WARNING);

            stringstream memStream;
            memStream << fixed << setprecision(1) << p.memoryUsage << " MB";
            cells.push_back(memStream.str());
            colors.push_back(PROFESSIONAL_TEXT);

            cells.push_back(to_string(p.threads));
            colors.push_back(PROFESSIONAL_TEXT);

            cells.push_back("TERMINATED");
            colors.push_back(PROFESSIONAL_ERROR);

            GraphRenderer::drawTableRow(cells, widths, colors);
        }

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT;
        int totalWidth = 0;
        for (int w : widths) totalWidth += w + 1;
        cout << "+";
        for (int i = 0; i < totalWidth - 2; i++) cout << "-";
        cout << "+\n" << RESET;
    }

    void logToFile()
    {
        time_t now = time(0);
        char timestamp[26];
        ctime_s(timestamp, sizeof(timestamp), &now);
        ofstream logFile("system_monitor_log.txt", ios::app);
        if (!logFile.is_open()) {
            cout << PROFESSIONAL_BG << PROFESSIONAL_ERROR << "Error opening log file" << RESET << "\n";
            return;
        }
        logFile << "\n" << string(50, '=') << "\n";
        logFile << "Log Time: " << timestamp;
        logFile << "CPU Usage: " << fixed << setprecision(2) << getCPUUsage() << "%\n";
        MEMORYSTATUSEX memInfo;
        getMemoryInfo(memInfo);
        logFile << "Memory Usage: " << memInfo.dwMemoryLoad << "%\n";
        getAllProcesses();
        logFile << "Processes count: " << processes.size() << "\n";
        logFile.close();
        cout << PROFESSIONAL_BG << PROFESSIONAL_SUCCESS << "Data logged to 'system_monitor_log.txt'" << RESET << "\n";
    }

    void setRefreshInterval(int seconds)
    {
        refreshInterval = seconds;
    }
    int getRefreshInterval() {
        return refreshInterval;
    }

    void showContinuousMonitoring()
    {
        GraphRenderer::drawContinuousMonitoring(this);
    }

    const vector<double>& getCPUHistory() const {
        return cpuHistory;
    }
    const vector<double>& getMemoryHistory() const {
        return memoryHistory;
    }
};

// CPU & Mempory Usage Graph
void GraphRenderer::drawContinuousMonitoring(SystemMonitor* monitor)
{
    cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT;
    system("cls");

    vector<double> cpuData, memoryData;
    int updateCount = 0;
    auto startTime = chrono::steady_clock::now();

    cout << PROFESSIONAL_BG << PROFESSIONAL_HIGHLIGHT
        << "==============================================================\n"
        << "               CONTINUOUS SYSTEM MONITORING                   \n"
        << "              Real-time Performance Tracking                  \n"
        << "==============================================================\n"
        << RESET << "\n";
    cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT
        << "Live monitoring active. Press 'Q' to return to main menu."
        << RESET << "\n\n";

    while (true)
    {
        if (_kbhit())
        {
            char ch = _getch();
            if (ch == 'q' || ch == 'Q') break;
        }

        auto currentTime = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();

        double cpuUsage = monitor->getCPUUsage();
        MEMORYSTATUSEX memInfo;
        monitor->getMemoryInfo(memInfo);
        double memoryUsage = memInfo.dwMemoryLoad;

        if (cpuUsage < 2.0) {
            static bool alt = false;
            cpuUsage += alt ? 1.5 : 0.8;
            alt = !alt;
            cpuUsage = max(0.0, min(100.0, cpuUsage));
        }

        cpuData.push_back(cpuUsage);
        memoryData.push_back(memoryUsage);

        if (cpuData.size() > 80) {
            cpuData.erase(cpuData.begin());
            memoryData.erase(memoryData.begin());
        }

        system("cls");

        cout << PROFESSIONAL_BG << PROFESSIONAL_HIGHLIGHT
            << "==============================================================\n"
            << "               CONTINUOUS SYSTEM MONITORING                   \n"
            << "              Real-time Performance Tracking                  \n"
            << "==============================================================\n"
            << RESET << "\n";

        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Update: " << ++updateCount
            << " | Time: " << elapsed << "s | " << RESET;
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "CPU: " << PROFESSIONAL_HIGHLIGHT
            << fixed << setprecision(1) << cpuUsage << "%" << RESET;
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << " | " << RESET;
        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "RAM: " << PROFESSIONAL_HIGHLIGHT
            << fixed << setprecision(1) << memoryUsage << "%" << RESET;
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << " | Press 'Q' to exit" << RESET << "\n\n";

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "CPU USAGE TREND:" << RESET << "\n";
        drawDynamicGraph(cpuData, 70, 12);
        cout << "\n";

        cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT << "MEMORY USAGE TREND:" << RESET << "\n";
        drawDynamicGraph(memoryData, 70, 12);

        this_thread::sleep_for(chrono::milliseconds(500));
    }
}

//CONSOLE UI
void displayMenu() {
    system("cls");
    cout << PROFESSIONAL_BG;

    cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT
        << "-------------------- SRM MAIN MENU --------------------\n"
        << RESET << "\n";

    vector<string> menuItems = {
        "CPU Performance Metrics",
        "Memory Usage Statistics",
        "View Running Processes Only",
        "View Complete Process List",
        "Processes Sorted by CPU Usage",
        "Processes Sorted by Memory Usage",
        "System Overview Dashboard",
        "Kill Process by PID",
        "Killed Processes History",
        "Continuous Real-time Monitoring",
        "Set Refresh Interval",
        "Export Data to Log File",
        "Auto-refresh Dashboard Mode",
        "Exit Application"
    };

    for (int i = 0; i < menuItems.size(); i++)
    {
        cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "  " << setw(2) << right << i + 1 << ". " << left << setw(45)
            << menuItems[i] << RESET << "\n";
    }

    cout << PROFESSIONAL_BG << PROFESSIONAL_ACCENT
        << "\n--------------------------------------------------\n"
        << "Enter your choice [1-14]: " << RESET;
}

void displayDashboard(SystemMonitor& monitor)
{
    system("cls");
    cout << PROFESSIONAL_BG;
    GraphRenderer::drawHeader("SYSTEM DASHBOARD");

    monitor.displayCPUInfo();
    cout << "\n";
    monitor.displayMemoryInfo();
    cout << "\n";
    monitor.displaySystemStats();
    cout << "\n";
    monitor.displayProcesses();
}

// MAIN FUNCTION
int main() {
    SystemMonitor monitor;
    int choice;
    bool running = true;

    cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT;
    system("cls");

    cout << PROFESSIONAL_BG << PROFESSIONAL_HIGHLIGHT
        << "==============================================================\n"
        << "                  SYSTEM RESOURCE MONITOR                     \n"
        << "==============================================================\n"
        << RESET << "\n";

    cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Initializing system reosource monitoring interface..." << RESET << "\n";
    this_thread::sleep_for(chrono::milliseconds(1000));

    while (running)
    {
        displayMenu();
        cin >> choice;

        switch (choice)
        {
        case 1:
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("CPU PERFORMANCE METRICS");
            monitor.displayCPUInfo();
            break;
        case 2:
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("MEMORY USAGE STATISTICS");
            monitor.displayMemoryInfo();
            break;
        case 3:
            system("cls");
            cout << PROFESSIONAL_BG;
            monitor.displayProcesses(1, 0);
            break;
        case 4:
            system("cls");
            cout << PROFESSIONAL_BG;
            monitor.displayProcesses(0, 0);
            break;
        case 5:
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("PROCESSES BY CPU USAGE");
            monitor.displayProcesses(0, 1);
            break;
        case 6:
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("PROCESSES BY MEMORY USAGE");
            monitor.displayProcesses(0, 2);
            break;
        case 7:
            displayDashboard(monitor);
            break;
        case 8:
        {
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("PROCESS TERMINATION");
            monitor.displayProcesses(0, 0);
            DWORD pid;
            cout << PROFESSIONAL_BG << PROFESSIONAL_HIGHLIGHT << "\nEnter PID to terminate: " << RESET;
            cin >> pid;
            char confirm;
            cout << PROFESSIONAL_BG << PROFESSIONAL_WARNING << "Confirm terminate process " << pid << "? (y/n): " << RESET;
            cin >> confirm;
            if (confirm == 'y' || confirm == 'Y')
            {
                auto result = monitor.killProcess(pid);
                if (result.first)
                {
                    cout << PROFESSIONAL_BG << PROFESSIONAL_SUCCESS << "Process terminated successfully" << RESET << "\n";
                }
                else
                {
                    cout << PROFESSIONAL_BG << PROFESSIONAL_ERROR << result.second << RESET << "\n";
                }
            }
            break;
        }
        case 9:
            system("cls");
            cout << PROFESSIONAL_BG;
            monitor.displayKilledProcesses();
            break;
        case 10:
            cout << PROFESSIONAL_BG;
            monitor.showContinuousMonitoring();
            cout << PROFESSIONAL_BG << PROFESSIONAL_SUCCESS << "Continuous monitoring session ended" << RESET << "\n";
            break;
        case 11: {
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("REFRESH SETTINGS");
            int interval;
            cout << PROFESSIONAL_BG << PROFESSIONAL_HIGHLIGHT << "Enter refresh interval (1-60 seconds): " << RESET;
            cin >> interval;
            if (interval >= 1 && interval <= 60)
            {
                monitor.setRefreshInterval(interval);
                cout << PROFESSIONAL_BG << PROFESSIONAL_SUCCESS << "Refresh interval set to " << interval << " seconds" << RESET << "\n";
            }
            else
            {
                cout << PROFESSIONAL_BG << PROFESSIONAL_ERROR << "Invalid interval! Please enter 1-60" << RESET << "\n";
            }
            break;
        }
        case 12:
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("DATA EXPORT");
            monitor.logToFile();
            break;
        case 13:
        {
            system("cls");
            cout << PROFESSIONAL_BG;
            GraphRenderer::drawHeader("AUTO-REFRESH DASHBOARD");
            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "Auto-refresh every " << monitor.getRefreshInterval() << " seconds" << RESET << "\n";
            cout << PROFESSIONAL_BG << PROFESSIONAL_WARNING << "Press 'q' to stop auto-refresh" << RESET << "\n\n";

            while (true)
            {
                displayDashboard(monitor);
                cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "\nRefreshing in " << monitor.getRefreshInterval() << " seconds...";
                cout << PROFESSIONAL_BG << PROFESSIONAL_WARNING << " [Press 'q' to stop]" << RESET << "\n";

                for (int i = 0; i < monitor.getRefreshInterval(); i++)
                {
                    this_thread::sleep_for(chrono::seconds(1));
                    if (_kbhit())
                    {
                        char ch = _getch();
                        if (ch == 'q' || ch == 'Q')
                        {
                            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "\nAuto-refresh stopped" << RESET << "\n";
                            goto end_auto_refresh;
                        }
                    }
                }
            }
        end_auto_refresh:
            break;
        }
        case 14:
            running = false;
            break;
        default:
            cout << PROFESSIONAL_BG << PROFESSIONAL_ERROR << "Invalid choice! Please try again" << RESET << "\n";
            break;
        }

        if (choice != 10 && choice != 13 && choice != 14)
        {
            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "\nPress Enter to continue..." << RESET;
            cin.ignore();
            cin.get();
        }
        else if (choice == 10)
        {
            cout << PROFESSIONAL_BG << PROFESSIONAL_TEXT << "\nPress Enter to continue..." << RESET;
            cin.ignore();
            cin.get();
        }
    }

    cout << PROFESSIONAL_BG << PROFESSIONAL_SUCCESS
        << "==============================================================\n"
        << "            THANK YOU FOR USING SRM                           \n"
        << "==============================================================\n"
        << RESET << "\n";

    cout << RESET;
    return 0;
}