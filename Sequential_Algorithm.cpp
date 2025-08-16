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
using namespace std;
using namespace std::chrono;
int th_count;
int Total_txns;
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
double tim1 = 0, tim2 = 0, tim3 = 0, tim4 = 0, tim_final = 0;
string productFile="products.txt";
string balanceFile="balance.txt";
class Block_Exec {
    public:
    void seq_execute() {
        auto time1 = high_resolution_clock::now();
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
        auto time2 = high_resolution_clock::now();
        productMap = loadProducts(productFile);
        balanceMap = loadBalance(balanceFile);
        auto time3 = high_resolution_clock::now();

        for (int val = 0; val < Total_txns; val++) {
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
        }
        auto time4 = high_resolution_clock::now();
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
        auto time5 = high_resolution_clock::now();
    }
};
int main(int argc, char *argv[]) {
    Block_Exec t;
    inp_fname = argv[1];
    exp_name = argv[2];
    th_count = atoi(argv[3]);
    auto start = high_resolution_clock::now();
    t.seq_execute();
    auto end = high_resolution_clock::now();
    auto duration_final = duration_cast<microseconds>(end - start);
    tim_final = duration_final.count()/1000.0 ;
    cout << "inp_fname: " << inp_fname << " total time: " << tim_final <<endl;
    ofstream fptr;
    fptr.open("seq_output_file.txt",std::ios_base::app);
    fptr << "inp_fname: " << inp_fname << " total time: " << tim_final <<endl;
    fptr.close();
    return 0;
}
