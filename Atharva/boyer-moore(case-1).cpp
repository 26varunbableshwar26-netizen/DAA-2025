#include <bits/stdc++.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

struct SensorEvent {
    TimePoint ts;
    std::string code; // short code sequence from a sensor, e.g. "HWHHD"
    int sensorId;
};

class BoyerMoore {
public:
    BoyerMoore(const std::string &pattern)
        : pat(pattern), m(pattern.size()),
          badChar(256, -1), suffix(m, -1), prefix(m, false)
    {
        preprocessBadChar();
        preprocessGoodSuffix();
    }

    // Find all occurrences of pattern in text, return starting indices
    std::vector<int> searchAll(const std::string &text) const {
        std::vector<int> res;
        int n = (int)text.size();
        if (m == 0 || n < m) return res;
        int i = 0;
        while (i <= n - m) {
            int j = m - 1;
            while (j >= 0 && text[i + j] == pat[j]) --j;
            if (j < 0) {
                res.push_back(i);
                i += m; // move past this occurrence
            } else {
                int bcShift = j - badChar[static_cast<unsigned char>(text[i + j])];
                int gsShift = 0;
                if (j < m - 1) gsShift = moveByGoodSuffix(j);
                i += std::max(1, std::max(bcShift, gsShift));
            }
        }
        return res;
    }

private:
    std::string pat;
    int m;
    std::vector<int> badChar;
    std::vector<int> suffix;
    std::vector<bool> prefix;

    void preprocessBadChar() {
        for (int i = 0; i < 256; ++i) badChar[i] = -1;
        for (int i = 0; i < m; ++i) badChar[static_cast<unsigned char>(pat[i])] = i;
    }

    void preprocessGoodSuffix() {
        for (int i = 0; i < m; ++i) {
            suffix[i] = -1;
            prefix[i] = false;
        }
        for (int i = 0; i < m - 1; ++i) {
            int j = i;
            int k = 0;
            while (j >= 0 && pat[j] == pat[m - 1 - k]) {
                --j;
                ++k;
                suffix[k] = j + 1;
            }
            if (j == -1) prefix[k] = true;
        }
    }

    int moveByGoodSuffix(int j) const {
        int k = m - 1 - j;
        if (suffix[k] != -1) return j - suffix[k] + 1;
        for (int r = j + 2; r <= m - 1; ++r) {
            if (prefix[m - r]) return r;
        }
        return m;
    }
};

class PatternTracker {
public:
    PatternTracker(std::string p, int windowSizeEvents, int alertThreshold)
        : pattern(std::move(p)), bm(pattern), windowSize(windowSizeEvents),
          alertThreshold(alertThreshold), occurrences(0) { }

    // process incoming event code; returns true if alert should be emitted now
    bool processEvent(const std::string &eventCode) {
        auto matches = bm.searchAll(eventCode);
        int count = (int)matches.size();
        pushEventCount(count);
        return checkAlert();
    }

    std::string name() const { return pattern; }

    // current count in window
    int currentWindowCount() const {
        std::lock_guard<std::mutex> lk(mu);
        return occurrences;
    }

private:
    std::string pattern;
    BoyerMoore bm;
    int windowSize;
    int alertThreshold;

    mutable std::mutex mu;
    std::deque<int> windowCounts;
    int occurrences;

    void pushEventCount(int c) {
        std::lock_guard<std::mutex> lk(mu);
        windowCounts.push_back(c);
        occurrences += c;
        if ((int)windowCounts.size() > windowSize) {
            occurrences -= windowCounts.front();
            windowCounts.pop_front();
        }
    }

    bool checkAlert() const {
        std::lock_guard<std::mutex> lk(mu);
        return occurrences >= alertThreshold && alertThreshold > 0;
    }
};

class EventBuffer {
public:
    EventBuffer(size_t capacity) : cap(capacity) { }

    void push(SensorEvent ev) {
        std::unique_lock<std::mutex> lk(mu);
        cvFull.wait(lk, [&]() { return q.size() < cap; });
        q.push_back(std::move(ev));
        lk.unlock();
        cvEmpty.notify_one();
    }

    bool pop(SensorEvent &out) {
        std::unique_lock<std::mutex> lk(mu);
        cvEmpty.wait(lk, [&]() { return !q.empty() || terminated; });
        if (q.empty()) return false;
        out = std::move(q.front());
        q.pop_front();
        lk.unlock();
        cvFull.notify_one();
        return true;
    }

    void terminate() {
        std::lock_guard<std::mutex> lk(mu);
        terminated = true;
        cvEmpty.notify_all();
    }

private:
    std::deque<SensorEvent> q;
    std::mutex mu;
    std::condition_variable cvEmpty;
    std::condition_variable cvFull;
    size_t cap;
    bool terminated{false};
};

class AlertManager {
public:
    void registerPattern(const std::string &pattern, int windowEvents, int threshold) {
        std::lock_guard<std::mutex> lk(mu);
        trackers.emplace_back(pattern, windowEvents, threshold);
    }

    void processEvent(const SensorEvent &ev) {
        for (auto &t : trackers) {
            bool shouldAlert = t.processEvent(ev.code);
            if (shouldAlert) emitAlert(t.name(), ev);
        }
    }

    void emitAlert(const std::string &pattern, const SensorEvent &ev) {
        auto now = Clock::now();
        std::time_t t = std::time(nullptr);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%lld", (long long)std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
        std::lock_guard<std::mutex> lk(outMu);
        std::cout << "[ALERT] pattern=" << pattern
                  << " sensor=" << ev.sensorId
                  << " code=" << ev.code
                  << " time=" << buf
                  << " window_count=" << getWindowCount(pattern)
                  << "\n";
    }

    int getWindowCount(const std::string &pattern) {
        for (auto &t : trackers) if (t.name() == pattern) return t.currentWindowCount();
        return 0;
    }

private:
    std::vector<PatternTracker> trackers;
    std::mutex mu;
    std::mutex outMu;
};

class SensorSimulator {
public:
    SensorSimulator(int sensors, int eventRateMs, double dangerProb = 0.01)
        : sensorCount(sensors), rateMs(eventRateMs), rng(std::random_device{}()),
          dangerProbability(dangerProb) {
        buildBasePatterns();
    }

    // generate a batch of events; push into provided buffer
    void run(EventBuffer &buffer, std::atomic<bool> &stopFlag) {
        std::uniform_int_distribution<int> sensorDist(0, sensorCount - 1);
        std::uniform_real_distribution<double> prob(0.0, 1.0);
        while (!stopFlag.load()) {
            SensorEvent ev;
            ev.ts = Clock::now();
            ev.sensorId = sensorDist(rng);
            if (prob(rng) < dangerProbability) {
                ev.code = makeDangerCode();
            } else {
                ev.code = makeNormalCode();
            }
            buffer.push(std::move(ev));
            std::this_thread::sleep_for(std::chrono::milliseconds(rateMs));
        }
    }

private:
    int sensorCount;
    int rateMs;
    std::mt19937 rng;
    double dangerProbability;
    std::vector<std::string> dangerFragments;

    void buildBasePatterns() {
        dangerFragments = { "HWHHD", "LWHHB", "HHWHD", "HHLBD", "HWLHD" };
    }

    std::string makeDangerCode() {
        std::uniform_int_distribution<int> d(0, (int)dangerFragments.size() - 1);
        return dangerFragments[d(rng)];
    }

    std::string makeNormalCode() {
        static const std::vector<std::string> normal = {
            "LWLHB","LWLHB","LWLHB","HWHHB","HWLHB","LWHLB","LWLHD","HWLHB"
        };
        std::uniform_int_distribution<int> d(0, (int)normal.size() - 1);
        return normal[d(rng)];
    }
};

int main(int argc, char **argv) {
    int sensorCount = 50;
    int rateMs = 50;
    int bufferCap = 1000;
    int windowEvents = 40;
    int alertThreshold = 10;
    double dangerProb = 0.02;

    EventBuffer buffer(bufferCap);
    AlertManager am;
    am.registerPattern("HWHHD", windowEvents, alertThreshold);
    am.registerPattern("LWHHB", windowEvents, alertThreshold);
    am.registerPattern("HHWHD", windowEvents, alertThreshold);

    SensorSimulator sim(sensorCount, rateMs, dangerProb);

    std::atomic<bool> stopFlag{false};

    std::thread producer([&]() { sim.run(buffer, stopFlag); });

    std::thread consumer([&]() {
        SensorEvent ev;
        while (buffer.pop(ev)) {
            am.processEvent(ev);
        }
    });

    std::cout << "Running simulation. Press Enter to stop.\n";
    std::string dummy;
    std::getline(std::cin, dummy);

    stopFlag.store(true);
    buffer.terminate();

    producer.join();
    consumer.join();

    std::cout << "Stopped.\n";
    return 0;
}
