// ConsoleApplication23.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// finance.cpp
// тут кода конечно гораздо больше..
#include <iostream>
#include <string>
#include <vector>
#include <map> // крч библиотека делает что то на подобии показателей. (на умном щас будет) Контейнер который сохраняет набор данных формату ключ - значение (другими словами асоциативный масив)
#include <ctime> // Библиотека, что бы работать с датой, временем.
#include <fstream>
#include <algorithm>
#include <sstream> // используется для потоковой работы со строками, и тд. UPD: нужно для oss <<

using namespace std;

struct Transaction {
    time_t timestamp;
    double amount;
    string category;
    string accountName;
    string note;
};

class Account {
public:
    string name;
    double balance;
    bool isCredit; // если есть карта кредитная = тру
    Account(const string& n = "", double b = 0.0, bool cr = false) : name(n), balance(b), isCredit(cr) {}
}; // уже константа говорит что данные аккаунта не изменяется. Крч тут создается класс (аккаунт), который будет создавать обьекты
// то лишь созданные аккаунты, в которых собственно будет хранится инфа которую мы вписали

// сделайте вид что этих 7 строк не существует, полтора часа ушло на то что бы ее пофиксить, в итоге пришлось взять с инета строчки
// тут работало на линуксе, я переделал что бы работало на виндоусе, не спрашивайте меня что это, я не смогу обьяснить
static void to_tm(time_t t, struct tm& out) {
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
}

// программа, сделанная через класс .-.
class Finance {
    vector<Account> accounts;
    vector<Transaction> transactions;
public:
    void addAccount(const string& name, double init = 0.0, bool isCredit = false) {
        accounts.push_back(Account(name, init, isCredit));
        // пуш бэк = массив расширяется
        // тут добавляется аккаунт
    }
    bool findAccount(const string& name) {
        for (auto& a : accounts) if (a.name == name) return true;
        return false;
        // тут ищем аккаунт, если слово что мы написали похожее на то что существует, находит короче.
    }
        // дэпать в казик. Пишем имя акка, число... подпись?
    void deposit(const string& accName, double amount, const string& note = "") {
        for (auto& a : accounts) if (a.name == accName) { a.balance += amount; break; }
        // чек
        Transaction t; t.timestamp = time(nullptr); t.amount = amount; t.category = "deposit"; t.accountName = accName; t.note = note;
        transactions.push_back(t);
        // расширение територии: 10000 вопросов в шаге
    }
        // потратить деньжата, пишем все тоже что и в прошлый раз, но пишим на что мы будем тратится.
    void spend(const string& accName, double amount, const string& category, const string& note = "") {
        for (auto& a : accounts) if (a.name == accName) { a.balance -= amount; break; }
        Transaction t; t.timestamp = time(nullptr); t.amount = -amount; t.category = category; t.accountName = accName; t.note = note;
        transactions.push_back(t);
        // следующие строчки обьяснять не буду, тупо копипаста
    }

    // как раз тут применяется все то что я писал ранее насчет тех 7 строк кода...
    // в 2 булах
    // (ШПАРГАЛКА) статик значит что значение статическое
    static bool same_day(time_t t1, time_t t2) {
        struct tm a, b;
        to_tm(t1, a);
        to_tm(t2, b);
        return a.tm_year == b.tm_year && a.tm_yday == b.tm_yday;
        // && = мини версия иф элз элиф
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

// скорее всего прошлые булы фиксируют когда были проведены транзакции.
// Тут функция фильтрует транзакции по дате.

    // отчет
    string report_period(const string& period) {
        vector<Transaction> sel = filter_period(period);
        ostringstream oss; 
        // вместо того что бы записывать в файл, записывает в строковый буфер (переделает типы данных в стринг)
        oss << "Report for period: " << period << "\n";
        double total_in = 0.0, total_out = 0.0;
        map<string, double> categorySums;
        vector<pair<string, double>> expenseByAccount;
        map<string, double> accMap;
        // стринг ключ, дабл значение. В будущем можно будет получить по ключу. (с accMap тоже самое все.)

        // Растраты
        for (auto& t : sel) {
            if (t.amount >= 0) total_in += t.amount;
            else {
                total_out += -t.amount;
                categorySums[t.category] += -t.amount;
            }
            accMap[t.accountName] += -min(0.0, t.amount);
        }

        // общие доходы, растраты, категория растрат
        oss << "Total income: " << total_in << "\n";
        oss << "Total expenses: " << total_out << "\n";
        oss << "\nCategory sums:\n";
        for (auto& p : categorySums) oss << p.first << " : " << p.second << "\n";

        // Следующие сортировки по убыванию
         
        // топ 3 транжира (по аккаунту)
        vector<pair<string, double>> accVec(accMap.begin(), accMap.end());
        sort(accVec.begin(), accVec.end(), [](const pair<string, double>& a, const pair<string, double>& b) { return a.second > b.second; });
        oss << "\nTOP-3 accounts by expense:\n";
        for (size_t i = 0;i < accVec.size() && i < 3;i++) oss << i + 1 << ". " << accVec[i].first << " - " << accVec[i].second << "\n";

        // топ 3 категорий растрат
        vector<pair<string, double>> catVec(categorySums.begin(), categorySums.end());
        sort(catVec.begin(), catVec.end(), [](const pair<string, double>& a, const pair<string, double>& b) { return a.second > b.second; });
        oss << "\nTOP-3 categories:\n";
        for (size_t i = 0;i < catVec.size() && i < 3;i++) oss << i + 1 << ". " << catVec[i].first << " - " << catVec[i].second << "\n";

        // список транзакций + когда была произведена
        oss << "\nTransactions (" << sel.size() << "):\n";
        for (auto& t : sel) {
            struct tm tmv;
            to_tm(t.timestamp, tmv);
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
        }
        return oss.str();
    }

    // Сохраняет отчет в файл
    
        void save_report_to_file(const string& period, const string& filename) {
        string r = report_period(period);
        ofstream fout(filename.c_str());
        fout << r;
        fout.close();
    }
    
    // я думаю не трудно догадаться
    
    void listAccounts() {
        cout << "Accounts:\n";
        for (auto& a : accounts) cout << " - " << a.name << " balance: " << a.balance << (a.isCredit ? " (credit)" : "") << "\n";
    }
};

// запускаем нашу дешевую копию приват24
int main() {
    Finance F;
    int choice = -1;
    // спрашиваем че делать, в каждом иф элз мы просто пишем данные и все...
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
    // фин
}
