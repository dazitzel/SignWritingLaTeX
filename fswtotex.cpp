#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

/*
    This parser works with as state machines within state machines.

    There is also a general error state which says that we failed in our matching and need
    to spit out what we have saved so far.

    Our outer state machine has the following states:
        Start       -- the obligatory starting state
        Punctuation -- processing a punctuation sign (can be done visually as well)
        Prefix      -- processing the temporal prefix (can be missing)
        Visual      -- processing the visual layout of the sign (required)
        End         -- Time to spit out what we have

    The Punctuation state machine has the following states:
        Symbol      -- processing a base symbol
        Placement   -- processing the placement of the symbol
        End         -- Time for the outer state machine to finish

    The prefix state machine has the following states:
        Start       -- the obligatory starting state
        Symbol      -- processing a base symbol
        End         -- If we used this state machine, time for the outer state machine to move to the next state

    The visual state machine has the following states:
        Start       -- the obligatory starting state
        Size        -- processing the size of the word
        Symbol      -- processing a base symbol
        Placement   -- processing the placement of the symbol
        End         -- Time for the outer state machine to finish

    The Symbol state machine has the following states
        Start       -- the obligatory starting state
        FirstDigit  -- the first digit
        SecondDigit -- the second digit
        ThirdDigt   -- the third digit
        Fill        -- the fill
        Rotation    -- the rotation
        End         -- Time for the outer state machine to finish

    The Size state machine has the following states:
        Start       -- the obligatory starting state
        FirstW      -- the first digit of the width
        SecondW     -- the second digit of the width
        ThirdW      -- the third digit of the width
        x           -- the letter `x'
        FirstH      -- the first digit of the height
        SecondH     -- the second digit of the height
        ThirdH      -- the third digit of the height
        End         -- Time for the outer state machine to finish

    The Placement state machine is exactly the same as the size state machine
*/

const int s_error=-1;
const int s_start=0;
const int s_punctuation=1;
const int s_prefix=2;
const int s_visual=3;
const int s_size=1;
const int s_symbol=2;
const int s_placement=3;
const int s_first=1;
const int s_second=2;
const int s_third=3;
const int s_fill=4;
const int s_rotation=5;
const int s_firstw=1;
const int s_secondw=2;
const int s_thirdw=3;
const int s_x=4;
const int s_firsth=5;
const int s_secondh=6;
const int s_thirdh=7;
const int s_end=8;

int state      =s_start;
int substate   =s_start;
int subsubstate=s_start;

/*
    Now we come to reading.

    If we want to be able to read from a wide variety of files, then we
    have to be ready for all the different encodings and be able to convert
    each of them into utf-32.

    We will begin here with a set of functions to read a character of a
    given type. Unknown or undetermined as of yet and utf8/16/32/le/be. We
    will then have a set of functions to convert to utf32(internal version)
    from all the other types of characters.

    Even though these are technically characters, because we are dealing
    with SignWriting characters which are outside of plane-0 we are going
    to pass them around as if they are unsigned integers anyway.
*/

enum theTextFormat
{
    unknown, utf8, utf16le, utf16be, utf32le, utf32be
} textFormat = unknown;

uint32_t getChar(istream* fileIn);

uint32_t getCharUnknown(istream* fileIn);
uint32_t getCharUtf8   (istream* fileIn);
uint32_t getCharUtf16le(istream* fileIn);
uint32_t getCharUtf16be(istream* fileIn);
uint32_t getCharUtf32le(istream* fileIn);
uint32_t getCharUtf32be(istream* fileIn);

int utf8ToUtf32   (uint8_t* c, uint32_t* fileOut);
int utf16leToUtf32(uint8_t* c, uint32_t* fileOut);
int utf16beToUtf32(uint8_t* c, uint32_t* fileOut);
int utf32leToUtf32(uint8_t* c, uint32_t* fileOut);
int utf32beToUtf32(uint8_t* c, uint32_t* fileOut);

uint32_t getCharUnknown(istream* fileIn)
{
    uint32_t result=0;
    // feff is the byte order
    // In each format, this can serve as a key
    // utf  8    ef bb bf
    // utf 16 le ff fe
    // utf 16 be fe ff
    // utf 32 le ff fe 00 00
    // utf 32 be 00 00 fe ff
    // So if we are unknown then we look for those,
    // and if we don't see any of them we default to utf8
    static uint8_t charBuff[4] = {0, 0, 0, 0};
    static int charBuffSize=0;
    if(charBuffSize==0)
    {
        for(;charBuffSize<4;charBuffSize++)
        {
            charBuff[charBuffSize]=static_cast<uint8_t>(fileIn->get());
        }
        if(charBuff[0]==0x0  && charBuff[1]==0x0 &&
           charBuff[2]==0xfe && charBuff[3]==0xff)
        {
            textFormat=utf32be;
            return getCharUtf32be(fileIn);
        }
        else if(charBuff[0]==0xff && charBuff[1]==0xfe &&
                charBuff[2]==0x0  && charBuff[3]==0x0)
        {
            textFormat=utf32le;
            return getCharUtf32le(fileIn);
        }
        else if(charBuff[0]==0xfe && charBuff[1]==0xff)
        {
            textFormat=utf16be;
            charBuff[0]=charBuff[2];
            charBuff[1]=charBuff[3];
            charBuff[2]=0;
            charBuff[3]=0;
            if(utf16beToUtf32(charBuff,&result)==2)
            {
                return result;
            }
            charBuff[2]=static_cast<uint8_t>(fileIn->get());
            charBuff[3]=static_cast<uint8_t>(fileIn->get());
            if(utf16beToUtf32(charBuff,&result)==4)
            {
                return result;
            }
            throw "Badly formed utf16be string.";
        }
        else if(charBuff[0]==0xff && charBuff[1]==0xfe)
        {
            textFormat=utf16le;
            charBuff[0]=charBuff[2];
            charBuff[1]=charBuff[3];
            charBuff[2]=0;
            charBuff[3]=0;
            if(utf16leToUtf32(charBuff,&result)==2)
            {
                return result;
            }
            charBuff[2]=static_cast<uint8_t>(fileIn->get());
            charBuff[3]=static_cast<uint8_t>(fileIn->get());
            if(utf16leToUtf32(charBuff,&result)==4)
            {
                return result;
            }
            throw "Badly formed ut16le string.";
        }
        else if(charBuff[0]==0xef && charBuff[1]==0xbb &&
                charBuff[2]==0xbf)
        {
            textFormat=utf8;
            charBuff[0]=charBuff[3];
            charBuff[1]=0;
            charBuff[2]=0;
            charBuff[3]=0;
            if(utf8ToUtf32(charBuff,&result)==1)
            {
                return result;
            }
            charBuff[1]=static_cast<uint8_t>(fileIn->get());
            if(utf8ToUtf32(charBuff,&result)==2)
            {
                return result;
            }
            charBuff[2]=static_cast<uint8_t>(fileIn->get());
            if(utf8ToUtf32(charBuff,&result)==3)
            {
                return result;
            }
            charBuff[3]=static_cast<uint8_t>(fileIn->get());
            if(utf8ToUtf32(charBuff,&result)==4)
            {
                return result;
            }
            throw "Badly formed utf8 string.";
        }
    }
    int offset = utf8ToUtf32(charBuff,&result);
    while(offset<0 && charBuffSize<4)
    {
        charBuff[charBuffSize]=static_cast<uint8_t>(fileIn->get());
        charBuffSize++;
        offset = utf8ToUtf32(charBuff,&result);
    }
    int i;
    for(i=0;i<charBuffSize-offset;i++)
    {
        charBuff[i]=charBuff[i+offset];
    }
    for(;i<charBuffSize;i++)
    {
        charBuff[i]=0;
    }
    charBuffSize-=offset;
    if(charBuffSize==0)
    {
        textFormat=utf8;
    }
    return result;
}

uint32_t getCharUtf8(istream* fileIn)
{
    uint8_t charBuff[4]={0,0,0,0};
    int read=0;
    int size=0;
    uint32_t result=0;
    do
    {
        read=(fileIn->get());
        if(read==-1)
            return 0xffffffff;
        charBuff[size++]=static_cast<uint8_t>(read);
    }
    while((size<4) && (utf8ToUtf32(charBuff,&result)!=size));
    if(utf8ToUtf32(charBuff,&result)==size)
    {
        return result;
    }
    throw "Badly formed utf8 string.";
}

uint32_t getCharUtf16le(istream* fileIn)
{
    uint8_t charBuff[4]={0,0,0,0};
    int read=0;
    int size=0;
    uint32_t result=0;
    do
    {
        read=(fileIn->get());
        if(read==-1)
            return 0xffffffff;
        charBuff[size++]=static_cast<uint8_t>(read);
        read=(fileIn->get());
        if(read==-1)
            return 0xffffffff;
        charBuff[size++]=static_cast<uint8_t>(read);
    }
    while((size<4) && (utf16leToUtf32(charBuff,&result)!=size));
    if(utf16leToUtf32(charBuff,&result)==size)
    {
        return result;
    }
    throw "Badly formed utf16le string.";
}

uint32_t getCharUtf16be(istream* fileIn)
{
    uint8_t charBuff[4]={0,0,0,0};
    int read=0;
    int size=0;
    uint32_t result=0;
    do
    {
        read=(fileIn->get());
        if(read==-1)
            return 0xffffffff;
        charBuff[size++]=static_cast<uint8_t>(read);
        read=(fileIn->get());
        if(read==-1)
            return 0xffffffff;
        charBuff[size++]=static_cast<uint8_t>(read);
    }
    while((size<4) && (utf16beToUtf32(charBuff,&result)!=size));
    if(utf16beToUtf32(charBuff,&result)==size)
    {
        return result;
    }
    throw "Badly formed utf16be string.";
}

uint32_t getCharUtf32le(istream* fileIn)
{
    uint8_t charBuff[4]={0,0,0,0};
    int read=0;
    int size=0;
    uint32_t result;
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    utf32leToUtf32(charBuff,&result);
    return result;
}

uint32_t getCharUtf32be(istream* fileIn)
{
    uint8_t charBuff[4]={0,0,0,0};
    int read=0;
    int size=0;
    uint32_t result;
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    read=(fileIn->get());
    if(read==-1)
        return 0xffffffff;
    charBuff[size++]=static_cast<uint8_t>(read);
    utf32beToUtf32(charBuff,&result);
    return result;
}

uint32_t getChar(istream* fileIn)
{
    switch(textFormat)
    {
        case utf8   : return getCharUtf8   (fileIn);
        case utf16le: return getCharUtf16le(fileIn);
        case utf16be: return getCharUtf16be(fileIn);
        case utf32le: return getCharUtf32le(fileIn);
        case utf32be: return getCharUtf32be(fileIn);
        default     : return getCharUnknown(fileIn);
    }
}

/*
    We always convert to utf-8 on output since we know that we are
    outputting a lot of XeLaTeX code with expanded characters of the
    format ``\char#xxxx''.
*/

string utf32ToUtf8(uint32_t c)
{
    string to;
    if(c<0x80)
    {
        to.push_back(static_cast<char>(c));
    }
    else if(c<0x800)
    {
        to.push_back(static_cast<char>(((c>>6)&0x1f)|0xc0));
        to.push_back(static_cast<char>(((c>>0)&0x3f)|0x80));
    }
    else if(c<0x10000)
    {
        to.push_back(static_cast<char>(((c>>12)&0xf)|0xe0));
        to.push_back(static_cast<char>(((c>>6)&0x3f)|0x80));
        to.push_back(static_cast<char>(((c>>0)&0x3f)|0x80));
    }
    else
    {
        to.push_back(static_cast<char>(((c>>18)&0x7)|0xf0));
        to.push_back(static_cast<char>(((c>>12)&0x3f)|0x80));
        to.push_back(static_cast<char>(((c>>6)&0x3f)|0x80));
        to.push_back(static_cast<char>(((c>>0)&0x3f)|0x80));
    }
    return to;
}

// And now we send it out
void sendOut(ostream* fileOut, uint32_t c)
{
    (*fileOut)<<utf32ToUtf8(c);
}

void sendOut(ostream* fileOut, vector<uint32_t> &l,uint32_t c)
{
    for(unsigned int i=0;i<l.size();i++)
        sendOut(fileOut, l[i]);
    sendOut(fileOut,c);
    l.clear();
    state=substate=subsubstate=s_start;
}

// Now that we have been using all those ``convert to uft32'', let's define them.

int utf8ToUtf32   (uint8_t* coming, uint32_t* going)
{
    // Quick review:
    //    0xxx xxxx
    //    110x xxxx  10xx xxxx
    //    1110 xxxx  10xx xxxx  10xx xxxx
    //    1111 -xxx  10xx xxxx  10xx xxxx  10xx xxxx
    if((coming[0]&0xf0)==0xf0)
    {
        going[0]=coming[0]&0x7;
        for(int i=1;i<4;i++)
        {
            if((coming[i]&0xc0)!=0x80)
                return 0;
            going[0]<<=6;
            going[0]|=(coming[i]&0x3f);
        }
        return 4;
    }
    if((coming[0]&0xf0)==0xe0)
    {
        going[0]=coming[0]&0xf;
        for(int i=1;i<3;i++)
        {
            if((coming[i]&0xc0)!=0x80)
                return 0;
            going[0]<<=6;
            going[0]|=(coming[i]&0x3f);
        }
        return 3;
    }
    if((coming[0]&0xe0)==0xc0)
    {
        going[0]=coming[0]&0x1f;
        for(int i=1;i<2;i++)
        {
            if((coming[i]&0xc0)!=0x80)
                return 0;
            going[0]<<=6;
            going[0]|=(coming[i]&0x3f);
        }
        return 2;
    }
    if((coming[0]&0xc0)==0x80)
    {
        throw "Malformed utf8 string.";
    }
    going[0]=coming[0];
    return 1;
}

int utf16leToUtf32(uint8_t* coming, uint32_t* going)
{
    // Quick review, after accounting for endianess
    // 0000--d7ff = pass along
    // d800--d8ff = a following dc00-dfff tells the low 10 bits
    // dc00--dfff = better be following d800-d8ff
    // e000--ffff = pass along
    going[0] =coming[0]<<0;
    going[0]|=coming[1]<<8;
    if(going[0]<0xd800)
        return 2;
    if(going[0]<0xdc00)
    {
        if(coming[3]<0xdc || coming[3]>0xdf)
            return 0;
        going[0] &=0x3ff;
        going[0] |=0x400;
        going[0]<<=10;
        going[0] |=coming[2];
        going[0] |=(coming[3]&0x3)<<8;
        return 2;
    }
    if(going[0]<0xe000)
        throw "Malformed utf16le string";
    return 2;
}

int utf16beToUtf32(uint8_t* coming, uint32_t* going)
{
    // Quick review, after accounting for endianess
    // 0000--d7ff = pass along
    // d800--d8ff = a following dc00-dfff tells the low 10 bits
    // dc00--dfff = better be following d800-d8ff
    going[0] =coming[0]<<8;
    going[0]|=coming[1]<<0;
    if(going[0]<0xd800)
        return 2;
    if(going[0]<0xdc00)
    {
        if(coming[2]<0xdc || coming[2]>0xdf)
            return 0;
        going[0] &=0x3ff;
        going[0] |=0x400;
        going[0]<<=10;
        going[0] |=(coming[2]&0x3)<<8;
        going[0] |=coming[3];
        return 2;
    }
    if(going[0]<0xe000)
        throw "Malformed utf16le string";
    return 2;
}

int utf32leToUtf32(uint8_t* coming, uint32_t* going)
{
    going[0] =coming[0]<< 0;
    going[0]|=coming[1]<< 8;
    going[0]|=coming[2]<<16;
    going[0]|=coming[3]<<24;
    return 4;
}

int utf32beToUtf32(uint8_t* coming, uint32_t* going)
{
    going[0] =coming[0]<<24;
    going[0]|=coming[1]<<16;
    going[0]|=coming[2]<< 8;
    going[0]|=coming[3]<< 0;
    return 4;
}

// Finally, let's explain and start the program

int usage()
{
    cout<<"This is fswtotex."<<endl;
    cout<<endl;
    cout<<"This program runs in one of three ways."<<endl;
    cout<<"If you provide no arguments, we read from standard in and send the results to standard out."<<endl;
    cout<<"If you provide one argument, we read from that file and send the results to standard out."<<endl;
    cout<<"If you provide two arguments, we read from the first file and send the results to the second file."<<endl;
    cout<<endl;
    cout<<"So what do we do?"<<endl;
    cout<<endl;
    cout<<"We read LaTeX code with embedded SignWriting words (see http://signwriting.org) and output LaTeX "
          "code with TiKz drawings of SignWriting symbols."<<endl;
    cout<<endl;
    cout<<"There are a few modifications you can take advantage of though:"<<endl;
    cout<<"--fsize <string>  By default we assume a string called f@size holds the size which is"<<endl;
    cout<<"                  generally a usable value to use in LaTex documents. You can change"<<endl;
    cout<<"                  this if (for instance) you want SignWriting text to be a different"<<endl;
    cout<<"                  size or you already have some other length being used. Fswtotex does"<<endl;
    cout<<"                  add a \\ in front of the string you provide."<<endl;
    cout<<"--nomirror        By default we mirror, this turns that off."<<endl;
    cout<<"--rotate <number> By default we assume a value of -90. The reason we default to mirrored"<<endl;
    cout<<"                  and rotated by -90 degrees pages is because SignWriting is a vertical"<<endl;
    cout<<"                  writing system. By rotating the page by -90 degrees we turn horizontal"<<endl;
    cout<<"                  lines into vertical columns. But this alone would make for columns of"<<endl;
    cout<<"                  text moving from right to left, so by adding a mirror to the text we"<<endl;
    cout<<"                  get vertial columns of text moving from left to right. The next most"<<endl;
    cout<<"                  likely settings you may want would be --nomirror --rotate 0, as this"<<endl;
    cout<<"                  allows for insertion of SignWriting in the midst of English text but"<<endl;
    cout<<"                  does require the author to handle things like placing the text into"<<endl;
    cout<<"                  a multi-column environment and adding linebreaks after each word."<<endl;
    cout<<"--spelling        By default, we don't spell. If this option is added then columns of"<<endl;
    cout<<"                  of symbols will appear above the word if it has a time domain prefix."<<endl;
    return 0;
}

int fswtotex(istream *fileIn,ostream *fileOut);

const string defaultfsize="f@size";
string fsize=defaultfsize;
bool hasat=true;
bool mirror=true;
int rotation=-90;
bool spelling=false;

int main(int argc,char**argv)
{
    //  We an run in about three different ways.
    //  1)  read from standard in and write to standard out (0 arguments)
    //  2)  read from a file and write to standard out (1 argument)
    //  3)  read from a file and write to a file (2 arguments)
    //  In order to accomplish this, we first process the arguments,
    //  and then set up in istream and ostream, and let it rip.
    //  When processing the arguments we also have to consider the arguments to control
    //    fsize string
    //    mirroring
    //    rotation

    int fileCounts=0;
    string fileNames[2];
    int result=-1;
    for(int i=1;i<argc;i++)
    {
        if(string(argv[i])=="--fsize")
        {
            i++;
            if(i>=argc)
            {
                cout<<"--fsize requires a following string"<<endl;
                return result;
            }
            fsize=argv[i];
            if(fsize.find("@")==string::npos)
                hasat = false;
        }
        else if(string(argv[i])=="--nomirror")
        {
            mirror=false;
        }
        else if(string(argv[i])=="--rotate")
        {
            i++;
            if(i>=argc)
            {
                cout<<"--rotate requires a following number"<<endl;
                return result;
            }
            rotation = atoi(argv[i]);
        }
        else if(string(argv[i])=="--spelling")
        {
            spelling=true;
        }
        else if(argv[i][0]=='-')
        {
            return usage();
        }
        else
        {
            if(fileCounts>=2)
            {
                cout<<"We can only accept two files, and the second one is overwritten."<<endl;
                return result;
            }
            fileNames[fileCounts++]=argv[i];
        }
    }
    try
    {
        if(fileCounts==0)
        {
            // standard in and standard out
            result=fswtotex(&cin,&cout);
            cout<<"% This file was generated by:"<<endl;
            cout<<"%    ";
            for(int i=0;i<argc;i++)
                cout<<argv[i]<<" ";
            cout<<endl;
        }
        else if(fileCounts==1)
        {
            // file to standard out
            fstream fin;
            fin.open(fileNames[0],ios::in);
            result=fswtotex(&fin,&cout);
            cout<<"% This file was generated by:"<<endl;
            cout<<"%    ";
            for(int i=0;i<argc;i++)
                cout<<argv[i]<<" ";
            cout<<endl;
        }
        else if(fileCounts==2)
        {
            // file to file
            fstream fin, fout;
            fin .open(fileNames[0],ios::in );
            fout.open(fileNames[1],ios::out);
            result=fswtotex(&fin,&fout);
            fout<<"% This file was generated by:"<<endl;
            fout<<"%    ";
            for(int i=0;i<argc;i++)
                fout<<argv[i]<<" ";
            fout<<endl;
        }
    }
    catch (char const *message)
    {
        cout<<"Failure: "<<message<<endl;
        result = -1;
    }
    return result;
}

/*
    Now we have our state processing functions.
    For each set of states we have a separate function that expects a uint32_t
    (which represents the Unicode character in question). There are three possible
    results from being in a state and receiving a character.
    1) Move forward to a new state.
    2) Notice an error and spit out the currently stored data unchanged.
    3) Notice that the match is complete and translate it.

    By far the largest section of code will be to move forward to a new state,
    and each potential move forward will hake a check for errors. Occasionally,
    an ``error'' state will actually indicate a successful completion and we
    will do a translation.

    Let's start with our storage and state management followed by declaring our
    state progression functions.

    One final note, this SignWriting converter is actually too permissive.
    In this converter you can use unicode for the symbol and ``text'' for
    the numbers, or the reverse. I believe that you should actually be
    required to do one or the other, but there it is. If I were ever to make a
    version that did not let you mix and match FSWA and FSWU, we would have two
    sets of functions.

    It's may also not be permissive enough in that if you have "AS123M", it may
    miss that M starts a word. I didn't bother testing for this behavior. I also
    don't look through a partially accepted string to see if a new one should
    start so if you say "M500x500S10000500x500S" then this is a failure and the
    first portion will not be translated.

    Each of these states tells us what is being expected. So, for instance,
    start_start_start is expecting to see a word start. If it doesn't, then it just
    sends the character along. But visual_size_secondw is expecting the second digit
    of the number expressing the width. If it doesn't, then it will need to spit out
    what it has already stored and then go back to start. This is also why there
    aren't any functions for "_end", that would mean that it's expecting one
    character past the last one.
*/

vector<uint32_t> line;

void start                        (ostream *fileOut, uint32_t c);
void punctuation                  (ostream *fileOut, uint32_t c);
void prefix                       (ostream *fileOut, uint32_t c);
void visual                       (ostream *fileOut, uint32_t c);
void start_start                  (ostream *fileOut, uint32_t c);
void punctuation_start            (ostream *fileOut, uint32_t c);
void punctuation_symbol           (ostream *fileOut, uint32_t c);
void punctuation_placement        (ostream *fileOut, uint32_t c);
void prefix_symbol                (ostream *fileOut, uint32_t c);
void visual_start                 (ostream *fileOut, uint32_t c);
void visual_size                  (ostream *fileOut, uint32_t c);
void visual_symbol                (ostream *fileOut, uint32_t c);
void visual_placement             (ostream *fileOut, uint32_t c);
void start_start_start            (ostream *fileOut, uint32_t c);
void punctuation_start_start      (ostream *fileOut, uint32_t c);
void punctuation_symbol_start     (ostream *fileOut, uint32_t c);
void punctuation_symbol_first     (ostream *fileOut, uint32_t c);
void punctuation_symbol_second    (ostream *fileOut, uint32_t c);
void punctuation_symbol_third     (ostream *fileOut, uint32_t c);
void punctuation_symbol_fill      (ostream *fileOut, uint32_t c);
void punctuation_symbol_rotation  (ostream *fileOut, uint32_t c);
void punctuation_placement_firstw (ostream *fileOut, uint32_t c);
void punctuation_placement_secondw(ostream *fileOut, uint32_t c);
void punctuation_placement_thirdw (ostream *fileOut, uint32_t c);
void punctuation_placement_x      (ostream *fileOut, uint32_t c);
void punctuation_placement_firsth (ostream *fileOut, uint32_t c);
void punctuation_placement_secondh(ostream *fileOut, uint32_t c);
void punctuation_placement_thirdh (ostream *fileOut, uint32_t c);
void punctuation_placement_end    (ostream *fileOut, uint32_t c);
void prefix_symbol_start          (ostream *fileOut, uint32_t c);
void prefix_symbol_first          (ostream *fileOut, uint32_t c);
void prefix_symbol_second         (ostream *fileOut, uint32_t c);
void prefix_symbol_third          (ostream *fileOut, uint32_t c);
void prefix_symbol_fill           (ostream *fileOut, uint32_t c);
void prefix_symbol_rotation       (ostream *fileOut, uint32_t c);
void visual_start_start           (ostream *fileOut, uint32_t c);
void visual_size_firstw           (ostream *fileOut, uint32_t c);
void visual_size_secondw          (ostream *fileOut, uint32_t c);
void visual_size_thirdw           (ostream *fileOut, uint32_t c);
void visual_size_x                (ostream *fileOut, uint32_t c);
void visual_size_firsth           (ostream *fileOut, uint32_t c);
void visual_size_secondh          (ostream *fileOut, uint32_t c);
void visual_size_thirdh           (ostream *fileOut, uint32_t c);
void visual_symbol_start          (ostream *fileOut, uint32_t c);
void visual_symbol_first          (ostream *fileOut, uint32_t c);
void visual_symbol_second         (ostream *fileOut, uint32_t c);
void visual_symbol_third          (ostream *fileOut, uint32_t c);
void visual_symbol_fill           (ostream *fileOut, uint32_t c);
void visual_symbol_rotation       (ostream *fileOut, uint32_t c);
void visual_placement_firstw      (ostream *fileOut, uint32_t c);
void visual_placement_secondw     (ostream *fileOut, uint32_t c);
void visual_placement_thirdw      (ostream *fileOut, uint32_t c);
void visual_placement_x           (ostream *fileOut, uint32_t c);
void visual_placement_firsth      (ostream *fileOut, uint32_t c);
void visual_placement_secondh     (ostream *fileOut, uint32_t c);
void visual_placement_thirdh      (ostream *fileOut, uint32_t c);
void visual_placement_end         (ostream *fileOut, uint32_t c);

int fswtotex(istream *fileIn, ostream *fileOut)
{
    state=substate=subsubstate=s_start;
    uint32_t c=0;
    while(c!=0xffffffff)
    {
        c=getChar(fileIn);
        if(c==0xffffffff)
            continue;
        if     (state==s_start)       start      (fileOut, c);
        else if(state==s_prefix)      prefix     (fileOut, c);
        else if(state==s_visual)      visual     (fileOut, c);
        else if(state==s_punctuation) punctuation(fileOut, c);
        else throw "Unknown state.";
    }
    (*fileOut)<<endl;
    (*fileOut)<<"% In order for this conversion to work your document needs a few things."<<endl;
    (*fileOut)<<"% \\usepackage{fontspec}"<<endl;
    (*fileOut)<<"% \\usepackage{tikz}"<<endl;
    if(mirror)
        (*fileOut)<<"% \\usepackage[mirror]{crop}"<<endl;
    if(rotation!=0)
    {
        (*fileOut)<<"% \\usepackage{everypage}"<<endl;
        (*fileOut)<<"% \\AddEverypageHook{\\special{pdf: put @thispage <</Rotate "<<rotation<<">>}}"<<endl;
    }
    (*fileOut)<<"% \\begin{document}"<<endl;
    (*fileOut)<<"% \\newfontfamily\\swfill{SuttonSignWritingFill.ttf}"<<endl;
    (*fileOut)<<"% \\newfontfamily\\swline{SuttonSignWritingLine.ttf}"<<endl;
    if(fsize!=defaultfsize)
    {
        (*fileOut)<<"% \\newlength{\\"<<fsize<<"}"<<endl;
        (*fileOut)<<"% \\setlength{\\"<<fsize<<"}{12pt}"<<endl;
    }
    return 0;
}

/*
    We are going to cover these states breadth first.
    That is, all the base states, then the two-level states, then ...
*/

void start                        (ostream* fileOut, uint32_t c)
{
    if(substate==s_start) start_start(fileOut, c);
    else throw "Unknown substate in start.";
}

void punctuation                   (ostream* fileOut, uint32_t c)
{
    if     (substate==s_start)     punctuation_start    (fileOut, c);
    else if(substate==s_symbol)    punctuation_symbol   (fileOut, c);
    else if(substate==s_placement) punctuation_placement(fileOut, c);
    else throw "Unknown substate in punctuation.";
}

void prefix                        (ostream* fileOut, uint32_t c)
{
    if(substate==s_symbol) prefix_symbol(fileOut, c);
    else throw "Unknown substate in prefix.";
}

void visual                        (ostream* fileOut, uint32_t c)
{
    if     (substate==s_start)     visual_start    (fileOut, c);
    else if(substate==s_size)      visual_size     (fileOut, c);
    else if(substate==s_symbol)    visual_symbol   (fileOut, c);
    else if(substate==s_placement) visual_placement(fileOut, c);
    else throw "Unknown substate in visual.";
}

void start_start                  (ostream *fileOut, uint32_t c)
{
    if(subsubstate==s_start) start_start_start(fileOut, c);
    else throw "Unknown subsubstate.";
}

void punctuation_start            (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_start) punctuation_start_start(fileOut, c);
    else throw "Unknown subsubstate in punctuation, start.";
}

void punctuation_symbol           (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_start)    punctuation_symbol_start   (fileOut, c);
    else if(subsubstate==s_first)    punctuation_symbol_first   (fileOut, c);
    else if(subsubstate==s_second)   punctuation_symbol_second  (fileOut, c);
    else if(subsubstate==s_third)    punctuation_symbol_third   (fileOut, c);
    else if(subsubstate==s_fill)     punctuation_symbol_fill    (fileOut, c);
    else if(subsubstate==s_rotation) punctuation_symbol_rotation(fileOut, c);
    else throw "Unknown subsubstate in punctuation, symbol.";
}

void punctuation_placement        (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_firstw)  punctuation_placement_firstw (fileOut, c);
    else if(subsubstate==s_secondw) punctuation_placement_secondw(fileOut, c);
    else if(subsubstate==s_thirdw)  punctuation_placement_thirdw (fileOut, c);
    else if(subsubstate==s_x)       punctuation_placement_x      (fileOut, c);
    else if(subsubstate==s_firsth)  punctuation_placement_firsth (fileOut, c);
    else if(subsubstate==s_secondh) punctuation_placement_secondh(fileOut, c);
    else if(subsubstate==s_thirdh)  punctuation_placement_thirdh (fileOut, c);
    else if(subsubstate==s_end)     punctuation_placement_end    (fileOut, c);
    else throw "Unknown subsubstate in punctuation, placement.";
}

void prefix_symbol                (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_start)    prefix_symbol_start   (fileOut, c);
    else if(subsubstate==s_first)    prefix_symbol_first   (fileOut, c);
    else if(subsubstate==s_second)   prefix_symbol_second  (fileOut, c);
    else if(subsubstate==s_third)    prefix_symbol_third   (fileOut, c);
    else if(subsubstate==s_fill)     prefix_symbol_fill    (fileOut, c);
    else if(subsubstate==s_rotation) prefix_symbol_rotation(fileOut, c);
    else throw "Unknown subsubstate.";
}

void visual_start                 (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_start)   visual_start_start   (fileOut, c);
    else throw "Unknown subsubstate.";
}

void visual_size                  (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_firstw)  visual_size_firstw (fileOut, c);
    else if(subsubstate==s_secondw) visual_size_secondw(fileOut, c);
    else if(subsubstate==s_thirdw)  visual_size_thirdw (fileOut, c);
    else if(subsubstate==s_x)       visual_size_x      (fileOut, c);
    else if(subsubstate==s_firsth)  visual_size_firsth (fileOut, c);
    else if(subsubstate==s_secondh) visual_size_secondh(fileOut, c);
    else if(subsubstate==s_thirdh)  visual_size_thirdh (fileOut, c);
    else throw "Unknown subsubstate.";
}

void visual_symbol                (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_start)    visual_symbol_start   (fileOut, c);
    else if(subsubstate==s_first)    visual_symbol_first   (fileOut, c);
    else if(subsubstate==s_second)   visual_symbol_second  (fileOut, c);
    else if(subsubstate==s_third)    visual_symbol_third   (fileOut, c);
    else if(subsubstate==s_fill)     visual_symbol_fill    (fileOut, c);
    else if(subsubstate==s_rotation) visual_symbol_rotation(fileOut, c);
    else throw "Unknown subsubstate.";
}

void visual_placement             (ostream *fileOut, uint32_t c)
{
    if     (subsubstate==s_firstw)  visual_placement_firstw (fileOut, c);
    else if(subsubstate==s_secondw) visual_placement_secondw(fileOut, c);
    else if(subsubstate==s_thirdw)  visual_placement_thirdw (fileOut, c);
    else if(subsubstate==s_x)       visual_placement_x      (fileOut, c);
    else if(subsubstate==s_firsth)  visual_placement_firsth (fileOut, c);
    else if(subsubstate==s_secondh) visual_placement_secondh(fileOut, c);
    else if(subsubstate==s_thirdh)  visual_placement_thirdh (fileOut, c);
    else if(subsubstate==s_end)     visual_placement_end    (fileOut, c);
    else throw "Unknown subsubstate.";
}

void start_start_start            (ostream *fileOut, uint32_t c)
{
    if(c=='A' || c==0x1d800)
    { line.push_back(c); state=s_prefix; substate=s_symbol; subsubstate=s_start; }
    else if(c=='B' || (c>='L'&&c<='M') || c=='R' || (c>=0x1d801&&c<=0x1d804))
    { line.push_back(c); state=s_visual; substate=s_size; subsubstate=s_firstw; }
    else if(c=='S')
    { line.push_back(c); state=s_punctuation; substate=s_symbol; subsubstate=s_first; }
    else
        sendOut(fileOut,c);
}


void punctuation_start_start      (ostream *fileOut, uint32_t c)
{
    if(c=='S')
    { line.push_back(c); substate=s_symbol; subsubstate=s_first; }
    else if(c>=0x4f424 && c<=0x4f428)
    { line.push_back(c); substate=s_placement; subsubstate=s_first; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_symbol_start     (ostream *fileOut, uint32_t c)
{
    if(c=='S')
    { line.push_back(c); substate=s_symbol; subsubstate=s_first; }
    else if(c>=0x4f424 && c<=0x4f428)
    { line.push_back(c); substate=s_placement; subsubstate=s_first; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_symbol_first     (ostream *fileOut, uint32_t c)
{
    if(c=='3')
    { line.push_back(c); subsubstate=s_second; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_symbol_second    (ostream *fileOut, uint32_t c)
{
    if(c=='8')
    { line.push_back(c); subsubstate=s_third; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_symbol_third     (ostream *fileOut, uint32_t c)
{
    if((c>='7'&&c<='9') || (c>='a'&&c<='b'))
    { line.push_back(c); subsubstate=s_fill; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_symbol_fill      (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='5')
    { line.push_back(c); subsubstate=s_rotation; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_symbol_rotation  (ostream *fileOut, uint32_t c)
{
    if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
    { line.push_back(c); substate=s_placement; subsubstate=s_firstw; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_placement_firstw (ostream *fileOut, uint32_t c)
{
    if(c>='2' && c<='7')
    { line.push_back(c); subsubstate=s_secondw; }
    else if(c>=0x1d80c && c<=0x1d9ff)
    { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_placement_secondw(ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut, line, c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut, line, c);
    }
    else // if(line[line.Length-1]=='7')
    {
        if( c>='0' && c<='4')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut, line, c);
    }
}

void punctuation_placement_thirdw (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='9')
    { line.push_back(c); subsubstate=s_x; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_placement_x      (ostream *fileOut, uint32_t c)
{
    if(c=='x')
    { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_placement_firsth (ostream *fileOut, uint32_t c)
{
    if(c>='2' && c<='7')
    { line.push_back(c); subsubstate=s_secondh; }
    else if(c>=0x1d80c && c<=0x1d9ff)
    { line.push_back(c); subsubstate=s_end; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_placement_secondh(ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut, line, c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut, line, c);
    }
    else // if(line[line.Length-1]=='7')
    {
        if( c>='0' && c<='4')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut, line, c);
    }
}

void punctuation_placement_thirdh (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='9')
    { line.push_back(c); subsubstate=s_end; }
    else
        sendOut(fileOut, line, c);
}

void punctuation_placement_end    (ostream *fileOut, uint32_t c)
{
    vector<uint32_t> temp;
    temp.push_back('M');
    temp.push_back('5');
    temp.push_back('0');
    temp.push_back('0');
    temp.push_back('x');
    temp.push_back('5');
    temp.push_back('0');
    temp.push_back('0');
    for(int i=0;i<line.size();i++)
        temp.push_back(line[i]);
    line.clear();
    for(int i=0;i<temp.size();i++)
        line.push_back(temp[i]);
    visual_placement_end(fileOut, c);
}

void prefix_symbol_start          (ostream *fileOut, uint32_t c)
{
    if(c=='S')
    { line.push_back(c); subsubstate=s_first; }
    else if(c>=0x40001 && c<=0x4f428)
    { line.push_back(c); state=s_visual; substate=s_start; subsubstate=s_start; }
    else
        sendOut(fileOut,line,c);
}

void prefix_symbol_first          (ostream *fileOut, uint32_t c)
{
    if(c>='1' && c<='3')
    { line.push_back(c); subsubstate=s_second; }
    else
        sendOut(fileOut,line,c);
}

void prefix_symbol_second         (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]>='0'&&line[line.size()-1]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
        { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(fileOut,line,c);
    }
    else
    {
        if(c>='0' && c<='8')
        { line.push_back(c); subsubstate=s_third; }
        else if(c=='f')
        { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(fileOut,line,c);
    }
}

void prefix_symbol_third          (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-2]>='0'&&line[line.size()-2]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
        { line.push_back(c); subsubstate=s_fill; }
        else
            sendOut(fileOut,line,c);
    }
    else
    {
        if(line[line.size()-1]>='0'&&line[line.size()-1]<='7')
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
            { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(fileOut,line,c);
        }
        else if(line[line.size()-1]=='8')
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='b'))
            { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(fileOut,line,c);
        }
        else // =='f'
        {
            if(c=='f')
            { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(fileOut,line,c);
        }
    }
}

void prefix_symbol_fill           (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='5')
    { line.push_back(c); subsubstate=s_rotation; }
    else
        sendOut(fileOut,line,c);
}

void prefix_symbol_rotation       (ostream *fileOut, uint32_t c)
{
    if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
    { line.push_back(c); state=s_visual; substate=subsubstate=s_start; }
    else
        sendOut(fileOut,line,c);
}

void visual_start_start           (ostream *fileOut, uint32_t c)
{
    if(c=='B' || (c>='L'&&c<='M') || c=='R' || (c>=0x1d801&&c<=0x1d804))
    { line.push_back(c); substate=s_size; subsubstate=s_firstw; }
    else if(c=='S')
    { line.push_back(c); state=s_prefix; substate=s_symbol; subsubstate=s_first; }
    else if(c>=0x40001 && c<=0x4f428)
    { line.push_back(c); state=s_visual; substate=s_start; subsubstate=s_start; }
    else
        sendOut(fileOut,line,c);
}

void visual_size_firstw           (ostream *fileOut, uint32_t c)
{
    if(c>='2' && c<='7')
    { line.push_back(c); subsubstate=s_secondw; }
    else if(c>=0x1d80c && c<=0x1d9ff)
    { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(fileOut,line,c);
}

void visual_size_secondw          (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut,line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut,line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut,line,c);
    }
}

void visual_size_thirdw           (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='9')
    { line.push_back(c); subsubstate=s_x; }
    else
        sendOut(fileOut,line,c);
}

void visual_size_x                (ostream *fileOut, uint32_t c)
{
    if(c=='x')
    { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(fileOut,line,c);
}

void visual_size_firsth           (ostream *fileOut, uint32_t c)
{
    if(c>='2' && c<='7')
    { line.push_back(c); subsubstate=s_secondh; }
    else if(c>=0x1d80c && c<=0x1d9ff)
    { line.push_back(c); substate=s_symbol; subsubstate=s_start; }
    else
        sendOut(fileOut,line,c);
}

void visual_size_secondh          (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut,line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut,line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut,line,c);
    }
}

void visual_size_thirdh           (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='9')
    { line.push_back(c); substate=s_symbol; subsubstate=s_start; }
    else
        sendOut(fileOut,line,c);
}

void visual_symbol_start          (ostream *fileOut, uint32_t c)
{
    if(c=='S')
    { line.push_back(c); subsubstate=s_first; }
    else if(c>=0x40001 && c<=0x4f428)
    { line.push_back(c); state=s_visual; substate=s_placement; subsubstate=s_first; }
    else
        sendOut(fileOut,line,c);
}

void visual_symbol_first          (ostream *fileOut, uint32_t c)
{
    if(c>='1' && c<='3')
    { line.push_back(c); subsubstate=s_second; }
    else
        sendOut(fileOut,line,c);
}

void visual_symbol_second         (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]>='0'&&line[line.size()-1]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
        { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(fileOut,line,c);
    }
    else
    {
        if(c>='0' && c<='8')
        { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(fileOut,line,c);
    }
}

void visual_symbol_third          (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-2]>='0'&&line[line.size()-2]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
        { line.push_back(c); subsubstate=s_fill; }
        else
            sendOut(fileOut,line,c);
    }
    else
    {
        if(line[line.size()-1]>='0'&&line[line.size()-1]<='7')
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
            { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(fileOut,line,c);
        }
        else
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='b'))
            { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(fileOut,line,c);
        }
    }
}

void visual_symbol_fill           (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='5')
    { line.push_back(c); subsubstate=s_rotation; }
    else
        sendOut(fileOut,line,c);
}

void visual_symbol_rotation       (ostream *fileOut, uint32_t c)
{
    if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
    { line.push_back(c); substate=s_placement; subsubstate=s_firstw; }
    else
        sendOut(fileOut,line,c);
}

void visual_placement_firstw      (ostream *fileOut, uint32_t c)
{
    if(c>='2' && c<='7')
    { line.push_back(c); subsubstate=s_secondw; }
    else if(c>=0x1d80c && c<=0x1d9ff)
    { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(fileOut,line,c);
}

void visual_placement_secondw     (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut,line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut,line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
        { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(fileOut,line,c);
    }
}

void visual_placement_thirdw      (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='9')
    { line.push_back(c); subsubstate=s_x; }
    else
        sendOut(fileOut,line,c);
}

void visual_placement_x           (ostream *fileOut, uint32_t c)
{
    if(c=='x')
    { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(fileOut,line,c);
}

void visual_placement_firsth      (ostream *fileOut, uint32_t c)
{
    if(c>='2' && c<='7')
    { line.push_back(c); subsubstate=s_secondh; }
    else if(c>=0x1d80c && c<=0x1d9ff)
    { line.push_back(c); subsubstate=s_end; }
    else
        sendOut(fileOut,line,c);
}

void visual_placement_secondh     (ostream *fileOut, uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut,line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut,line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
        { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(fileOut,line,c);
    }
}

void visual_placement_thirdh      (ostream *fileOut, uint32_t c)
{
    if(c>='0' && c<='9')
    { line.push_back(c); subsubstate=s_end; }
    else
        sendOut(fileOut,line,c);
}

void visual_placement_end         (ostream *fileOut, uint32_t c)
{
    vector< vector< uint32_t > > sorting;
    if(c=='S')
    { line.push_back(c); substate=s_symbol; subsubstate=s_first; }
    else if(c>=0x40001 && c<=0x4f428)
    { line.push_back(c); state=s_visual; substate=s_placement; subsubstate=s_first; }
    else
    {
        state=substate=subsubstate=s_start;
        unsigned int place=0;
        char lane='B';
        if(line[place]=='A' || line[place]==0x1d800)
        {
             place++;
             while(line[place]=='S' || (line[place]>=0x40001 && line[place]<=0x4f428))
             {
                 int s=0;
                 if(line[place]=='S')
                 {
                     place++;
                     s=(line[place]-'0')*256;
                     if(line[place+1]>='0' && line[place+1]<='9')
                         s+=(line[place+1]-'0')*16;
                     else // if(line[place+1]>='a' && line[place+1]<='f')
                         s+=(line[place+1]-'a'+10)*16;
                     if(line[place+2]>='0' && line[place+2]<='9')
                         s+=(line[place+2]-'0')*1;
                     else // if(line[place+2]>='a' && line[place+2]<='f')
                         s+=(line[place+2]-'a'+10)*1;
                     place+=3;
                     s-=0x100;
                     s*=(6*16);
                     s+=(line[place]-'0')*16;
                     place++;
                     if(line[place]>='0' && line[place]<='9')
                         s+=(line[place]-'0');
                     else // if(line[place+1]>='a' && line[place+1]<='f')
                         s+=(line[place]-'a'+10);
                     place++;
                 }
                 else // if(line[place]>=0x40001 && line[place]<=0x4f428)
                 {
                     s=line[place]-0x40001;
                     place++;
                 }
                 if(s==(0x3ff-0x100)*(6*16))
                 {
                     sorting.resize(sorting.size()+1);
                 }
                 else
                 {
                     if(sorting.size()==0)
                         sorting.resize(sorting.size()+1);
                     sorting[sorting.size()-1].resize(sorting[sorting.size()-1].size()+1);
                     //for(int i=sorting[sorting.size()-1].size()-1;i>0;i--)
                     //    sorting[sorting.size()-1][i]=sorting[sorting.size()-1][i-1];
                     //sorting[sorting.size()-1][0] = s;
                     sorting[sorting.size()-1][sorting[sorting.size()-1].size()-1] = s;
                 }
             }
        }
        if(line[place]=='B' || line[place]==0x1d801)
            lane='B';
        if(line[place]=='L' || line[place]==0x1d802)
            lane='L';
        if(line[place]=='M' || line[place]==0x1d803)
            lane='M';
        if(line[place]=='R' || line[place]==0x1d804)
            lane='R';
        place++;
        // We currently ignore the height and width, so we just skip over them.
        if(line[place]>'2' && line[place]<='7')
        {
            place += 3;
            // and skip the x
            place++;
        }
        else
        {
            place++;
        }
        if(line[place]>'2' && line[place]<='7')
        {
            place += 3;
        }
        else
        {
            place++;
        }
        (*fileOut)<<"{";
        if(hasat)
            (*fileOut)<<"\\makeatletter";
        (*fileOut)<<"\\begin{tikzpicture}";
        if((rotation!=0) || (mirror==true))
            (*fileOut)<<"[";
        if(rotation!=0)
            (*fileOut)<<"rotate="<<rotation;
        if((rotation!=0) && (mirror==true))
            (*fileOut)<<",";
        if(mirror==true)
            (*fileOut)<<"yscale=-1";
        if((rotation!=0) || (mirror==true))
            (*fileOut)<<"]";
        // The idea was, initially, to place a thin rectangle behind each word from 0--1000.
        // Unfortunately, this made it so that I could reasonably fit about two columns of
        // \normalsize text to a page. We are now only extend 100 each direction to allow for about
        // five columns of \normalsize text. This also decided our lane shift amount later on.

        // Yes, I know the number 100 doesn't appear. That's because I found through experimentation
        // that a character anchored at (0,0) along with a rectangle around (0,0) does not place a
        // rectangle around the expected corner.
        if(lane != 'B')
            (*fileOut)<<"\\draw[white](\\"<<fsize<<"/30*-90 pt,\\"<<fsize<<"/30*-12 pt)rectangle(\\"<<fsize<<"/30*110 pt,\\"<<fsize<<"/30*-10 pt);";
        if(spelling)
        {
            int maxsize=0;
            for(int x=0;x<sorting.size();x++)
            {
                if(sorting[x].size() > maxsize)
                    maxsize = sorting[x].size();
            }
            for(int x=0;x<sorting.size();x++)
            {
                for(int y=0;y<sorting[x].size();y++)
                {
                    (*fileOut)<<"\\begin{scope}[xshift="<<(x*12-7*static_cast<int>(sorting.size()-1)-(1*static_cast<int>(sorting.size()%2)))<<"pt, yshift="<<((maxsize-y-2)*12+18)<<"pt]";
                    (*fileOut)<<"\\draw(0,0) rectangle (12pt,12pt);";
                    (*fileOut)<<"\\draw(0,13pt) node [";
                    if(mirror==true)
                        (*fileOut)<<"xscale=-1";
                    if((mirror==true) && (rotation!=0))
                        (*fileOut)<<",";
                    if(rotation!=0)
                        (*fileOut)<<"rotate="<<rotation;
                    if((rotation!=0) || (mirror==true))
                        (*fileOut)<<",";
                    (*fileOut)<<"anchor=north west] {\\swline";
                    (*fileOut)<<"\\fontsize{6pt}{6pt}\\selectfont";
                    (*fileOut)<<"\\char";
                    (*fileOut)<<(0xf0001+sorting[x][y]);
                    (*fileOut)<<"};";
                    (*fileOut)<<"\\end{scope}";
                }
            }
        }
        while(place<line.size())
        {
            int s;
            if(line[place]=='S')
            {
                place++;
                s=(line[place]-'0')*256;
                if(line[place+1]>='0' && line[place+1]<='9')
                    s+=(line[place+1]-'0')*16;
                else // if(line[place+1]>='a' && line[place+1]<='f')
                    s+=(line[place+1]-'a'+10)*16;
                if(line[place+2]>='0' && line[place+2]<='9')
                    s+=(line[place+2]-'0')*1;
                else // if(line[place+2]>='a' && line[place+2]<='f')
                    s+=(line[place+2]-'a'+10)*1;
                place+=3;
                s-=0x100;
                s*=(6*16);
                s+=(line[place]-'0')*16;
                place++;
                if(line[place]>='0' && line[place]<='9')
                    s+=(line[place]-'0');
                else // if(line[place+1]>='a' && line[place+1]<='f')
                    s+=(line[place]-'a'+10);
                place++;
            }
            else // if(line[place]>=0x40001 && line[place]<=0x4f428)
            {
                s=line[place]-0x40001;
                place++;
            }
            int sx;
            if(line[place]>'2' && line[place]<='7')
            {
                sx=(line[place]-'0')*100 + (line[place+1]-'0')*10 + (line[place+2]-'0');
                place += 3;
                // and skip the x
                place++;
            }
            else
            {
                sx=line[place]-0x1d80c+250;
                place++;
            }
            int sy;
            if(line[place]>'2' && line[place]<='7')
            {
                sy=(line[place]-'0')*100 + (line[place+1]-'0')*10 + (line[place+2]-'0');
                place += 3;
            }
            else
            {
                sy=line[place]-0x1d80c+250;
                place++;
            }
            /*
            At this point, assuming well formed F/USW strings, we will
            Have a symbol centered around (500,500).
            For 'B' (meaning horizontal SW) we center it around (0,0).
            For 'L' we shift it left 250.
            For 'M' it is correct.
            For 'R' we shift it right 250.
            */
            if(lane=='B')
                sx-=500;
            if(lane=='L')
                sx-=550;
            if(lane=='M')
                sx-=500;
            if(lane=='R')
                sx-=450;
            sy-=500;
            // Now we know where, but for SignWriting the white space can be important too.
            (*fileOut)<<"\\draw(\\"<<fsize<<"/30*";
            (*fileOut)<<sx;
            (*fileOut)<<" pt,\\"<<fsize<<"/30*";
            (*fileOut)<<(-sy);
            (*fileOut)<<" pt) node [";
            if(mirror==true)
                (*fileOut)<<"xscale=-1";
            if((mirror==true) && (rotation!=0))
                (*fileOut)<<",";
            if(rotation!=0)
                (*fileOut)<<"rotate="<<rotation;
            if((rotation!=0) || (mirror==true))
                (*fileOut)<<",";
            (*fileOut)<<"color=white,anchor=north west] {\\swfill";
            if(fsize!=defaultfsize)
                (*fileOut)<<"\\fontsize{\\"<<fsize<<"}{\\"<<fsize<<"}\\selectfont";
            (*fileOut)<<"\\char";
            (*fileOut)<<(0x100001+s);
            (*fileOut)<<"};";
            (*fileOut)<<"\\draw(\\"<<fsize<<"/30*";
            (*fileOut)<<sx;
            (*fileOut)<<" pt,\\"<<fsize<<"/30*";
            (*fileOut)<<(-sy);
            (*fileOut)<<" pt) node [";
            if(mirror==true)
                (*fileOut)<<"xscale=-1";
            if((mirror==true) && (rotation!=0))
                (*fileOut)<<",";
            if(rotation!=0)
                (*fileOut)<<"rotate="<<rotation;
            if((rotation!=0) || (mirror==true))
                (*fileOut)<<",";
            (*fileOut)<<"anchor=north west] {\\swline";
            if(fsize!=defaultfsize)
                (*fileOut)<<"\\fontsize{\\"<<fsize<<"}{\\"<<fsize<<"}\\selectfont";
            (*fileOut)<<"\\char";
            (*fileOut)<<(0xf0001+s);
            (*fileOut)<<"};";
        }
        (*fileOut)<<"\\end{tikzpicture}";
        (*fileOut)<<"}";
        line.clear();
        (*fileOut)<<static_cast<char>(c);
        state=substate=subsubstate=s_start;
    }
}

