#ifndef ProductOrderAtomacity_H
#define ProductOrderAtomacity_H

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>

using namespace std;

struct Order {
    string productId;
    int quantity;
};

// Declare a global productMap shared by all files
extern unordered_map<string, int> productMap;

// Function declarations
unordered_map<string, int> loadProducts(const string& productFile);
bool checkSufficientQuantity(const unordered_map<string, int>& productMap, const vector<Order>& orderList);
bool updateProductQuantities(unordered_map<string, int>& productMap, const vector<Order>& orderList);
bool processOrder(unordered_map<string, int>& productMap, const vector<Order>& orderList);
void processOrders(const int txn_id, const string& filename);
bool saveProducts( const string& productFile,const unordered_map<string, int>& productMap);

#endif
