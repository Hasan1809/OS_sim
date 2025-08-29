# Operating System Simulator
> A comprehensive educational tool demonstrating core OS concepts through visual simulation

## Project Overview
This operating system simulator implements fundamental OS concepts including process scheduling, memory management, and resource synchronization. Built with C and GTK-3, it provides real-time visualization of internal OS mechanisms, making it an ideal learning tool for understanding operating system principles.

## Core Features

### Custom Instruction Interpreter
- Supports basic operations:
  - `print` - Output to console
  - `assign` - Variable assignment
  - `writeFile` - File writing operations
  - `readFile` - File reading operations
  - `printFromTo` - Range-based printing
  - `semWait` - Semaphore wait operation
  - `semSignal` - Semaphore signal operation

### Memory Management
- Fixed memory size (60 words)
- Partitioned storage for:
  - Process Control Blocks (PCBs)
  - Program variables
  - Process instructions
- Real-time memory allocation visualization

### Process Management
- Process Control Block (PCB) tracking:
  - Process ID
  - Process State (Ready/Running/Blocked)
  - Priority Level
  - Program Counter
  - Memory Boundaries

### Scheduler Implementation
Supports multiple scheduling algorithms:
- First Come First Serve (FCFS)
- Round Robin (RR) with configurable quantum
- Multi-level Feedback Queue (MLFQ)
  - Multiple priority levels
  - Dynamic quantum adjustment

### Synchronization Mechanisms
Mutex implementation for:
- User Input operations
- User Output operations
- File Access control
- Process blocking/unblocking logic

### GUI Features
- System Dashboard
  - Clock cycle counter
  - Active scheduling algorithm
  - Process state overview
- Queue Visualization
  - Ready Queue
  - Blocked Queue
  - Currently Running Process
- Interactive Controls
  - Algorithm Selection
  - Quantum Configuration
  - Start/Stop/Reset
- Memory Viewer
  - 60-word memory visualization
  - Allocation status
- Resource Monitor
  - Mutex states
  - Blocked queue status
- Execution Modes
  - Step-by-step execution
  - Continuous simulation

## Project Structure
```
os_project/
├── src/                  # Source files
│   ├── gui.c            # GUI implementation
│   ├── interpreter.c    # Interpreter
│   ├── memory.c        # Memory management
│   ├── mutex.c         # Synchronization
│   ├── process.c       # Process handling
│   ├── queue.c         # Queue operations
│   └── scheduler.c     # Scheduling algorithms
├── include/             # Header files
│   └── *.h             # Corresponding headers
├── example programs/    # Sample programs
│   ├── Program_1.txt
│   ├── Program_2.txt
│   └── Program_3.txt
└── Makefile
```

## Dependencies
- GTK-3.0
- GLib-2.0
- Pango
- Cairo

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential libgtk-3-dev
```

## Build Instructions
```bash
make
```

## Run Instructions
```bash
./program
```

## How It Works
1. System loads process files containing instructions
2. Interpreter parses and executes instructions
3. Memory manager allocates space for processes
4. Scheduler determines execution order
5. Mutexes control resource access
6. GUI updates each clock cycle

## Usage Example
1. Launch the simulator
2. Load a sample program (e.g., Program_1.txt)
3. Select scheduling algorithm
4. Configure quantum (if applicable)
5. Start simulation
6. Observe process execution and resource usage
