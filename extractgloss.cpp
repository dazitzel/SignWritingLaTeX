#include <fstream>
#include <iostream>

using namespace std;

int main(int argc,char**argv)
{
    /*
        This program will search through a XeLaTeX file for a magic string:
            \begin{glossary}
        Once it finds it, it will then process everything until it finds a
            \end{glossary}
        In between these items, it is expecting pairs of lines:
        one in English and one in ASL.

        While we are doing that we should account for TeX commands so that
        we don't inadvertently think that one of the vocabulary words
        is '\columnbreak', but I don't. Instead I add in keywords that
        have troubled me.
    */
    if((argc!=2) || (argv[1][0]=='-'))
    {
        cout<<"makegloss"<<endl;
        cout<<endl;
        cout<<"This program looks in a file and tries to extract a list or word pairs."<<endl;
        cout<<endl;
        cout<<"These are just bare word pairs, without any formatting."<<endl;
        cout<<"At least, that's the intent."<<endl;
        cout<<endl;
        cout<<"The next steps would be to concatenate the results, sort them (both ways), "
              "and place them within a larger context for printing."<<endl;
        return 0;
    }
    string startmatch("\\begin{glossary}");
    string endmatch("\\end{glossary}");
    string current;
    string first;
    fstream fin;
    fin.open(argv[1],ios::in);
    const int start=0;
    const int glossing=1;
    int state=start;
    while(fin.peek()!=-1)
    {
        if(state==start)
        {
            current.push_back(static_cast<char>(fin.get()));
            if(current.length()>startmatch.length())
                current=current.substr(1);
            if(current==startmatch)
            {
                current="";
                first="";
                state=glossing;
            }
        }
        else // if(state==glossing)
        {
            if(fin.peek()=='\n')
            {
                fin.get();
                if((first != "") && (current!=""))
                {
                    if(first.substr(0,8)!="\\textbf{")
                        throw "Thought this was english!";
                    if(first.substr(first.length()-3)!="}\\\\")
                        throw "Thought this was english!";
                    first=first.substr(8,first.length()-11);
                    while(first.find('/')!=string::npos)
                    {
                        string unu = first.substr(0,first.find('/')-1);
                        first=first.substr(first.find('/')+2);
                        cout<<unu<<endl;
                        cout<<current<<endl;
                    }
                    cout<<first<<endl;
                    cout<<current<<endl;
                }
                first=current;
                current="";
            }
            else
                current.push_back(static_cast<char>(fin.get()));
            if(current==endmatch)
            {
                current="";
                state=start;
            }
        }
    }
}

