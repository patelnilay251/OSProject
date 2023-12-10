#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using std::cerr;
using std::cin;
using std::condition_variable;
using std::cout;
using std::endl;
using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;
using std::unique_lock;
using std::vector;
using std::chrono::system_clock;

// Process state enumeration
enum ProcessState
{
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

// Process structure
struct Process
{
    string name;
    int priority;
    ProcessState state;
    int processId;                        // Process ID
    int cpuTime;                          // CPU time in seconds
    int memory;                           // Memory requirement in MB
    system_clock::time_point arrivalTime; // Arrival time in the system
    system_clock::time_point startTime;   // Start time for execution
    system_clock::time_point endTime;     // End time of execution

    Process(string n, int p, ProcessState st, int pid, int cpu, int mem)
        : name(n), priority(p), state(st), processId(pid), cpuTime(cpu), memory(mem)
    {
        arrivalTime = system_clock::now();
        // Initialize startTime and endTime to a default (e.g., the epoch)
        startTime = system_clock::time_point();
        endTime = system_clock::time_point();
    }
};

// Define the SchedulingAlgorithm enumeration
enum SchedulingAlgorithm
{
    PRIORITY_SCHEDULING,
    ROUND_ROBIN
};

// Process Scheduler class
class ProcessScheduler
{
private:
    vector<Process> processes;
    mutex queueMutex;
    condition_variable cv;
    bool stopScheduler = false;
    int timeQuantum; // Only used for Round Robin
    SchedulingAlgorithm algorithm;

    // Comparator for Priority Scheduling
    static bool priorityComparator(const Process &a, const Process &b)
    {
        return a.priority < b.priority;
    }

public:
    ProcessScheduler(SchedulingAlgorithm algo, int tq = 1) : algorithm(algo), timeQuantum(tq) {}

    void addProcess(const Process &process)
    {
        unique_lock<mutex> lock(queueMutex);
        processes.push_back(process);
        cv.notify_one();
    }

    void run()
    {
        while (!stopScheduler)
        {
            unique_lock<mutex> lock(queueMutex);
            cv.wait(lock, [this]
                    { return !processes.empty() || stopScheduler; });

            if (stopScheduler)
                break;

            // Sort processes based on selected algorithm
            if (algorithm == PRIORITY_SCHEDULING)
            {
                std::sort(processes.begin(), processes.end(), priorityComparator);
            }

            // Execute and manage processes...
            // Detailed implementation goes here...

            // After processing
            lock.unlock();
        }
    }

    void stop()
    {
        stopScheduler = true;
        cv.notify_all();
    }
};

// Memory Manager class
class MemoryManager
{
    const int MEMORY_SIZE = 1000; // Total memory size
    bool memoryPool[1000];        // Memory pool representation

public:
    MemoryManager()
    {
        std::fill_n(memoryPool, MEMORY_SIZE, false); // Initialize memory as free
    }

    int allocate(int size)
    {
        // Simple first-fit allocation strategy
        for (int i = 0; i < MEMORY_SIZE - size; ++i)
        {
            if (std::all_of(memoryPool + i, memoryPool + i + size, [](bool v)
                            { return !v; }))
            {
                std::fill_n(memoryPool + i, size, true);
                return i; // Return starting address
            }
        }
        return -1; // Allocation failed
    }

    void deallocate(int address, int size)
    {
        std::fill_n(memoryPool + address, size, false);
    }
};

// File System class
class FileSystem
{
public:
    void createFile(const string &fileName, const string &content)
    {
        std::ofstream file(fileName);
        if (!file)
        {
            cerr << "Error opening file for writing: " << fileName << endl;
            return;
        }
        file << content;
        file.close();
    }

    string readFile(const string &fileName)
    {
        std::ifstream file(fileName);
        if (!file)
        {
            cerr << "Error opening file for reading: " << fileName << endl;
            return "";
        }
        string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }

    void deleteFile(const string &fileName)
    {
        if (remove(fileName.c_str()) != 0)
        {
            cerr << "Error deleting file: " << fileName << endl;
        }
    }
};

void simulateProcess(const Process &process)
{
    cout << "Simulating " << process.name << endl;
    // Simulating process work, which is represented by a sleep/delay
    std::this_thread::sleep_for(std::chrono::seconds(process.cpuTime));

    // Output to indicate completion of process simulation
    cout << "Completed simulation of " << process.name << endl;
}

class SharedMemory
{
    string data;
    std::mutex dataMutex; // Renamed the mutex to avoid naming conflict

public:
    void writeData(const string &newData)
    {
        std::lock_guard<std::mutex> lock(dataMutex); // Use the renamed mutex
        data = newData;
    }

    string readData()
    {
        std::lock_guard<std::mutex> lock(dataMutex); // Use the renamed mutex
        return data;
    }
};

int main()
{
    cout << "Starting program..." << endl;

    // Creating a vector of Process instances
    cout << "\nCreating processes..." << endl;
    vector<Process> processes = {
        Process("Process A", 2, READY, 1, 4, 10),
        Process("Process B", 1, READY, 2, 3, 15),
        Process("Process C", 3, READY, 3, 5, 8)};

    // Sorting processes based on priority (for demonstration)
    cout << "Sorting processes based on priority..." << endl;
    std::sort(processes.begin(), processes.end(), [](const Process &a, const Process &b)
              { return a.priority < b.priority; });

    // Simulating process execution in separate threads
    cout << "Simulating process execution in separate threads..." << endl;
    vector<thread> threads;
    for (const Process &process : processes)
    {
        threads.push_back(thread(simulateProcess, process));
    }

    for (thread &t : threads)
    {
        t.join(); // Ensuring all threads complete
    }

    // Demonstrating file operations
    cout << "\nDemonstrating file operations..." << endl;
    FileSystem fileSystem;
    fileSystem.createFile("sample.txt", "This is a simple file created by a process.");
    string fileContent = fileSystem.readFile("sample.txt");
    cout << "File Content: " << fileContent << endl;
    fileSystem.deleteFile("sample.txt");

    // Memory management demonstration
    cout << "\nMemory management demonstration..." << endl;
    MemoryManager memoryManager;
    int allocatedAddress = memoryManager.allocate(100);
    if (allocatedAddress != -1)
    {
        cout << "Memory allocated at address: " << allocatedAddress << endl;
        memoryManager.deallocate(allocatedAddress, 100);
        cout << "Memory deallocated." << endl;
    }
    else
    {
        cout << "Memory allocation failed." << endl;
    }

    // Shared memory IPC demonstration
    cout << "\nShared memory IPC demonstration..." << endl;
    SharedMemory sharedMemory;
    sharedMemory.writeData("IPC Data");
    string ipcData = sharedMemory.readData();
    cout << "IPC Data: " << ipcData << endl;

    // Process scheduler demonstration
    cout << "\nProcess scheduler demonstration..." << endl;
    ProcessScheduler scheduler(ROUND_ROBIN, 2); // Example: Round Robin with a time quantum of 2 seconds

    // Add processes to the scheduler
    cout << "Adding processes to the scheduler..." << endl;
    for (const auto &process : processes)
    {
        scheduler.addProcess(process);
    }

    // Start the scheduler in a separate thread
    cout << "Starting the scheduler in a separate thread..." << endl;
    thread schedulerThread(&ProcessScheduler::run, &scheduler);

    // Simulate some operation time
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop the scheduler and join the thread
    cout << "Stopping the scheduler and joining the thread..." << endl;
    scheduler.stop();
    schedulerThread.join();

    // Network communication demonstration
    // cout << "\nNetwork communication demonstration..." << endl;
    // std::thread serverThread(startServer);
    // serverThread.detach(); // Detach the server thread to run independently
    // std::thread clientThread(startClient);
    // clientThread.join(); // Join the client thread to wait for its completion

    cout << "\nSimulation complete." << endl;

    return 0;
}
