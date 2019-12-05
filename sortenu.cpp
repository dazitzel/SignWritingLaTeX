#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using namespace std;

fstream fout;
map <string, pair <string, string> > entries;

void usage();
string getline(fstream &fin);
void sort(string filename);

int main(int argc, char**argv)
{
    if(argc==0)
    {
        usage();
        return 0;
    }
    string filename;
    for(int i=1;i<argc;i++)
    {
        filename = argv[i];
        if(filename[0] == '-')
        {
            usage();
            return 0;
        }
        sort(filename);
    }
    map<string,pair<string,string> >::iterator i;
    for(i=entries.begin();i!=entries.end();i++)
    {
        cout<<i->second.first<<endl;
        cout<<i->second.second<<endl;
    }
}

void usage()
{
    cout<<"sorteng"<<endl;
    cout<<endl;
    cout<<"This is a small tool which is used as part of a larger system."<<endl;
    cout<<"What this tool does is search through a file where odd lines are english glosses and even lines are ASL."<<endl;
    cout<<"It then sorts them according to english order."<<endl;
    cout<<"This tools job is to sort all inputs prior repackaging into a multi-lesson glossary."<<endl;
    cout<<endl;
    cout<<"If you really must know, it's expecting a list of files to do the sorting from."<<endl;
    cout<<endl;
    cout<<"Good Luck!"<<endl;
}

string getline(fstream &fin)
{
    string result;
    char c=0;
    while(true)
    {
        if(fin.peek()==-1)
        {
            return result;
        }
        c = static_cast<char>(fin.get());
        if(c==0xa)
        {
            if(fin.peek()==0xd)
            {
                fin.get();
            }
            return result;
        }
        if(c==0xd)
        {
            if(fin.peek()==0xa)
            {
                fin.peek();
            }
            return result;
        }
        result.push_back(c);
    }
}

void sort(string filename)
{
    fstream fin;
    fin.open(filename, ios::in);
    while(fin.peek()>-1)
    {
        string key,enu,asl;
        key = enu = getline(fin);
        asl = getline(fin);
        transform(key.begin(), key.end(), key.begin(), ::tolower);
        asl = asl.substr(0,asl.find('M')) + "B" + asl.substr(asl.find('M')+1);
        entries[key]=make_pair(enu,asl);
    }
    fin.close();
}

