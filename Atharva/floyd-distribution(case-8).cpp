#include <bits/stdc++.h>
using namespace std;

const double INF = 1e18;

struct Location {
    int id;
    string name;
    string type; // warehouse, shop, hub
};

struct RouteResult {
    double cost;
    vector<int> path;
};

class DenseGraph {
public:
    DenseGraph(int n = 0) { init(n); }

    void init(int n) {
        N = n;
        dist.assign(N, vector<double>(N, INF));
        next.assign(N, vector<int>(N, -1));
        for (int i = 0; i < N; ++i) {
            dist[i][i] = 0.0;
            next[i][i] = i;
        }
    }

    void setEdge(int u, int v, double w) {
        if (u < 0 || u >= N || v < 0 || v >= N) return;
        dist[u][v] = w;
        next[u][v] = v;
    }

    void removeEdge(int u, int v) {
        if (u < 0 || u >= N || v < 0 || v >= N) return;
        dist[u][v] = INF;
        next[u][v] = -1;
    }

    int size() const { return N; }

    const vector<vector<double>>& distances() const { return dist; }
    const vector<vector<int>>& nextHop() const { return next; }

    RouteResult reconstructPath(int s, int t) const {
        RouteResult res;
        if (s < 0 || s >= N || t < 0 || t >= N) { res.cost = INF; return res; }
        if (next[s][t] == -1) { res.cost = INF; return res; }
        int u = s;
        res.path.push_back(u);
        while (u != t) {
            u = next[u][t];
            if (u == -1) { res.cost = INF; res.path.clear(); return res; }
            res.path.push_back(u);
        }
        res.cost = dist[s][t];
        return res;
    }

private:
    int N = 0;
    vector<vector<double>> dist;
    vector<vector<int>> next;
};

// Floyd-Warshall with path reconstruction
struct FWResult {
    vector<vector<double>> dist;
    vector<vector<int>> next;
};

class FloydWarshall {
public:
    static FWResult run(const DenseGraph &g) {
        int n = g.size();
        FWResult res;
        res.dist = g.distances();
        res.next = g.nextHop();

        for (int k = 0; k < n; ++k) {
            for (int i = 0; i < n; ++i) {
                if (res.dist[i][k] >= INF) continue;
                for (int j = 0; j < n; ++j) {
                    if (res.dist[k][j] >= INF) continue;
                    double nd = res.dist[i][k] + res.dist[k][j];
                    if (nd < res.dist[i][j]) {
                        res.dist[i][j] = nd;
                        res.next[i][j] = res.next[i][k];
                    }
                }
            }
        }
        return res;
    }

    static vector<int> buildPath(const vector<vector<int>> &next, int s, int t) {
        vector<int> path;
        if (s < 0 || t < 0) return path;
        if (next[s][t] == -1) return path;
        int u = s;
        path.push_back(u);
        while (u != t) {
            u = next[u][t];
            if (u == -1) { path.clear(); return path; }
            path.push_back(u);
        }
        return path;
    }
};

// Aroha Nagar distribution model
class DistributionSystem {
public:
    void addLocation(const string &name, const string &type) {
        int id = (int)locations.size();
        locations.push_back({id, name, type});
    }

    bool setRoute(int u, int v, double cost) {
        if (!valid(u) || !valid(v)) return false;
        graph.setEdge(u, v, cost);
        return true;
    }

    bool removeRoute(int u, int v) {
        if (!valid(u) || !valid(v)) return false;
        graph.removeEdge(u, v);
        return true;
    }

    void allocateGraph() {
        int n = (int)locations.size();
        graph.init(n);
    }

    void computeAllPairs() {
        fw = FloydWarshall::run(graph);
    }

    RouteResult shortestPath(int s, int t) {
        RouteResult r;
        if (!valid(s) || !valid(t)) { r.cost = INF; return r; }
        if (fw.dist.empty()) computeAllPairs();
        r.cost = fw.dist[s][t];
        r.path = FloydWarshall::buildPath(fw.next, s, t);
        return r;
    }

    vector<vector<double>> distanceMatrix() {
        if (fw.dist.empty()) computeAllPairs();
        return fw.dist;
    }

    int countLocations() const { return (int)locations.size(); }

    const Location& getLocation(int id) const { return locations.at(id); }

    vector<Location> listLocations() const { return locations; }

    // find best warehouse for each shop (min cost)
    vector<pair<int,int>> bestWarehouseForShops() {
        vector<pair<int,int>> res;
        int n = countLocations();
        if (fw.dist.empty()) computeAllPairs();
        for (int shop = 0; shop < n; ++shop) {
            if (!isType(shop, "shop")) continue;
            double bestCost = INF;
            int bestWid = -1;
            for (int w = 0; w < n; ++w) {
                if (!isType(w, "warehouse")) continue;
                if (fw.dist[w][shop] < bestCost) {
                    bestCost = fw.dist[w][shop];
                    bestWid = w;
                }
            }
            res.emplace_back(shop, bestWid);
        }
        return res;
    }

    bool isType(int id, const string &t) const {
        if (!valid(id)) return false;
        return locations[id].type == t;
    }

    void exportMatrixCSV(const string &filename) {
        ofstream out(filename);
        if (!out.is_open()) return;
        int n = countLocations();
        if (fw.dist.empty()) computeAllPairs();
        out << "from,to,cost\n";
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                double c = fw.dist[i][j];
                if (c >= INF/2) out << i << "," << j << ",INF\n";
                else out << i << "," << j << "," << fixed << setprecision(3) << c << "\n";
            }
        }
        out.close();
    }

    void exportBestWarehouses(const string &filename) {
        ofstream out(filename);
        if (!out.is_open()) return;
        auto v = bestWarehouseForShops();
        out << "shopId,shopName,warehouseId,warehouseName\n";
        for (auto &p : v) {
            int shop = p.first;
            int wid = p.second;
            string wname = (wid == -1 ? "NONE" : locations[wid].name);
            out << shop << "," << locations[shop].name << "," << wid << "," << wname << "\n";
        }
        out.close();
    }

    void randomPopulateDefault() {
        addLocation("Central Warehouse", "warehouse");
        addLocation("North Warehouse", "warehouse");
        addLocation("East Warehouse", "warehouse");
        addLocation("Central Market", "shop");
        addLocation("East Mall", "shop");
        addLocation("West Bazaar", "shop");
        addLocation("South Depot", "hub");
        // allocate graph with current count
        allocateGraph();
        // set routes (directed) with symmetric costs
        setRoute(0,3,5.0); setRoute(3,0,5.0);
        setRoute(1,3,7.0); setRoute(3,1,7.0);
        setRoute(2,4,4.0); setRoute(4,2,4.0);
        setRoute(0,5,8.5); setRoute(5,0,8.5);
        setRoute(1,5,6.0); setRoute(5,1,6.0);
        setRoute(2,3,9.0); setRoute(3,2,9.0);
        setRoute(6,0,3.0); setRoute(0,6,3.0);
        setRoute(6,1,3.5); setRoute(1,6,3.5);
        computeAllPairs();
    }

private:
    bool valid(int id) const { return id >= 0 && id < (int)locations.size(); }
    vector<Location> locations;
    DenseGraph graph;
    FWResult fw;
};

// Utilities for CLI
static string readLine(const string &prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    // trim
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static int readIntDefault(const string &prompt, int dflt) {
    string s = readLine(prompt);
    if (s.empty()) return dflt;
    try { return stoi(s); } catch (...) { return dflt; }
}

static double readDoubleDefault(const string &prompt, double dflt) {
    string s = readLine(prompt);
    if (s.empty()) return dflt;
    try { return stod(s); } catch (...) { return dflt; }
}

static void printPath(const vector<int> &path, const DistributionSystem &ds) {
    if (path.empty()) {
        cout << "No path\n";
        return;
    }
    for (size_t i = 0; i < path.size(); ++i) {
        int id = path[i];
        cout << ds.getLocation(id).name;
        if (i + 1 < path.size()) cout << " -> ";
    }
    cout << "\n";
}

// CLI driver
class DistributionCLI {
public:
    DistributionCLI(DistributionSystem &ds) : system(ds) {}

    void run() {
        printHeader();
        bool stop = false;
        while (!stop) {
            printMenu();
            string cmd = readLine("Choice> ");
            if (cmd == "1") actionListLocations();
            else if (cmd == "2") actionAddLocation();
            else if (cmd == "3") actionSetRoute();
            else if (cmd == "4") actionRemoveRoute();
            else if (cmd == "5") actionComputeAllPairs();
            else if (cmd == "6") actionQueryPath();
            else if (cmd == "7") actionBestWarehouses();
            else if (cmd == "8") actionExportMatrix();
            else if (cmd == "9") actionExportBest();
            else if (cmd == "r") actionRandomPopulate();
            else if (cmd == "q" || cmd == "quit") stop = true;
            else cout << "Unknown option\n";
        }
        cout << "Exiting distribution CLI.\n";
    }

private:
    DistributionSystem &system;

    void printHeader() {
        cout << "Aroha Nagar — Distribution Route Planner\n";
    }

    void printMenu() {
        cout << "\nOptions:\n";
        cout << " 1) List locations\n";
        cout << " 2) Add location\n";
        cout << " 3) Set route cost\n";
        cout << " 4) Remove route\n";
        cout << " 5) Compute all-pairs shortest paths\n";
        cout << " 6) Query shortest path and cost\n";
        cout << " 7) Best warehouse for each shop\n";
        cout << " 8) Export distance matrix to CSV\n";
        cout << " 9) Export best warehouse assignment to CSV\n";
        cout << " r) Random populate default example\n";
        cout << " q) Quit\n";
    }

    void actionListLocations() {
        auto v = system.listLocations();
        cout << "Locations (" << v.size() << "):\n";
        for (auto &loc : v) {
            cout << loc.id << " | " << loc.name << " | " << loc.type << "\n";
        }
    }

    void actionAddLocation() {
        string name = readLine("Name: ");
        if (name.empty()) { cout << "Name required\n"; return; }
        string type = readLine("Type (warehouse/shop/hub): ");
        if (type.empty()) type = "hub";
        system.addLocation(name, type);
        system.allocateGraph();
        cout << "Added location. Total now: " << system.countLocations() << "\n";
    }

    void actionSetRoute() {
        int u = readIntDefault("From id: ", -1);
        int v = readIntDefault("To id: ", -1);
        if (u < 0 || v < 0) { cout << "Invalid ids\n"; return; }
        double cost = readDoubleDefault("Cost: ", 1.0);
        if (system.setRoute(u, v, cost)) {
            cout << "Route set.\n";
            system.computeAllPairs();
        } else cout << "Failed to set route\n";
    }

    void actionRemoveRoute() {
        int u = readIntDefault("From id: ", -1);
        int v = readIntDefault("To id: ", -1);
        if (system.removeRoute(u, v)) {
            cout << "Route removed.\n";
            system.computeAllPairs();
        } else cout << "Failed to remove route or invalid ids\n";
    }

    void actionComputeAllPairs() {
        system.computeAllPairs();
        cout << "All-pairs shortest paths computed.\n";
    }

    void actionQueryPath() {
        int s = readIntDefault("Source id: ", -1);
        int t = readIntDefault("Target id: ", -1);
        auto res = system.shortestPath(s, t);
        if (res.cost >= INF/2) {
            cout << "No path or unreachable\n";
            return;
        }
        cout << "Cost: " << fixed << setprecision(3) << res.cost << " Path: ";
        printPath(res.path, system);
    }

    void actionBestWarehouses() {
        auto assignments = system.bestWarehouseForShops();
        cout << "Shop -> best warehouse\n";
        for (auto &p : assignments) {
            int shop = p.first;
            int wid = p.second;
            cout << system.getLocation(shop).name << " -> ";
            if (wid == -1) cout << "NONE\n"; else cout << system.getLocation(wid).name << "\n";
        }
    }

    void actionExportMatrix() {
        string fn = readLine("Filename (default=matrix.csv): ");
        if (fn.empty()) fn = "matrix.csv";
        system.exportMatrixCSV(fn);
        cout << "Exported matrix\n";
    }

    void actionExportBest() {
        string fn = readLine("Filename (default=best.csv): ");
        if (fn.empty()) fn = "best.csv";
        system.exportBestWarehouses(fn);
        cout << "Exported best assignments\n";
    }

    void actionRandomPopulate() {
        system.randomPopulateDefault();
        cout << "Random default populate done and matrix computed.\n";
    }
};

// Main
int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    DistributionSystem ds;
    DistributionCLI cli(ds);

    if (argc >= 2 && string(argv[1]) == "--demo") {
        ds.randomPopulateDefault();
        ds.computeAllPairs();
        ds.exportMatrixCSV("demo_matrix.csv");
        ds.exportBestWarehouses("demo_best.csv");
        cout << "Demo exports written.\n";
        return 0;
    }

    cli.run();
    return 0;
}
