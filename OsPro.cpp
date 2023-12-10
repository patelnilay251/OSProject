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

void startServer()
{
    cout << "Server: Starting server..." << endl;

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Server: Socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Server: setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Server: Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Server: Listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server: Waiting for connections..." << endl;
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("Server: Accept");
        exit(EXIT_FAILURE);
    }

    cout << "Server: Connection established." << endl;
    read(new_socket, buffer, 1024);
    cout << "Server: Message received - " << buffer << endl;
    send(new_socket, buffer, strlen(buffer), 0);
    cout << "Server: Echo message sent" << endl;

    close(new_socket);
    close(server_fd);
    cout << "Server: Connection closed." << endl;
}

void startClient()
{
    cout << "Client: Starting client..." << endl;

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    const char *message = "Hello from client";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "Client: Socket creation error" << endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        cout << "Client: Invalid address/ Address not supported" << endl;
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "Client: Connection Failed" << std::endl;
        return;
    }

    send(sock, message, strlen(message), 0);
    cout << "Client: Hello message sent" << endl;
    read(sock, buffer, 1024);
    cout << "Client: Message received - " << buffer << endl;

    close(sock);
    cout << "Client: Connection closed." << endl;
}

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

    cout << "\n--- Network Communication ---" << endl;

    // Start the server in a separate thread and detach it
    cout << "Main: Starting server thread..." << endl;
    std::thread serverThread(startServer);
    serverThread.detach(); // Detach the server thread to run independently
    cout << "Main: Server thread detached and running." << endl;

    // Give the server some time to start up
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Start the client in a separate thread and wait for it to complete
    cout << "Main: Starting client thread..." << endl;
    std::thread clientThread(startClient);
    clientThread.join(); // Join the client thread to wait for its completion
    cout << "Main: Client thread finished." << endl;

    cout << "\nSimulation complete." << endl;

    return 0;
}
