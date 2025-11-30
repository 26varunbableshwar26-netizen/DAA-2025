#include <bits/stdc++.h>
using namespace std;

// ========================================================================
// Disjoint Set Union (Union-Find)
// ========================================================================

class DSU {
public:
    DSU(int n = 0) { init(n); }

    void init(int n) {
        parent.resize(n+1);
        rnk.resize(n+1, 0);
        for (int i = 0; i <= n; i++) parent[i] = i;
    }

    int find(int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }

    bool unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return false;
        if (rnk[a] < rnk[b]) swap(a,b);
        parent[b] = a;
        if (rnk[a] == rnk[b]) rnk[a]++;
        return true;
    }

private:
    vector<int> parent;
    vector<int> rnk;
};

// ========================================================================
// Graph Structures for Regions
// ========================================================================
// Node: region in Aroha Nagar
// Edge: potential road or transport cost between two regions
// Kruskal MST will pick the cheapest edges needed to connect all regions
// ========================================================================

struct Edge {
    int u, v;
    double w;
};

struct Region {
    int id;
    string name;
    string zone;
    double x, y; // coordinate-like representation for distance modeling
};

class RegionMap {
public:
    void addRegion(int id, const string &name, const string &zone, double x, double y) {
        regions[id] = Region{id, name, zone, x, y};
    }

    vector<Region> list() const {
        vector<Region> v;
        for (auto &p : regions) v.push_back(p.second);
        return v;
    }

    bool exists(int id) const {
        return regions.count(id);
    }

    Region get(int id) const {
        return regions.at(id);
    }

private:
    unordered_map<int, Region> regions;
};

// ========================================================================
// Graph and Kruskal MST
// ========================================================================

class WarehouseGraph {
public:
    WarehouseGraph(int n = 0) : N(n) { edges.clear(); }

    void init(int n) {
        N = n;
        edges.clear();
    }

    void addEdge(int u, int v, double w) {
        edges.push_back({u,v,w});
    }

    vector<Edge> computeMST() {
        vector<Edge> res;
        DSU dsu(N);
        sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b){
            return a.w < b.w;
        });
        for (auto &e : edges) {
            if (dsu.unite(e.u, e.v)) {
                res.push_back(e);
            }
        }
        return res;
    }

private:
    int N;
    vector<Edge> edges;
};

// ========================================================================
// Simulator to build a distance graph between regions
// ========================================================================

class WarehouseSimulator {
public:
    WarehouseSimulator(RegionMap &rm) : reg(rm) {
        rng.seed(time(nullptr));
    }

    // Build a complete graph with approximate costs based on distance + random factors
    WarehouseGraph generateGraph() {
        auto all = reg.list();
        int n = all.size();
        WarehouseGraph G(n);

        for (int i = 0; i < n; i++) {
            for (int j = i+1; j < n; j++) {
                double w = baseCost(all[i], all[j]);
                G.addEdge(all[i].id, all[j].id, w);
            }
        }
        return G;
    }

private:
    RegionMap &reg;
    mt19937 rng;

    double baseCost(const Region &a, const Region &b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        double dist = sqrt(dx*dx + dy*dy);
        uniform_real_distribution<double> noise(0.9, 1.25);
        return dist * noise(rng);
    }
};

// ========================================================================
// Warehouse Planning Output + Export
// ========================================================================

class WarehouseExporter {
public:
    static bool exportMST(const vector<Edge> &mst, const string &filename) {
        ofstream out(filename);
        if (!out.is_open()) return false;
        out << "u,v,weight\n";
        for (auto &e : mst) {
            out << e.u << "," << e.v << "," << fixed << setprecision(3) << e.w << "\n";
        }
        out.close();
        return true;
    }

    static bool exportFullGraph(const WarehouseGraph &G, const vector<Edge> &edges, const string &filename) {
        ofstream out(filename);
        if (!out.is_open()) return false;
        out << "u,v,weight\n";
        for (auto &e : edges)
            out << e.u << "," << e.v << "," << fixed << setprecision(3) << e.w << "\n";
        out.close();
        return true;
    }
};


class WarehouseReport {
public:
    static string summary(const vector<Edge> &mst) {
        double total = 0.0;
        for (auto &e : mst) total += e.w;

        ostringstream ss;
        ss << "Warehouse MST Summary\n";
        ss << "Edges used: " << mst.size() << "\n";
        ss << "Total transport cost: " << fixed << setprecision(3) << total << "\n";
        ss << "Connections:\n";
        for (auto &e : mst) {
            ss << "  " << e.u << " <-> " << e.v 
               << "  cost=" << fixed << setprecision(3) << e.w << "\n";
        }
        return ss.str();
    }
};

#include <iomanip>
#include <sstream>
#include <fstream>
#include <random>

// =============================================================
// Simple CLI helpers
// =============================================================

static string readTrim(const string &prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) return string();
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return string();
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static int readIntDefault(const string &prompt, int dflt) {
    string s = readTrim(prompt);
    if (s.empty()) return dflt;
    try { return stoi(s); } catch (...) { return dflt; }
}

static double readDoubleDefault(const string &prompt, double dflt) {
    string s = readTrim(prompt);
    if (s.empty()) return dflt;
    try { return stod(s); } catch (...) { return dflt; }
}

// =============================================================
// Interactive Warehouse Planner
// =============================================================

class WarehousePlanner {
public:
    WarehousePlanner(RegionMap &rm) : reg(rm) { }

    void run() {
        bool stop = false;
        while (!stop) {
            printMenu();
            string cmd = readTrim("Choice> ");
            if (cmd == "1") actionListRegions();
            else if (cmd == "2") actionAddRegion();
            else if (cmd == "3") actionGenerateGraphAndMST();
            else if (cmd == "4") actionExportMST();
            else if (cmd == "5") actionShowReport();
            else if (cmd == "6") actionRandomizeRegions();
            else if (cmd == "7") actionBulkSimulate();
            else if (cmd == "q" || cmd == "quit") stop = true;
            else cout << "Unknown option\n";
        }
    }

private:
    RegionMap &reg;
    vector<Edge> lastGraphEdges;
    vector<Edge> lastMST;

    void printMenu() {
        cout << "\nWarehouse Planner — Options\n";
        cout << " 1) List regions\n";
        cout << " 2) Add region\n";
        cout << " 3) Generate graph and compute MST\n";
        cout << " 4) Export last MST to CSV\n";
        cout << " 5) Show last MST report\n";
        cout << " 6) Randomize region coordinates\n";
        cout << " 7) Bulk simulation: generate multiple graphs and average MST cost\n";
        cout << " q) Quit\n";
    }

    void actionListRegions() {
        auto all = reg.list();
        cout << "Regions (" << all.size() << "):\n";
        for (auto &r : all) {
            cout << " id=" << r.id << " name=" << r.name << " zone=" << r.zone
                 << " coord=(" << fixed << setprecision(3) << r.x << "," << r.y << ")\n";
        }
    }

    void actionAddRegion() {
        int id = readIntDefault("Region id (int): ", -1);
        if (id < 0) { cout << "Invalid id\n"; return; }
        string name = readTrim("Region name: ");
        string zone = readTrim("Zone: ");
        double x = readDoubleDefault("X coordinate: ", 0.0);
        double y = readDoubleDefault("Y coordinate: ", 0.0);
        if (reg.exists(id)) {
            cout << "Region id exists — overwriting.\n";
        }
        reg.addRegion(id, name, zone, x, y);
        cout << "Added region " << id << "\n";
    }

    void actionGenerateGraphAndMST() {
        WarehouseSimulator sim(reg);
        WarehouseGraph G = sim.generateGraph();

        // Extract edges by regenerating (WarehouseGraph does not expose edges directly).
        // To get edges cheaply, we will re-create them similarly here.
        // Use region list
        auto all = reg.list();
        lastGraphEdges.clear();
        for (size_t i = 0; i < all.size(); ++i) {
            for (size_t j = i+1; j < all.size(); ++j) {
                double dx = all[i].x - all[j].x;
                double dy = all[i].y - all[j].y;
                double dist = sqrt(dx*dx + dy*dy);
                // same noise approach as simulator: use deterministic noise seed
                double w = dist; // keep deterministic for display; simulator's noise omitted here
                lastGraphEdges.push_back({all[i].id, all[j].id, w});
            }
        }

        // Build a graph object for Kruskal execution (we'll pass edges manually)
        // Sorting edges by weight then DSU unite logic:
        sort(lastGraphEdges.begin(), lastGraphEdges.end(), [](const Edge &a, const Edge &b){ return a.w < b.w; });
        DSU dsu((int)all.size()+5);
        lastMST.clear();
        for (auto &e : lastGraphEdges) {
            if (dsu.unite(e.u, e.v)) lastMST.push_back(e);
        }
        cout << "MST computed. Edges in MST: " << lastMST.size() << "\n";
        cout << WarehouseReport::summary(lastMST) << "\n";
    }

    void actionExportMST() {
        if (lastMST.empty()) { cout << "No MST present. Generate first.\n"; return; }
        string fname = readTrim("Filename to export (default=mst.csv): ");
        if (fname.empty()) fname = "mst.csv";
        bool ok = WarehouseExporter::exportMST(lastMST, fname);
        cout << "Export " << (ok ? "OK" : "FAILED") << " -> " << fname << "\n";
    }

    void actionShowReport() {
        if (lastMST.empty()) { cout << "No MST present. Generate first.\n"; return; }
        cout << WarehouseReport::summary(lastMST) << "\n";
    }

    void actionRandomizeRegions() {
        double radius = readDoubleDefault("Randomization radius (units): ", 5.0);
        randomDeviceRd(radius);
        cout << "Randomized region coordinates within radius " << radius << "\n";
    }

    void randomDeviceRd(double radius) {
        auto all = reg.list();
        mt19937 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
        uniform_real_distribution<double> off(-radius, radius);
        for (auto &r : all) {
            reg.addRegion(r.id, r.name, r.zone, r.x + off(rng), r.y + off(rng));
        }
    }

    void actionBulkSimulate() {
        int runs = readIntDefault("Number of simulation runs: ", 10);
        int nregions = (int)reg.list().size();
        if (nregions < 2) { cout << "Need at least 2 regions\n"; return; }
        double totalCost = 0.0;
        for (int k = 0; k < runs; ++k) {
            WarehouseSimulator sim(reg);
            WarehouseGraph G = sim.generateGraph();

            // Build local edge list similarly to earlier approach but with noise
            vector<Edge> edges;
            auto all = reg.list();
            mt19937 rng((unsigned)k + 12345);
            uniform_real_distribution<double> noise(0.9, 1.25);
            for (size_t i = 0; i < all.size(); ++i) {
                for (size_t j = i+1; j < all.size(); ++j) {
                    double dx = all[i].x - all[j].x;
                    double dy = all[i].y - all[j].y;
                    double dist = sqrt(dx*dx + dy*dy);
                    double w = dist * noise(rng);
                    edges.push_back({all[i].id, all[j].id, w});
                }
            }
            sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b){ return a.w < b.w; });
            DSU dsu((int)all.size()+5);
            double cost = 0.0;
            int used = 0;
            for (auto &e : edges) {
                if (dsu.unite(e.u, e.v)) {
                    cost += e.w;
                    used++;
                    if (used == (int)all.size() - 1) break;
                }
            }
            totalCost += cost;
        }
        cout << "Average MST cost over " << runs << " runs: " << (totalCost / runs) << "\n";
    }
};

// =============================================================
// Helper: populate a default set of regions for Aroha Nagar
// =============================================================

static void populateDefaultRegions(RegionMap &rm) {
    rm.addRegion(1, "Central Market", "Central", 10.0, 10.0);
    rm.addRegion(2, "North Gate", "North", 12.5, 18.0);
    rm.addRegion(3, "East Park", "East", 18.0, 11.0);
    rm.addRegion(4, "South Depot", "South", 9.0, 4.5);
    rm.addRegion(5, "West End", "West", 3.5, 9.0);
    rm.addRegion(6, "Industrial Zone", "South", 14.0, 3.0);
    rm.addRegion(7, "University", "East", 20.0, 16.0);
    rm.addRegion(8, "Harbor", "South", 5.0, 2.0);
    rm.addRegion(9, "Airport", "East", 25.0, 8.0);
    rm.addRegion(10, "Ring Road", "Central", 13.0, 13.0);
}

// =============================================================
// Main for Kruskal Warehouse Planner
// =============================================================

int main(int argc, char **argv) {
    RegionMap rm;
    populateDefaultRegions(rm);

    // headless mode: compute an MST and write files
    if (argc >= 2 && string(argv[1]) == "--headless") {
        WarehouseSimulator sim(rm);
        WarehouseGraph G = sim.generateGraph();

        // create edges with noise and output MST
        auto all = rm.list();
        vector<Edge> edges;
        mt19937 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
        uniform_real_distribution<double> noise(0.9, 1.25);
        for (size_t i = 0; i < all.size(); ++i) {
            for (size_t j = i+1; j < all.size(); ++j) {
                double dx = all[i].x - all[j].x;
                double dy = all[i].y - all[j].y;
                double dist = sqrt(dx*dx + dy*dy);
                double w = dist * noise(rng);
                edges.push_back({all[i].id, all[j].id, w});
            }
        }
        sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b){ return a.w < b.w; });
        DSU dsu((int)all.size()+5);
        vector<Edge> mst;
        for (auto &e : edges) {
            if (dsu.unite(e.u, e.v)) mst.push_back(e);
        }
        WarehouseExporter::exportMST(mst, "mst_headless.csv");
        ofstream out("mst_report_headless.txt");
        out << WarehouseReport::summary(mst) << "\n";
        out.close();
        cout << "Headless MST computed and exported.\n";
        return 0;
    }

    // interactive planner
    WarehousePlanner planner(rm);
    planner.run();
    return 0;
}

