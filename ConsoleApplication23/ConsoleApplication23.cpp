// ConsoleApplication23.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// finance.cpp
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <sstream>

using namespace std;

struct Transaction {
    time_t timestamp;
    double amount; // отрицательное = расход
    string category;
    string accountName;
    string note;
};

class Account {
public:
    string name;
    double balance;
    bool isCredit; // true = credit card
    Account(const string& n = "", double b = 0.0, bool cr = false) : name(n), balance(b), isCredit(cr) {}
};

static void to_tm(time_t t, struct tm& out) {
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
}

class Finance {
    vector<Account> accounts;
    vector<Transaction> transactions;
public:
    void addAccount(const string& name, double init = 0.0, bool isCredit = false) {
        accounts.push_back(Account(name, init, isCredit));
    }
    bool findAccount(const string& name) {
        for (auto& a : accounts) if (a.name == name) return true;
        return false;
    }
    void deposit(const string& accName, double amount, const string& note = "") {
        for (auto& a : accounts) if (a.name == accName) { a.balance += amount; break; }
        Transaction t; t.timestamp = time(nullptr); t.amount = amount; t.category = "deposit"; t.accountName = accName; t.note = note;
        transactions.push_back(t);
    }
    void spend(const string& accName, double amount, const string& category, const string& note = "") {
        for (auto& a : accounts) if (a.name == accName) { a.balance -= amount; break; }
        Transaction t; t.timestamp = time(nullptr); t.amount = -amount; t.category = category; t.accountName = accName; t.note = note;
        transactions.push_back(t);
    }

    // helpers to check period
    static bool same_day(time_t t1, time_t t2) {
        struct tm a, b;
        to_tm(t1, a);
        to_tm(t2, b);
        return a.tm_year == b.tm_year && a.tm_yday == b.tm_yday;
    }

    static bool same_month(time_t t1, time_t t2) {
        struct tm a, b;
        to_tm(t1, a);
        to_tm(t2, b);
        return a.tm_year == b.tm_year && a.tm_mon == b.tm_mon;
    }

    static bool within_week(time_t t, time_t ref) {
        // ISO-ish: last 7 days relative to ref (including ref)
        double diff = difftime(ref, t);
        return diff >= 0 && diff <= 7 * 24 * 3600;
    }

    vector<Transaction> filter_period(const string& period) {
        time_t now = time(nullptr);
        vector<Transaction> out;
        for (auto& t : transactions) {
            if (period == "day") {
                if (same_day(t.timestamp, now)) out.push_back(t);
            }
            else if (period == "week") {
                if (within_week(t.timestamp, now)) out.push_back(t);
            }
            else if (period == "month") {
                if (same_month(t.timestamp, now)) out.push_back(t);
            }
        }
        return out;
    }

    // report: returns text
    string report_period(const string& period) {
        vector<Transaction> sel = filter_period(period);
        ostringstream oss;
        oss << "Report for period: " << period << "\n";
        double total_in = 0.0, total_out = 0.0;
        map<string, double> categorySums;
        vector<pair<string, double>> expenseByAccount;
        map<string, double> accMap;

        for (auto& t : sel) {
            if (t.amount >= 0) total_in += t.amount;
            else {
                total_out += -t.amount;
                categorySums[t.category] += -t.amount;
            }
            accMap[t.accountName] += -min(0.0, t.amount); // expense per account
        }

        oss << "Total income: " << total_in << "\n";
        oss << "Total expenses: " << total_out << "\n";

        oss << "\nCategory sums:\n";
        for (auto& p : categorySums) oss << p.first << " : " << p.second << "\n";

        // TOP-3 expenses (by account)
        vector<pair<string, double>> accVec(accMap.begin(), accMap.end());
        sort(accVec.begin(), accVec.end(), [](const pair<string, double>& a, const pair<string, double>& b) { return a.second > b.second; });
        oss << "\nTOP-3 accounts by expense:\n";
        for (size_t i = 0;i < accVec.size() && i < 3;i++) oss << i + 1 << ". " << accVec[i].first << " - " << accVec[i].second << "\n";

        // TOP-3 categories
        vector<pair<string, double>> catVec(categorySums.begin(), categorySums.end());
        sort(catVec.begin(), catVec.end(), [](const pair<string, double>& a, const pair<string, double>& b) { return a.second > b.second; });
        oss << "\nTOP-3 categories:\n";
        for (size_t i = 0;i < catVec.size() && i < 3;i++) oss << i + 1 << ". " << catVec[i].first << " - " << catVec[i].second << "\n";

        // List transactions
        oss << "\nTransactions (" << sel.size() << "):\n";
        for (auto& t : sel) {
            struct tm tmv;
            to_tm(t.timestamp, tmv);
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
        }
        return oss.str();
    }

    void save_report_to_file(const string& period, const string& filename) {
        string r = report_period(period);
        ofstream fout(filename.c_str());
        fout << r;
        fout.close();
    }

    void listAccounts() {
        cout << "Accounts:\n";
        for (auto& a : accounts) cout << " - " << a.name << " balance: " << a.balance << (a.isCredit ? " (credit)" : "") << "\n";
    }
};

// ѕростое меню в main
int main() {
    Finance F;
    int choice = -1;
    while (true) {
        cout << "\n--- Personal Finance ---\n";
        cout << "1. Create account/wallet\n2. Deposit\n3. Spend\n4. List accounts\n5. Report (day/week/month)\n6. Save report to file\n0. Exit\nChoice: ";
        if (!(cin >> choice)) break;
        if (choice == 1) {
            string name; double init; int iscr;
            cout << "Name: "; cin >> ws; getline(cin, name);
            cout << "Initial balance: "; cin >> init;
            cout << "Is credit? (1/0): "; cin >> iscr;
            F.addAccount(name, init, iscr != 0);
        }
        else if (choice == 2) {
            string acc; double amt; string note;
            cout << "Account name: "; cin >> ws; getline(cin, acc);
            cout << "Amount: "; cin >> amt;
            cout << "Note: "; cin >> ws; getline(cin, note);
            if (!F.findAccount(acc)) cout << "No such account.\n"; else F.deposit(acc, amt, note);
        }
        else if (choice == 3) {
            string acc, cat, note; double amt;
            cout << "Account name: "; cin >> ws; getline(cin, acc);
            cout << "Amount: "; cin >> amt;
            cout << "Category: "; cin >> ws; getline(cin, cat);
            cout << "Note: "; cin >> ws; getline(cin, note);
            if (!F.findAccount(acc)) cout << "No such account.\n"; else F.spend(acc, amt, cat, note);
        }
        else if (choice == 4) {
            F.listAccounts();
        }
        else if (choice == 5) {
            string p;
            cout << "Period (day/week/month): "; cin >> p;
            cout << F.report_period(p) << endl;
        }
        else if (choice == 6) {
            string p, fn;
            cout << "Period (day/week/month): "; cin >> p;
            cout << "Filename: "; cin >> fn;
            F.save_report_to_file(p, fn);
            cout << "Saved to " << fn << "\n";
        }
        else if (choice == 0) break;
    }
    return 0;
}
