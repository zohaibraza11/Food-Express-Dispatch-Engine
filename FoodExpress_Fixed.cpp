// ==============================================================================
//  FoodExpress Dispatch Optimization Engine v5.0 (Complete)
//  --------------------------------------------------------
//  Module 1 - Dynamic Order Scheduling Engine     (Max-Heap)
//  Module 2 - Kitchen Load Balancing Module        (Linked-list queues)
//  Module 3 - Rider Dispatch & Assignment Module   (Min-Heap + route distance)
//  Module 4 - Real-Time Route Optimization Module  (Dijkstra / Adjacency Matrix)
//  Module 5 - Search & Retrieval Engine            (Hash Table + filtered search)
//  Module 6 - Order History & Tracking Module      (Timeline replay + Undo stack)
//  Module 7 - Performance Analysis Module          (Benchmarking & scalability)
//  Persistent Storage                              (CSV File Handling for all data)
// ==============================================================================

#include <iostream>
#include <string>
#include <iomanip>
#include <climits>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>

using namespace std;

// ==============================================================================
//  SECTION 1: Enumerations & Helper Conversion Functions
// ==============================================================================

enum class Priority
{
    NORMAL = 1,
    PREMIUM = 2,
    VIP = 3
};

enum class OrderStatus
{
    QUEUED,
    PREPARING,
    READY,
    DISPATCHED,
    DELIVERED,
    CANCELLED
};

enum class ActionType
{
    ORDER_ADDED,
    ORDER_CANCELLED,
    ORDER_PROCESSED,
    STATUS_CHANGED
};

static string priorityToString(Priority p)
{
    switch (p)
    {
    case Priority::VIP:
        return "VIP";
    case Priority::PREMIUM:
        return "Premium";
    case Priority::NORMAL:
        return "Normal";
    default:
        return "Unknown";
    }
}

static string statusToString(OrderStatus s)
{
    switch (s)
    {
    case OrderStatus::QUEUED:
        return "Queued";
    case OrderStatus::PREPARING:
        return "Preparing";
    case OrderStatus::READY:
        return "Ready";
    case OrderStatus::DISPATCHED:
        return "Dispatched";
    case OrderStatus::DELIVERED:
        return "Delivered";
    case OrderStatus::CANCELLED:
        return "Cancelled";
    default:
        return "Unknown";
    }
}

static string actionToString(ActionType a)
{
    switch (a)
    {
    case ActionType::ORDER_ADDED:
        return "Order Added";
    case ActionType::ORDER_CANCELLED:
        return "Order Cancelled";
    case ActionType::ORDER_PROCESSED:
        return "Order Processed";
    case ActionType::STATUS_CHANGED:
        return "Status Changed";
    default:
        return "Unknown";
    }
}

// ==============================================================================
//  SECTION 2: Memory-Safe Dynamic Array (Rule-of-Five)
// ==============================================================================

template <typename T>
class Array
{
private:
    T *mData;
    int mSize;
    int mCapacity;

public:
    Array() : mData(nullptr), mSize(0), mCapacity(0) {}
    ~Array() { delete[] mData; }

    // Copy constructor
    Array(const Array &other) : mData(nullptr), mSize(other.mSize), mCapacity(other.mCapacity)
    {
        if (mCapacity > 0)
        {
            mData = new T[mCapacity];
            for (int i = 0; i < mSize; i++)
                mData[i] = other.mData[i];
        }
    }

    // Copy assignment
    Array &operator=(const Array &other)
    {
        if (this != &other)
        {
            delete[] mData;
            mSize = other.mSize;
            mCapacity = other.mCapacity;
            mData = (mCapacity > 0) ? new T[mCapacity] : nullptr;
            for (int i = 0; i < mSize; i++)
                mData[i] = other.mData[i];
        }
        return *this;
    }

    // Move constructor
    Array(Array &&other) noexcept : mData(other.mData), mSize(other.mSize), mCapacity(other.mCapacity)
    {
        other.mData = nullptr;
        other.mSize = other.mCapacity = 0;
    }

    // Move assignment
    Array &operator=(Array &&other) noexcept
    {
        if (this != &other)
        {
            delete[] mData;
            mData = other.mData;
            mSize = other.mSize;
            mCapacity = other.mCapacity;
            other.mData = nullptr;
            other.mSize = other.mCapacity = 0;
        }
        return *this;
    }

    int size() const { return mSize; }
    bool empty() const { return mSize == 0; }

    T &operator[](int i) { return mData[i]; }
    const T &operator[](int i) const { return mData[i]; }

    void push(const T &val)
    {
        if (mSize == mCapacity)
        {
            mCapacity = (mCapacity == 0) ? 4 : mCapacity * 2;
            T *newData = new T[mCapacity];
            for (int i = 0; i < mSize; i++)
                newData[i] = mData[i];
            delete[] mData;
            mData = newData;
        }
        mData[mSize++] = val;
    }

    void removeLast()
    {
        if (mSize > 0)
            mSize--;
    }

    void removeAt(int idx)
    {
        if (idx < 0 || idx >= mSize)
            return;
        for (int i = idx; i < mSize - 1; i++)
            mData[i] = mData[i + 1];
        mSize--;
    }

    void clear() { mSize = 0; }
};

// ==============================================================================
//  SECTION 3: Input & UI Helper Classes
// ==============================================================================

class InputHelper
{
public:
    static int readInt(const string &prompt, int minVal = INT_MIN, int maxVal = INT_MAX)
    {
        string line;
        while (true)
        {
            cout << prompt;
            if (!getline(cin, line))
                return minVal;
            size_t start = line.find_first_not_of(" \t");
            if (start == string::npos)
            {
                cout << "  [Error] Please enter a valid number.\n";
                continue;
            }
            try
            {
                size_t pos = 0;
                int value = stoi(line.substr(start), &pos);
                string rest = line.substr(start + pos);
                bool clean = true;
                for (size_t ci = 0; ci < rest.size(); ci++)
                    if (!isspace(static_cast<unsigned char>(rest[ci])))
                    {
                        clean = false;
                        break;
                    }
                if (!clean)
                {
                    cout << "  [Error] Please enter a valid number.\n";
                    continue;
                }
                if (value < minVal || value > maxVal)
                {
                    cout << "  [Error] Value must be between " << minVal << " and " << maxVal << ".\n";
                    continue;
                }
                return value;
            }
            catch (...)
            {
                cout << "  [Error] Please enter a valid number.\n";
            }
        }
    }

    static string readLine(const string &prompt)
    {
        string value;
        while (true)
        {
            cout << prompt;
            if (!getline(cin, value))
                return "";
            if (!value.empty())
                return value;
            cout << "  [Error] Input cannot be empty.\n";
        }
    }

    static void pause()
    {
        cout << "\n  Press Enter to continue...";
        string dummy;
        getline(cin, dummy);
    }
};

class UI
{
public:
    static void header(const string &title)
    {
        const int WIDTH = 64;
        cout << "\n  ";
        for (int i = 0; i < WIDTH; i++) cout << "=";
        cout << "\n  ||  " << left << setw(WIDTH - 5) << title << "||\n  ";
        for (int i = 0; i < WIDTH; i++) cout << "=";
        cout << "\n";
    }

    static void subHeader(const string &title)
    {
        cout << "\n  --- " << title << " ";
        int remaining = 55 - static_cast<int>(title.length());
        for (int i = 0; i < remaining && i < 55; i++)
            cout << "-";
        cout << "\n";
    }

    static void success(const string &msg) { cout << "  [+] " << msg << "\n"; }
    static void error(const string &msg)   { cout << "  [X] " << msg << "\n"; }
    static void info(const string &msg)    { cout << "  [i] " << msg << "\n"; }
    static void warning(const string &msg) { cout << "  [!] " << msg << "\n"; }
};

// ==============================================================================
//  SECTION 4: Core Entities (Order, Rider, OrderRecord)
// ==============================================================================

class Order
{
private:
    int mId;
    string mCustomerName;
    string mItem;
    string mCategory;       // customer category: "Regular", "Premium", "VIP"
    Priority mPriority;
    int mPrepTime;          // estimated preparation time in minutes
    int mDeadline;          // delivery deadline in minutes
    OrderStatus mStatus;
    int mAssignedRider;
    int mAssignedKitchen;
    bool mDelayed;          // flag for delayed orders

public:
    Order() : mId(0), mCategory("Regular"), mPriority(Priority::NORMAL), mPrepTime(10),
              mDeadline(0), mStatus(OrderStatus::QUEUED), mAssignedRider(-1),
              mAssignedKitchen(-1), mDelayed(false) {}

    Order(int id, const string &customer, const string &item, const string &category,
          Priority pri, int prepTime, int deadline)
        : mId(id), mCustomerName(customer), mItem(item), mCategory(category),
          mPriority(pri), mPrepTime(prepTime), mDeadline(deadline),
          mStatus(OrderStatus::QUEUED), mAssignedRider(-1), mAssignedKitchen(-1),
          mDelayed(false) {}

    int getId() const { return mId; }
    const string &getCustomerName() const { return mCustomerName; }
    const string &getItem() const { return mItem; }
    const string &getCategory() const { return mCategory; }
    Priority getPriority() const { return mPriority; }
    int getPrepTime() const { return mPrepTime; }
    int getDeadline() const { return mDeadline; }
    OrderStatus getStatus() const { return mStatus; }
    int getAssignedRider() const { return mAssignedRider; }
    int getAssignedKitchen() const { return mAssignedKitchen; }
    bool isValid() const { return mId != 0; }
    bool isDelayed() const { return mDelayed; }

    void setStatus(OrderStatus s) { mStatus = s; }
    void setPriority(Priority p) { mPriority = p; }
    void setAssignedRider(int rid) { mAssignedRider = rid; }
    void setAssignedKitchen(int kid) { mAssignedKitchen = kid; }
    void setDelayed(bool d) { mDelayed = d; }
    void setPrepTime(int t) { mPrepTime = t; }
};

class Rider
{
private:
    int mId;
    string mName;
    int mCurrentLoad;
    int mCapacity;
    int mLocationNode;
    bool mAvailable;
    int mTotalDeliveries;

public:
    Rider() : mId(0), mCurrentLoad(0), mCapacity(3), mLocationNode(0),
              mAvailable(true), mTotalDeliveries(0) {}

    Rider(int id, const string &name, int capacity, int location)
        : mId(id), mName(name), mCurrentLoad(0), mCapacity(capacity),
          mLocationNode(location), mAvailable(true), mTotalDeliveries(0) {}

    int getId() const { return mId; }
    const string &getName() const { return mName; }
    int getCurrentLoad() const { return mCurrentLoad; }
    int getCapacity() const { return mCapacity; }
    int getLocationNode() const { return mLocationNode; }
    bool isAvailable() const { return mAvailable; }
    int getTotalDeliveries() const { return mTotalDeliveries; }
    bool canTakeOrder() const { return mAvailable && mCurrentLoad < mCapacity; }

    void setAvailable(bool a) { mAvailable = a; }
    void setLocationNode(int loc) { mLocationNode = loc; }
    void incrementLoad() { mCurrentLoad++; }
    bool decrementLoad()
    {
        if (mCurrentLoad <= 0)
            return false;
        mCurrentLoad--;
        mTotalDeliveries++;
        return true;
    }

    friend class RiderHeap;
};

class OrderRecord
{
private:
    int mOrderId;
    string mCustomerName;
    string mItem;
    string mCategory;
    Priority mPriority;
    OrderStatus mFinalStatus;
    string mKitchenName;
    string mRiderName;
    int mRouteDistance;
    int mEstimatedETA;

public:
    OrderRecord() : mOrderId(0), mPriority(Priority::NORMAL),
                    mFinalStatus(OrderStatus::QUEUED), mRouteDistance(-1), mEstimatedETA(-1) {}

    OrderRecord(const Order &o, const string &kitchen = "", const string &rider = "",
                int dist = -1, int eta = -1)
        : mOrderId(o.getId()), mCustomerName(o.getCustomerName()), mItem(o.getItem()),
          mCategory(o.getCategory()), mPriority(o.getPriority()),
          mFinalStatus(o.getStatus()), mKitchenName(kitchen), mRiderName(rider),
          mRouteDistance(dist), mEstimatedETA(eta) {}

    // Constructor for loading from file
    OrderRecord(int id, const string &cust, const string &item, const string &cat,
                Priority pri, OrderStatus stat, const string &kitchen, const string &rider,
                int dist, int eta)
        : mOrderId(id), mCustomerName(cust), mItem(item), mCategory(cat),
          mPriority(pri), mFinalStatus(stat), mKitchenName(kitchen), mRiderName(rider),
          mRouteDistance(dist), mEstimatedETA(eta) {}

    int getId() const { return mOrderId; }
    const string &getCustomerName() const { return mCustomerName; }
    const string &getItem() const { return mItem; }
    const string &getCategory() const { return mCategory; }
    Priority getPriority() const { return mPriority; }
    OrderStatus getFinalStatus() const { return mFinalStatus; }
    const string &getKitchenName() const { return mKitchenName; }
    const string &getRiderName() const { return mRiderName; }
    int getRouteDistance() const { return mRouteDistance; }
    int getEstimatedETA() const { return mEstimatedETA; }

    void display() const
    {
        cout << "  " << left << setw(7) << mOrderId << setw(14) << mCustomerName
             << setw(14) << mItem << setw(10) << mCategory
             << setw(10) << priorityToString(mPriority)
             << setw(12) << statusToString(mFinalStatus)
             << setw(16) << (mKitchenName.empty() ? "---" : mKitchenName)
             << setw(12) << (mRiderName.empty() ? "---" : mRiderName)
             << setw(8) << (mRouteDistance >= 0 ? to_string(mRouteDistance) : "---")
             << setw(8) << (mEstimatedETA >= 0 ? (to_string(mEstimatedETA) + "m") : "---")
             << "\n";
    }
};

// ==============================================================================
//  SECTION 5: Primary Data Structures
// ==============================================================================

// --- MODULE 1: Dynamic Order Scheduling Engine (Max-Heap) ---

class OrderHeap
{
private:
    Array<Order> mHeap;

    int parent(int i) const { return (i - 1) / 2; }
    int leftChild(int i) const { return 2 * i + 1; }
    int rightChild(int i) const { return 2 * i + 2; }

    // VIP > Premium > Normal; if equal priority, earlier deadline comes first
    bool higherPriority(const Order &a, const Order &b) const
    {
        if (static_cast<int>(a.getPriority()) != static_cast<int>(b.getPriority()))
            return static_cast<int>(a.getPriority()) > static_cast<int>(b.getPriority());
        return a.getDeadline() < b.getDeadline();
    }

    void swapAt(int i, int j)
    {
        Order tmp = mHeap[i];
        mHeap[i] = mHeap[j];
        mHeap[j] = tmp;
    }

    void siftUp(int i)
    {
        while (i > 0 && higherPriority(mHeap[i], mHeap[parent(i)]))
        {
            swapAt(i, parent(i));
            i = parent(i);
        }
    }

    void siftDown(int i)
    {
        int n = mHeap.size();
        while (true)
        {
            int best = i, l = leftChild(i), r = rightChild(i);
            if (l < n && higherPriority(mHeap[l], mHeap[best]))
                best = l;
            if (r < n && higherPriority(mHeap[r], mHeap[best]))
                best = r;
            if (best == i)
                break;
            swapAt(i, best);
            i = best;
        }
    }

public:
    // Insert a new order into the scheduling queue
    void insert(const Order &o)
    {
        mHeap.push(o);
        siftUp(mHeap.size() - 1);
    }

    // Extract the highest-priority order
    Order extractTop()
    {
        if (mHeap.empty())
            return Order();
        Order top = mHeap[0];
        mHeap[0] = mHeap[mHeap.size() - 1];
        mHeap.removeLast();
        if (!mHeap.empty())
            siftDown(0);
        return top;
    }

    // Cancel an order by ID
    bool cancelOrder(int orderId)
    {
        for (int i = 0; i < mHeap.size(); i++)
        {
            if (mHeap[i].getId() == orderId)
            {
                mHeap[i] = mHeap[mHeap.size() - 1];
                mHeap.removeLast();
                if (i < mHeap.size())
                {
                    siftUp(i);
                    siftDown(i);
                }
                return true;
            }
        }
        return false;
    }

    // Find an order by ID (returns pointer or nullptr)
    Order *findOrder(int orderId)
    {
        for (int i = 0; i < mHeap.size(); i++)
            if (mHeap[i].getId() == orderId)
                return &mHeap[i];
        return nullptr;
    }

    // Update priority of an existing order and re-heapify
    bool updatePriority(int orderId, Priority newPri)
    {
        for (int i = 0; i < mHeap.size(); i++)
        {
            if (mHeap[i].getId() == orderId)
            {
                mHeap[i].setPriority(newPri);
                siftUp(i);
                siftDown(i);
                return true;
            }
        }
        return false;
    }

    // Mark an order as delayed
    bool markDelayed(int orderId)
    {
        for (int i = 0; i < mHeap.size(); i++)
        {
            if (mHeap[i].getId() == orderId)
            {
                mHeap[i].setDelayed(true);
                return true;
            }
        }
        return false;
    }

    // Get all delayed orders for review
    Array<Order> getDelayedOrders() const
    {
        Array<Order> result;
        for (int i = 0; i < mHeap.size(); i++)
            if (mHeap[i].isDelayed())
                result.push(mHeap[i]);
        return result;
    }

    // Peek at the top without removing
    Order peekTop() const
    {
        if (mHeap.empty())
            return Order();
        return mHeap[0];
    }

    int getSize() const { return mHeap.size(); }
    bool isEmpty() const { return mHeap.empty(); }

    void display() const
    {
        if (mHeap.empty())
        {
            cout << "  (order queue is empty)\n";
            return;
        }
        cout << "  " << left << setw(7) << "ID" << setw(16) << "Customer"
             << setw(16) << "Item" << setw(10) << "Category"
             << setw(10) << "Priority" << setw(10) << "Deadline"
             << setw(10) << "PrepTime" << setw(10) << "Status"
             << setw(8) << "Delayed" << "\n";
        cout << "  " << string(95, '-') << "\n";
        for (int i = 0; i < mHeap.size(); i++)
        {
            const Order &o = mHeap[i];
            cout << "  " << left << setw(7) << o.getId()
                 << setw(16) << o.getCustomerName()
                 << setw(16) << o.getItem()
                 << setw(10) << o.getCategory()
                 << setw(10) << priorityToString(o.getPriority())
                 << setw(10) << (to_string(o.getDeadline()) + "m")
                 << setw(10) << (to_string(o.getPrepTime()) + "m")
                 << setw(10) << statusToString(o.getStatus())
                 << setw(8) << (o.isDelayed() ? "Yes" : "No")
                 << "\n";
        }
    }

    // Return all orders (for file saving)
    Array<Order> getAllOrders() const
    {
        Array<Order> result;
        for (int i = 0; i < mHeap.size(); i++)
            result.push(mHeap[i]);
        return result;
    }
};

// --- MODULE 4: Real-Time Route Optimization (Adjacency Matrix + Dijkstra) ---

class CityGraph
{
public:
    static const int MAX_NODES = 20;

private:
    static const int INF = INT_MAX / 2;
    int mNodeCount;
    string mNames[MAX_NODES];
    int mAdj[MAX_NODES][MAX_NODES];
    bool mBlocked[MAX_NODES][MAX_NODES];

public:
    CityGraph() : mNodeCount(0)
    {
        for (int i = 0; i < MAX_NODES; i++)
            for (int j = 0; j < MAX_NODES; j++)
            {
                mAdj[i][j] = (i == j) ? 0 : INF;
                mBlocked[i][j] = false;
            }
    }

    int getNodeCount() const { return mNodeCount; }
    bool isValidNode(int n) const { return n >= 0 && n < mNodeCount; }

    const string &getLocationName(int idx) const
    {
        static const string unknown = "???";
        return (idx >= 0 && idx < mNodeCount) ? mNames[idx] : unknown;
    }

    bool addLocation(const string &name)
    {
        if (mNodeCount >= MAX_NODES)
            return false;
        mNames[mNodeCount++] = name;
        return true;
    }

    bool addRoad(int u, int v, int weight)
    {
        if (!isValidNode(u) || !isValidNode(v) || weight <= 0)
            return false;
        mAdj[u][v] = mAdj[v][u] = weight;
        return true;
    }

    bool blockRoad(int u, int v)
    {
        if (!isValidNode(u) || !isValidNode(v))
            return false;
        mBlocked[u][v] = mBlocked[v][u] = true;
        return true;
    }

    bool unblockRoad(int u, int v)
    {
        if (!isValidNode(u) || !isValidNode(v))
            return false;
        mBlocked[u][v] = mBlocked[v][u] = false;
        return true;
    }

    bool isBlocked(int u, int v) const
    {
        if (!isValidNode(u) || !isValidNode(v))
            return false;
        return mBlocked[u][v];
    }

    int getRoadWeight(int u, int v) const
    {
        if (!isValidNode(u) || !isValidNode(v))
            return -1;
        return (mAdj[u][v] >= INF) ? -1 : mAdj[u][v];
    }

    // Dijkstra's algorithm: returns shortest distance and fills path array
    int findShortestPath(int src, int dest, Array<int> &path) const
    {
        if (!isValidNode(src) || !isValidNode(dest))
            return -1;

        int dist[MAX_NODES], prev[MAX_NODES];
        bool visited[MAX_NODES];
        for (int i = 0; i < mNodeCount; i++)
        {
            dist[i] = INF;
            prev[i] = -1;
            visited[i] = false;
        }
        dist[src] = 0;

        for (int iter = 0; iter < mNodeCount; iter++)
        {
            int u = -1;
            for (int i = 0; i < mNodeCount; i++)
                if (!visited[i] && (u == -1 || dist[i] < dist[u]))
                    u = i;
            if (u == -1 || dist[u] >= INF)
                break;
            visited[u] = true;
            for (int v = 0; v < mNodeCount; v++)
            {
                if (mAdj[u][v] < INF && !mBlocked[u][v] && !visited[v])
                {
                    int nd = dist[u] + mAdj[u][v];
                    if (nd < dist[v])
                    {
                        dist[v] = nd;
                        prev[v] = u;
                    }
                }
            }
        }

        if (dist[dest] >= INF)
            return -1;

        // Reconstruct path
        Array<int> rev;
        for (int at = dest; at != -1; at = prev[at])
            rev.push(at);
        for (int i = rev.size() - 1; i >= 0; i--)
            path.push(rev[i]);
        return dist[dest];
    }

    // Shortest distance only (no path reconstruction)
    int shortestDistance(int src, int dest) const
    {
        if (!isValidNode(src) || !isValidNode(dest))
            return -1;

        int dist[MAX_NODES];
        bool visited[MAX_NODES];
        for (int i = 0; i < mNodeCount; i++)
        {
            dist[i] = INF;
            visited[i] = false;
        }
        dist[src] = 0;

        for (int iter = 0; iter < mNodeCount; iter++)
        {
            int u = -1;
            for (int i = 0; i < mNodeCount; i++)
                if (!visited[i] && (u == -1 || dist[i] < dist[u]))
                    u = i;
            if (u == -1 || dist[u] >= INF)
                break;
            visited[u] = true;
            for (int v = 0; v < mNodeCount; v++)
            {
                if (mAdj[u][v] < INF && !mBlocked[u][v] && !visited[v])
                {
                    if (dist[u] + mAdj[u][v] < dist[v])
                        dist[v] = dist[u] + mAdj[u][v];
                }
            }
        }
        return (dist[dest] >= INF) ? -1 : dist[dest];
    }

    // Compare two routes (returns which destination is closer from src)
    void compareRoutes(int src, int dest1, int dest2) const
    {
        Array<int> path1, path2;
        int d1 = findShortestPath(src, dest1, path1);
        int d2 = findShortestPath(src, dest2, path2);

        UI::subHeader("Route Comparison");
        cout << "  From: " << getLocationName(src) << "\n\n";

        cout << "  Route A -> " << getLocationName(dest1) << ": ";
        if (d1 >= 0)
        {
            cout << d1 << " units  [";
            for (int i = 0; i < path1.size(); i++)
            {
                if (i > 0)
                    cout << " -> ";
                cout << getLocationName(path1[i]);
            }
            cout << "]\n";
        }
        else
            cout << "UNREACHABLE\n";

        cout << "  Route B -> " << getLocationName(dest2) << ": ";
        if (d2 >= 0)
        {
            cout << d2 << " units  [";
            for (int i = 0; i < path2.size(); i++)
            {
                if (i > 0)
                    cout << " -> ";
                cout << getLocationName(path2[i]);
            }
            cout << "]\n";
        }
        else
            cout << "UNREACHABLE\n";

        if (d1 >= 0 && d2 >= 0)
        {
            if (d1 < d2)
                cout << "\n  => Route A is shorter by " << (d2 - d1) << " units.\n";
            else if (d2 < d1)
                cout << "\n  => Route B is shorter by " << (d1 - d2) << " units.\n";
            else
                cout << "\n  => Both routes are equal distance.\n";
        }
    }

    // Estimate delivery cost based on distance (rate = cost per unit distance)
    int estimateDeliveryCost(int src, int dest, int ratePerUnit = 5) const
    {
        int dist = shortestDistance(src, dest);
        if (dist < 0)
            return -1;
        return dist * ratePerUnit;
    }

    void displayMap() const
    {
        cout << "\n  Locations:\n";
        for (int i = 0; i < mNodeCount; i++)
            cout << "    [" << i << "] " << mNames[i] << "\n";

        cout << "\n  Adjacency Matrix (--- = no road, X = blocked):\n        ";
        for (int i = 0; i < mNodeCount; i++)
            cout << left << setw(8) << i;
        cout << "\n";
        for (int i = 0; i < mNodeCount; i++)
        {
            cout << "  " << left << setw(5) << i;
            for (int j = 0; j < mNodeCount; j++)
            {
                if (mAdj[i][j] >= INF)
                    cout << setw(8) << "---";
                else
                {
                    string val = to_string(mAdj[i][j]);
                    if (mBlocked[i][j])
                        val += "X";
                    cout << setw(8) << val;
                }
            }
            cout << "\n";
        }
    }
};

// --- Hash Map (Chaining, for Search & Retrieval) ---

template <typename V>
class HashMap
{
private:
    struct Node
    {
        int key;
        V value;
        Node *next;
        Node(int k, const V &v, Node *n = nullptr) : key(k), value(v), next(n) {}
    };

    static const int BUCKET_COUNT = 127;
    Node *mBuckets[BUCKET_COUNT];
    int mCount;

    int hashFunc(int key) const
    {
        return ((key % BUCKET_COUNT) + BUCKET_COUNT) % BUCKET_COUNT;
    }

public:
    HashMap() : mCount(0)
    {
        for (int i = 0; i < BUCKET_COUNT; i++)
            mBuckets[i] = nullptr;
    }

    ~HashMap() { clear(); }
    HashMap(const HashMap &) = delete;
    HashMap &operator=(const HashMap &) = delete;

    int getCount() const { return mCount; }

    void insert(int key, const V &value)
    {
        int idx = hashFunc(key);
        Node *cur = mBuckets[idx];
        while (cur)
        {
            if (cur->key == key)
            {
                cur->value = value;
                return;
            }
            cur = cur->next;
        }
        mBuckets[idx] = new Node(key, value, mBuckets[idx]);
        mCount++;
    }

    V *find(int key)
    {
        int idx = hashFunc(key);
        Node *cur = mBuckets[idx];
        while (cur)
        {
            if (cur->key == key)
                return &cur->value;
            cur = cur->next;
        }
        return nullptr;
    }

    const V *find(int key) const
    {
        int idx = hashFunc(key);
        Node *cur = mBuckets[idx];
        while (cur)
        {
            if (cur->key == key)
                return &cur->value;
            cur = cur->next;
        }
        return nullptr;
    }

    bool remove(int key)
    {
        int idx = hashFunc(key);
        Node *prev = nullptr;
        Node *cur = mBuckets[idx];
        while (cur)
        {
            if (cur->key == key)
            {
                if (prev)
                    prev->next = cur->next;
                else
                    mBuckets[idx] = cur->next;
                delete cur;
                mCount--;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    Array<V> getAllValues() const
    {
        Array<V> result;
        for (int i = 0; i < BUCKET_COUNT; i++)
        {
            Node *cur = mBuckets[i];
            while (cur)
            {
                result.push(cur->value);
                cur = cur->next;
            }
        }
        return result;
    }

    void clear()
    {
        for (int i = 0; i < BUCKET_COUNT; i++)
        {
            Node *cur = mBuckets[i];
            while (cur)
            {
                Node *next = cur->next;
                delete cur;
                cur = next;
            }
            mBuckets[i] = nullptr;
        }
        mCount = 0;
    }
};

// --- MODULE 3: Rider Dispatch & Assignment (Min-Heap by load) ---

class RiderHeap
{
private:
    Array<Rider> mHeap;

    int parent(int i) const { return (i - 1) / 2; }
    int leftChild(int i) const { return 2 * i + 1; }
    int rightChild(int i) const { return 2 * i + 2; }

    void swapAt(int i, int j)
    {
        Rider tmp = mHeap[i];
        mHeap[i] = mHeap[j];
        mHeap[j] = tmp;
    }

    void siftUp(int i)
    {
        while (i > 0 && mHeap[i].mCurrentLoad < mHeap[parent(i)].mCurrentLoad)
        {
            swapAt(i, parent(i));
            i = parent(i);
        }
    }

    void siftDown(int i)
    {
        int n = mHeap.size();
        while (true)
        {
            int best = i, l = leftChild(i), r = rightChild(i);
            if (l < n && mHeap[l].mCurrentLoad < mHeap[best].mCurrentLoad)
                best = l;
            if (r < n && mHeap[r].mCurrentLoad < mHeap[best].mCurrentLoad)
                best = r;
            if (best == i)
                break;
            swapAt(i, best);
            i = best;
        }
    }

public:
    void addRider(const Rider &r)
    {
        mHeap.push(r);
        siftUp(mHeap.size() - 1);
    }

    // Basic assignment: pick the rider with lowest load who can take an order
    Rider assignBestRider()
    {
        if (mHeap.empty())
            return Rider();
        int bestIdx = -1;
        for (int i = 0; i < mHeap.size(); i++)
        {
            if (mHeap[i].canTakeOrder())
            {
                bestIdx = i;
                break;
            }
        }
        if (bestIdx == -1)
            return Rider();
        mHeap[bestIdx].incrementLoad();
        Rider assigned = mHeap[bestIdx];
        siftDown(bestIdx);
        return assigned;
    }

    // Advanced assignment: considers route distance to destination for minimal delay
    Rider assignBestRiderWithRoute(const CityGraph &city, int destNode)
    {
        if (mHeap.empty())
            return Rider();

        int bestIdx = -1;
        int bestScore = INT_MAX;

        for (int i = 0; i < mHeap.size(); i++)
        {
            if (!mHeap[i].canTakeOrder())
                continue;

            int dist = city.shortestDistance(mHeap[i].getLocationNode(), destNode);
            if (dist < 0)
                dist = 9999; // unreachable penalty

            // Score = current_load * 100 + route_distance (balances load and distance)
            int score = mHeap[i].getCurrentLoad() * 100 + dist;
            if (bestIdx == -1 || score < bestScore)
            {
                bestIdx = i;
                bestScore = score;
            }
        }

        if (bestIdx == -1)
            return Rider();

        mHeap[bestIdx].incrementLoad();
        Rider assigned = mHeap[bestIdx];
        siftDown(bestIdx);
        return assigned;
    }

    // Complete a delivery for a rider (decrement load)
    bool completeDelivery(int riderId)
    {
        for (int i = 0; i < mHeap.size(); i++)
        {
            if (mHeap[i].getId() == riderId)
            {
                if (!mHeap[i].decrementLoad())
                    return false;
                siftUp(i);
                return true;
            }
        }
        return false;
    }

    // Toggle rider availability
    bool toggleAvailability(int riderId)
    {
        for (int i = 0; i < mHeap.size(); i++)
        {
            if (mHeap[i].getId() == riderId)
            {
                mHeap[i].setAvailable(!mHeap[i].isAvailable());
                return true;
            }
        }
        return false;
    }

    int getSize() const { return mHeap.size(); }

    void display() const
    {
        if (mHeap.empty())
        {
            cout << "  (no riders registered)\n";
            return;
        }
        cout << "  " << left << setw(6) << "ID" << setw(16) << "Name"
             << setw(10) << "Load" << setw(10) << "Capacity"
             << setw(12) << "Available" << setw(12) << "Location"
             << setw(12) << "Delivered" << "\n";
        cout << "  " << string(78, '-') << "\n";
        for (int i = 0; i < mHeap.size(); i++)
        {
            const Rider &r = mHeap[i];
            cout << "  " << left << setw(6) << r.getId()
                 << setw(16) << r.getName()
                 << setw(10) << (to_string(r.getCurrentLoad()) + "/" + to_string(r.getCapacity()))
                 << setw(10) << r.getCapacity()
                 << setw(12) << (r.isAvailable() ? "Yes" : "No")
                 << setw(12) << ("Node " + to_string(r.getLocationNode()))
                 << setw(12) << r.getTotalDeliveries() << "\n";
        }
    }

    Array<Rider> getAllRiders() const
    {
        Array<Rider> result;
        for (int i = 0; i < mHeap.size(); i++)
            result.push(mHeap[i]);
        return result;
    }
};

// --- MODULE 2: Kitchen Load Balancing (Linked-List Queues) ---

class KitchenQueue
{
private:
    struct Node
    {
        int orderId;
        string item;
        int prepTime;
        Node *next;
        Node(int id, const string &it, int pt) : orderId(id), item(it), prepTime(pt), next(nullptr) {}
    };

    Node *mFront;
    Node *mRear;
    int mSize;
    int mMaxCapacity;

    void clear()
    {
        while (mFront)
        {
            Node *t = mFront;
            mFront = mFront->next;
            delete t;
        }
        mFront = mRear = nullptr;
        mSize = 0;
    }

    void copyFrom(const KitchenQueue &other)
    {
        mMaxCapacity = other.mMaxCapacity;
        Node *cur = other.mFront;
        while (cur)
        {
            enqueue(cur->orderId, cur->item, cur->prepTime);
            cur = cur->next;
        }
    }

public:
    KitchenQueue() : mFront(nullptr), mRear(nullptr), mSize(0), mMaxCapacity(5) {}
    ~KitchenQueue() { clear(); }

    KitchenQueue(const KitchenQueue &other) : mFront(nullptr), mRear(nullptr), mSize(0), mMaxCapacity(5)
    {
        copyFrom(other);
    }

    KitchenQueue &operator=(const KitchenQueue &other)
    {
        if (this != &other)
        {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    int getSize() const { return mSize; }
    int getMaxCapacity() const { return mMaxCapacity; }
    bool isFull() const { return mSize >= mMaxCapacity; }
    bool isEmpty() const { return mSize == 0; }
    void setMaxCapacity(int cap) { mMaxCapacity = cap; }

    int totalWaitTime() const
    {
        int t = 0;
        Node *c = mFront;
        while (c)
        {
            t += c->prepTime;
            c = c->next;
        }
        return t;
    }

    void enqueue(int orderId, const string &item, int prepTime)
    {
        Node *n = new Node(orderId, item, prepTime);
        if (!mRear)
            mFront = mRear = n;
        else
        {
            mRear->next = n;
            mRear = n;
        }
        mSize++;
    }

    int dequeue()
    {
        if (!mFront)
            return -1;
        int id = mFront->orderId;
        Node *t = mFront;
        mFront = mFront->next;
        if (!mFront)
            mRear = nullptr;
        delete t;
        mSize--;
        return id;
    }

    // Peek at front order ID
    int peekFront() const
    {
        return mFront ? mFront->orderId : -1;
    }

    void display(const string &kitchenName) const
    {
        string bar = "[";
        for (int i = 0; i < mMaxCapacity; i++)
            bar += (i < mSize) ? "#" : ".";
        bar += "]";
        cout << "  " << left << setw(18) << kitchenName << " " << bar
             << "  " << mSize << "/" << mMaxCapacity
             << "  Wait: " << setw(4) << (to_string(totalWaitTime()) + "m")
             << (isFull() ? "  ** OVERLOADED **" : "") << "\n";

        Node *c = mFront;
        int p = 1;
        while (c)
        {
            cout << "      " << p++ << ". Order #" << c->orderId
                 << "  " << left << setw(16) << c->item
                 << " (" << c->prepTime << " min)\n";
            c = c->next;
        }
    }
};

class KitchenSystem
{
private:
    static const int MAX_KITCHENS = 10;
    KitchenQueue mKitchens[MAX_KITCHENS];
    string mNames[MAX_KITCHENS];
    int mCount;

public:
    KitchenSystem() : mCount(0) {}

    int getNumKitchens() const { return mCount; }

    const string &getKitchenName(int idx) const
    {
        static const string unknown = "???";
        return (idx >= 0 && idx < mCount) ? mNames[idx] : unknown;
    }

    bool isValidKitchen(int idx) const { return idx >= 0 && idx < mCount; }

    bool addKitchen(const string &name, int cap)
    {
        if (mCount >= MAX_KITCHENS)
            return false;
        mNames[mCount] = name;
        mKitchens[mCount].setMaxCapacity(cap);
        mCount++;
        return true;
    }

    // Distribute order to kitchen with lowest load (load balancing)
    int distributeOrder(int orderId, const string &item, int prepTime)
    {
        int best = -1;
        for (int i = 0; i < mCount; i++)
            if (!mKitchens[i].isFull())
                if (best == -1 || mKitchens[i].getSize() < mKitchens[best].getSize())
                    best = i;
        if (best == -1)
            return -1;
        mKitchens[best].enqueue(orderId, item, prepTime);
        return best;
    }

    // Complete the front order in a kitchen
    int completeOrder(int kitchenIdx)
    {
        if (!isValidKitchen(kitchenIdx) || mKitchens[kitchenIdx].isEmpty())
            return -1;
        return mKitchens[kitchenIdx].dequeue();
    }

    int getKitchenWaitTime(int idx) const
    {
        return isValidKitchen(idx) ? mKitchens[idx].totalWaitTime() : 0;
    }

    int getKitchenLoad(int idx) const
    {
        return isValidKitchen(idx) ? mKitchens[idx].getSize() : 0;
    }

    // Detect overloaded kitchens
    void detectOverloaded() const
    {
        bool found = false;
        for (int i = 0; i < mCount; i++)
        {
            if (mKitchens[i].isFull())
            {
                if (!found)
                {
                    UI::subHeader("Overloaded Kitchens Detected");
                    found = true;
                }
                cout << "  [!!] " << mNames[i] << " is at full capacity ("
                     << mKitchens[i].getSize() << "/" << mKitchens[i].getMaxCapacity()
                     << "), wait time: " << mKitchens[i].totalWaitTime() << " min\n";
            }
        }
        if (!found)
            UI::success("No kitchens are overloaded.");
    }

    // Rebalance: move an order from the most loaded kitchen to the least loaded
    bool rebalance()
    {
        if (mCount < 2)
            return false;

        int maxIdx = 0, minIdx = 0;
        for (int i = 1; i < mCount; i++)
        {
            if (mKitchens[i].getSize() > mKitchens[maxIdx].getSize())
                maxIdx = i;
            if (mKitchens[i].getSize() < mKitchens[minIdx].getSize())
                minIdx = i;
        }

        if (maxIdx == minIdx || mKitchens[maxIdx].getSize() - mKitchens[minIdx].getSize() <= 1)
            return false;

        if (mKitchens[minIdx].isFull())
            return false;

        int orderId = mKitchens[maxIdx].dequeue();
        if (orderId < 0)
            return false;

        // Re-enqueue with a default prep time estimate
        mKitchens[minIdx].enqueue(orderId, "Rebalanced", 10);
        return true;
    }

    // Estimate wait time for a new order (finds best kitchen)
    int estimateWaitTime() const
    {
        int minWait = INT_MAX;
        for (int i = 0; i < mCount; i++)
        {
            if (!mKitchens[i].isFull())
            {
                int w = mKitchens[i].totalWaitTime();
                if (w < minWait)
                    minWait = w;
            }
        }
        return (minWait == INT_MAX) ? -1 : minWait;
    }

    void displayAll() const
    {
        if (mCount == 0)
        {
            cout << "  (no kitchens configured)\n";
            return;
        }
        for (int i = 0; i < mCount; i++)
        {
            cout << "  [" << i << "] ";
            mKitchens[i].display(mNames[i]);
        }
    }
};

// ==============================================================================
//  SECTION 6: Search & Retrieval Engine (MODULE 5)
// ==============================================================================

class SearchEngine
{
private:
    HashMap<OrderRecord> mIndex;
    Array<OrderRecord> mAllRecords;

    static bool containsIgnoreCase(const string &haystack, const string &needle)
    {
        if (needle.empty())
            return true;
        if (needle.size() > haystack.size())
            return false;
        for (size_t i = 0; i <= haystack.size() - needle.size(); i++)
        {
            bool match = true;
            for (size_t j = 0; j < needle.size(); j++)
            {
                if (tolower(static_cast<unsigned char>(haystack[i + j])) !=
                    tolower(static_cast<unsigned char>(needle[j])))
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return true;
        }
        return false;
    }

public:
    void indexOrder(const OrderRecord &record)
    {
        mIndex.insert(record.getId(), record);
        mAllRecords.push(record);
    }

    int getCount() const { return mIndex.getCount(); }

    OrderRecord *findById(int orderId)
    {
        return mIndex.find(orderId);
    }

    // Search by customer name (partial, case-insensitive)
    Array<OrderRecord> searchByCustomer(const string &name) const
    {
        Array<OrderRecord> results;
        for (int i = 0; i < mAllRecords.size(); i++)
            if (containsIgnoreCase(mAllRecords[i].getCustomerName(), name))
                results.push(mAllRecords[i]);
        return results;
    }

    // Search by item name
    Array<OrderRecord> searchByItem(const string &item) const
    {
        Array<OrderRecord> results;
        for (int i = 0; i < mAllRecords.size(); i++)
            if (containsIgnoreCase(mAllRecords[i].getItem(), item))
                results.push(mAllRecords[i]);
        return results;
    }

    // Search by order status
    Array<OrderRecord> searchByStatus(OrderStatus status) const
    {
        Array<OrderRecord> results;
        for (int i = 0; i < mAllRecords.size(); i++)
            if (mAllRecords[i].getFinalStatus() == status)
                results.push(mAllRecords[i]);
        return results;
    }

    // Search by priority
    Array<OrderRecord> searchByPriority(Priority priority) const
    {
        Array<OrderRecord> results;
        for (int i = 0; i < mAllRecords.size(); i++)
            if (mAllRecords[i].getPriority() == priority)
                results.push(mAllRecords[i]);
        return results;
    }

    // Search by customer category
    Array<OrderRecord> searchByCategory(const string &cat) const
    {
        Array<OrderRecord> results;
        for (int i = 0; i < mAllRecords.size(); i++)
            if (containsIgnoreCase(mAllRecords[i].getCategory(), cat))
                results.push(mAllRecords[i]);
        return results;
    }

    // Search by kitchen name
    Array<OrderRecord> searchByKitchen(const string &kitchen) const
    {
        Array<OrderRecord> results;
        for (int i = 0; i < mAllRecords.size(); i++)
            if (containsIgnoreCase(mAllRecords[i].getKitchenName(), kitchen))
                results.push(mAllRecords[i]);
        return results;
    }

    // Search by rider name
    Array<OrderRecord> searchByRider(const string &rider) const
    {
        Array<OrderRecord> results;
        for (int i = 0; i < mAllRecords.size(); i++)
            if (containsIgnoreCase(mAllRecords[i].getRiderName(), rider))
                results.push(mAllRecords[i]);
        return results;
    }

    // Get all records
    const Array<OrderRecord> &getAllRecords() const { return mAllRecords; }

    static void displayResultsHeader()
    {
        cout << "  " << left << setw(7) << "ID" << setw(14) << "Customer"
             << setw(14) << "Item" << setw(10) << "Category"
             << setw(10) << "Priority" << setw(12) << "Status"
             << setw(16) << "Kitchen" << setw(12) << "Rider"
             << setw(8) << "Dist" << setw(8) << "ETA" << "\n";
        cout << "  " << string(109, '-') << "\n";
    }

    static void displayResults(const Array<OrderRecord> &results)
    {
        if (results.empty())
        {
            cout << "  No matching records found.\n";
            return;
        }
        displayResultsHeader();
        for (int i = 0; i < results.size(); i++)
            results[i].display();
        cout << "\n  Found " << results.size() << " record(s).\n";
    }

    void displayAll() const
    {
        displayResults(mAllRecords);
    }
};

// ==============================================================================
//  SECTION 7: Order Tracking & Undo Engine (MODULE 6)
// ==============================================================================

class StateTransition
{
public:
    OrderStatus fromState;
    OrderStatus toState;
    int timestamp;
    string description;

    StateTransition() : fromState(OrderStatus::QUEUED), toState(OrderStatus::QUEUED), timestamp(0) {}
    StateTransition(OrderStatus from, OrderStatus to, int ts, const string &desc)
        : fromState(from), toState(to), timestamp(ts), description(desc) {}
};

class OrderTimeline
{
public:
    int orderId;
    string customerName;
    string item;
    Array<StateTransition> transitions;

    OrderTimeline() : orderId(0) {}
    OrderTimeline(int id, const string &cust, const string &it)
        : orderId(id), customerName(cust), item(it) {}

    void addTransition(const StateTransition &t)
    {
        transitions.push(t);
    }

    OrderStatus currentState() const
    {
        return transitions.empty() ? OrderStatus::QUEUED : transitions[transitions.size() - 1].toState;
    }

    // Replay the full timeline of this order
    void replay() const
    {
        cout << "\n  Order #" << orderId << " | " << customerName << " | " << item
             << "\n  " << string(65, '-') << "\n";
        if (transitions.empty())
        {
            cout << "  (no state transitions recorded)\n";
            return;
        }
        for (int i = 0; i < transitions.size(); i++)
        {
            const StateTransition &t = transitions[i];
            cout << "  [T=" << left << setw(4) << t.timestamp << "]  "
                 << setw(12) << statusToString(t.fromState) << " --> "
                 << setw(12) << statusToString(t.toState)
                 << " : " << t.description << "\n";
        }
        cout << "  " << string(65, '-') << "\n";
        cout << "  Current State: " << statusToString(currentState()) << "\n";
    }
};

class UndoAction
{
public:
    ActionType type;
    Order snapshot;
    string description;
    int timestamp;

    UndoAction() : type(ActionType::ORDER_ADDED), timestamp(0) {}
    UndoAction(ActionType t, const Order &snap, const string &desc, int ts)
        : type(t), snapshot(snap), description(desc), timestamp(ts) {}
};

class OrderTracker
{
private:
    HashMap<OrderTimeline> mTimelines;
    Array<UndoAction> mUndoStack;
    int mClock;

public:
    OrderTracker() : mClock(0) {}
    int getClock() const { return mClock; }

    void recordOrderCreated(const Order &order)
    {
        OrderTimeline tl(order.getId(), order.getCustomerName(), order.getItem());
        tl.addTransition(StateTransition(OrderStatus::QUEUED, OrderStatus::QUEUED,
                                         mClock++, "Order created and queued"));
        mTimelines.insert(order.getId(), tl);
        mUndoStack.push(UndoAction(ActionType::ORDER_ADDED, order,
                                   "Added Order #" + to_string(order.getId()), mClock));
    }

    void recordStateChange(int orderId, OrderStatus from, OrderStatus to, const string &desc)
    {
        OrderTimeline *tl = mTimelines.find(orderId);
        if (tl)
            tl->addTransition(StateTransition(from, to, mClock++, desc));
    }

    void recordOrderCancelled(const Order &order)
    {
        recordStateChange(order.getId(), order.getStatus(), OrderStatus::CANCELLED, "Order cancelled");
        mUndoStack.push(UndoAction(ActionType::ORDER_CANCELLED, order,
                                   "Cancelled Order #" + to_string(order.getId()), mClock));
    }

    void recordOrderProcessed(const Order &order)
    {
        mUndoStack.push(UndoAction(ActionType::ORDER_PROCESSED, order,
                                   "Processed Order #" + to_string(order.getId()), mClock));
    }

    // Undo the last action
    bool undoLastAction(OrderHeap &orderQueue)
    {
        if (mUndoStack.empty())
            return false;

        UndoAction action = mUndoStack[mUndoStack.size() - 1];
        mUndoStack.removeLast();

        if (action.type == ActionType::ORDER_ADDED)
        {
            orderQueue.cancelOrder(action.snapshot.getId());
            mTimelines.remove(action.snapshot.getId());
            UI::success("Undone: " + action.description + " (removed from queue)");
        }
        else if (action.type == ActionType::ORDER_CANCELLED ||
                 action.type == ActionType::ORDER_PROCESSED)
        {
            orderQueue.insert(action.snapshot);
            OrderTimeline *tl = mTimelines.find(action.snapshot.getId());
            if (tl)
            {
                OrderStatus prevState = (action.type == ActionType::ORDER_CANCELLED)
                                            ? OrderStatus::CANCELLED
                                            : OrderStatus::DISPATCHED;
                tl->addTransition(StateTransition(prevState, action.snapshot.getStatus(),
                                                  mClock++, "Undo: restored order"));
            }
            UI::success("Undone: " + action.description + " (restored to queue)");
        }
        return true;
    }

    // Replay timeline for a specific order
    bool replayTimeline(int orderId) const
    {
        const OrderTimeline *tl = mTimelines.find(orderId);
        if (!tl)
            return false;
        tl->replay();
        return true;
    }

    // Display summary of all tracked orders
    void displayAllTimelines() const
    {
        Array<OrderTimeline> all = mTimelines.getAllValues();
        if (all.empty())
        {
            cout << "  (no orders tracked)\n";
            return;
        }
        cout << "  " << left << setw(7) << "ID" << setw(16) << "Customer"
             << setw(16) << "Item" << setw(14) << "Current State"
             << setw(14) << "Transitions" << "\n";
        cout << "  " << string(67, '-') << "\n";
        for (int i = 0; i < all.size(); i++)
        {
            cout << "  " << left << setw(7) << all[i].orderId
                 << setw(16) << all[i].customerName
                 << setw(16) << all[i].item
                 << setw(14) << statusToString(all[i].currentState())
                 << setw(14) << all[i].transitions.size() << "\n";
        }
    }

    // Display undo stack
    void displayUndoStack() const
    {
        if (mUndoStack.empty())
        {
            cout << "  (undo stack is empty)\n";
            return;
        }
        cout << "  " << left << setw(5) << "#" << setw(18) << "Action"
             << setw(8) << "Ord.ID" << setw(30) << "Description"
             << setw(6) << "T" << "\n";
        cout << "  " << string(67, '-') << "\n";
        for (int i = mUndoStack.size() - 1; i >= 0; i--)
        {
            const UndoAction &a = mUndoStack[i];
            cout << "  " << left << setw(5) << (mUndoStack.size() - i)
                 << setw(18) << actionToString(a.type)
                 << setw(8) << a.snapshot.getId()
                 << setw(30) << a.description
                 << setw(6) << a.timestamp << "\n";
        }
    }

    int getUndoCount() const { return mUndoStack.size(); }
    int getTrackedCount() const { return mTimelines.getCount(); }
};

// ==============================================================================
//  SECTION 8: Delivery Pipeline (MODULE 5 - Integrated End-to-End)
// ==============================================================================

class DeliveryPipeline
{
private:
    OrderHeap &mOrderQueue;
    RiderHeap &mRiderPool;
    CityGraph &mCity;
    KitchenSystem &mKitchens;
    SearchEngine &mSearchEngine;
    OrderTracker &mTracker;

public:
    DeliveryPipeline(OrderHeap &oq, RiderHeap &rp, CityGraph &cg,
                     KitchenSystem &ks, SearchEngine &se, OrderTracker &tr)
        : mOrderQueue(oq), mRiderPool(rp), mCity(cg),
          mKitchens(ks), mSearchEngine(se), mTracker(tr) {}

    bool processNextOrder(int destNode)
    {
        if (mOrderQueue.isEmpty())
        {
            UI::warning("No orders in the scheduling queue.");
            return false;
        }

        Order order = mOrderQueue.extractTop();
        cout << "\n";
        UI::header("Processing Order #" + to_string(order.getId()));
        cout << "  Customer : " << order.getCustomerName() << "\n";
        cout << "  Item     : " << order.getItem() << "\n";
        cout << "  Category : " << order.getCategory() << "\n";
        cout << "  Priority : " << priorityToString(order.getPriority()) << "\n";
        cout << "  PrepTime : " << order.getPrepTime() << " min\n";
        cout << "  Deadline : " << order.getDeadline() << " min\n";

        string kitchenName = "", riderName = "";
        int routeDist = -1, eta = -1;

        // Step 1: Kitchen Assignment
        UI::subHeader("Step 1: Kitchen Assignment");
        int kitchenIdx = mKitchens.distributeOrder(order.getId(), order.getItem(), order.getPrepTime());
        if (kitchenIdx >= 0)
        {
            order.setAssignedKitchen(kitchenIdx);
            order.setStatus(OrderStatus::PREPARING);
            kitchenName = mKitchens.getKitchenName(kitchenIdx);
            mTracker.recordStateChange(order.getId(), OrderStatus::QUEUED, OrderStatus::PREPARING,
                                       "Assigned to " + kitchenName);
            UI::success("Assigned to " + kitchenName + " (wait: " +
                        to_string(mKitchens.getKitchenWaitTime(kitchenIdx)) + " min)");
        }
        else
        {
            UI::error("All kitchens are at full capacity!");
        }

        // Step 2: Rider Dispatch (with route optimization)
        UI::subHeader("Step 2: Rider Dispatch");
        Rider rider = mRiderPool.assignBestRiderWithRoute(mCity, destNode);
        if (rider.getId() != 0)
        {
            order.setAssignedRider(rider.getId());
            order.setStatus(OrderStatus::DISPATCHED);
            riderName = rider.getName();
            mTracker.recordStateChange(order.getId(), OrderStatus::PREPARING, OrderStatus::DISPATCHED,
                                       "Dispatched to rider " + riderName);
            UI::success("Dispatched to " + riderName +
                        " (Load: " + to_string(rider.getCurrentLoad()) +
                        "/" + to_string(rider.getCapacity()) +
                        ", Location: Node " + to_string(rider.getLocationNode()) + ")");
        }
        else
        {
            UI::error("No riders available for dispatch!");
        }

        // Step 3: Route Optimization
        UI::subHeader("Step 3: Route Optimization");
        if (rider.getId() != 0 && mCity.isValidNode(destNode))
        {
            Array<int> path;
            routeDist = mCity.findShortestPath(rider.getLocationNode(), destNode, path);
            if (routeDist >= 0)
            {
                int kitchenWait = (kitchenIdx >= 0) ? mKitchens.getKitchenWaitTime(kitchenIdx) : 0;
                eta = routeDist + kitchenWait;
                cout << "  Route: ";
                for (int i = 0; i < path.size(); i++)
                {
                    if (i > 0)
                        cout << " -> ";
                    cout << mCity.getLocationName(path[i]);
                }
                cout << "\n  Distance: " << routeDist << " units\n";
                cout << "  Kitchen Wait: " << kitchenWait << " min\n";
                cout << "  Estimated ETA: " << eta << " min\n";

                int cost = mCity.estimateDeliveryCost(rider.getLocationNode(), destNode);
                if (cost >= 0)
                    cout << "  Delivery Cost: Rs. " << cost << "\n";

                if (eta > order.getDeadline())
                {
                    UI::warning("ETA (" + to_string(eta) + " min) exceeds deadline (" +
                                to_string(order.getDeadline()) + " min)!");
                    order.setDelayed(true);
                }
            }
            else
            {
                UI::error("No route available to destination!");
            }
        }

        // Record in search engine and tracker
        order.setStatus(OrderStatus::DISPATCHED);
        OrderRecord rec(order, kitchenName, riderName, routeDist, eta);
        mSearchEngine.indexOrder(rec);
        mTracker.recordOrderProcessed(order);

        UI::subHeader("Order #" + to_string(order.getId()) + " Processing Complete");
        return true;
    }
};

// ==============================================================================
//  SECTION 9: Performance Analysis Module (MODULE 7)
// ==============================================================================

class PerformanceAnalyzer
{
private:
    // Simple high-resolution timer using clock()
    static double getTimeMs()
    {
        return (double)clock() / CLOCKS_PER_SEC * 1000.0;
    }

public:
    // Benchmark order heap insertion
    static void benchmarkInsertion(int count)
    {
        UI::subHeader("Benchmark: Order Insertion (" + to_string(count) + " orders)");
        OrderHeap testHeap;
        double start = getTimeMs();
        for (int i = 1; i <= count; i++)
        {
            Priority p = static_cast<Priority>((i % 3) + 1);
            Order o(i, "Cust" + to_string(i), "Item" + to_string(i), "Regular", p, 10 + (i % 20), 30 + (i % 60));
            testHeap.insert(o);
        }
        double elapsed = getTimeMs() - start;
        cout << "  Inserted " << count << " orders in " << fixed << setprecision(3) << elapsed << " ms\n";
        cout << "  Avg per insertion: " << fixed << setprecision(4) << (elapsed / count) << " ms\n";
    }

    // Benchmark order heap extraction (deletion)
    static void benchmarkDeletion(int count)
    {
        UI::subHeader("Benchmark: Order Extraction (" + to_string(count) + " orders)");
        OrderHeap testHeap;
        for (int i = 1; i <= count; i++)
        {
            Priority p = static_cast<Priority>((i % 3) + 1);
            testHeap.insert(Order(i, "C" + to_string(i), "I" + to_string(i), "Regular", p, 10, 30));
        }
        double start = getTimeMs();
        for (int i = 0; i < count; i++)
            testHeap.extractTop();
        double elapsed = getTimeMs() - start;
        cout << "  Extracted " << count << " orders in " << fixed << setprecision(3) << elapsed << " ms\n";
        cout << "  Avg per extraction: " << fixed << setprecision(4) << (elapsed / count) << " ms\n";
    }

    // Benchmark hash map search
    static void benchmarkSearching(int count)
    {
        UI::subHeader("Benchmark: Hash Map Search (" + to_string(count) + " lookups)");
        HashMap<OrderRecord> testMap;
        for (int i = 1; i <= count; i++)
        {
            Order o(i, "C" + to_string(i), "I" + to_string(i), "Regular",
                    static_cast<Priority>((i % 3) + 1), 10, 30);
            testMap.insert(i, OrderRecord(o));
        }
        double start = getTimeMs();
        int found = 0;
        for (int i = 1; i <= count; i++)
        {
            if (testMap.find(i))
                found++;
        }
        double elapsed = getTimeMs() - start;
        cout << "  Searched " << count << " records in " << fixed << setprecision(3) << elapsed << " ms\n";
        cout << "  Found: " << found << "/" << count << "\n";
        cout << "  Avg per lookup: " << fixed << setprecision(4) << (elapsed / count) << " ms\n";
    }

    // Benchmark scheduling (insert + extract cycle)
    static void benchmarkScheduling(int count)
    {
        UI::subHeader("Benchmark: Scheduling Cycle (" + to_string(count) + " insert+extract)");
        OrderHeap testHeap;
        double start = getTimeMs();
        for (int i = 1; i <= count; i++)
        {
            Priority p = static_cast<Priority>((i % 3) + 1);
            testHeap.insert(Order(i, "C" + to_string(i), "I" + to_string(i), "Regular", p, 10, 30));
            if (i % 2 == 0)
                testHeap.extractTop();
        }
        while (!testHeap.isEmpty())
            testHeap.extractTop();
        double elapsed = getTimeMs() - start;
        cout << "  Completed " << count << " scheduling cycles in " << fixed << setprecision(3) << elapsed << " ms\n";
    }

    // Benchmark Dijkstra routing
    static void benchmarkRouting()
    {
        UI::subHeader("Benchmark: Dijkstra Routing (20-node graph, 100 queries)");
        CityGraph testGraph;
        for (int i = 0; i < 20; i++)
            testGraph.addLocation("N" + to_string(i));

        // Create a connected graph
        for (int i = 0; i < 19; i++)
            testGraph.addRoad(i, i + 1, 5 + (i % 10));
        for (int i = 0; i < 15; i++)
            testGraph.addRoad(i, (i + 3) % 20, 8 + (i % 7));

        int queries = 100;
        double start = getTimeMs();
        for (int q = 0; q < queries; q++)
        {
            Array<int> path;
            testGraph.findShortestPath(q % 20, (q * 7 + 3) % 20, path);
        }
        double elapsed = getTimeMs() - start;
        cout << "  Ran " << queries << " shortest-path queries in " << fixed << setprecision(3) << elapsed << " ms\n";
        cout << "  Avg per query: " << fixed << setprecision(4) << (elapsed / queries) << " ms\n";
    }

    // Scalability test: measures time as dataset grows
    static void benchmarkScalability()
    {
        UI::subHeader("Benchmark: Scalability Analysis");
        cout << "  " << left << setw(12) << "Dataset" << setw(15) << "Insert(ms)"
             << setw(15) << "Extract(ms)" << setw(15) << "Search(ms)" << "\n";
        cout << "  " << string(57, '-') << "\n";

        int sizes[] = {100, 500, 1000, 2000, 5000};
        for (int s = 0; s < 5; s++)
        {
            int n = sizes[s];

            // Insertion
            OrderHeap heap;
            double t1 = getTimeMs();
            for (int i = 1; i <= n; i++)
                heap.insert(Order(i, "C" + to_string(i), "I" + to_string(i), "Regular",
                                  static_cast<Priority>((i % 3) + 1), 10, 30));
            double insertTime = getTimeMs() - t1;

            // Extraction
            double t2 = getTimeMs();
            for (int i = 0; i < n; i++)
                heap.extractTop();
            double extractTime = getTimeMs() - t2;

            // Search
            HashMap<OrderRecord> map;
            for (int i = 1; i <= n; i++)
            {
                Order o(i, "C" + to_string(i), "I" + to_string(i), "Regular",
                        static_cast<Priority>((i % 3) + 1), 10, 30);
                map.insert(i, OrderRecord(o));
            }
            double t3 = getTimeMs();
            for (int i = 1; i <= n; i++)
                map.find(i);
            double searchTime = getTimeMs() - t3;

            cout << "  " << left << setw(12) << n
                 << setw(15) << fixed << setprecision(3) << insertTime
                 << setw(15) << fixed << setprecision(3) << extractTime
                 << setw(15) << fixed << setprecision(3) << searchTime << "\n";
        }
    }

    // Memory usage estimation
    static void estimateMemoryUsage(int orderCount, int riderCount, int kitchenCount, int graphNodes)
    {
        UI::subHeader("Memory Usage Estimation");
        long long orderMem = (long long)orderCount * sizeof(Order);
        long long riderMem = (long long)riderCount * sizeof(Rider);
        long long graphMem = (long long)graphNodes * graphNodes * (sizeof(int) + sizeof(bool));
        long long hashMem = (long long)127 * sizeof(void *) + (long long)orderCount * (sizeof(OrderRecord) + sizeof(void *) + sizeof(int));
        long long totalMem = orderMem + riderMem + graphMem + hashMem;

        cout << "  Orders (" << orderCount << "): ~" << orderMem << " bytes\n";
        cout << "  Riders (" << riderCount << "): ~" << riderMem << " bytes\n";
        cout << "  Graph (" << graphNodes << " nodes): ~" << graphMem << " bytes\n";
        cout << "  Hash Index: ~" << hashMem << " bytes\n";
        cout << "  " << string(40, '-') << "\n";
        cout << "  Total Estimated: ~" << totalMem << " bytes ("
             << fixed << setprecision(2) << (totalMem / 1024.0) << " KB)\n";
    }

    // Run all benchmarks
    static void runFullAnalysis()
    {
        UI::header("Performance Analysis Module");

        benchmarkInsertion(1000);
        benchmarkDeletion(1000);
        benchmarkSearching(1000);
        benchmarkScheduling(1000);
        benchmarkRouting();
        benchmarkScalability();
        estimateMemoryUsage(1000, 50, 10, 20);

        cout << "\n";
        UI::success("Performance analysis complete.");
    }
};

// ==============================================================================
//  SECTION 10: File Handler (Persistent CSV Storage for ALL Data)
// ==============================================================================

class FileHandler
{
private:
    // Helper: strip trailing \r and whitespace (Windows line-ending fix)
    static string trimLine(const string &s)
    {
        int end = (int)s.size() - 1;
        while (end >= 0 && (s[end] == '\r' || s[end] == '\n' || s[end] == ' '))
            end--;
        return s.substr(0, end + 1);
    }

    // Helper: safe stoi with fallback
    static int safeStoi(const string &s, int fallback = 0)
    {
        try
        {
            string trimmed = trimLine(s);
            if (trimmed.empty())
                return fallback;
            return stoi(trimmed);
        }
        catch (...)
        {
            return fallback;
        }
    }

public:
    // ---- Save Orders to CSV ----
    static void saveOrders(const OrderHeap &heap, const string &filename = "orders.csv")
    {
        ofstream file(filename);
        if (!file.is_open())
        {
            UI::error("Cannot open " + filename + " for writing.");
            return;
        }
        file << "ID,CustomerName,Item,Category,Priority,PrepTime,Deadline,Status,Delayed\n";
        Array<Order> orders = heap.getAllOrders();
        for (int i = 0; i < orders.size(); i++)
        {
            const Order &o = orders[i];
            file << o.getId() << ","
                 << o.getCustomerName() << ","
                 << o.getItem() << ","
                 << o.getCategory() << ","
                 << static_cast<int>(o.getPriority()) << ","
                 << o.getPrepTime() << ","
                 << o.getDeadline() << ","
                 << static_cast<int>(o.getStatus()) << ","
                 << (o.isDelayed() ? 1 : 0) << "\n";
        }
        file.close();
        UI::success("Orders saved to " + filename + " (" + to_string(orders.size()) + " records)");
    }

    // ---- Load Orders from CSV ----
    static int loadOrders(OrderHeap &heap, OrderTracker &tracker, const string &filename = "orders.csv")
    {
        ifstream file(filename);
        if (!file.is_open())
            return 0;

        string line;
        getline(file, line); // skip header
        int count = 0;

        while (getline(file, line))
        {
            line = trimLine(line);
            if (line.empty())
                continue;
            try
            {
                stringstream ss(line);
                string tok, customer, item, category;

                getline(ss, tok, ',');       int id = safeStoi(tok);
                getline(ss, customer, ',');
                getline(ss, item, ',');
                getline(ss, category, ',');
                getline(ss, tok, ',');        int pri = safeStoi(tok, 1);
                getline(ss, tok, ',');        int prepTime = safeStoi(tok, 10);
                getline(ss, tok, ',');        int deadline = safeStoi(tok, 30);
                getline(ss, tok, ',');        int status = safeStoi(tok, 0);
                getline(ss, tok);            int delayed = safeStoi(tok, 0); // last field: no comma

                Order o(id, customer, item, category, static_cast<Priority>(pri), prepTime, deadline);
                o.setStatus(static_cast<OrderStatus>(status));
                o.setDelayed(delayed == 1);
                heap.insert(o);
                tracker.recordOrderCreated(o);
                count++;
            }
            catch (...) { /* skip malformed line */ }
        }
        file.close();
        return count;
    }

    // ---- Save Riders to CSV ----
    static void saveRiders(const RiderHeap &riders, const string &filename = "riders.csv")
    {
        ofstream file(filename);
        if (!file.is_open())
        {
            UI::error("Cannot open " + filename + " for writing.");
            return;
        }
        file << "ID,Name,Capacity,Location,Available,CurrentLoad,TotalDeliveries\n";
        Array<Rider> all = riders.getAllRiders();
        for (int i = 0; i < all.size(); i++)
        {
            const Rider &r = all[i];
            file << r.getId() << ","
                 << r.getName() << ","
                 << r.getCapacity() << ","
                 << r.getLocationNode() << ","
                 << (r.isAvailable() ? 1 : 0) << ","
                 << r.getCurrentLoad() << ","
                 << r.getTotalDeliveries() << "\n";
        }
        file.close();
        UI::success("Riders saved to " + filename + " (" + to_string(all.size()) + " records)");
    }

    // ---- Load Riders from CSV ----
    static int loadRiders(RiderHeap &riders, const string &filename = "riders.csv")
    {
        ifstream file(filename);
        if (!file.is_open())
            return 0;

        string line;
        getline(file, line); // skip header
        int count = 0;

        while (getline(file, line))
        {
            line = trimLine(line);
            if (line.empty())
                continue;
            try
            {
                stringstream ss(line);
                string tok, name;

                getline(ss, tok, ',');    int id = safeStoi(tok);
                getline(ss, name, ',');
                getline(ss, tok, ',');    int cap = safeStoi(tok, 3);
                getline(ss, tok, ',');    int loc = safeStoi(tok, 0);
                getline(ss, tok, ',');    int avail = safeStoi(tok, 1);
                getline(ss, tok, ',');    int load = safeStoi(tok, 0);
                getline(ss, tok);        int delivered = safeStoi(tok, 0); // last field

                Rider r(id, name, cap, loc);
                if (!avail)
                    r.setAvailable(false);
                for (int i = 0; i < load; i++)
                    r.incrementLoad();
                riders.addRider(r);
                count++;
            }
            catch (...) { /* skip malformed line */ }
        }
        file.close();
        return count;
    }

    // ---- Save Delivery Records to CSV ----
    static void saveRecords(const SearchEngine &engine, const string &filename = "delivery_records.csv")
    {
        ofstream file(filename);
        if (!file.is_open())
        {
            UI::error("Cannot open " + filename + " for writing.");
            return;
        }
        file << "ID,Customer,Item,Category,Priority,Status,Kitchen,Rider,Distance,ETA\n";
        const Array<OrderRecord> &records = engine.getAllRecords();
        for (int i = 0; i < records.size(); i++)
        {
            const OrderRecord &r = records[i];
            file << r.getId() << ","
                 << r.getCustomerName() << ","
                 << r.getItem() << ","
                 << r.getCategory() << ","
                 << static_cast<int>(r.getPriority()) << ","
                 << static_cast<int>(r.getFinalStatus()) << ","
                 << r.getKitchenName() << ","
                 << r.getRiderName() << ","
                 << r.getRouteDistance() << ","
                 << r.getEstimatedETA() << "\n";
        }
        file.close();
        UI::success("Delivery records saved to " + filename + " (" + to_string(records.size()) + " records)");
    }

    // ---- Load Delivery Records from CSV ----
    static int loadRecords(SearchEngine &engine, const string &filename = "delivery_records.csv")
    {
        ifstream file(filename);
        if (!file.is_open())
            return 0;

        string line;
        getline(file, line); // skip header
        int count = 0;

        while (getline(file, line))
        {
            line = trimLine(line);
            if (line.empty())
                continue;
            try
            {
                stringstream ss(line);
                string tok, customer, item, category, kitchen, rider;

                getline(ss, tok, ',');       int id = safeStoi(tok);
                getline(ss, customer, ',');
                getline(ss, item, ',');
                getline(ss, category, ',');
                getline(ss, tok, ',');        int pri = safeStoi(tok, 1);
                getline(ss, tok, ',');        int stat = safeStoi(tok, 0);
                getline(ss, kitchen, ',');
                getline(ss, rider, ',');
                getline(ss, tok, ',');        int dist = safeStoi(tok, -1);
                getline(ss, tok);            int eta = safeStoi(tok, -1); // last field

                OrderRecord rec(id, customer, item, category,
                                static_cast<Priority>(pri), static_cast<OrderStatus>(stat),
                                kitchen, rider, dist, eta);
                engine.indexOrder(rec);
                count++;
            }
            catch (...) { /* skip malformed line */ }
        }
        file.close();
        return count;
    }

    // ---- Save City Graph to CSV ----
    static void saveGraph(const CityGraph &graph, const string &filename = "city_graph.csv")
    {
        ofstream file(filename);
        if (!file.is_open())
        {
            UI::error("Cannot open " + filename + " for writing.");
            return;
        }

        int n = graph.getNodeCount();
        file << "LOCATIONS," << n << "\n";
        for (int i = 0; i < n; i++)
            file << i << "," << graph.getLocationName(i) << "\n";

        file << "ROADS\n";
        for (int i = 0; i < n; i++)
        {
            for (int j = i + 1; j < n; j++)
            {
                int w = graph.getRoadWeight(i, j);
                if (w > 0)
                {
                    file << i << "," << j << "," << w << ","
                         << (graph.isBlocked(i, j) ? 1 : 0) << "\n";
                }
            }
        }
        file.close();
        UI::success("City graph saved to " + filename);
    }

    // ---- Load City Graph from CSV ----
    static bool loadGraph(CityGraph &graph, const string &filename = "city_graph.csv")
    {
        ifstream file(filename);
        if (!file.is_open())
            return false;

        try
        {
            string line;
            // Read LOCATIONS header: "LOCATIONS,8"
            getline(file, line);
            line = trimLine(line);
            stringstream hss(line);
            string tag;
            getline(hss, tag, ',');  // "LOCATIONS"
            getline(hss, tag);      // node count (last field, no comma)
            int nodeCount = safeStoi(tag, 0);

            for (int i = 0; i < nodeCount; i++)
            {
                getline(file, line);
                line = trimLine(line);
                stringstream ss(line);
                string idx, name;
                getline(ss, idx, ',');
                getline(ss, name); // last field, no comma after name
                name = trimLine(name);
                graph.addLocation(name);
            }

            // Read ROADS header
            getline(file, line); // "ROADS"
            while (getline(file, line))
            {
                line = trimLine(line);
                if (line.empty())
                    continue;
                stringstream ss(line);
                string tok;
                getline(ss, tok, ','); int u = safeStoi(tok);
                getline(ss, tok, ','); int v = safeStoi(tok);
                getline(ss, tok, ','); int w = safeStoi(tok);
                getline(ss, tok);      int blocked = safeStoi(tok); // last field
                graph.addRoad(u, v, w);
                if (blocked)
                    graph.blockRoad(u, v);
            }
        }
        catch (...) { /* parsing error */ }

        file.close();
        return true;
    }

    // ---- Save Kitchen Config to CSV ----
    static void saveKitchens(const KitchenSystem &kitchens, const string &filename = "kitchens.csv")
    {
        ofstream file(filename);
        if (!file.is_open())
        {
            UI::error("Cannot open " + filename + " for writing.");
            return;
        }
        file << "Name,MaxCapacity\n";
        for (int i = 0; i < kitchens.getNumKitchens(); i++)
        {
            file << kitchens.getKitchenName(i) << ","
                 << 5 << "\n";
        }
        file.close();
        UI::success("Kitchen config saved to " + filename);
    }

    // ---- Load Kitchens from CSV ----
    static int loadKitchens(KitchenSystem &kitchens, const string &filename = "kitchens.csv")
    {
        ifstream file(filename);
        if (!file.is_open())
            return 0;

        string line;
        getline(file, line); // skip header
        int count = 0;

        while (getline(file, line))
        {
            line = trimLine(line);
            if (line.empty())
                continue;
            try
            {
                stringstream ss(line);
                string name, tok;
                getline(ss, name, ',');
                getline(ss, tok);       // last field
                int cap = safeStoi(tok, 5);
                kitchens.addKitchen(name, cap);
                count++;
            }
            catch (...) { /* skip malformed line */ }
        }
        file.close();
        return count;
    }

    // ---- Save All Data (verbose, for explicit save) ----
    static void saveAll(const OrderHeap &heap, const RiderHeap &riders,
                        const CityGraph &graph, const KitchenSystem &kitchens,
                        const SearchEngine &engine)
    {
        UI::header("Saving All Data to Files");
        saveOrders(heap);
        saveRiders(riders);
        saveGraph(graph);
        saveKitchens(kitchens);
        saveRecords(engine);
        cout << "\n";
        UI::success("All data saved successfully!");
    }

    // ---- Load All Data ----
    static void loadAll(OrderHeap &heap, RiderHeap &riders, CityGraph &graph,
                        KitchenSystem &kitchens, SearchEngine &engine, OrderTracker &tracker)
    {
        UI::header("Loading Data from Files");
        int orders = loadOrders(heap, tracker);
        int riderCount = loadRiders(riders);
        bool graphLoaded = loadGraph(graph);
        int kitchenCount = loadKitchens(kitchens);
        int records = loadRecords(engine);

        cout << "  Orders loaded    : " << orders << "\n";
        cout << "  Riders loaded    : " << riderCount << "\n";
        cout << "  Graph loaded     : " << (graphLoaded ? "Yes" : "No") << "\n";
        cout << "  Kitchens loaded  : " << kitchenCount << "\n";
        cout << "  Records loaded   : " << records << "\n";

        if (orders > 0 || riderCount > 0 || graphLoaded || kitchenCount > 0 || records > 0)
            UI::success("Data loaded from previous session!");
        else
            UI::info("No previous data found. Starting fresh.");
    }
};



// ==============================================================================
//  SECTION 11: Demo Data Seeder (only used if no files exist)
// ==============================================================================

static bool filesExist()
{
    ifstream f1("orders.csv");
    ifstream f2("riders.csv");
    ifstream f3("city_graph.csv");
    bool exists = f1.is_open() || f2.is_open() || f3.is_open();
    f1.close();
    f2.close();
    f3.close();
    return exists;
}

static void seedDemoData(OrderHeap &oq, RiderHeap &rp, CityGraph &cg,
                         KitchenSystem &ks, SearchEngine &se, OrderTracker &tr)
{
    // City Map (8 locations)
    cg.addLocation("Central Hub");     // 0
    cg.addLocation("North Market");    // 1
    cg.addLocation("East Plaza");      // 2
    cg.addLocation("South Block");     // 3
    cg.addLocation("West Avenue");     // 4
    cg.addLocation("University");      // 5
    cg.addLocation("Airport Road");    // 6
    cg.addLocation("Tech Park");       // 7

    cg.addRoad(0, 1, 5);
    cg.addRoad(0, 2, 8);
    cg.addRoad(0, 3, 6);
    cg.addRoad(0, 4, 4);
    cg.addRoad(1, 2, 3);
    cg.addRoad(1, 5, 7);
    cg.addRoad(2, 3, 4);
    cg.addRoad(2, 6, 9);
    cg.addRoad(3, 4, 5);
    cg.addRoad(3, 7, 6);
    cg.addRoad(4, 5, 8);
    cg.addRoad(5, 6, 4);
    cg.addRoad(6, 7, 3);
    cg.addRoad(5, 7, 5);

    // Kitchens
    ks.addKitchen("Alpha Grill", 5);
    ks.addKitchen("Beta Pizza", 4);
    ks.addKitchen("Gamma Wok", 3);
    ks.addKitchen("Delta Diner", 4);

    // Riders
    rp.addRider(Rider(1, "Kamran", 3, 0));
    rp.addRider(Rider(2, "Zubair", 2, 1));
    rp.addRider(Rider(3, "Ahmed", 4, 3));
    rp.addRider(Rider(4, "Fatima", 3, 5));

    // Demo Orders
    Order o1(101, "Ali Khan", "Biryani", "VIP", Priority::VIP, 15, 25);
    Order o2(102, "Sara Ahmed", "Pizza", "Regular", Priority::NORMAL, 12, 40);
    Order o3(103, "Hassan Raza", "Burger", "Premium", Priority::PREMIUM, 8, 20);
    Order o4(104, "Ayesha Noor", "Pasta", "Regular", Priority::NORMAL, 18, 45);
    Order o5(105, "Usman Ali", "Shawarma", "VIP", Priority::VIP, 10, 15);

    oq.insert(o1); tr.recordOrderCreated(o1);
    oq.insert(o2); tr.recordOrderCreated(o2);
    oq.insert(o3); tr.recordOrderCreated(o3);
    oq.insert(o4); tr.recordOrderCreated(o4);
    oq.insert(o5); tr.recordOrderCreated(o5);

    // Index initial orders in search engine
    se.indexOrder(OrderRecord(o1));
    se.indexOrder(OrderRecord(o2));
    se.indexOrder(OrderRecord(o3));
    se.indexOrder(OrderRecord(o4));
    se.indexOrder(OrderRecord(o5));

    UI::success("Demo data seeded (8 locations, 4 kitchens, 4 riders, 5 orders).");
}

// ==============================================================================
//  SECTION 12: Main Application Menu System
// ==============================================================================

static int gNextOrderId = 200; // auto-incrementing order ID

void showMainMenu()
{
    cout << "\n";
    cout << "  +======================================================================+\n";
    cout << "  |                                                                      |\n";
    cout << "  |     FFFFFFF  OOO   OOO  DDDD   EEEEE X   X PPPP                     |\n";
    cout << "  |     F       O   O O   O D   D  E      X X  P   P                    |\n";
    cout << "  |     FFFF    O   O O   O D   D  EEEE    X   PPPP                     |\n";
    cout << "  |     F       O   O O   O D   D  E      X X  P                        |\n";
    cout << "  |     F        OOO   OOO  DDDD   EEEEE X   X P                        |\n";
    cout << "  |                                                                      |\n";
    cout << "  |         D I S P A T C H   O P T I M I Z A T I O N   E N G I N E      |\n";
    cout << "  |                          Version 5.0                                 |\n";
    cout << "  |                                                                      |\n";
    cout << "  +======================================================================+\n";
    cout << "  |                                                                      |\n";
    cout << "  |  [MODULE 1] ORDER SCHEDULING ENGINE             [Data: Max-Heap]     |\n";
    cout << "  |  --------------------------------------------------------------------|\n";
    cout << "  |    1.  Add New Order                                                  |\n";
    cout << "  |    2.  View Order Queue                                               |\n";
    cout << "  |    3.  Cancel Order                                                   |\n";
    cout << "  |    4.  Update Order Priority                                          |\n";
    cout << "  |    5.  Mark Order as Delayed                                          |\n";
    cout << "  |    6.  View Delayed Orders                                            |\n";
    cout << "  |                                                                      |\n";
    cout << "  |  [MODULE 2] KITCHEN LOAD BALANCER               [Data: Linked-List]  |\n";
    cout << "  |  --------------------------------------------------------------------|\n";
    cout << "  |    7.  View Kitchen Status                                            |\n";
    cout << "  |    8.  Detect Overloaded Kitchens                                     |\n";
    cout << "  |    9.  Rebalance Kitchen Workload                                     |\n";
    cout << "  |    10. Estimate Wait Time                                             |\n";
    cout << "  |                                                                      |\n";
    cout << "  |  [MODULE 3] RIDER DISPATCH & ASSIGNMENT         [Data: Min-Heap]     |\n";
    cout << "  |  --------------------------------------------------------------------|\n";
    cout << "  |    11. Register New Rider                                             |\n";
    cout << "  |    12. View All Riders                                                |\n";
    cout << "  |    13. Complete a Delivery                                            |\n";
    cout << "  |    14. Toggle Rider Availability                                      |\n";
    cout << "  |                                                                      |\n";
    cout << "  |  [MODULE 4] REAL-TIME ROUTE OPTIMIZATION        [Data: Adj. Matrix]  |\n";
    cout << "  |  --------------------------------------------------------------------|\n";
    cout << "  |    15. View City Map                                                  |\n";
    cout << "  |    16. Find Shortest Route (Dijkstra)                                 |\n";
    cout << "  |    17. Compare Two Routes                                             |\n";
    cout << "  |    18. Estimate Delivery Cost                                         |\n";
    cout << "  |    19. Block / Unblock Road                                           |\n";
    cout << "  |                                                                      |\n";
    cout << "  |  [MODULE 5] SEARCH & RETRIEVAL ENGINE           [Data: Hash Table]   |\n";
    cout << "  |  --------------------------------------------------------------------|\n";
    cout << "  |    20. Search Orders (Multi-Filter)                                   |\n";
    cout << "  |    21. View All Delivery Records                                      |\n";
    cout << "  |                                                                      |\n";
    cout << "  |  [MODULE 6] ORDER HISTORY & TRACKING            [Data: Stack + List] |\n";
    cout << "  |  --------------------------------------------------------------------|\n";
    cout << "  |    22. Replay Order Timeline                                          |\n";
    cout << "  |    23. View All Tracked Orders                                        |\n";
    cout << "  |    24. Undo Last Action                                               |\n";
    cout << "  |    25. View Undo Stack                                                |\n";
    cout << "  |                                                                      |\n";
    cout << "  |  [MODULE 7] PERFORMANCE ANALYSIS                [Benchmarking Suite] |\n";
    cout << "  |  --------------------------------------------------------------------|\n";
    cout << "  |    26. Run Performance Benchmarks                                     |\n";
    cout << "  |                                                                      |\n";
    cout << "  +======================================================================+\n";
    cout << "  |                                                                      |\n";
    cout << "  |    27. >>> Process Next Order (Full Delivery Pipeline) <<<            |\n";
    cout << "  |                                                                      |\n";
    cout << "  |     0. Save All & Exit                                               |\n";
    cout << "  |                                                                      |\n";
    cout << "  +======================================================================+\n";
}

// Helper: auto-save all data to files after any modification (silent)
static void autoSave(const OrderHeap &oq, const RiderHeap &rp, const CityGraph &cg,
                     const KitchenSystem &ks, const SearchEngine &se)
{
    // Save silently without printing [OK] messages
    { // Orders
        ofstream file("orders.csv");
        if (file.is_open())
        {
            file << "ID,CustomerName,Item,Category,Priority,PrepTime,Deadline,Status,Delayed\n";
            Array<Order> orders = oq.getAllOrders();
            for (int i = 0; i < orders.size(); i++)
            {
                const Order &o = orders[i];
                file << o.getId() << "," << o.getCustomerName() << "," << o.getItem() << ","
                     << o.getCategory() << "," << static_cast<int>(o.getPriority()) << ","
                     << o.getPrepTime() << "," << o.getDeadline() << ","
                     << static_cast<int>(o.getStatus()) << "," << (o.isDelayed() ? 1 : 0) << "\n";
            }
            file.close();
        }
    }
    { // Riders
        ofstream file("riders.csv");
        if (file.is_open())
        {
            file << "ID,Name,Capacity,Location,Available,CurrentLoad,TotalDeliveries\n";
            Array<Rider> all = rp.getAllRiders();
            for (int i = 0; i < all.size(); i++)
            {
                const Rider &r = all[i];
                file << r.getId() << "," << r.getName() << "," << r.getCapacity() << ","
                     << r.getLocationNode() << "," << (r.isAvailable() ? 1 : 0) << ","
                     << r.getCurrentLoad() << "," << r.getTotalDeliveries() << "\n";
            }
            file.close();
        }
    }
    { // Graph
        ofstream file("city_graph.csv");
        if (file.is_open())
        {
            int n = cg.getNodeCount();
            file << "LOCATIONS," << n << "\n";
            for (int i = 0; i < n; i++)
                file << i << "," << cg.getLocationName(i) << "\n";
            file << "ROADS\n";
            for (int i = 0; i < n; i++)
                for (int j = i + 1; j < n; j++)
                {
                    int w = cg.getRoadWeight(i, j);
                    if (w > 0)
                        file << i << "," << j << "," << w << "," << (cg.isBlocked(i, j) ? 1 : 0) << "\n";
                }
            file.close();
        }
    }
    { // Kitchens
        ofstream file("kitchens.csv");
        if (file.is_open())
        {
            file << "Name,MaxCapacity\n";
            for (int i = 0; i < ks.getNumKitchens(); i++)
                file << ks.getKitchenName(i) << "," << 5 << "\n";
            file.close();
        }
    }
    { // Delivery Records
        ofstream file("delivery_records.csv");
        if (file.is_open())
        {
            file << "ID,Customer,Item,Category,Priority,Status,Kitchen,Rider,Distance,ETA\n";
            const Array<OrderRecord> &records = se.getAllRecords();
            for (int i = 0; i < records.size(); i++)
            {
                const OrderRecord &r = records[i];
                file << r.getId() << "," << r.getCustomerName() << "," << r.getItem() << ","
                     << r.getCategory() << "," << static_cast<int>(r.getPriority()) << ","
                     << static_cast<int>(r.getFinalStatus()) << "," << r.getKitchenName() << ","
                     << r.getRiderName() << "," << r.getRouteDistance() << ","
                     << r.getEstimatedETA() << "\n";
            }
            file.close();
        }
    }
}

int main()
{
    OrderHeap orderQueue;
    RiderHeap riderPool;
    CityGraph cityGraph;
    KitchenSystem kitchens;
    SearchEngine searchEngine;
    OrderTracker tracker;

    // Try loading from files first
    if (filesExist())
    {
        FileHandler::loadAll(orderQueue, riderPool, cityGraph, kitchens, searchEngine, tracker);
        // Determine next order ID from loaded orders
        Array<Order> loaded = orderQueue.getAllOrders();
        for (int i = 0; i < loaded.size(); i++)
            if (loaded[i].getId() >= gNextOrderId)
                gNextOrderId = loaded[i].getId() + 1;
        // Also check delivery records for max ID
        const Array<OrderRecord> &recs = searchEngine.getAllRecords();
        for (int i = 0; i < recs.size(); i++)
            if (recs[i].getId() >= gNextOrderId)
                gNextOrderId = recs[i].getId() + 1;
    }
    else
    {
        seedDemoData(orderQueue, riderPool, cityGraph, kitchens, searchEngine, tracker);
        // Save demo data immediately so files exist for next run
        UI::info("Saving initial data to files...");
        autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
    }

    DeliveryPipeline pipeline(orderQueue, riderPool, cityGraph, kitchens, searchEngine, tracker);

    int choice = -1;
    do
    {
        showMainMenu();
        choice = InputHelper::readInt("  Enter choice: ", 0, 27);
        cout << "\n";

        switch (choice)
        {
        // ================================================================
        //  MODULE 1: ORDER SCHEDULING
        // ================================================================
        case 1:
        {
            UI::header("Add New Order");
            int id = InputHelper::readInt("  Order ID (0 for auto): ", 0, 99999);
            if (id == 0)
                id = gNextOrderId++;
            else if (id >= gNextOrderId)
                gNextOrderId = id + 1;

            string cust = InputHelper::readLine("  Customer Name: ");
            string item = InputHelper::readLine("  Food Item: ");

            cout << "  Customer Category:\n    1. Regular\n    2. Premium\n    3. VIP\n";
            int catChoice = InputHelper::readInt("  Select (1-3): ", 1, 3);
            string category = (catChoice == 3) ? "VIP" : (catChoice == 2) ? "Premium" : "Regular";

            cout << "  Priority:\n    1. Normal\n    2. Premium\n    3. VIP\n";
            int priChoice = InputHelper::readInt("  Select (1-3): ", 1, 3);
            Priority pri = static_cast<Priority>(priChoice);

            int prepTime = InputHelper::readInt("  Preparation Time (minutes): ", 1, 120);
            int deadline = InputHelper::readInt("  Delivery Deadline (minutes): ", 1, 180);

            Order o(id, cust, item, category, pri, prepTime, deadline);
            orderQueue.insert(o);
            tracker.recordOrderCreated(o);

            // Also index in search engine as queued
            searchEngine.indexOrder(OrderRecord(o));

            UI::success("Order #" + to_string(id) + " added successfully!");
            autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            break;
        }

        case 2:
        {
            UI::header("Order Scheduling Queue");
            cout << "  Total orders in queue: " << orderQueue.getSize() << "\n\n";
            orderQueue.display();
            break;
        }

        case 3:
        {
            UI::header("Cancel Order");
            int id = InputHelper::readInt("  Order ID to cancel: ", 1, 99999);
            Order *found = orderQueue.findOrder(id);
            if (found)
            {
                Order snapshot = *found;
                tracker.recordOrderCancelled(snapshot);
                orderQueue.cancelOrder(id);
                UI::success("Order #" + to_string(id) + " cancelled.");
                autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            }
            else
            {
                UI::error("Order #" + to_string(id) + " not found in queue.");
            }
            break;
        }

        case 4:
        {
            UI::header("Update Order Priority");
            int id = InputHelper::readInt("  Order ID: ", 1, 99999);
            cout << "  New Priority:\n    1. Normal\n    2. Premium\n    3. VIP\n";
            int newPri = InputHelper::readInt("  Select (1-3): ", 1, 3);
            if (orderQueue.updatePriority(id, static_cast<Priority>(newPri)))
            {
                tracker.recordStateChange(id, OrderStatus::QUEUED, OrderStatus::QUEUED,
                                          "Priority updated to " + priorityToString(static_cast<Priority>(newPri)));
                UI::success("Priority updated for Order #" + to_string(id));
                autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            }
            else
            {
                UI::error("Order #" + to_string(id) + " not found in queue.");
            }
            break;
        }

        case 5:
        {
            UI::header("Mark Order as Delayed");
            int id = InputHelper::readInt("  Order ID to mark delayed: ", 1, 99999);
            if (orderQueue.markDelayed(id))
            {
                tracker.recordStateChange(id, OrderStatus::QUEUED, OrderStatus::QUEUED,
                                          "Order marked as delayed");
                UI::success("Order #" + to_string(id) + " marked as delayed.");
                autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            }
            else
            {
                UI::error("Order #" + to_string(id) + " not found in queue.");
            }
            break;
        }

        case 6:
        {
            UI::header("Delayed Orders");
            Array<Order> delayed = orderQueue.getDelayedOrders();
            if (delayed.empty())
            {
                UI::info("No delayed orders found.");
            }
            else
            {
                cout << "  Found " << delayed.size() << " delayed order(s):\n\n";
                cout << "  " << left << setw(7) << "ID" << setw(16) << "Customer"
                     << setw(16) << "Item" << setw(10) << "Priority"
                     << setw(10) << "Deadline" << "\n";
                cout << "  " << string(59, '-') << "\n";
                for (int i = 0; i < delayed.size(); i++)
                {
                    const Order &o = delayed[i];
                    cout << "  " << left << setw(7) << o.getId()
                         << setw(16) << o.getCustomerName()
                         << setw(16) << o.getItem()
                         << setw(10) << priorityToString(o.getPriority())
                         << setw(10) << (to_string(o.getDeadline()) + "m") << "\n";
                }
            }
            break;
        }

        // ================================================================
        //  MODULE 2: KITCHEN MANAGEMENT
        // ================================================================
        case 7:
        {
            UI::header("Kitchen Load Status");
            kitchens.displayAll();
            break;
        }

        case 8:
        {
            UI::header("Overload Detection");
            kitchens.detectOverloaded();
            break;
        }

        case 9:
        {
            UI::header("Kitchen Rebalancing");
            if (kitchens.rebalance())
            {
                UI::success("Workload rebalanced between kitchens.");
                autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            }
            else
                UI::info("No rebalancing needed (kitchens are already balanced).");
            kitchens.displayAll();
            break;
        }

        case 10:
        {
            UI::header("Wait Time Estimation");
            int est = kitchens.estimateWaitTime();
            if (est >= 0)
                cout << "  Minimum estimated wait time for a new order: " << est << " min\n";
            else
                UI::warning("All kitchens are at full capacity! No slot available.");
            break;
        }

        // ================================================================
        //  MODULE 3: RIDER MANAGEMENT
        // ================================================================
        case 11:
        {
            UI::header("Register New Rider");
            int id = InputHelper::readInt("  Rider ID: ", 1, 999);
            string name = InputHelper::readLine("  Rider Name: ");
            int cap = InputHelper::readInt("  Max Delivery Capacity: ", 1, 10);
            cout << "  Available Locations:\n";
            cityGraph.displayMap();
            int loc = InputHelper::readInt("  Starting Location Node: ", 0, cityGraph.getNodeCount() - 1);
            riderPool.addRider(Rider(id, name, cap, loc));
            UI::success("Rider " + name + " registered at " + cityGraph.getLocationName(loc));
            autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            break;
        }

        case 12:
        {
            UI::header("All Riders");
            riderPool.display();
            break;
        }

        case 13:
        {
            UI::header("Complete Delivery");
            riderPool.display();
            int rid = InputHelper::readInt("  Rider ID who completed delivery: ", 1, 999);
            if (riderPool.completeDelivery(rid))
            {
                UI::success("Delivery completed for Rider #" + to_string(rid));
                autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            }
            else
                UI::error("Could not complete delivery (rider not found or no active deliveries).");
            break;
        }

        case 14:
        {
            UI::header("Toggle Rider Availability");
            riderPool.display();
            int rid = InputHelper::readInt("  Rider ID: ", 1, 999);
            if (riderPool.toggleAvailability(rid))
            {
                UI::success("Availability toggled for Rider #" + to_string(rid));
                autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            }
            else
                UI::error("Rider #" + to_string(rid) + " not found.");
            break;
        }

        // ================================================================
        //  MODULE 4: ROUTE OPTIMIZATION
        // ================================================================
        case 15:
        {
            UI::header("City Map");
            cityGraph.displayMap();
            break;
        }

        case 16:
        {
            UI::header("Find Shortest Route");
            cityGraph.displayMap();
            cout << "\n";
            int src = InputHelper::readInt("  Source Node: ", 0, cityGraph.getNodeCount() - 1);
            int dest = InputHelper::readInt("  Destination Node: ", 0, cityGraph.getNodeCount() - 1);
            Array<int> path;
            int dist = cityGraph.findShortestPath(src, dest, path);
            if (dist >= 0)
            {
                cout << "\n  Shortest Distance: " << dist << " units\n";
                cout << "  Path: ";
                for (int i = 0; i < path.size(); i++)
                {
                    if (i > 0)
                        cout << " -> ";
                    cout << cityGraph.getLocationName(path[i]);
                }
                cout << "\n";
            }
            else
            {
                UI::error("No route available between these locations!");
            }
            break;
        }

        case 17:
        {
            UI::header("Compare Two Routes");
            cityGraph.displayMap();
            cout << "\n";
            int src = InputHelper::readInt("  Source Node: ", 0, cityGraph.getNodeCount() - 1);
            int d1 = InputHelper::readInt("  Destination A Node: ", 0, cityGraph.getNodeCount() - 1);
            int d2 = InputHelper::readInt("  Destination B Node: ", 0, cityGraph.getNodeCount() - 1);
            cityGraph.compareRoutes(src, d1, d2);
            break;
        }

        case 18:
        {
            UI::header("Estimate Delivery Cost");
            cityGraph.displayMap();
            cout << "\n";
            int src = InputHelper::readInt("  Source Node: ", 0, cityGraph.getNodeCount() - 1);
            int dest = InputHelper::readInt("  Destination Node: ", 0, cityGraph.getNodeCount() - 1);
            int rate = InputHelper::readInt("  Cost per distance unit (Rs.): ", 1, 100);
            int cost = cityGraph.estimateDeliveryCost(src, dest, rate);
            if (cost >= 0)
            {
                cout << "\n  Distance: " << cityGraph.shortestDistance(src, dest) << " units\n";
                cout << "  Rate: Rs. " << rate << "/unit\n";
                cout << "  Estimated Cost: Rs. " << cost << "\n";
            }
            else
            {
                UI::error("No route available for cost estimation!");
            }
            break;
        }

        case 19:
        {
            UI::header("Block / Unblock Road");
            cityGraph.displayMap();
            cout << "\n  1. Block a road\n  2. Unblock a road\n";
            int action = InputHelper::readInt("  Select (1-2): ", 1, 2);
            int u = InputHelper::readInt("  Node A: ", 0, cityGraph.getNodeCount() - 1);
            int v = InputHelper::readInt("  Node B: ", 0, cityGraph.getNodeCount() - 1);
            if (action == 1)
            {
                if (cityGraph.blockRoad(u, v))
                {
                    UI::success("Road between " + cityGraph.getLocationName(u) +
                                " and " + cityGraph.getLocationName(v) + " BLOCKED.");
                    autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
                }
                else
                    UI::error("Invalid road.");
            }
            else
            {
                if (cityGraph.unblockRoad(u, v))
                {
                    UI::success("Road between " + cityGraph.getLocationName(u) +
                                " and " + cityGraph.getLocationName(v) + " UNBLOCKED.");
                    autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
                }
                else
                    UI::error("Invalid road.");
            }
            break;
        }

        // ================================================================
        //  MODULE 5: SEARCH & RETRIEVAL
        // ================================================================
        case 20:
        {
            UI::header("Search & Retrieval Engine");
            cout << "  Search by:\n";
            cout << "    1. Order ID\n";
            cout << "    2. Customer Name\n";
            cout << "    3. Item Name\n";
            cout << "    4. Priority\n";
            cout << "    5. Status\n";
            cout << "    6. Customer Category\n";
            cout << "    7. Kitchen Name\n";
            cout << "    8. Rider Name\n";
            int filter = InputHelper::readInt("  Select filter (1-8): ", 1, 8);

            switch (filter)
            {
            case 1:
            {
                int id = InputHelper::readInt("  Order ID: ", 1, 99999);
                OrderRecord *rec = searchEngine.findById(id);
                if (rec)
                {
                    SearchEngine::displayResultsHeader();
                    rec->display();
                }
                else
                    UI::error("Order #" + to_string(id) + " not found in records.");
                break;
            }
            case 2:
            {
                string name = InputHelper::readLine("  Customer Name: ");
                SearchEngine::displayResults(searchEngine.searchByCustomer(name));
                break;
            }
            case 3:
            {
                string item = InputHelper::readLine("  Item Name: ");
                SearchEngine::displayResults(searchEngine.searchByItem(item));
                break;
            }
            case 4:
            {
                cout << "    1. Normal  2. Premium  3. VIP\n";
                int p = InputHelper::readInt("  Select (1-3): ", 1, 3);
                SearchEngine::displayResults(searchEngine.searchByPriority(static_cast<Priority>(p)));
                break;
            }
            case 5:
            {
                cout << "    0. Queued  1. Preparing  2. Ready  3. Dispatched  4. Delivered  5. Cancelled\n";
                int s = InputHelper::readInt("  Select (0-5): ", 0, 5);
                SearchEngine::displayResults(searchEngine.searchByStatus(static_cast<OrderStatus>(s)));
                break;
            }
            case 6:
            {
                string cat = InputHelper::readLine("  Category (Regular/Premium/VIP): ");
                SearchEngine::displayResults(searchEngine.searchByCategory(cat));
                break;
            }
            case 7:
            {
                string kitchen = InputHelper::readLine("  Kitchen Name: ");
                SearchEngine::displayResults(searchEngine.searchByKitchen(kitchen));
                break;
            }
            case 8:
            {
                string rider = InputHelper::readLine("  Rider Name: ");
                SearchEngine::displayResults(searchEngine.searchByRider(rider));
                break;
            }
            }
            break;
        }

        case 21:
        {
            UI::header("All Delivery Records");
            searchEngine.displayAll();
            break;
        }

        // ================================================================
        //  MODULE 6: ORDER TRACKING & UNDO
        // ================================================================
        case 22:
        {
            UI::header("Replay Order Timeline");
            int id = InputHelper::readInt("  Order ID: ", 1, 99999);
            if (!tracker.replayTimeline(id))
                UI::error("No timeline found for Order #" + to_string(id));
            break;
        }

        case 23:
        {
            UI::header("All Tracked Orders");
            cout << "  Tracked: " << tracker.getTrackedCount() << " orders\n\n";
            tracker.displayAllTimelines();
            break;
        }

        case 24:
        {
            UI::header("Undo Last Action");
            if (tracker.undoLastAction(orderQueue))
                autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            else
                UI::warning("Nothing to undo.");
            break;
        }

        case 25:
        {
            UI::header("Undo Stack");
            cout << "  Actions on stack: " << tracker.getUndoCount() << "\n\n";
            tracker.displayUndoStack();
            break;
        }

        // ================================================================
        //  MODULE 7: PERFORMANCE ANALYSIS
        // ================================================================
        case 26:
        {
            PerformanceAnalyzer::runFullAnalysis();
            break;
        }

        // ================================================================
        //  FULL PIPELINE
        // ================================================================
        case 27:
        {
            UI::header("Process Next Order (Full Pipeline)");
            if (orderQueue.isEmpty())
            {
                UI::warning("No orders in the queue to process.");
                break;
            }
            cout << "  Next order to process: #" << orderQueue.peekTop().getId()
                 << " (" << orderQueue.peekTop().getCustomerName() << " - "
                 << orderQueue.peekTop().getItem() << ")\n\n";
            cityGraph.displayMap();
            cout << "\n";
            int dest = InputHelper::readInt("  Delivery Destination Node: ", 0, cityGraph.getNodeCount() - 1);
            pipeline.processNextOrder(dest);
            autoSave(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            break;
        }

        // ================================================================
        //  SAVE & EXIT
        // ================================================================
        case 0:
        {
            FileHandler::saveAll(orderQueue, riderPool, cityGraph, kitchens, searchEngine);
            cout << "\n  Thank you for using FoodExpress! Goodbye.\n\n";
            break;
        }

        default:
            UI::error("Invalid choice. Please try again.");
            break;
        }

        if (choice != 0)
            InputHelper::pause();

    } while (choice != 0);

    return 0;
}
