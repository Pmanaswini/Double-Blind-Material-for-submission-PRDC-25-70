#include "ProductOrderAtomacity.h"
#include "simplewallet.h"
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <sstream>
#include <map>
#include <unistd.h>
#include <unordered_set>
#include <unordered_map>
using namespace std;
using namespace std::chrono;
typedef int int_32;
int th_count;
std::atomic<int> atomic_count{0};
std::atomic<int> completed{0};
int Total_txns;
atomic<int> pos{0};
atomic_int in_deg[12000];
int txn_adj[12000][12000] = {0};
struct transaction {
    string txn_id;
    int txn_no;
    int input_len;
    vector<string> inputs;
    int output_len;
    vector<string> outputs;
    bool flag = false;
};
vector<transaction> curr_txns;
string inp_fname, exp_name;
double tim2 = 0, tim3 = 0, tim4 = 0;
double loadtime = 0, savetime = 0;
string productFile="products.txt";
string balanceFile="balance.txt";
class Block_Exec {
public:
    static int select_txn() {
        for (int i = pos; i < Total_txns; i++) {
            int var_zero = 0;
            if (in_deg[i] == 0) {
                if (in_deg[i].compare_exchange_strong(var_zero, -1)) {
                    completed++;
                    return curr_txns[i].txn_no;
                }
            }
        }
        for (int i = 0; i < pos; i++) {
            int var_zero = 0;
            if (in_deg[i] == 0) {
                if (in_deg[i].compare_exchange_strong(var_zero, -1)) {
                    pos = i;
                    completed++;
                    return curr_txns[i].txn_no;
                }
            }
        }
        return -1;
    }
    static void add_nodes(int PID) {
        while (true) {
            int txn = atomic_count++;
            if (txn >= Total_txns) {
                atomic_count--;
                return;
            }
            int inp_lo = curr_txns[txn].input_len;
            int out_lo = curr_txns[txn].output_len;
            std::unordered_set<string> txn_inputs(curr_txns[txn].inputs.begin(), curr_txns[txn].inputs.end());
            std::unordered_set<string> txn_outputs(curr_txns[txn].outputs.begin(), curr_txns[txn].outputs.end());
            for (int i = 0; i < txn; i++) {
                bool flag = false;
                int edge_txn = curr_txns[i].txn_no;
                if (in_deg[edge_txn] != -2) {
                    int inp_ex = curr_txns[i].input_len;
                    int out_ex = curr_txns[i].output_len;
                    unordered_set<string> edge_inputs(curr_txns[i].inputs.begin(), curr_txns[i].inputs.end());
                    unordered_set<string> edge_outputs(curr_txns[i].outputs.begin(), curr_txns[i].outputs.end());
                    for (const string& inp : edge_inputs) {
                        if (txn_outputs.find(inp) != txn_outputs.end()) {
                            flag = true;
                            break;
                        }
                    }
                    if (!flag) {
                        for (const string& out : edge_outputs) {
                            if (txn_inputs.find(out) != txn_inputs.end()) {
                                flag = true;
                                break;
                            }
                        }
                    }
                    if (!flag) {
                        for (const string& out : edge_outputs) {
                            if (txn_outputs.find(out) != txn_outputs.end()) {
                                flag = true;
                                break;
                            }
                        }
                    }
                    if (flag) {
                        txn_adj[edge_txn][txn] = 1;
                        in_deg[txn]++;
                    }
                }
            }
        }
    }
    static void DAG_delete(int n) {
        for (int i = 0; i < Total_txns; i++) {
            if (txn_adj[n][i] == 1 && in_deg[i] > 0) {
                in_deg[i]--;
            }
        }
        in_deg[n]--;
    }
    static void select_start_trans() {
        while (completed < Total_txns) {
            int val = select_txn();
            if (val == -1) {
                continue;
            } else {
                if (!curr_txns[val].inputs.empty()) {
                    string first_input = curr_txns[val].inputs[0];
                    if (first_input.substr(0, 3) == "pid") {
                        // Call processOrders
                        processOrders(val + 1, inp_fname);
                    } else if (first_input.substr(0, 3) == "cid") {
                        // Call performTransfers
                        performTransfers(val + 1, inp_fname);
                    }
                }
                usleep(6000);
                DAG_delete(val);
                in_deg[val] = -1;
            }
        }
    }
    static void execute() {
        thread threads[th_count];
        for (int i = 0; i < th_count; i++) {
            threads[i] = thread(select_start_trans);
        }
        for (int i = 0; i < th_count; i++) {
            threads[i].join();
        }
    }
    void DAG_create_graph() {
        ifstream batch_file(inp_fname);
        if (!batch_file.is_open()) {
            cerr << "Unable to open file: " << inp_fname << endl;
            exit(1);
        }
        string line;
        int txn_no = 0;
        while (getline(batch_file, line)) {
            istringstream ss(line);
            string token;
            transaction txn;
            getline(ss, txn.txn_id, ',');
            txn.txn_no = txn_no;
            getline(ss, token, ',');
            txn.input_len = stoi(token);
            while (getline(ss, token, ',')) {
                string product_id = token;
                getline(ss, token, ',');
                txn.inputs.push_back(product_id);
                txn.outputs.push_back(product_id);
            }
            txn.output_len = txn.input_len;
            curr_txns.push_back(txn);
            txn_no++;
        }
        batch_file.close();
        Total_txns = curr_txns.size();
        thread threads[th_count];
        for (int i = 0; i < th_count; i++) {
            threads[i] = thread(add_nodes, i);
        }
        for (int i = 0; i < th_count; i++) {
            threads[i].join();
        }
    }
};
int main(int argc, char *argv[]) {
    Block_Exec t;
    inp_fname = argv[1];
    exp_name = argv[2];
    th_count = atoi(argv[3]);
    auto loadtime1 = high_resolution_clock::now();
    productMap = loadProducts(productFile);
    balanceMap = loadBalance(balanceFile);
    auto loadtime2 = high_resolution_clock::now();
    t.DAG_create_graph();
    auto start1 = high_resolution_clock::now();
    t.execute();
    auto end = high_resolution_clock::now();
    if (saveProducts("products.txt", productMap)) {
        cout << "Products file successfully updated for successfull transactions .\n";
    } else {
        cout << "Failed to update the products file.\n";
    }
    if (saveBalance("balance.txt", balanceMap)) {
        cout << "Balance file successfully updated for successfull transactions .\n";
    } else {
        cout << "Failed to update the balance file.\n";
    }
    auto savetime2 = high_resolution_clock::now();
    auto durationload = duration_cast<microseconds>(loadtime2 - loadtime1);
    auto durationsave = duration_cast<microseconds>(savetime2 - end);
    loadtime = durationload.count()/1000.0 ;
    savetime = durationsave.count()/1000.0 ;
    cout << "Loading time: " << loadtime <<endl;
    cout << "Saving time: " << savetime <<endl;
    auto duration2 = duration_cast<microseconds>(start1 - loadtime2);
    auto duration3 = duration_cast<microseconds>(end - start1);
    auto duration4 = duration_cast<microseconds>(savetime2 - loadtime1);
    tim2 = duration2.count()/1000.0 ;  // first DAG creation
    tim3 = duration3.count()/1000.0 ;  // execute
	tim4 = duration4.count()/1000.0;   // total
    ofstream fptr;
    fptr.open("graph_trans_DAG_part2.txt",std::ios_base::app);
    fptr<<"DAG "<<exp_name<<" "<<th_count<<" "<<inp_fname<<" "<<tim2<<endl;
    fptr.close();
    fptr.open("graph_trans_DAG_part3.txt",std::ios_base::app);
    fptr<<"DAG "<<exp_name<<" "<<th_count<<" "<<inp_fname<<" "<<tim3<<endl;
    fptr.close();
    fptr.open("graph_trans_part2.txt",std::ios_base::app);
    fptr<<tim2<<endl;
    fptr.close();
    fptr.open("graph_trans_part3.txt",std::ios_base::app);
    fptr<<tim3<<endl;
    fptr.close();
    fptr.open("graph_trans_part4.txt",std::ios_base::app);
    fptr<<tim4<<endl;
    fptr.close();
    return 0;
}
