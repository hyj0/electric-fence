#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <vector>
using namespace std;

class Cl {
public:
    Cl(int a,  string str)
    {
            this->a = a;
            this->str = make_shared<string>(str);
    }
    int a;
    shared_ptr<string> str;
};

int main()
{
        map<string, shared_ptr<Cl>> strClMap;
        vector<string>  ks = {
                "a", "b", "c"
        };
        vector<string> vs = {
                "x", "y", "z"
        };
        for (int i = 0; i < ks.size(); ++i) {
                strClMap.insert(make_pair(ks[i], make_shared<Cl>(i, vs[i])));
        }
        cout << "end" << endl;
}