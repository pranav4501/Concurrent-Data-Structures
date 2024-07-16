#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <atomic>

typedef std::chrono::high_resolution_clock::time_point time_point;
typedef std::chrono::milliseconds ms;
typedef std::chrono::nanoseconds ns;

using namespace std;

template<typename T>
T random(T range_from, T range_to) {
    std::random_device                  rand_dev;
    std::mt19937                        generator(rand_dev());
    std::uniform_int_distribution<T>    distr(range_from, range_to);
    return distr(generator);
}

struct Node {
    int key;
    Node* next;

    Node(int value) : key(value), next(nullptr) {}
};

struct LinkedList {
private:
    Node* head;
    mutable std::shared_mutex mutex;

public:
    LinkedList(): head(nullptr) {}

    bool search(int key){
       shared_lock lock(mutex);
       Node* curr = head;
       while(curr != nullptr){
           if(curr->key == key) return true;
           curr = curr->next;
       }
       return false;
    }
    
    bool insert(int key){
        if(search(key)) return false;
        unique_lock lock(mutex);
        Node* newNode = new Node(key);
        if(head == nullptr || head->key > key){
            newNode->next = head;
            head = newNode;
            return true;
        }
        else{
            Node* curr = head;
            while(curr->next != nullptr && curr->next->key < key){
                curr = curr->next;
            }
            newNode->next = curr->next;
            curr->next = newNode;
        }
        return true;
    }   

    bool remove(int key){
        if(!search(key)) return false;
        unique_lock lock(mutex);

        Node* curr = head;
        Node* prev = nullptr;
        if(curr != nullptr && curr->key == key){
            head = head -> next;
            delete curr;
            return true;
        }
        while(curr != nullptr && curr->key != key){
            prev = curr;
            curr = curr->next;
        }
        if(curr == nullptr) return false;
        prev->next = curr->next;
        delete curr;
        return true;
    }

    ~LinkedList(){
        Node* curr = head;
        while(curr != nullptr){
            Node* temp = curr;
            curr = curr->next;
            delete temp;
        }
    }

};

atomic<uint64_t> total{0};

void searchth(LinkedList& list, int keyspace){
    list.search(random(1, keyspace));
}
void insertth(LinkedList& list, int keyspace){
    list.insert(random(1,keyspace));
}
void removeth(LinkedList& list, int keyspace){
    list.remove(random(1,keyspace));
}

const int operations = 100000;

void readThread(LinkedList& list, int keyspace){
    while(total.load() < operations){
        total++;
        int r = random(1, 100);
        if(r<91){
            searchth(list, keyspace);
        }
        else if(r<96){
            insertth(list, keyspace);
        }
        else{
            removeth(list, keyspace);
        }
    }
}

void writeThread(LinkedList& list, int keyspace){
    while(total.load() < operations){
        total++;
        int r = random(1,100);
        if(r<51){
            searchth(list, keyspace);
        }
        else{
            removeth(list, keyspace);
        }
    }
}

void testThreads(int num_threads, int keyspace, string work_type){

    LinkedList list;
    vector<thread> threads;

    for(int i=0; i<keyspace/2; i++){
        list.insert(random(1,keyspace));
    }

    if(work_type == "read"){
        for(int i=0; i<num_threads; i++){
            threads.emplace_back(readThread, ref(list), keyspace);
        }
    }
    else{
        for(int i=0; i<num_threads; i++){
            threads.emplace_back(writeThread, ref(list), keyspace);
        }
    }
    for(auto& t : threads){
        t.join();
    }
}

void experiment(int iterations, int keyspace, int num_threads, string work_type){
    time_point start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; i++){
        total.store(0);
        testThreads(num_threads, keyspace, work_type);
    }
    time_point end = std::chrono::high_resolution_clock::now();
    ms duration = chrono::duration_cast<ms>(end-start);
    cout << "Time taken for " << num_threads << " threads to " << work_type << " " << keyspace << " keyspace: " << duration.count()/iterations << " ms" << endl;
}

int main(){
    int num_threads = 10;
    int keyspace = 100;
    int iterations = 10;

    cout << "Read work" << endl;
    for(int i=1; i<=num_threads; i++){
        cout << "Read work with " << i << " threads" << endl;
        experiment(iterations, keyspace, i, "read");
    }

    cout << "Write work" << endl;
    for(int i=1; i<=num_threads; i++){
        cout << "Write work with " << i << " threads" << endl;
        experiment(iterations, keyspace, i, "write");
    }

    keyspace = 10000;
    cout << "Keyspace: " << keyspace << endl;

    cout << "Read work" << endl;
    for(int i=1; i<=num_threads; i++){
        cout << "Read work with " << i << " threads" << endl;
        experiment(iterations, keyspace, i, "read");
    }

    cout << "Write work" << endl;
    for(int i=1; i<=num_threads; i++){
        cout << "Write work with " << i << " threads" << endl;
        experiment(iterations, keyspace, i, "write");
    }   
    return 0;
}
