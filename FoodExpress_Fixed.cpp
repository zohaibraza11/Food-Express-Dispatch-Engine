#include <iostream>
#include <string>
#include <climits>
#include <limits>
using namespace std;

template <typename T>
struct Array {
    T* data;
    int size;
    int cap;

    Array() : data(nullptr), size(0), cap(0) {}

    void push(T val) {
        if (size == cap) {
            cap = (cap == 0) ? 4 : cap * 2;
            T* newData = new T[cap];
            for (int i = 0; i < size; i++) newData[i] = data[i];
            delete[] data;
            data = newData;
        }
        data[size++] = val;
    }

    T& operator[](int i) { return data[i]; }
    const T& operator[](int i) const { return data[i]; }

    ~Array() { delete[] data; }
};
struct Order {
    int    id;
    string customerName;
    string item;
    int    priority;
    int    deadline;
    string status;

    Order() : id(0), priority(0), deadline(0) {}
    Order(int i, string cn, string it, int p, int d)
        : id(i), customerName(cn), item(it),
        priority(p), deadline(d), status("queued") {
    }
};

struct OrderHeap {
    Array<Order> heap;

    int parent(int i) { return (i - 1) / 2; }
    int left(int i) { return 2 * i + 1; }
    int right(int i) { return 2 * i + 2; }

    bool higherPriority(const Order& a, const Order& b) {
        if (a.priority != b.priority) return a.priority > b.priority;
        return a.deadline < b.deadline;
    }

    void swap(int i, int j) {
        Order tmp = heap[i];
        heap[i] = heap[j];
        heap[j] = tmp;
    }

    void heapifyUp(int i) {
        while (i > 0 && higherPriority(heap[i], heap[parent(i)])) {
            swap(i, parent(i));
            i = parent(i);
        }
    }

    void heapifyDown(int i) {
        int n = heap.size;
        while (true) {
            int best = i;
            int l = left(i), r = right(i);
            if (l < n && higherPriority(heap[l], heap[best])) best = l;
            if (r < n && higherPriority(heap[r], heap[best])) best = r;
            if (best == i) break;
            swap(i, best);
            i = best;
        }
    }

    void insert(Order o) {
        heap.push(o);
        heapifyUp(heap.size - 1);
        cout << "[Scheduling] Order #" << o.id << " by " << o.customerName
            << " inserted. Priority=" << o.priority
            << ", Deadline=" << o.deadline << " min\n";
    }

    Order extractTop() {
        if (heap.size == 0) {
            cout << "[Scheduling] No orders in queue!\n";
            return Order();
        }
        Order top = heap[0];
        heap[0] = heap[heap.size - 1];
        heap.size--;
        heapifyDown(0);
        return top;
    }

    void cancelOrder(int orderId) {
        for (int i = 0; i < heap.size; i++) {
            if (heap[i].id == orderId) {
                heap[i].status = "cancelled";
                heap[i].priority = -1;
                heapifyDown(i);
                heap[i] = heap[heap.size - 1];
                heap.size--;
                if (i < heap.size) {
                    heapifyUp(i);
                    heapifyDown(i);
                }
                cout << "[Scheduling] Order #" << orderId << " cancelled.\n";
                return;
            }
        }
        cout << "[Scheduling] Order #" << orderId << " not found.\n";
    }

    void display() {
        if (heap.size == 0) { cout << "  No orders in queue.\n"; return; }
        cout << "  ID  | Customer       | Item            | Priority | Deadline\n";
        cout << "  ----|----------------|-----------------|----------|---------\n";
        for (int i = 0; i < heap.size; i++) {
            string pStr = heap[i].priority == 3 ? "VIP" :
                heap[i].priority == 2 ? "Premium" : "Normal";
            cout << "  " << heap[i].id << "   | " << heap[i].customerName
                << "\t\t | " << heap[i].item
                << "\t | " << pStr
                << "\t | " << heap[i].deadline << " min\n";
        }
    }
};

struct Rider {
    int    id;
    string name;
    int    currentLoad;
    int    capacity;
    int    locationNode;
    bool   available;

    Rider() : id(0), currentLoad(0), capacity(3), locationNode(0), available(true) {}
    Rider(int i, string n, int cap, int loc)
        : id(i), name(n), currentLoad(0),
        capacity(cap), locationNode(loc), available(true) {
    }
};

struct RiderHeap {
    Array<Rider> heap;

    int parent(int i) { return (i - 1) / 2; }
    int left(int i) { return 2 * i + 1; }
    int right(int i) { return 2 * i + 2; }

    void swp(int i, int j) {
        Rider tmp = heap[i]; heap[i] = heap[j]; heap[j] = tmp;
    }

    void heapifyUp(int i) {
        while (i > 0 && heap[i].currentLoad < heap[parent(i)].currentLoad) {
            swp(i, parent(i)); i = parent(i);
        }
    }

    void heapifyDown(int i) {
        int n = heap.size;
        while (true) {
            int best = i, l = left(i), r = right(i);
            if (l < n && heap[l].currentLoad < heap[best].currentLoad) best = l;
            if (r < n && heap[r].currentLoad < heap[best].currentLoad) best = r;
            if (best == i) break;
            swp(i, best); i = best;
        }
    }

    void addRider(Rider r) {
        heap.push(r);
        heapifyUp(heap.size - 1);
        cout << "[Dispatch] Rider " << r.name << " added. Capacity=" << r.capacity << "\n";
    }

    Rider* assignRider() {
        for (int i = 0; i < heap.size; i++) {
            if (heap[i].available && heap[i].currentLoad < heap[i].capacity) {
                heap[i].currentLoad++;
                heapifyDown(i);
                return &heap[i];
            }
        }
        return nullptr;
    }

    void completeDelivery(int riderId) {
        for (int i = 0; i < heap.size; i++) {
            if (heap[i].id == riderId) {
                if (heap[i].currentLoad > 0) heap[i].currentLoad--;
                heapifyUp(i);
                cout << "[Dispatch] Rider #" << riderId
                    << " completed a delivery. Load=" << heap[i].currentLoad << "\n";
                return;
            }
        }
        cout << "[Dispatch] Rider #" << riderId << " not found.\n";
    }

    void display() {
        if (heap.size == 0) { cout << "  No riders registered.\n"; return; }
        cout << "  ID | Name          | Load | Capacity | Available\n";
        cout << "  ---|---------------|------|----------|-----------\n";
        for (int i = 0; i < heap.size; i++) {
            cout << "  " << heap[i].id
                << "  | " << heap[i].name
                << "\t | " << heap[i].currentLoad
                << "    | " << heap[i].capacity
                << "        | " << (heap[i].available ? "Yes" : "No") << "\n";
        }
    }
};

const int MAX_NODES = 10;
const int INF = INT_MAX / 2;

struct CityGraph {
    int  nodes;
    string locationNames[MAX_NODES];
    int  adj[MAX_NODES][MAX_NODES];
    bool blocked[MAX_NODES][MAX_NODES];

    CityGraph() : nodes(0) {
        for (int i = 0; i < MAX_NODES; i++)
            for (int j = 0; j < MAX_NODES; j++) {
                adj[i][j] = (i == j) ? 0 : INF;
                blocked[i][j] = false;
            }
    }

    void addLocation(string name) {
        if (nodes < MAX_NODES) locationNames[nodes++] = name;
    }

    void addRoad(int u, int v, int weight) {
        adj[u][v] = weight;
        adj[v][u] = weight;
    }

    void blockRoad(int u, int v) {
        blocked[u][v] = blocked[v][u] = true;
        cout << "[Route] Road blocked: "
            << locationNames[u] << " <-> " << locationNames[v] << "\n";
    }

    void unblockRoad(int u, int v) {
        blocked[u][v] = blocked[v][u] = false;
        cout << "[Route] Road unblocked: "
            << locationNames[u] << " <-> " << locationNames[v] << "\n";
    }

    void dijkstra(int src, int dist[], int prev[]) {
        bool visited[MAX_NODES] = { false };
        for (int i = 0; i < nodes; i++) { dist[i] = INF; prev[i] = -1; }
        dist[src] = 0;

        for (int iter = 0; iter < nodes; iter++) {
            int u = -1;
            for (int i = 0; i < nodes; i++)
                if (!visited[i] && (u == -1 || dist[i] < dist[u])) u = i;

            if (u == -1 || dist[u] == INF) break;
            visited[u] = true;

            for (int v = 0; v < nodes; v++) {
                if (adj[u][v] != INF && !blocked[u][v] && !visited[v]) {
                    int newDist = dist[u] + adj[u][v];
                    if (newDist < dist[v]) {
                        dist[v] = newDist;
                        prev[v] = u;
                    }
                }
            }
        }
    }

    void printShortestPath(int src, int dest) {
        int dist[MAX_NODES], prev[MAX_NODES];
        dijkstra(src, dist, prev);

        if (dist[dest] == INF) {
            cout << "[Route] No path from " << locationNames[src]
                << " to " << locationNames[dest] << " (all roads blocked?)\n";
            return;
        }

        Array<int> path;
        for (int at = dest; at != -1; at = prev[at]) path.push(at);

        cout << "[Route] Shortest path: ";
        for (int i = path.size - 1; i >= 0; i--) {
            cout << locationNames[path[i]];
            if (i > 0) cout << " -> ";
        }
        cout << "\n[Route] Total distance/time: " << dist[dest] << " units\n";
    }

    void displayMap() {
        cout << "  City Map Locations:\n";
        for (int i = 0; i < nodes; i++)
            cout << "  [" << i << "] " << locationNames[i] << "\n";
        cout << "\n  Adjacency Matrix (INF = no direct road):\n  \t";
        for (int i = 0; i < nodes; i++) cout << i << "\t";
        cout << "\n";
        for (int i = 0; i < nodes; i++) {
            cout << "  " << i << "\t";
            for (int j = 0; j < nodes; j++) {
                if (adj[i][j] == INF) cout << "INF\t";
                else cout << adj[i][j] << (blocked[i][j] ? "X\t" : "\t");
            }
            cout << "\n";
        }
    }
};

struct QueueNode {
    int orderId;
    string item;
    int prepTime;
    QueueNode* next;
    QueueNode(int id, string it, int pt)
        : orderId(id), item(it), prepTime(pt), next(nullptr) {
    }
};

struct KitchenQueue {
    QueueNode* front;
    QueueNode* rear;
    int size;
    int maxCapacity;

    KitchenQueue() : front(nullptr), rear(nullptr), size(0), maxCapacity(5) {}

    void enqueue(int orderId, string item, int prepTime) {
        QueueNode* node = new QueueNode(orderId, item, prepTime);
        if (!rear) { front = rear = node; }
        else { rear->next = node; rear = node; }
        size++;
    }

    void dequeue() {
        if (!front) return;
        QueueNode* tmp = front;
        front = front->next;
        if (!front) rear = nullptr;
        delete tmp;
        size--;
    }

    bool isFull() { return size >= maxCapacity; }
    bool isEmpty() { return size == 0; }

    int totalWaitTime() {
        int total = 0;
        QueueNode* cur = front;
        while (cur) { total += cur->prepTime; cur = cur->next; }
        return total;
    }

    void display(string kitchenName) {
        cout << "  Kitchen: " << kitchenName
            << " | Orders: " << size << "/" << maxCapacity
            << " | Est. Wait: " << totalWaitTime() << " min"
            << (isFull() ? " [OVERLOADED]" : "") << "\n";
        QueueNode* cur = front;
        int pos = 1;
        while (cur) {
            cout << "    " << pos++ << ". Order #" << cur->orderId
                << " - " << cur->item
                << " (" << cur->prepTime << " min)\n";
            cur = cur->next;
        }
    }

    ~KitchenQueue() {
        while (front) dequeue();
    }
};

struct KitchenSystem {
    static const int MAX_KITCHENS = 5;
    KitchenQueue kitchens[MAX_KITCHENS];
    string       kitchenNames[MAX_KITCHENS];
    int          numKitchens;

    KitchenSystem() : numKitchens(0) {}

    void addKitchen(string name, int cap) {
        if (numKitchens < MAX_KITCHENS) {
            kitchenNames[numKitchens] = name;
            kitchens[numKitchens].maxCapacity = cap;
            numKitchens++;
            cout << "[Kitchen] Added: " << name << " (capacity=" << cap << ")\n";
        }
    }

    void distributeOrder(int orderId, string item, int prepTime) {
        int best = -1;
        for (int i = 0; i < numKitchens; i++) {
            if (!kitchens[i].isFull()) {
                if (best == -1 || kitchens[i].size < kitchens[best].size)
                    best = i;
            }
        }
        if (best == -1) {
            cout << "[Kitchen] ALL KITCHENS FULL! Order #" << orderId << " cannot be queued.\n";
            return;
        }
        kitchens[best].enqueue(orderId, item, prepTime);
        cout << "[Kitchen] Order #" << orderId << " (" << item << ")"
            << " assigned to " << kitchenNames[best]
            << ". Queue=" << kitchens[best].size
            << ", Wait=" << kitchens[best].totalWaitTime() << " min\n";
    }

    void completeOrder(int kitchenIdx) {
        if (kitchenIdx < 0 || kitchenIdx >= numKitchens) {
            cout << "[Kitchen] Invalid kitchen index.\n"; return;
        }
        if (kitchens[kitchenIdx].isEmpty()) {
            cout << "[Kitchen] " << kitchenNames[kitchenIdx] << " has no orders.\n"; return;
        }
        cout << "[Kitchen] Order completed at " << kitchenNames[kitchenIdx] << "\n";
        kitchens[kitchenIdx].dequeue();
    }

    void displayAll() {
        for (int i = 0; i < numKitchens; i++)
            kitchens[i].display(kitchenNames[i]);
    }
};

void seedData(OrderHeap& oHeap, RiderHeap& rHeap,
    CityGraph& city, KitchenSystem& ks) {
    oHeap.insert(Order(101, "Ali", "Biryani", 3, 15));
    oHeap.insert(Order(102, "Sara", "Pizza", 1, 30));
    oHeap.insert(Order(103, "Usman", "Burger", 2, 20));
    oHeap.insert(Order(104, "Ayesha", "Pasta", 3, 10));
    oHeap.insert(Order(105, "Bilal", "Shawarma", 1, 25));

    rHeap.addRider(Rider(1, "Kamran", 3, 0));
    rHeap.addRider(Rider(2, "Zubair", 2, 1));
    rHeap.addRider(Rider(3, "Hassan", 4, 2));

    city.addLocation("Restaurant Hub");
    city.addLocation("Block A");
    city.addLocation("Block B");
    city.addLocation("Block C");
    city.addLocation("City Center");
    city.addRoad(0, 1, 5);
    city.addRoad(0, 2, 8);
    city.addRoad(1, 3, 6);
    city.addRoad(2, 3, 3);
    city.addRoad(3, 4, 4);
    city.addRoad(1, 4, 10);
    city.addRoad(2, 4, 7);

    ks.addKitchen("Kitchen Alpha", 4);
    ks.addKitchen("Kitchen Beta", 3);
    ks.addKitchen("Kitchen Gamma", 5);
    ks.distributeOrder(101, "Biryani", 20);
    ks.distributeOrder(102, "Pizza", 15);
    ks.distributeOrder(103, "Burger", 10);
    ks.distributeOrder(104, "Pasta", 18);
    ks.distributeOrder(105, "Shawarma", 12);
}

// ── FIX: clears entire input buffer before waiting for Enter ──
void pause() {
    cout << "\n  Press Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void header(const string& title) {
    cout << "\n============================================================\n";
    cout << "  " << title << "\n";
    cout << "============================================================\n";
}

void orderSchedulingMenu(OrderHeap& oHeap) {
    int ch;
    do {
        header("MODULE 1: Dynamic Order Scheduling");
        cout << "  1. View Order Queue\n";
        cout << "  2. Add New Order\n";
        cout << "  3. Process Next Order (Extract Top)\n";
        cout << "  4. Cancel Order\n";
        cout << "  0. Back\n";
        cout << "  Choice: "; cin >> ch;

        if (ch == 1) {
            header("Current Order Queue");
            oHeap.display();
        }
        else if (ch == 2) {
            int id, pr, dl;
            string cn, it;
            cout << "  Order ID: "; cin >> id;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  Customer Name: "; getline(cin, cn);
            cout << "  Item: "; getline(cin, it);
            cout << "  Priority (1=Normal, 2=Premium, 3=VIP): "; cin >> pr;
            cout << "  Deadline (minutes): "; cin >> dl;
            oHeap.insert(Order(id, cn, it, pr, dl));
        }
        else if (ch == 3) {
            Order o = oHeap.extractTop();
            if (o.id != 0) {
                cout << "[Scheduling] Processing Order #" << o.id
                    << " for " << o.customerName
                    << " | Item: " << o.item << "\n";
            }
        }
        else if (ch == 4) {
            int id;
            cout << "  Enter Order ID to cancel: "; cin >> id;
            oHeap.cancelOrder(id);
        }
        if (ch != 0) pause();
    } while (ch != 0);
}

void riderDispatchMenu(RiderHeap& rHeap) {
    int ch;
    do {
        header("MODULE 2: Rider Dispatch Optimization");
        cout << "  1. View All Riders\n";
        cout << "  2. Add Rider\n";
        cout << "  3. Assign Best Rider for Delivery\n";
        cout << "  4. Complete Delivery (update load)\n";
        cout << "  0. Back\n";
        cout << "  Choice: "; cin >> ch;

        if (ch == 1) {
            header("Rider Status");
            rHeap.display();
        }
        else if (ch == 2) {
            int id, cap, loc;
            string nm;
            cout << "  Rider ID: "; cin >> id;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  Name: "; getline(cin, nm);
            cout << "  Capacity: "; cin >> cap;
            cout << "  Location Node: "; cin >> loc;
            rHeap.addRider(Rider(id, nm, cap, loc));
        }
        else if (ch == 3) {
            Rider* r = rHeap.assignRider();
            if (r)
                cout << "[Dispatch] Assigned Rider: " << r->name
                << " (ID=" << r->id
                << ", Load=" << r->currentLoad << ")\n";
            else
                cout << "[Dispatch] No available rider!\n";
        }
        else if (ch == 4) {
            int id;
            cout << "  Rider ID: "; cin >> id;
            rHeap.completeDelivery(id);
        }
        if (ch != 0) pause();
    } while (ch != 0);
}

void routeOptMenu(CityGraph& city) {
    int ch;
    do {
        header("MODULE 3: Route Optimization (Dijkstra)");
        cout << "  1. View City Map\n";
        cout << "  2. Find Shortest Path\n";
        cout << "  3. Block a Road\n";
        cout << "  4. Unblock a Road\n";
        cout << "  5. Add Location\n";
        cout << "  6. Add Road\n";
        cout << "  0. Back\n";
        cout << "  Choice: "; cin >> ch;

        if (ch == 1) {
            header("City Map");
            city.displayMap();
        }
        else if (ch == 2) {
            int src, dst;
            city.displayMap();
            cout << "  Source node: "; cin >> src;
            cout << "  Destination node: "; cin >> dst;
            city.printShortestPath(src, dst);
        }
        else if (ch == 3) {
            int u, v;
            cout << "  Node 1: "; cin >> u;
            cout << "  Node 2: "; cin >> v;
            city.blockRoad(u, v);
        }
        else if (ch == 4) {
            int u, v;
            cout << "  Node 1: "; cin >> u;
            cout << "  Node 2: "; cin >> v;
            city.unblockRoad(u, v);
        }
        else if (ch == 5) {
            string nm;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  Location name: "; getline(cin, nm);
            city.addLocation(nm);
            cout << "[Route] Added location: " << nm
                << " (Node " << city.nodes - 1 << ")\n";
        }
        else if (ch == 6) {
            int u, v, w;
            cout << "  Node 1: "; cin >> u;
            cout << "  Node 2: "; cin >> v;
            cout << "  Distance/Weight: "; cin >> w;
            city.addRoad(u, v, w);
            cout << "[Route] Road added.\n";
        }
        if (ch != 0) pause();
    } while (ch != 0);
}

void kitchenMenu(KitchenSystem& ks) {
    int ch;
    do {
        header("MODULE 4: Kitchen Load Analysis");
        cout << "  1. View All Kitchens\n";
        cout << "  2. Distribute New Order to Kitchen\n";
        cout << "  3. Complete Order at a Kitchen\n";
        cout << "  0. Back\n";
        cout << "  Choice: "; cin >> ch;

        if (ch == 1) {
            header("Kitchen Status");
            ks.displayAll();
        }
        else if (ch == 2) {
            int id, pt;
            string it;
            cout << "  Order ID: "; cin >> id;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  Item: "; getline(cin, it);
            cout << "  Prep time (min): "; cin >> pt;
            ks.distributeOrder(id, it, pt);
        }
        else if (ch == 3) {
            int ki;
            cout << "  Kitchen index (0-" << ks.numKitchens - 1 << "): "; cin >> ki;
            ks.completeOrder(ki);
        }
        if (ch != 0) pause();
    } while (ch != 0);
}

int main() {
    OrderHeap     orderQueue;
    RiderHeap     riderPool;
    CityGraph     city;
    KitchenSystem ks;

    cout << "\n  Loading demo data...\n";
    seedData(orderQueue, riderPool, city, ks);
    cout << "  Demo data loaded!\n";
    pause();

    int choice;
    do {
        header("FoodExpress Dispatch Optimization Engine");
        cout << "  1. Dynamic Order Scheduling\n";
        cout << "  2. Rider Dispatch Optimization\n";
        cout << "  3. Route Optimization\n";
        cout << "  4. Kitchen Load Analysis\n";
        cout << "  0. Exit\n";
        cout << "  Choice: "; cin >> choice;

        switch (choice) {
        case 1: orderSchedulingMenu(orderQueue); break;
        case 2: riderDispatchMenu(riderPool);    break;
        case 3: routeOptMenu(city);              break;
        case 4: kitchenMenu(ks);                 break;
        case 0: cout << "\n  Goodbye!\n";        break;
        default: cout << "  Invalid choice.\n";  break;
        }
    } while (choice != 0);

    return 0;
}
