#include "simplewallet.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <cstdio>

std::unordered_map<std::string, int> balanceMap;

std::unordered_map<std::string, int> loadBalance(const std::string &balanceFileName)
{
    std::unordered_map<std::string, int> tempBalanceMap;
    std::ifstream balanceFile(balanceFileName);
    if (!balanceFile)
    {
        std::cerr << "Error opening balance file for reading!" << std::endl;
        return tempBalanceMap;
    }

    std::string line;
    while (std::getline(balanceFile, line))
    {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string accountID;
        int balance;

        std::getline(iss, accountID, ',');
        if (accountID.empty() || !(iss >> balance))
        {
            continue;
        }

        tempBalanceMap[accountID] = balance;
    }
    balanceFile.close();
    return tempBalanceMap;
}

bool saveBalance(const std::string &balanceFileName, const std::unordered_map<std::string, int> &balanceMap)
{
    std::ifstream originalFile(balanceFileName);
    if (!originalFile)
    {
        std::cerr << "Error opening balance file for reading!" << std::endl;
        return false;
    }

    std::ofstream tempFile("temp_balance.txt");
    if (!tempFile)
    {
        std::cerr << "Error opening temporary file for writing!" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(originalFile, line))
    {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string accountID;
        int balance;

        std::getline(iss, accountID, ',');
        if (accountID.empty() || !(iss >> balance))
        {
            continue;
        }

        auto it = balanceMap.find(accountID);
        if (it != balanceMap.end())
        {
            tempFile << accountID << "," << it->second << std::endl;
        }
        else
        {
            tempFile << accountID << "," << balance << std::endl;
        }
    }

    originalFile.close();
    tempFile.close();

    std::remove(balanceFileName.c_str());
    std::rename("temp_balance.txt", balanceFileName.c_str());

    return true;
}

bool updateBalances(const std::string &senderID, int transferringBalance, const std::vector<std::string> &recipientAccounts, const std::vector<int> &recipientBalances)
{
    auto senderIter = balanceMap.find(senderID);
    if (senderIter == balanceMap.end() || senderIter->second < transferringBalance)
    {
        std::cerr << "Transaction failed: Sender does not have sufficient balance or account not found!" << std::endl;
        return false;
    }

    senderIter->second -= transferringBalance;

    for (size_t i = 0; i < recipientAccounts.size(); i++)
    {
        auto recipientIter = balanceMap.find(recipientAccounts[i]);
        if (recipientIter != balanceMap.end())
        {
            recipientIter->second += recipientBalances[i];
        }
        else
        {
            balanceMap[recipientAccounts[i]] = recipientBalances[i];
        }
    }

    return true;
}


void performTransfers(int searchTransactionID, const std::string &fileName)
{
    std::ifstream transactionFile(fileName);
    if (!transactionFile)
    {
        std::cerr << "Error opening transaction file!" << std::endl;
        return;
    }

    std::string line;
    bool transactionFound = false;

    while (std::getline(transactionFile, line))
    {
        std::istringstream iss(line);
        std::string transactionIDStr;
        int k;
        std::string senderID;
        int transferringBalance;

        std::getline(iss, transactionIDStr, ',');
        iss >> k;
        iss.ignore();
        std::getline(iss, senderID, ',');
        iss >> transferringBalance;

        if (transactionIDStr == "txn" + std::to_string(searchTransactionID))
        {
            transactionFound = true;

            std::vector<std::string> recipientAccounts;
            std::vector<int> recipientBalances;

            for (int i = 0; i < k - 1; ++i)
            {
                std::string recipientID;
                int balance;

                std::getline(iss, recipientID, ',');
                iss >> balance;
                iss.ignore();

                recipientAccounts.push_back(recipientID);
                recipientBalances.push_back(balance);
            }

            if (updateBalances(senderID, transferringBalance, recipientAccounts, recipientBalances))
            {
                std::cout << "Transaction successful: " << searchTransactionID << " and balances updated." << std::endl;
            }
            else
            {
                std::cout << "Transaction failed. No balances were updated." << std::endl;
            }

            break;
        }
    }

    if (!transactionFound)
    {
        std::cout << "Transaction ID " << searchTransactionID << " not found in the file." << std::endl;
    }

    transactionFile.close();
}