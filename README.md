# SignWritingLaTeX

A set of tools to turn Formal SignWriting into LaTeX documents.

This project was started as I tried to teach myself American Sign Language (B525x535S2e748483x510S10011501x466S2e704510x500S10019476x475).

I always find it helps to read and write along with listen and speak, so I have been making supplements to [ASL University](http://www.lifeprint.com/asl101/lessons/lessons.htm) based on information from [SignWriting](http://signwriting.com/).

So why [ASL University](http://www.lifeprint.com/asl101/lessons/lessons.htm)? In a word, licensing. He let's any teacher use the materials in in-person classes. I am teaching myself in-person, so I am utilizing his materials. This wasn't my first time trying to learn but I can't publish anything based on them without getting into copyright violations.

So why [SignWriting](http://signwriting.com/)? That's a little more involved but the short version is that I tried several writing systems and eventually got to a point where I couldn't record something semantically important, except for SignWriting.

Here's the tools so far.

## fswtotex

This program takes text files with Formal SignWriting in them and converts them into text files with [TikZ](https://github.com/pgf-tikz/pgf) instructions to make SignWriting words.

This program runs in about three different methods.

* If you provide no arguments, it reads from standard in and send the results to standard out.
* If you provide one argument, it reads from that file and send the results to standard out.
* If you provide two arguments, it reads from the first file and send the results to the second file.

My most common usage is something along the lines of:

```
./fswtotex supplement01.sw.tex supplement01.tex
```

This program is fairly minimal and assumes that you will place enough LaTeX code before your first SignWriting word to ensure it works. At a minimum you need something along the lines of:

```
\\usepackage{fontspec}
\\usepackage{tikz}
\\begin{document}
\\newfontfamily\\swfill{SuttonSignWritingFill.ttf}
\\newfontfamily\\swline{SuttonSignWritingLine.ttf}
```

The 7-bit FSW strings have been tested extensively, because that's what I happen to be using. The Unicode FSW strings have not been tested. There's really no excuse for it, I just haven't bothered.

## extractgloss

This tool is not designed to be general purpose. It can be used if you happen to format a SignWritingLaTeX document just right.
It's purpose is to find the glossary section of each supplement and spit out English/ASL pairs of words.


## sortenu

This tool is just a shade more general purpose. It expects pairs of lines with English/ASL pairs of words. It also assumes that the ASL words are in the middle lane and transforms them into horizontal words since the expected use is in an English list.

# Building

```
make
```

Simple and to the point.
I don't install (and you may have noticed that my example call was "./fswtotex ..." indicating that it's not in my path. Maybe some day, but for right now my focus is on my supplements.

# Future

## More tools

I currently have a set of tools to extract and make a glossary, which I will share when they are tested a bit more thouroughly.

## Sorting

I have a pair of tools that I use to sort my glossary. I don't (yet) follow the official sorting at [SignWriting](http://signwriting.com/), but I haven't totally figured out what type of sort works for me. Perhaps the one at [SignWriting](http://signwriting.com/) will eventually make sense to me?

## Cheetsheet

I have a SignWriting cheatsheet that I consult on occassion. To my benefit, I look at it less and less, but it includes information on sorting so I'm not ready to share it yet either.

## Supplements

My current focus is supplements for lessons 1-15. I've settled down on the idea that you should understand the writing system within a semester. I'm still working on the exact order of presentation (sorting again), but I plan to place these up here as well.
