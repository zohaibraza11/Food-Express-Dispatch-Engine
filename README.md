#  FoodExpress Dispatch Optimization Engine

> A Data Structures & Algorithms driven food delivery dispatch system implemented in C++ — built as part of **CSC-200L: Data Structures and Algorithms Lab** at UET Lahore (New Campus).

---

#  Project Overview

FoodExpress is a CLI-based simulation of a real-time food delivery platform. During peak hours, thousands of orders arrive simultaneously from different customers and restaurants. This system solves the core computational challenges using custom-implemented data structures and algorithms — no STL containers used.

---

#  Data Structures & Algorithms Used

| Module | Data Structure | Algorithm | Time Complexity |
|---|---|---|---|
| Dynamic Order Scheduling | Max-Heap (Priority Queue) | HeapifyUp / HeapifyDown | O(log n) |
| Rider Dispatch Optimization | Min-Heap | Greedy Assignment | O(log n) |
| Route Optimization | Adjacency Matrix Graph | Dijkstra's Shortest Path | O(V²) |
| Kitchen Load Analysis | Linked List Queue | Load Balancing | O(1) |

---

# Modules

### 1. 📋 Dynamic Order Scheduling
- Orders managed using a **custom Max-Heap**
- Priority levels: `Normal (1)` → `Premium (2)` → `VIP (3)`
- Same priority? → Urgent deadline processed first
- Supports: Insert, Cancel, Extract-Top

# 2. Rider Dispatch Optimization
- Riders managed using a **custom Min-Heap** (by current load)
- Assigns the least-loaded available rider automatically
- Supports: Add Rider, Assign, Complete Delivery

# 3.  Route Optimization
- City map represented as a **weighted undirected graph** (Adjacency Matrix)
- **Dijkstra's Algorithm** finds shortest delivery path
- Road blocking/unblocking supported for rerouting
- Supports: Shortest Path, Block Road, Add Location/Road

# 4.  Kitchen Load Analysis
- Each kitchen has a **linked-list Queue**
- Orders auto-distributed to least-loaded kitchen
- Detects overloaded kitchens, estimates wait time
- Supports: Distribute Order, Complete Order, View Status

---

# How to Run

### Prerequisites
- g++ compiler with C++17 support
- Any terminal (Linux / macOS / Windows via Git Bash or VS Code Terminal)

 # Compile & Run
```bash
g++ -o FoodExpress FoodExpress_Fixed.cpp -std=c++17
./FoodExpress
```

**Note:** Run in **Terminal**, not in VS Code's Output panel. Output panel is read-only.

---

# System Menu

```
============================================================
  FoodExpress Dispatch Optimization Engine
============================================================
  1. Dynamic Order Scheduling
  2. Rider Dispatch Optimization
  3. Route Optimization
  4. Kitchen Load Analysis
  0. Exit
```

---

 Complexity Analysis

| Operation | Best Case | Average Case | Worst Case |
|---|---|---|---|
| Insert Order (Heap) | O(1) | O(log n) | O(log n) |
| Extract Top Order | O(log n) | O(log n) | O(log n) |
| Cancel Order | O(n) | O(n) | O(n) |
| Assign Rider | O(1) | O(n) | O(n) |
| Dijkstra Shortest Path | O(V²) | O(V²) | O(V²) |
| Kitchen Enqueue/Dequeue | O(1) | O(1) | O(1) |

---

 File Structure

```
📦 FoodExpress
 ┣ 📄 FoodExpress_Fixed.cpp     # Complete source code
 ┗ 📄 README.md                 # Project documentation
```

---

 Implementation Rules

- ✅ All data structures implemented **from scratch**
- ✅ No STL containers (`vector`, `queue`, `stack`, `map`) used
- ✅ Custom dynamic array used instead of `std::vector`
- ✅ Custom linked-list queue for kitchen management
- ✅ Every design choice justified with complexity analysis

---

#  Course Information

| | |
|---|---|
| **University** | University of Engineering & Technology, Lahore (New Campus) |
| **Department** | Computer Science |
| **Course** | CSC-200L: Data Structures and Algorithms Lab |
| **Project Type** | Open Ended Lab (OEL) |
| **Deadline** | 7 June 2026 |

---

#  Author

**Zohaib** — BS Computer Science, UET Lahore (New Campus)
`Student ID: 2024-CS-710`

---

*"The right data structure for the right problem — that's the art of DSA."*
