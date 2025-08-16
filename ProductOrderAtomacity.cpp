#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>

using namespace std;

struct Order {
    string productId;
    int quantity;
};

unordered_map<string, int> productMap;

bool isValidInteger(const string& str) {
    return !str.empty() && all_of(str.begin(), str.end(), ::isdigit);
}

string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    return (start == string::npos) ? "" : str.substr(start, end - start + 1);
}

unordered_map<string, int> loadProducts(const string& productFile) {
    unordered_map<string, int> productMap;
    ifstream file(productFile);

    if (!file.is_open()) {
        cerr << "Failed to open product file!" << endl;
        return productMap;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string id, name, quantity_str, price_str;

        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, quantity_str, ',');
        getline(ss, price_str, ',');

        quantity_str = trim(quantity_str);

        if (!isValidInteger(quantity_str)) {
            cerr << "Invalid quantity format for product ID: " << id << endl;
            continue;
        }

        productMap[id] = stoi(quantity_str);
    }

    file.close();
    return productMap;
}

bool saveProducts(const string& productFile, const unordered_map<string, int>& productMap) {
    ifstream file(productFile);
    if (!file.is_open()) {
        cerr << "Failed to open product file!" << endl;
        return false;
    }

    vector<string> fileLines;
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string id, name, quantity_str, price_str;

        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, quantity_str, ',');
        getline(ss, price_str, ',');

        if (productMap.find(id) != productMap.end()) {
            int newQuantity = productMap.at(id);
            line = id + "," + name + "," + to_string(newQuantity) + "," + price_str;
        }

        fileLines.push_back(line);
    }

    file.close();

    ofstream outFile(productFile);
    if (!outFile.is_open()) {
        cerr << "Failed to open product file for writing!" << endl;
        return false;
    }

    for (const auto& updatedLine : fileLines) {
        outFile << updatedLine << "\n";
    }

    outFile.close();
    return true;
}

bool checkSufficientQuantity(const unordered_map<string, int>& productMap, const vector<Order>& orderList) {
    for (const auto& order : orderList) {
        auto it = productMap.find(order.productId);
        if (it == productMap.end()) {
            cerr << "Product ID not found: " << order.productId << endl;
            return false;
        }
        if (it->second < order.quantity) {
            cerr << "Transaction failed: Insufficient quantity for product ID: " << order.productId << endl;
            return false;
        }
    }
    return true;
}

bool updateProductQuantities(unordered_map<string, int>& productMap, const vector<Order>& orderList) {
    for (const auto& order : orderList) {
        productMap[order.productId] -= order.quantity;
    }
    return true;
}

bool processOrder(unordered_map<string, int>& productMap, const vector<Order>& orderList) {
    if (!checkSufficientQuantity(productMap, orderList)) {
        return false;
    }
    return updateProductQuantities(productMap, orderList);
}

void processOrders(const int txn_id, const string& filename) {
    string txn_id_str = "txn" + to_string(txn_id);

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open order file!" << endl;
        return;
    }

    string line;
    bool found = false;
    while (getline(file, line)) {
        stringstream ss(line);
        string transactionId, no_of_products_str;

        getline(ss, transactionId, ',');
        getline(ss, no_of_products_str, ',');

        if (transactionId == txn_id_str) {
            found = true;
            int no_of_products = stoi(no_of_products_str);
            vector<Order> orderList;

            for (int i = 0; i < no_of_products; ++i) {
                string product_id, quantity_str;

                if (!getline(ss, product_id, ',')) {
                    cerr << "Failed to read product ID for transaction ID: " << txn_id << endl;
                    return;
                }

                if (!getline(ss, quantity_str, ',')) {
                    getline(ss, quantity_str);
                }

                product_id.erase(remove_if(product_id.begin(), product_id.end(), ::isspace), product_id.end());
                quantity_str.erase(remove_if(quantity_str.begin(), quantity_str.end(), ::isspace), quantity_str.end());
                if (!product_id.empty() && isValidInteger(quantity_str)) {
                    orderList.push_back({product_id, stoi(quantity_str)});
                } else {
                    cerr << "Invalid data format for product ID: " << product_id << endl;
                    return;
                }
            }

            if (processOrder(productMap, orderList)) {
                cout << "Transaction successful for Transaction ID " << txn_id << ".\n";
            } else {
                cout << "Transaction aborted for Transaction ID " << txn_id << ".\n";
                return;
            }
            break;
        }
    }

    if (!found) {
        cout << "Transaction ID " << txn_id << " not found.\n";
    }

    file.close();
}