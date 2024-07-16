#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <thread>

#include <chrono>
#include <condition_variable>
#include <random>
#include <atomic>


typedef std::chrono::high_resolution_clock::time_point time_point;
typedef std::chrono::milliseconds ms;
typedef std::chrono::nanoseconds ns;

using namespace std;

// Function template to generate random numbers in a given range
template <typename T>
T random(T min, T max){
    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<mt19937::result_type> distr(min, max);
    return distr(generator);
}

//Node struct to represent a node in the stack
struct Node{
    int key;
    Node* next;

    Node(int key)
    : key(key), next(nullptr)
    {}
};

//Thread safe stack implementation
struct Stack{
    private: 
        Node* top;
        mutable shared_mutex mtx;
        int size;

    public:
        Stack()
        : top(nullptr), size{0}
        {}

        void push(int key){
            unique_lock lock(mtx);
            Node* newNode = new Node(key);
            newNode->next = top;
            top = newNode;
            size++;
        }

        int pop(){
            unique_lock lock(mtx);
            if(top==nullptr) return -1;
            int key = top->key;
            Node* temp = top;
            top = top->next;
            delete temp;
            size--;
            return key;
        }

        ~Stack(){
            Node* temp = top;
            while(temp != nullptr){
                Node* temp2 = temp->next;
                delete temp;
                temp = temp2;
            }
        }

        int getSize(){
            return size;
        }
};


std::atomic<std::uint64_t> ct;
const int N = 100000;

void thPush(Stack& s, int n){
    s.push(random(1,n));
}

void thPop(Stack& s){
    s.pop();
}

//Thread function to push and pop elements from the stack
void threadWork(Stack& s, int keyspace){
    while(ct.load() < N){
        ct ++;
        int r = random(1,100);
        if(r<51){
            thPush(s, keyspace);
        }
        else{
            thPop(s);
        }
    }   
}


void testThread(int numThreads, int keyspace){
    Stack s;
    vector<thread> threads;
    
    for(int i=0; i<keyspace/2;i++){
        s.push(random(1,keyspace));
    }

    for(int i=0; i<numThreads; i++){
        threads.emplace_back(threadWork, ref(s), keyspace);
    }

    for(auto& t : threads){
        t.join();
    }
    // cout << "Size of stack "<< s.getSize() << endl;

}


void experiment(int iterations, int numThreads, int keyspace){
    time_point start = chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; i++){
        ct.store(0);
        testThread(numThreads, keyspace);
    }
    time_point end = chrono::high_resolution_clock::now();
    ms diff = chrono::duration_cast<ms>(end - start);
    cout << "Time taken for " << numThreads << " threads and " << iterations << " iterations: " << diff.count()/iterations <<" ms" << endl;
}

int main(){
    int numThreads = 10;
    int keyspace = 100;
    int iterations = 10;
    for(int i=1; i<=numThreads; i++){
        cout << "Running with " << i << " threads" << endl;
        experiment(iterations, i, keyspace);
    }
    return 0;   
}

