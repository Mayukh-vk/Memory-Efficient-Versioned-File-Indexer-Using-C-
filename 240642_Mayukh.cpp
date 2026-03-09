#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>

using namespace std;


template <typename T>
void printResult(string label, T value) {
    cout << label << ": " << value << endl;
}


bool compareFrequencies(pair<string, int> a, pair<string, int> b) {
    if (a.second == b.second) {
        return a.first < b.first; // Alphabetical tie-breaker if frequencies are equal
    }
    return a.second > b.second; 
}


class Query {
public:
    virtual ~Query() {}
    
    virtual void execute(map<string, map<string, int> >& allVersions) = 0;
};

// Derived Class 1
class WordQuery : public Query {
private:
    string vName;
    string word;
public:
    WordQuery(string v, string w) {
        vName = v;
        word = w;
    }
    void execute(map<string, map<string, int> >& allVersions) {
        int count = 0;
        
        if (allVersions.find(vName) != allVersions.end()) {
            if (allVersions[vName].find(word) != allVersions[vName].end()) {
                count = allVersions[vName][word];
            }
        }
        cout << "Version: " << vName << endl;
        printResult("Frequency of '" + word + "'", count);
    }
};

// Derived Class 2
class TopKQuery : public Query {
private:
    string vName;
    int k;
public:
    TopKQuery(string v, int topK) {
        vName = v;
        k = topK;
    }
    void execute(map<string, map<string, int> >& allVersions) {
        cout << "Version: " << vName << endl;
        if (allVersions.find(vName) == allVersions.end()) {
            cout << "Version not found." << endl;
            return;
        }

        
        vector< pair<string, int> > freqVec;
        map<string, int>::iterator it;
        for (it = allVersions[vName].begin(); it != allVersions[vName].end(); ++it) {
            freqVec.push_back(*it);
        }

        sort(freqVec.begin(), freqVec.end(), compareFrequencies);

        cout << "Top " << k << " words:" << endl;
        for (int i = 0; i < k && i < freqVec.size(); ++i) {
            cout << i + 1 << ". " << freqVec[i].first << " -> " << freqVec[i].second << endl;
        }
    }
};

// Derived Class 3
class DiffQuery : public Query {
private:
    string v1;
    string v2;
    string word;
public:
    DiffQuery(string ver1, string ver2, string w) {
        v1 = ver1;
        v2 = ver2;
        word = w;
    }
    void execute(map<string, map<string, int> >& allVersions) {
        int c1 = 0;
        int c2 = 0;
        
        if (allVersions.find(v1) != allVersions.end() && allVersions[v1].find(word) != allVersions[v1].end()) {
            c1 = allVersions[v1][word];
        }
        if (allVersions.find(v2) != allVersions.end() && allVersions[v2].find(word) != allVersions[v2].end()) {
            c2 = allVersions[v2][word];
        }
        
        cout << "Versions: " << v1 << " and " << v2 << endl;
        int diff = c1 - c2;
        if (diff < 0) diff = -diff; 
        printResult("Difference for '" + word + "'", diff);
    }
};


class FileProcessor {
private:
    int bufferSize;
public:
    FileProcessor(int kb) {
        // exception handling requirement
        if (kb < 256 || kb > 1024) {
            throw runtime_error("Buffer size must be between 256 and 1024 KB");
        }
        bufferSize = kb * 1024; 
    }

    void buildIndex(string path, map<string, int>& index) {
        ifstream file(path.c_str(), ios::binary);
        if (!file.is_open()) {
            throw runtime_error("Could not open file: " + path);
        }

        
        char* buffer = new char[bufferSize];
        string leftover = "";

        // Read incrementally
        while (file.read(buffer, bufferSize) || file.gcount() > 0) {
            int bytesRead = file.gcount();
            string chunk = leftover;
            chunk.append(buffer, bytesRead);
            leftover = "";

            string currentWord = "";
            for (int i = 0; i < chunk.length(); i++) {
                char c = chunk[i];
                if (isalnum(c)) {
                    //Case-insensitive
                    if (c >= 'A' && c <= 'Z') {
                        currentWord += (c + 32); 
                    } else {
                        currentWord += c;
                    }
                } else {
                    if (currentWord.length() > 0) {
                        index[currentWord]++;
                        currentWord = "";
                    }
                }
            }
            leftover = currentWord; 
        }
        
        //for last word if the file ends without a space
        if (leftover.length() > 0) {
            index[leftover]++;
        }
        
        delete[] buffer;
        file.close();
    }
};

// VERSION MANAGEMENT
class VersionManager {
private:
    map<string, map<string, int> > versions;
    int bufSizeKB;
public:
    VersionManager(int kb) {
        bufSizeKB = kb;
    }

    //Single File
    void loadVersion(string name, string path) {
        FileProcessor fp(bufSizeKB);
        fp.buildIndex(path, versions[name]);
    }

    //Two Files
    void loadVersion(string n1, string p1, string n2, string p2) {
        loadVersion(n1, p1);
        loadVersion(n2, p2);
    }

    map<string, map<string, int> >& getVersions() {
        return versions;
    }
};


int main(int argc, char* argv[]) {
    // Start timing 
    auto start = chrono::high_resolution_clock::now();

    try {
        
        string file1 = "", file2 = "", version1 = "", version2 = "", queryType = "", word = "";
        int bufferKB = 0, topK = 0;

        
        for (int i = 1; i < argc; i++) {
            string arg = argv[i];
            if (arg == "--file" || arg == "--file1") { file1 = argv[++i]; }
            else if (arg == "--file2") { file2 = argv[++i]; }
            else if (arg == "--version" || arg == "--version1") { version1 = argv[++i]; }
            else if (arg == "--version2") { version2 = argv[++i]; }
            else if (arg == "--buffer") { bufferKB = stoi(argv[++i]); }
            else if (arg == "--query") { queryType = argv[++i]; }
            else if (arg == "--word") { word = argv[++i]; }
            else if (arg == "--top") { topK = stoi(argv[++i]); }
        }

        
        if (bufferKB == 0 || queryType == "") {
            throw runtime_error("Missing required arguments (--buffer or --query).");
        }

        
        VersionManager vm(bufferKB);
        Query* query = NULL;

        
        if (queryType == "word") {
            vm.loadVersion(version1, file1);
            query = new WordQuery(version1, word);
        } 
        else if (queryType == "top") {
            vm.loadVersion(version1, file1);
            query = new TopKQuery(version1, topK);
        } 
        else if (queryType == "diff") {
            vm.loadVersion(version1, file1, version2, file2);
            query = new DiffQuery(version1, version2, word);
        } 
        else {
            throw runtime_error("Invalid query type. Use word, top, or diff.");
        }

        
        if (query != NULL) {
            query->execute(vm.getVersions());
            delete query;
        }

        //execution time
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;

        cout << "-----------------------------------" << endl;
        printResult("Allocated Buffer Size (KB)", bufferKB);
        printResult("Execution Time (seconds)", elapsed.count());

    } catch (const exception& e) {
        // Catch and print exceptions gracefully
        cout << "Exception Occurred: " << e.what() << endl;
        return 1;
    }

    return 0;
}