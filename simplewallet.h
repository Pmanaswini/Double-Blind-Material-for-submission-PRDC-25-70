#ifndef SIMPLEWALLET_H
#define SIMPLEWALLET_H

#include <unordered_map>
#include <string>
#include <vector>

extern std::unordered_map<std::string, int> balanceMap;

// Function to load balances from a file into the balanceMap
std::unordered_map<std::string, int> loadBalance(const std::string &balanceFileName);

// Function to save the balanceMap to a file
bool saveBalance(const std::string &balanceFileName, const std::unordered_map<std::string, int> &balanceMap);

// Function to update balances in the balanceMap
bool updateBalances(const std::string &senderID, int transferringBalance, const std::vector<std::string> &recipientAccounts, const std::vector<int> &recipientBalances);

// Function to perform transfers based on the transaction ID
void performTransfers(int searchTransactionID, const std::string &fileName);


// Function to display balance information
// void displayFinalBalances(const std::string &transactionID, int k, const std::string &senderID, double transferringBalance, const std::vector<std::string> &recipientAccounts, const std::vector<double> &recipientBalances);

#endif // SIMPLEWALLET_H



