#include <bits/stdc++.h>
using namespace std;

struct Date {
    int y, m, d;

    bool operator<(const Date &o) const {
        if (y != o.y) return y < o.y;
        if (m != o.m) return m < o.m;
        return d < o.d;
    }

    bool operator>(const Date &o) const {
        if (y != o.y) return y > o.y;
        if (m != o.m) return m > o.m;
        return d > o.d;
    }

    bool operator==(const Date &o) const {
        return y == o.y && m == o.m && d == o.d;
    }
};

Date makeDate(int y, int m, int d) {
    Date x; x.y = y; x.m = m; x.d = d; return x;
}

struct Device {
    int deviceId;
    string type;
    string brand;
    Date expiry;
    bool requiresRepair;
};

struct Node {
    Device dev;
    Node* left;
    Node* right;

    Node(const Device &d) : dev(d), left(nullptr), right(nullptr) {}
};

class DeviceBST {
public:
    DeviceBST() : root(nullptr) { }

    void insert(const Device &d) {
        root = insertRec(root, d);
    }

    void remove(const Date &expiry, int deviceId) {
        root = removeRec(root, expiry, deviceId);
    }

    vector<Device> allExpiringBefore(const Date &limit) {
        vector<Device> res;
        collectBefore(root, limit, res);
        return res;
    }

    vector<Device> allExpiringInRange(const Date &start, const Date &end) {
        vector<Device> res;
        collectRange(root, start, end, res);
        return res;
    }

    Device earliestExpiring() {
        Node* cur = root;
        if (!cur) return Device{-1,"","",makeDate(0,0,0),false};
        while (cur->left) cur = cur->left;
        return cur->dev;
    }

    bool containsDevice(const Date &expiry, int deviceId) {
        return containsRec(root, expiry, deviceId);
    }

    void inorderPrint() {
        inorder(root);
    }

private:
    Node* root;

    Node* insertRec(Node* node, const Device &d) {
        if (!node) return new Node(d);
        if (d.expiry < node->dev.expiry) node->left = insertRec(node->left, d);
        else if (d.expiry > node->dev.expiry) node->right = insertRec(node->right, d);
        else {
            if (d.deviceId < node->dev.deviceId) node->left = insertRec(node->left, d);
            else node->right = insertRec(node->right, d);
        }
        return node;
    }

    Node* removeRec(Node* node, const Date &expiry, int deviceId) {
        if (!node) return nullptr;

        if (expiry < node->dev.expiry)
            node->left = removeRec(node->left, expiry, deviceId);
        else if (expiry > node->dev.expiry)
            node->right = removeRec(node->right, expiry, deviceId);
        else {
            if (deviceId < node->dev.deviceId)
                node->left = removeRec(node->left, expiry, deviceId);
            else if (deviceId > node->dev.deviceId)
                node->right = removeRec(node->right, expiry, deviceId);
            else {
                if (!node->left) {
                    Node* r = node->right;
                    delete node;
                    return r;
                } else if (!node->right) {
                    Node* l = node->left;
                    delete node;
                    return l;
                } else {
                    Node* succ = node->right;
                    while (succ->left) succ = succ->left;
                    node->dev = succ->dev;
                    node->right = removeRec(node->right, succ->dev.expiry, succ->dev.deviceId);
                }
            }
        }
        return node;
    }

    void collectBefore(Node* node, const Date &limit, vector<Device> &res) {
        if (!node) return;
        if (node->dev.expiry < limit) {
            res.push_back(node->dev);
            collectBefore(node->left, limit, res);
            collectBefore(node->right, limit, res);
        } else {
            collectBefore(node->left, limit, res);
        }
    }

    void collectRange(Node* node, const Date &start, const Date &end, vector<Device> &res) {
        if (!node) return;
        if (node->dev.expiry > start)
            collectRange(node->left, start, end, res);
        if (node->dev.expiry > start && node->dev.expiry < end)
            res.push_back(node->dev);
        if (node->dev.expiry < end)
            collectRange(node->right, start, end, res);
    }

    bool containsRec(Node* node, const Date &expiry, int deviceId) {
        if (!node) return false;
        if (expiry < node->dev.expiry) return containsRec(node->left, expiry, deviceId);
        if (expiry > node->dev.expiry) return containsRec(node->right, expiry, deviceId);
        if (deviceId < node->dev.deviceId) return containsRec(node->left, expiry, deviceId);
        if (deviceId > node->dev.deviceId) return containsRec(node->right, expiry, deviceId);
        return true;
    }

    void inorder(Node* node) {
        if (!node) return;
        inorder(node->left);
        cout << node->dev.deviceId << " | " << node->dev.type
             << " | " << node->dev.brand << " | "
             << node->dev.expiry.y << "-" << node->dev.expiry.m << "-" << node->dev.expiry.d
             << " | repair=" << node->dev.requiresRepair << "\n";
        inorder(node->right);
    }
};

class RoutingSystem {
public:
    RoutingSystem() { }

    string route(const Device &d) {
        if (d.requiresRepair == true)
            return "Send to Repair Centre A";
        int age = deviceAgeCategory(d.expiry);
        if (age <= 30)
            return "Send to Light Refurbishing Unit";
        if (age <= 90)
            return "Send to Heavy Refurbishing Unit";
        return "Send to Recycling Plant";
    }

private:
    int deviceAgeCategory(const Date &exp) {
        int days = (exp.y * 365 + exp.m * 30 + exp.d);
        return abs(days % 200);
    }
};

class DeviceStream {
public:
    DeviceStream(DeviceBST &tree, RoutingSystem &router)
        : bst(tree), routing(router) {
        rng.seed(time(nullptr));
    }

    void simulate(int cycles) {
        for (int i = 0; i < cycles; i++) {
            Device d = generateRandomDevice(i+1);
            bst.insert(d);
            string dest = routing.route(d);
            cout << "Inserted Device " << d.deviceId
                 << " | expiry=" << d.expiry.y << "-" << d.expiry.m << "-" << d.expiry.d
                 << " | route=" << dest << "\n";
        }
    }

private:
    DeviceBST &bst;
    RoutingSystem &routing;
    mt19937 rng;

    Device generateRandomDevice(int id) {
        vector<string> types = {"phone","laptop","tablet","router","camera"};
        vector<string> brands = {"vivo","lenovo","hp","acer","dell","samsung"};

        uniform_int_distribution<int> t(0, types.size()-1);
        uniform_int_distribution<int> b(0, brands.size()-1);
        uniform_int_distribution<int> y(2024, 2026);
        uniform_int_distribution<int> m(1, 12);
        uniform_int_distribution<int> d(1, 28);
        uniform_int_distribution<int> rep(0, 1);

        Device dv;
        dv.deviceId = id * 10 + (rng() % 7);
        dv.type = types[t(rng)];
        dv.brand = brands[b(rng)];
        dv.expiry = makeDate(y(rng), m(rng), d(rng));
        dv.requiresRepair = rep(rng);
        return dv;
    }
};

void printDeviceList(const vector<Device> &v) {
    for (auto &d : v) {
        cout << d.deviceId << " | "
             << d.type << " | "
             << d.brand << " | "
             << d.expiry.y << "-" << d.expiry.m << "-" << d.expiry.d
             << " | repair=" << d.requiresRepair << "\n";
    }
}

int main() {
    DeviceBST bst;
    RoutingSystem router;

    DeviceStream stream(bst, router);
    stream.simulate(40);

    cout << "\nEarliest Expiring Device:\n";
    Device early = bst.earliestExpiring();
    cout << early.deviceId << " | " << early.type << "\n";

    cout << "\nDevices expiring before 2025-06-01:\n";
    auto list1 = bst.allExpiringBefore(makeDate(2025,6,1));
    printDeviceList(list1);

    cout << "\nDevices expiring between 2025-01-01 and 2025-12-31:\n";
    auto list2 = bst.allExpiringInRange(makeDate(2025,1,1), makeDate(2025,12,31));
    printDeviceList(list2);

    return 0;
}
