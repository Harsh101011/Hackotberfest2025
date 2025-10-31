#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
using namespace std;

template<typename T>
class ThreadSafeQueue {
private:
    mutable mutex mtx;
    condition_variable cv;
    queue<T> q;
    bool finished;
    
public:
    ThreadSafeQueue() : finished(false) {}
    
    // Push item to queue
    void push(T value) {
        lock_guard<mutex> lock(mtx);
        q.push(move(value));
        cv.notify_one();
    }
    
    // Pop item from queue (blocking)
    bool pop(T& value) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty() || finished; });
        
        if (q.empty()) return false;
        
        value = move(q.front());
        q.pop();
        return true;
    }
    
    // Try to pop (non-blocking)
    bool tryPop(T& value) {
        lock_guard<mutex> lock(mtx);
        if (q.empty()) return false;
        
        value = move(q.front());
        q.pop();
        return true;
    }
    
    // Check if queue is empty
    bool empty() const {
        lock_guard<mutex> lock(mtx);
        return q.empty();
    }
    
    // Get queue size
    size_t size() const {
        lock_guard<mutex> lock(mtx);
        return q.size();
    }
    
    // Signal that no more items will be added
    void finish() {
        lock_guard<mutex> lock(mtx);
        finished = true;
        cv.notify_all();
    }
};

// Producer function
void producer(ThreadSafeQueue<int>& queue, int id, int count) {
    for (int i = 0; i < count; i++) {
        int value = id * 100 + i;
        queue.push(value);
        cout << "Producer " << id << " pushed: " << value << endl;
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

// Consumer function
void consumer(ThreadSafeQueue<int>& queue, int id) {
    while (true) {
        int value;
        if (queue.pop(value)) {
            cout << "Consumer " << id << " popped: " << value << endl;
            this_thread::sleep_for(chrono::milliseconds(150));
        } else {
            break;  // Queue is finished and empty
        }
    }
}

int main() {
    ThreadSafeQueue<int> queue;
    
    // Create producers
    thread p1(producer, ref(queue), 1, 5);
    thread p2(producer, ref(queue), 2, 5);
    
    // Create consumers
    thread c1(consumer, ref(queue), 1);
    thread c2(consumer, ref(queue), 2);
    
    // Wait for producers to finish
    p1.join();
    p2.join();
    
    // Signal queue is finished
    queue.finish();
    
    // Wait for consumers to finish
    c1.join();
    c2.join();
    
    cout << "\nAll threads completed!" << endl;
    
    return 0;
}
