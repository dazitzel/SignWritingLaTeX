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
        Prefix      -- processing the temporal prefix (can be missing)
        Visual      -- processing the visual layout of the sign (required)
        End         -- Time to spit out what we have

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
const int s_prefix=1;
const int s_visual=2;
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
*/

enum
{
    unknown, utf8, utf16le, utf16be, utf32le, utf32be
} textFormat = unknown;

uint32_t getChar(istream* in);

uint32_t getCharUnknown(istream* in);
uint32_t getCharUtf8   (istream* in);
uint32_t getCharUtf16le(istream* in);
uint32_t getCharUtf16be(istream* in);
uint32_t getCharUtf32le(istream* in);
uint32_t getCharUtf32be(istream* in);

int utf8ToUtf32   (uint8_t* in, uint32_t* out);
int utf16leToUtf32(uint8_t* in, uint32_t* out);
int utf16beToUtf32(uint8_t* in, uint32_t* out);
int utf32leToUtf32(uint8_t* in, uint32_t* out);
int utf32beToUtf32(uint8_t* in, uint32_t* out);

uint32_t getCharUnknown(istream* in)
{
    // feff is the byte order
    // In each format, this can serve as a key
    // utf  8    ef bb bf
    // utf 16 le ff fe
    // utf 16 be fe ff
    // utf 32 le ff fe 00 00
    // utf 32 be 00 00 fe ff
    // So if we are unknown then we look for those,
    // and if we don't see any of them we default to utf8
    static uint8_t c[4] = {0, 0, 0, 0};
    static int cSize=0;
    if(cSize==0)
    {
        for(;cSize<4;cSize++)
        {
            c[cSize]=static_cast<uint8_t>(in->get());
        }
        if(c[0]==0x0 && c[1]==0x0 && c[2]==0xfe && c[3]==0xff)
        {
            textFormat=utf32be;
            return getCharUtf32be(in);
        }
        else if(c[0]==0xff && c[1]==0xfe && c[2]==0x0 && c[3]==0x0)
        {
            textFormat=utf32le;
            return getCharUtf32le(in);
        }
        else if(c[0]==0xfe && c[1]==0xff)
        {
            textFormat=utf16be;
            c[0]=c[2];
            c[1]=c[3];
            c[2]=0;
            c[3]=0;
            uint32_t result;
            if(utf16beToUtf32(c,&result)==2)
            {
                return result;
            }
            c[2]=static_cast<uint8_t>(in->get());
            c[3]=static_cast<uint8_t>(in->get());
            if(utf16beToUtf32(c,&result)==4)
            {
                return result;
            }
            throw "Badly formed utf16be string.";
        }
        else if(c[0]==0xff && c[1]==0xfe)
        {
            textFormat=utf16le;
            c[0]=c[2];
            c[1]=c[3];
            c[2]=0;
            c[3]=0;
            uint32_t result;
            if(utf16leToUtf32(c,&result)==2)
            {
                return result;
            }
            c[2]=static_cast<uint8_t>(in->get());
            c[3]=static_cast<uint8_t>(in->get());
            if(utf16leToUtf32(c,&result)==4)
            {
                return result;
            }
            throw "Badly formed ut16le string.";
        }
        else if(c[0]==0xef && c[1]==0xbb && c[2]==0xbf)
        {
            textFormat=utf8;
            c[0]=c[3];
            c[1]=0;
            c[2]=0;
            c[3]=0;
            uint32_t result;
            if(utf8ToUtf32(c,&result)==1)
            {
                return result;
            }
            c[1]=static_cast<uint8_t>(in->get());
            if(utf8ToUtf32(c,&result)==2)
            {
                return result;
            }
            c[2]=static_cast<uint8_t>(in->get());
            if(utf8ToUtf32(c,&result)==3)
            {
                return result;
            }
            c[3]=static_cast<uint8_t>(in->get());
            if(utf8ToUtf32(c,&result)==4)
            {
                return result;
            }
            throw "Badly formed utf8 string.";
        }
    }
    uint32_t result;
    int offset = utf8ToUtf32(c,&result);
    while(offset<0 && cSize<4)
    {
        c[cSize]=static_cast<uint8_t>(in->get());
        cSize++;
        offset = utf8ToUtf32(c,&result);
    }
    int i;
    for(i=0;i<cSize-offset;i++)
    {
        c[i]=c[i+offset];
    }
    for(;i<cSize;i++)
    {
        c[i]=0;
    }
    cSize-=offset;
    if(cSize==0)
    {
        textFormat=utf8;
    }
    return result;
}

uint32_t getCharUtf8(istream* in)
{
    uint8_t c[4]={0,0,0,0};
    int size=0;
    uint32_t result;
    do
    {
        c[size++]=static_cast<uint8_t>(in->get());
    }
    while((size<4) && (utf8ToUtf32(c,&result)!=size));
    if(utf8ToUtf32(c,&result)==size)
    {
        return result;
    }
    throw "Badly formed utf8 string.";
}

uint32_t getCharUtf16le(istream* in)
{
    uint8_t c[4]={0,0,0,0};
    int size=0;
    uint32_t result;
    do
    {
        c[size++]=static_cast<uint8_t>(in->get());
        c[size++]=static_cast<uint8_t>(in->get());
    }
    while((size<4) && (utf16leToUtf32(c,&result)!=size));
    if(utf16leToUtf32(c,&result)==size)
    {
        return result;
    }
    throw "Badly formed utf16le string.";
}

uint32_t getCharUtf16be(istream* in)
{
    uint8_t c[4]={0,0,0,0};
    int size=0;
    uint32_t result;
    do
    {
        c[size++]=static_cast<uint8_t>(in->get());
        c[size++]=static_cast<uint8_t>(in->get());
    }
    while((size<4) && (utf16beToUtf32(c,&result)!=size));
    if(utf16beToUtf32(c,&result)==size)
    {
        return result;
    }
    throw "Badly formed utf16be string.";
}

uint32_t getCharUtf32le(istream* in)
{
    uint8_t c[4]={0,0,0,0};
    int size=0;
    uint32_t result;
    c[size++]=static_cast<uint8_t>(in->get());
    c[size++]=static_cast<uint8_t>(in->get());
    c[size++]=static_cast<uint8_t>(in->get());
    c[size++]=static_cast<uint8_t>(in->get());
    utf32leToUtf32(c,&result);
    return result;
}

uint32_t getCharUtf32be(istream* in)
{
    uint8_t c[4]={0,0,0,0};
    int size=0;
    uint32_t result;
    c[size++]=static_cast<uint8_t>(in->get());
    c[size++]=static_cast<uint8_t>(in->get());
    c[size++]=static_cast<uint8_t>(in->get());
    c[size++]=static_cast<uint8_t>(in->get());
    utf32beToUtf32(c,&result);
    return result;
}

uint32_t getChar(istream* in)
{
    switch(textFormat)
    {
        case utf8   : return getCharUtf8   (in);
        case utf16le: return getCharUtf16le(in);
        case utf16be: return getCharUtf16be(in);
        case utf32le: return getCharUtf32le(in);
        case utf32be: return getCharUtf32be(in);
        default     : return getCharUnknown(in);
    }
}

// We always convert to utf-8 since we know that we are outputting
// a lot of TeX code with expanded characters of the format ``\char#xxxx''.

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

// And we use that to send out the results
ostream *sout;
void sendOut(uint32_t c)
{
    (*sout)<<utf32ToUtf8(c);
}

void sendOut(vector<uint32_t> &l,uint32_t c)
{
    for(unsigned int i=0;i<l.size();i++)
        sendOut(l[i]);
    sendOut(c);
    l.clear();
    state=substate=subsubstate=s_start;
}

// Now that we have utilized all those ``convert to uft32'', let's define them.

int utf8ToUtf32   (uint8_t* in, uint32_t* out)
{
    // Quick review:
    //    0xxx xxxx
    //    110x xxxx  10xx xxxx
    //    1110 xxxx  10xx xxxx  10xx xxxx
    //    1111 -xxx  10xx xxxx  10xx xxxx  10xx xxxx
    if((in[0]&0xf0)==0xf0)
    {
        out[0]=in[0]&0x7;
        for(int i=1;i<4;i++)
        {
            if((out[i]&0xc0)!=0x80)
                throw "Malformed utf8 string.";
            out[0]<<=6;
            out[0]|=(in[i]&0x3f);
        }
        return 4;
    }
    if((in[0]&0xf0)==0xe0)
    {
        out[0]=in[0]&0xf;
        for(int i=1;i<3;i++)
        {
            if((out[i]&0xc0)!=0x80)
                throw "Malformed utf8 string.";
            out[0]<<=6;
            out[0]|=(in[i]&0x3f);
        }
        return 3;
    }
    if((in[0]&0xe0)==0xc0)
    {
        out[0]=in[0]&0x1f;
        for(int i=1;i<2;i++)
        {
            if((out[i]&0xc0)!=0x80)
                throw "Malformed utf8 string.";
            out[0]<<=6;
            out[0]|=(in[i]&0x3f);
        }
        return 2;
    }
    if((in[0]&0xc0)==0x80)
    {
        throw "Malformed utf8 string.";
    }
    out[0]=in[0];
    return 1;
}

int utf16leToUtf32(uint8_t* in, uint32_t* out)
{
    // Quick review, after accounting for endianess
    // 0000--d7ff = pass along
    // d800--d8ff = a following dc00-dfff tells the low 10 bits
    // dc00--dfff = better be following d800-d8ff
    // e000--ffff = pass along
    out[0] =in[0]<<0;
    out[0]|=in[1]<<8;
    if(out[0]<0xd800)
        return 2;
    if(out[0]<0xdc00)
    {
        if(in[3]<0xdc || in[3]>0xdf)
            throw "Malformed utf16le string";
        out[0] &=0x3ff;
        out[0] |=0x400;
        out[0]<<=10;
        out[0] |=in[2];
        out[0] |=(in[3]&0x3)<<8;
        return 2;
    }
    if(out[0]<0xe000)
        throw "Malformed utf16le string";
    return 2;
}

int utf16beToUtf32(uint8_t* in, uint32_t* out)
{
    // Quick review, after accounting for endianess
    // 0000--d7ff = pass along
    // d800--d8ff = a following dc00-dfff tells the low 10 bits
    // dc00--dfff = better be following d800-d8ff
    out[0] =in[0]<<8;
    out[0]|=in[1]<<0;
    if(out[0]<0xd800)
        return 2;
    if(out[0]<0xdc00)
    {
        if(in[2]<0xdc || in[2]>0xdf)
            throw "Malformed utf16le string";
        out[0] &=0x3ff;
        out[0] |=0x400;
        out[0]<<=10;
        out[0] |=(in[2]&0x3)<<8;
        out[0] |=in[3];
        return 2;
    }
    if(out[0]<0xe000)
        throw "Malformed utf16le string";
    return 2;
}

int utf32leToUtf32(uint8_t* in, uint32_t* out)
{
    out[0] =in[0]<< 0;
    out[0]|=in[1]<< 8;
    out[0]|=in[2]<<16;
    out[0]|=in[3]<<24;
    return 4;
}

int utf32beToUtf32(uint8_t* in, uint32_t* out)
{
    out[0] =in[0]<<24;
    out[0]|=in[1]<<16;
    out[0]|=in[2]<< 8;
    out[0]|=in[3]<< 0;
    return 4;
}

// Finally, let's process some characters.

int usage()
{
    cout<<"This is fswtotex."<<endl;
    cout<<endl;
    cout<<"This program runs in about three different methods."<<endl;
    cout<<"If you provide no arguments, we read from standard in and send the results to standard out."<<endl;
    cout<<"If you provide one argument, we read from that file and send the results to standard out."<<endl;
    cout<<"If you provide two arguments, we read from the first file and send the results to the second file."<<endl;
    cout<<endl;
    cout<<"So what do we do?"<<endl;
    cout<<endl;
    cout<<"We read LaTeX code with embedded SignWriting words (see http://signwriting.org) and output LaTeX "
          "code with TiKz drawings of SignWriting symbols."<<endl;
    cout<<endl;
    return 0;
}

int fswtotex(istream *sin,ostream *sout);

int main(int argc,char**argv)
{
    //  We an run in about three different methods.
    //  1)  read from standard in and write to standard out (0 arguments)
    //  2)  read from a file and write to standard out (1 argument)
    //  3)  read from a file and write to a file (2 arguments)
    //  In order to accomplish this, we first process the arguments,
    //  and then set up in istream and ostream, and let it rip.

    if(argc==1)
    {
        // standard in and standard out
        cout<<"% This file was generated by:"<<endl;
        cout<<"%    "<<argv[0]<<endl;
        return fswtotex(&cin,&cout);
    }
    if(argv[1][0]=='-')
    {
        return usage();
    }
    if(argc==2)
    {
        // file to standard out
        fstream fin;
        fin.open(argv[1],ios::in);
        cout<<"% This file was generated by:"<<endl;
        cout<<"%    "<<argv[0]<<" "<<argv[1]<<endl;
        return fswtotex(&fin,&cout);
    }
    if(argc==3)
    {
        // file to file
        fstream fin, fout;
        fin .open(argv[1],ios::in );
        fout.open(argv[2],ios::out);
        fout<<"% This file was generated by:"<<endl;
        fout<<"%    "<<argv[0]<<" "<<argv[1]<<" "<<argv[2]<<endl;
        return fswtotex(&fin,&fout);
    }
    return usage();
}

/*  Now we have our state processing functions.
    For each set of states we have a separate function that expects a uint32_t
    (which represents the Unicode character in question). There are three possible
    results from being in a state and receiving a character.
    1) Move forward to a new state.
    2) Notice an error and spit out the currently stored data unchangedv
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
    miss that M starts a word. I didn't bother testing for this behavior and I
    also don't look through a partially accepted string to see if a new one should
    start.

    Each of these states tells us what is being expected. So, for instance,
    start_start_start is expecting to see a word start. If it doesn't, then it just
    sends the character along. But visual_size_secondw is expecting the second digit
    of the number expressing the width. If it doesn't, then it will need to spit out
    what it has already stored and then go back to start. This is also why there
    aren't any functions for "_end", that would mean that it's expecting one
    character past the last one.
*/

vector<uint32_t> line;

void start                   (uint32_t c);
void prefix                  (uint32_t c);
void visual                  (uint32_t c);
void start_start             (uint32_t c);
void prefix_symbol           (uint32_t c);
void visual_start            (uint32_t c);
void visual_size             (uint32_t c);
void visual_symbol           (uint32_t c);
void visual_placement        (uint32_t c);
void start_start_start       (uint32_t c);
void prefix_symbol_start     (uint32_t c);
void prefix_symbol_first     (uint32_t c);
void prefix_symbol_second    (uint32_t c);
void prefix_symbol_third     (uint32_t c);
void prefix_symbol_fill      (uint32_t c);
void prefix_symbol_rotation  (uint32_t c);
void visual_start_start      (uint32_t c);
void visual_size_firstw      (uint32_t c);
void visual_size_secondw     (uint32_t c);
void visual_size_thirdw      (uint32_t c);
void visual_size_x           (uint32_t c);
void visual_size_firsth      (uint32_t c);
void visual_size_secondh     (uint32_t c);
void visual_size_thirdh      (uint32_t c);
void visual_symbol_start     (uint32_t c);
void visual_symbol_first     (uint32_t c);
void visual_symbol_second    (uint32_t c);
void visual_symbol_third     (uint32_t c);
void visual_symbol_fill      (uint32_t c);
void visual_symbol_rotation  (uint32_t c);
void visual_placement_firstw (uint32_t c);
void visual_placement_secondw(uint32_t c);
void visual_placement_thirdw (uint32_t c);
void visual_placement_x      (uint32_t c);
void visual_placement_firsth (uint32_t c);
void visual_placement_secondh(uint32_t c);
void visual_placement_thirdh (uint32_t c);
void visual_placement_end    (uint32_t c);

int fswtotex(istream *sin, ostream *sout_local)
{
    sout = sout_local;
    state=substate=subsubstate=s_start;
    int32_t c;
    (*sout)<<"% In order for this conversion to work your document needs a few things."<<endl;
    (*sout)<<"% \\usepackage{fontspec}"<<endl;
    (*sout)<<"% \\usepackage{tikz}"<<endl;
    (*sout)<<"% \\begin{document}"<<endl;
    (*sout)<<"% \\newfontfamily\\swfill{SuttonSignWritingFill.ttf}"<<endl;
    (*sout)<<"% \\newfontfamily\\swline{SuttonSignWritingLine.ttf}"<<endl;
    (*sout)<<endl;
    while(sin->peek()!=-1)
    {
        c=getChar(sin);
        if     (state==s_start)  start (c);
        else if(state==s_prefix) prefix(c);
        else if(state==s_visual) visual(c);
        else throw "Unknown state.";
    }
    return 0;
}

void start                   (uint32_t c)
{
    if(substate==s_start) start_start(c);
    else throw "Unknown substate.";
}

void prefix                  (uint32_t c)
{
    if(substate==s_symbol) prefix_symbol(c);
    else throw "Unknown substate.";
}

void visual                  (uint32_t c)
{
    if     (substate==s_start)     visual_start    (c);
    else if(substate==s_size)      visual_size     (c);
    else if(substate==s_symbol)    visual_symbol   (c);
    else if(substate==s_placement) visual_placement(c);
    else throw "Unknown substate.";
}

void start_start             (uint32_t c)
{
    if(subsubstate==s_start) start_start_start(c);
    else throw "Unknown subsubstate.";
}

void prefix_symbol           (uint32_t c)
{
    if     (subsubstate==s_start)   prefix_symbol_start   (c);
    else if(subsubstate==s_first)    prefix_symbol_first   (c);
    else if(subsubstate==s_second)   prefix_symbol_second  (c);
    else if(subsubstate==s_third)    prefix_symbol_third   (c);
    else if(subsubstate==s_fill)     prefix_symbol_fill    (c);
    else if(subsubstate==s_rotation) prefix_symbol_rotation(c);
    else throw "Unknown subsubstate.";
}

void visual_start            (uint32_t c)
{
    if     (subsubstate==s_start)   visual_start_start   (c);
    else throw "Unknown subsubstate.";
}

void visual_size             (uint32_t c)
{
    if     (subsubstate==s_firstw)  visual_size_firstw (c);
    else if(subsubstate==s_secondw) visual_size_secondw(c);
    else if(subsubstate==s_thirdw)  visual_size_thirdw (c);
    else if(subsubstate==s_x)       visual_size_x      (c);
    else if(subsubstate==s_firsth)  visual_size_firsth (c);
    else if(subsubstate==s_secondh) visual_size_secondh(c);
    else if(subsubstate==s_thirdh)  visual_size_thirdh (c);
    else throw "Unknown subsubstate.";
}

void visual_symbol           (uint32_t c)
{
    if     (subsubstate==s_start)    visual_symbol_start   (c);
    else if(subsubstate==s_first)    visual_symbol_first   (c);
    else if(subsubstate==s_second)   visual_symbol_second  (c);
    else if(subsubstate==s_third)    visual_symbol_third   (c);
    else if(subsubstate==s_fill)     visual_symbol_fill    (c);
    else if(subsubstate==s_rotation) visual_symbol_rotation(c);
    else throw "Unknown subsubstate.";
}

void visual_placement        (uint32_t c)
{
    if     (subsubstate==s_firstw)  visual_placement_firstw (c);
    else if(subsubstate==s_secondw) visual_placement_secondw(c);
    else if(subsubstate==s_thirdw)  visual_placement_thirdw (c);
    else if(subsubstate==s_x)       visual_placement_x      (c);
    else if(subsubstate==s_firsth)  visual_placement_firsth (c);
    else if(subsubstate==s_secondh) visual_placement_secondh(c);
    else if(subsubstate==s_thirdh)  visual_placement_thirdh (c);
    else if(subsubstate==s_end)     visual_placement_end    (c);
    else throw "Unknown subsubstate.";
}

void start_start_start       (uint32_t c)
{
    if(c=='A' || c==0x1d800)
        { line.push_back(c); state=s_prefix; substate=s_symbol; subsubstate=s_start; }
    else if(c=='B' || (c>='L'&&c<='M') || c=='R' || (c>=0x1d801&&c<=0x1d804))
        { line.push_back(c); state=s_visual; substate=s_size; subsubstate=s_firstw; }
    else
        sendOut(c);
}

void prefix_symbol_start     (uint32_t c)
{
    if(c=='S')
        { line.push_back(c); subsubstate=s_first; }
    else if(c>=0x40001 && c<=0x4f428)
        { line.push_back(c); state=s_visual; substate=s_start; subsubstate=s_start; }
    else
        sendOut(line,c);
}

void prefix_symbol_first     (uint32_t c)
{
    if(c>='1' && c<='3')
        { line.push_back(c); subsubstate=s_second; }
    else
        sendOut(line,c);
}

void prefix_symbol_second    (uint32_t c)
{
    if(line[line.size()-1]>='0'&&line[line.size()-1]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
            { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(line,c);
    }
    else
    {
        if(c>='0' && c<='8')
            { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(line,c);
    }
}

void prefix_symbol_third     (uint32_t c)
{
    if(line[line.size()-2]>='0'&&line[line.size()-2]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
            { line.push_back(c); subsubstate=s_fill; }
        else
            sendOut(line,c);
    }
    else
    {
        if(line[line.size()-1]>='0'&&line[line.size()-1]<='7')
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
                { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(line,c);
        }
        else
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='b'))
                { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(line,c);
        }
    }
}

void prefix_symbol_fill      (uint32_t c)
{
    if(c>='0' && c<='5')
        { line.push_back(c); subsubstate=s_rotation; }
    else
        sendOut(line,c);
}

void prefix_symbol_rotation  (uint32_t c)
{
    if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
        { line.push_back(c); state=s_visual; substate=subsubstate=s_start; }
    else
        sendOut(line,c);
}

void visual_start_start      (uint32_t c)
{
    if(c=='B' || (c>='L'&&c<='M') || c=='R' || (c>=0x1d801&&c<=0x1d804))
        { line.push_back(c); substate=s_size; subsubstate=s_firstw; }
    else if(c=='S')
        { line.push_back(c); state=s_prefix; substate=s_symbol; subsubstate=s_first; }
    else if(c>=0x40001 && c<=0x4f428)
        { line.push_back(c); state=s_visual; substate=s_start; subsubstate=s_start; }
    else
        sendOut(line,c);
}

void visual_size_firstw      (uint32_t c)
{
    if(c>='2' && c<='7')
        { line.push_back(c); subsubstate=s_secondw; }
    else if(c>=0x1d80c && c<=0x1d9ff)
        { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(line,c);
}

void visual_size_secondw     (uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
            { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
            { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
            { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(line,c);
    }
}

void visual_size_thirdw      (uint32_t c)
{
    if(c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_x; }
    else
        sendOut(line,c);
}

void visual_size_x           (uint32_t c)
{
    if(c=='x')
        { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(line,c);
}

void visual_size_firsth      (uint32_t c)
{
    if(c>='2' && c<='7')
        { line.push_back(c); subsubstate=s_secondh; }
    else if(c>=0x1d80c && c<=0x1d9ff)
        { line.push_back(c); substate=s_symbol; subsubstate=s_start; }
    else
        sendOut(line,c);
}

void visual_size_secondh     (uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
            { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
            { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
            { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(line,c);
    }
}

void visual_size_thirdh      (uint32_t c)
{
    if(c>='0' && c<='9')
        { line.push_back(c); substate=s_symbol; subsubstate=s_start; }
    else
        sendOut(line,c);
}

void visual_symbol_start     (uint32_t c)
{
    if(c=='S')
        { line.push_back(c); subsubstate=s_first; }
    else if(c>=0x40001 && c<=0x4f428)
        { line.push_back(c); state=s_visual; substate=s_placement; subsubstate=s_first; }
    else
        sendOut(line,c);
}

void visual_symbol_first     (uint32_t c)
{
    if(c>='1' && c<='3')
        { line.push_back(c); subsubstate=s_second; }
    else
        sendOut(line,c);
}

void visual_symbol_second    (uint32_t c)
{
    if(line[line.size()-1]>='0'&&line[line.size()-1]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
            { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(line,c);
    }
    else
    {
        if(c>='0' && c<='8')
            { line.push_back(c); subsubstate=s_third; }
        else
            sendOut(line,c);
    }
}

void visual_symbol_third     (uint32_t c)
{
    if(line[line.size()-2]>='0'&&line[line.size()-2]<='2')
    {
        if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
            { line.push_back(c); subsubstate=s_fill; }
        else
            sendOut(line,c);
    }
    else
    {
        if(line[line.size()-1]>='0'&&line[line.size()-1]<='7')
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
                { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(line,c);
        }
        else
        {
            if((c>='0'&&c<='9') || (c>='a'&&c<='b'))
                { line.push_back(c); subsubstate=s_fill; }
            else
                sendOut(line,c);
        }
    }
}

void visual_symbol_fill      (uint32_t c)
{
    if(c>='0' && c<='5')
        { line.push_back(c); subsubstate=s_rotation; }
    else
        sendOut(line,c);
}

void visual_symbol_rotation  (uint32_t c)
{
    if((c>='0'&&c<='9') || (c>='a'&&c<='f'))
        { line.push_back(c); substate=s_placement; subsubstate=s_firstw; }
    else
        sendOut(line,c);
}

void visual_placement_firstw (uint32_t c)
{
    if(c>='2' && c<='7')
        { line.push_back(c); subsubstate=s_secondw; }
    else if(c>=0x1d80c && c<=0x1d9ff)
        { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(line,c);
}

void visual_placement_secondw(uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
            { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
            { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
            { line.push_back(c); subsubstate=s_thirdw; }
        else
            sendOut(line,c);
    }
}

void visual_placement_thirdw (uint32_t c)
{
    if(c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_x; }
    else
        sendOut(line,c);
}

void visual_placement_x      (uint32_t c)
{
    if(c=='x')
        { line.push_back(c); subsubstate=s_firsth; }
    else
        sendOut(line,c);
}

void visual_placement_firsth (uint32_t c)
{
    if(c>='2' && c<='7')
        { line.push_back(c); subsubstate=s_secondh; }
    else if(c>=0x1d80c && c<=0x1d9ff)
        { line.push_back(c); subsubstate=s_end; }
    else
        sendOut(line,c);
}

void visual_placement_secondh(uint32_t c)
{
    if(line[line.size()-1]=='2')
    {
        if( c>='5' && c<='9')
            { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(line,c);
    }
    else if(line[line.size()-1]>='3'&&line[line.size()-1]<='6')
    {
        if( c>='0' && c<='9')
            { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(line,c);
    }
    else // if(line[line.size()-1]=='7')
    {
        if( c>='0' && c<='4')
            { line.push_back(c); subsubstate=s_thirdh; }
        else
            sendOut(line,c);
    }
}

void visual_placement_thirdh (uint32_t c)
{
    if(c>='0' && c<='9')
        { line.push_back(c); subsubstate=s_end; }
    else
        sendOut(line,c);
}

void visual_placement_end    (uint32_t c)
{
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
                 if(line[place]=='S') place+=6;
                 else                 place++;
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
        (*sout)<<"{\\makeatletter";//\\setmainfont{SuttonSignWritingLine.ttf}";
        (*sout)<<"\\begin{tikzpicture}";
        // The idea was, initally, to place a thin rectangle behind each character from 0--1000.
        // Unfortunately, this made it so that I could reasonably fit about two columns of
        // \normalsize text to a page. We are now only extend 100 each direction to allow for about
        // five columns of \normalsize text. This also decided our lane shift amount later on.

        // Yes, I know the number 100 doesn't appear. That's because I found through experimentation
        // that a character anchored at (0,0) along with a rectangle around (0,0) does not place a
        // rectangle around the expected corner.
        if(lane != 'B')
              (*sout)<<"\\draw[white](\\f@size/30*-90 pt,\\f@size/30*-12 pt)rectangle(\\f@size/30*110 pt,\\f@size/30*-10 pt);";
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
            (*sout)<<"\\draw(\\f@size/30*";
            (*sout)<<sx;
            (*sout)<<" pt,\\f@size/30*";
            (*sout)<<(-sy);
            (*sout)<<" pt) node [color=white,anchor=north west] {\\swfill\\char";
            (*sout)<<(0x100001+s);
            (*sout)<<"};";
            (*sout)<<"\\draw(\\f@size/30*";
            (*sout)<<sx;
            (*sout)<<" pt,\\f@size/30*";
            (*sout)<<(-sy);
            (*sout)<<" pt) node [anchor=north west] {\\swline\\char";
            (*sout)<<(0xf0001+s);
            (*sout)<<"};";
        }
        (*sout)<<"\\end{tikzpicture}";
        (*sout)<<"}";
        if(lane!='B')
            (*sout)<<"\\\\";
        line.clear();
        (*sout)<<static_cast<char>(c);
        state=substate=subsubstate=s_start;
    }
}

