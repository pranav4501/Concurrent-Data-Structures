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

struct Node{
    int key;
    Node* left;
    Node* right;

    Node(int value) : key(value), left(nullptr), right(nullptr) {}
};

struct BinarySearchTree {
private:
    Node* root;
    mutable std::shared_mutex mtx;

public:
    BinarySearchTree(): root(nullptr) {}

    bool search(int key){
        shared_lock lock(mtx);
        Node* curr = root;
        while(curr != nullptr){
            if(curr->key == key) return true;
            if(curr->key > key) curr = curr->left;
            else curr = curr->right;
        }
        return false;
    }

    bool insert(int key){
        if(search(key)) return false;
        unique_lock lock(mtx);
        Node* newNode = new Node(key);

        if(!root){
            root = newNode;
            return true;
        }

        Node* curr = root;
        Node* prev = root;
        while(curr){
            if(key < curr->key){
                prev = curr;
                curr = curr->left;
            }
            else{
                prev = curr;
                curr = curr->right;
            }
        }

        if(prev->key < key){
            prev->left = newNode;
        }
        else{
            prev->right = newNode;
        }
        return true;
    }

    bool remove(int key){
        if(!search(key)) return false;
        unique_lock lock(mtx);
        root = deleteNode(root, key);
        return true;
    }

    Node* deleteNode(Node* curr, int key){
        if(!curr) return nullptr;

        if(curr->key > key){
            curr->left = deleteNode(curr->left, key);
            return curr;
        }
        else if(curr->key < key){
            curr->right = deleteNode(curr->right, key);
            return curr;
        }

        if(!curr->left){
            Node* temp = curr->right;
            delete curr;
            return temp;
        }

        else if(!curr->right){
            Node* temp = curr->left;
            delete curr;
            return temp;
        }
        else{

            Node* par = curr;
            Node* suc = curr->right;
            while(suc->left){
                par = suc;
                suc = suc->left;
            }

            if(par != curr){
                par->left = suc->right;
            }
            else{
                par->right = suc->right;
            }
            curr->key = suc->key;
            delete suc;
            return curr;
        }
    }

    ~BinarySearchTree(){
        destroy(root);
    }

    void destroy(Node* curr){
        if(!curr) return;
        destroy(curr->left);
        destroy(curr->right);
        delete curr;
    }
};

atomic<uint64_t> total{0};

int operations = 100000;

void searchth(BinarySearchTree& tree, int keyspace){
    tree.search(random(1, keyspace));
}
void insertth(BinarySearchTree& tree, int keyspace){
    tree.insert(random(1,keyspace));
}
void removeth(BinarySearchTree& tree, int keyspace){
    tree.remove(random(1,keyspace));
}


void readThread(BinarySearchTree& tree, int keyspace){
    while(total.load() < operations){
        total++;
        int r = random(1, 100);
        if(r<91){
            searchth(tree, keyspace);
        }
        else if(r<96){
            insertth(tree, keyspace);
        }
        else{
            removeth(tree, keyspace);
        }
    }
}

void writeThread(BinarySearchTree& tree, int keyspace){
    while(total.load() < operations){
        total++;
        int r = random(1,100);
        if(r<51){
            insertth(tree, keyspace);
        }
        else{
            removeth(tree, keyspace);
        }
    }
}

void testThreads(int num_threads, int keyspace, string work_type){
    BinarySearchTree tree;
    vector<thread> threads;

    if(work_type == "read"){
        for(int i=0; i<num_threads; i++){
            threads.emplace_back(readThread, ref(tree), keyspace);
        }
    }
    else{
        for(int i=0; i<num_threads; i++){
            threads.emplace_back(writeThread, ref(tree), keyspace);
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
