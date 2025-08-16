#include "ProductOrderAtomacity.h"
#include "simplewallet.h"
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <pthread.h>
#include <atomic>
#include <chrono>
#include <vector>
#include <sstream>
#include<unistd.h> 
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <sys/stat.h>
#include <errno.h>
#include <cstring>
#include <sstream>
using namespace std;
using namespace std::this_thread; 
using namespace std::chrono; 
using std::vector;
typedef int int_32;
int th_count;
bool val_flag=true;
std::map<string, int> txn_dict;
std::atomic<int> atomic_count{0}, g_atomic_count{0}, AU{0}, SV_counter{0}, barrier{-1};
std::atomic<bool> mMiner(false);
int Total_txns, thcount;
static int duration_tot[56];
int txn_count,mapSize=0,temp=0;
int txn_adj[12000][12000]={0};
atomic_int in_deg[12000];
atomic_int pos;
vector<int> colorCount;
atomic<int> completed(0);
int total_color;
struct transaction 
{
    string txn_id;
    int txn_no;
    int input_len;
    vector<string> inputs;
    int output_len;
    vector<string> outputs;
    bool flag=false;
    int color=-1; 
};
static vector<transaction> curr_txns,empty_txns;
string inp_fname;
string exp_name;
double tim1=0;
double tim2=0;
double tim3=0;
double tim4=0;
double tim0 = 0;
double loadtime = 0, savetime = 0;
string productFile="products.txt";
string balanceFile="balance.txt";
class Block_Exec {
    public:
    static int select_txn() {
        for (int i = pos; i < Total_txns; i++) {    
            int var_zero=0;
            if(in_deg[i]==0) {
                if(in_deg[i].compare_exchange_strong(var_zero,-1) == true) {
                    completed++;
                    return curr_txns[i].txn_no;
                }
            }
        }
        for (int i = 0; i < pos; i++) {
            int var_zero=0;
            if(in_deg[i]==0) {
                if(in_deg[i].compare_exchange_strong(var_zero,-1)) {
                    pos=i;
                    completed++;
                    return curr_txns[i].txn_no;;
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
                    if (flag == true) {
                        txn_adj[edge_txn][txn] = 1;
                        txn_adj[txn][edge_txn] = 1;
                    }
                }
            }
        }
    }
    static void addNodeGraphColoring(int PID) {
	    int txn;
	    while (1) {
            txn = g_atomic_count++;
            if (txn >= Total_txns) {
                g_atomic_count--;
                return;
            }
            for (int j = txn; j < Total_txns; j++) {
                int u = txn;
                int v = j;
                if (u != v && txn_adj[u][v] == 1) {
                    int u_color = colorCount[curr_txns[u].color];
                    int v_color = colorCount[curr_txns[v].color];
                    if (u_color > v_color || (u_color == v_color && curr_txns[u].color < curr_txns[v].color)) {
                        txn_adj[u][v]=1;
                        txn_adj[v][u]=0;
                        in_deg[v]++;
                    } else {
                        txn_adj[v][u]=1;
                        txn_adj[u][v]=0;
                        in_deg[u]++;
                    }
                }
            }
        }
    };
    static void DAG_delete(int n) {
        for (int i=0;i<Total_txns;i++) {
            if (txn_adj[n][i] == 1 and in_deg[i]>0) {
                in_deg[i]--;    
            }
        }
        in_deg[n]--;
    };
    void colorGraph() {
        vector<bool> available(Total_txns, false);
        colorCount.resize(Total_txns, 0);
        int maxcol = 0;
        curr_txns[0].color = 0;
        colorCount[0]++;
        for (int u = 1; u < Total_txns; u++) {
            for (int i = 0; i < u; i++) {
                if (txn_adj[u][i] == 1 && curr_txns[i].color != -1) {
                    available[curr_txns[i].color] = true;
                }
            }
            int cr;
            for (cr = 0; cr < Total_txns; cr++) {
                if (available[cr]==false) {
                    break;
                }
            }
            curr_txns[u].color = cr;
            maxcol = max(maxcol, cr);
            colorCount[cr]++;
            for (int i = 0; i < u; i++) {
                if (txn_adj[u][i] == 1 && curr_txns[i].color != -1) {
                    available[curr_txns[i].color] = false;
                }
            }
        }
        total_color = maxcol+1;
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
    static void select_start_trans() {
        while (completed < Total_txns) {
            int val = select_txn();
            if (val == -1) {
                continue;
            }else {
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
        for(int i=0;i<th_count;i++)	{
            threads[i]= thread(select_start_trans); 
        }
        for(int i=0;i<th_count;i++) {
            threads[i].join(); 
        }
    }

    static void addNodeGC() {
        thread threads[th_count];
        for (int i = 0; i < th_count; i++) {
            threads[i] = thread(addNodeGraphColoring,i);
        }
        for (int i = 0; i < th_count; i++) {
            threads[i].join();
        }
    }
};
int main(int argc, char* argv[]) {
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
    t.colorGraph();
    auto start2 = high_resolution_clock::now();
    t.addNodeGC();
    auto start3 = high_resolution_clock::now();
    t.execute();
    auto end = high_resolution_clock::now();
    if (saveProducts("products.txt", productMap)) {
        cout << "Products file successfully updated for successfull transactions.\n";
    } else {
        cout << "Failed to update the products file.\n";
    }
    if (saveBalance("balance.txt", balanceMap)) {
        cout << "Balance file successfully updated for successfull transactions.\n";
    } else {
        cout << "Failed to update the balance file.\n";
    }
    auto savetime2 = high_resolution_clock::now();
    auto durationload = duration_cast<microseconds>(loadtime2 - loadtime1);
    auto durationsave = duration_cast<microseconds>(savetime2 - end);
    loadtime = durationload.count()/1000.0;
    savetime = durationsave.count()/1000.0;
    cout << "Loading time: " << loadtime << endl;
    cout << "Saving time: " << savetime << endl;
    auto duration0 = duration_cast<microseconds>(start2 - start1);
    auto duration1 = duration_cast<microseconds>(start3 - start2);
    auto duration2 = duration_cast<microseconds>(start1 - loadtime2);
    auto duration3 = duration_cast<microseconds>(end - start3);
    auto duration4 = duration_cast<microseconds>(savetime2 - loadtime1);
    tim0 = duration0.count()/1000.0;
    tim1 = duration1.count()/1000.0;
    tim2 = duration2.count()/1000.0;
    tim3 = duration3.count()/1000.0;
    tim4 = duration4.count()/1000.0;
    ofstream fptr;
    fptr.open("graph_trans_MAT_part0.txt", std::ios_base::app);
    fptr << "MAT " << exp_name << " " << th_count << " " << inp_fname << " " << tim0 << endl;
    fptr.close();
    fptr.open("graph_trans_MAT_part1.txt", std::ios_base::app);
    fptr << "MAT " << exp_name << " " << th_count << " " << inp_fname << " " << tim1 << endl;
    fptr.close();
    fptr.open("graph_trans_MAT_part2.txt", std::ios_base::app);
    fptr << "MAT " << exp_name << " " << th_count << " " << inp_fname << " " << tim2 << endl;
    fptr.close();
    fptr.open("graph_trans_MAT_part3.txt", std::ios_base::app);
    fptr << "MAT " << exp_name << " " << th_count << " " << inp_fname << " " << tim3 << endl;
    fptr.close();
    fptr.open("graph_trans_part0.txt", std::ios_base::app);
    fptr << tim0 << endl;
    fptr.close();
    fptr.open("graph_trans_part1.txt", std::ios_base::app);
    fptr << tim1 << endl;
    fptr.close();
    fptr.open("graph_trans_part2.txt", std::ios_base::app);
    fptr << tim2 << endl;
    fptr.close();
    fptr.open("graph_trans_part3.txt", std::ios_base::app);
    fptr << tim3 << endl;
    fptr.close();
    fptr.open("graph_trans_part4.txt", std::ios_base::app);
    fptr << tim4 << endl;
    fptr.close();
    fptr.open("graph_trans_MAT_colors_file.txt", std::ios_base::app);
    fptr << "MAT " << exp_name << " " << th_count << " " << inp_fname << " the number of color used " << total_color << endl;
    fptr.close();
    struct stat st = {0};
    if (stat("color_info", &st) == -1) {
        if (mkdir("color_info", 0700) != 0) {
            cerr << "Failed to create directory 'color_info': " << strerror(errno) << endl;
            return 1;
        }
    }
    string inputFileName = inp_fname;
    size_t lastSlash = inputFileName.find_last_of("/\\");
    if (lastSlash != string::npos) {
        inputFileName = inputFileName.substr(lastSlash + 1);
    }
    size_t dot = inputFileName.find_last_of('.');
    if (dot != string::npos) {
        inputFileName = inputFileName.substr(0, dot);
    }
    string outputFileName = "color_info/" + inputFileName + ".txt";
    ofstream colorFile(outputFileName);
    if (colorFile.is_open()) {
        for (int i = 0; i < total_color; ++i) {
            colorFile << "color" << i << " : " << colorCount[i] << endl;
        }
        colorFile.close();
        cout << "Color information written to " << outputFileName << endl;
    } else {
        cerr << "Failed to open file for writing color info: " << outputFileName << endl;
    }
    return 0;
}
