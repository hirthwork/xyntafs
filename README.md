# XYNTA FS [![Coverage Status](https://coveralls.io/repos/github/hirthwork/xyntafs/badge.svg?branch=master)](https://coveralls.io/github/hirthwork/xyntafs?branch=master)
## Overview
Tired of tons of disorganized exploitable memes pictures on your hard drive, but can't remember where you saved appropriate ones when you so desperately need them? Forget about this pain. XYNTA FS will help you to organize your pictures, photos, books and documents so you could find desired one in no time.
## How does it work?
If your eyes a red enough, you probably heard about extended file attributes in Linux. If they are crimson red, then you know what is File System in Userspace. So, this is what you are going to do with your files:

1. For each file, set extended file attribute `user.xynta.tags` to tag list separated with space. For example: `setfattr -n user.xynta.tags -v "pepe sad meme exploitable pic" data/pepe-the-frog.jpg`
2. Tag two files or more. We are all so diligent, so most of us will stop after dozen of files. Don't worry, this is mostly about tagging incoming files, and as the time passes, you will find that most of your collection is organized, handy and useful.
3. OK! It's time to turn the directory with tagged files (let is be `data`) into an useful file system: `make && mkdir fsroot && ./xynta $(pwd)/data fsroot`.
4. Done. Try to list files in `fsroot` folders. You will find that in every subfolder of `fsroot` there are only files which match tags present in the path:
```
$ getfattr -n user.xynta.tags ../data/*
# file: ../test_data/arnold-taskbook.pdf
user.xynta.tags=0sbWF0aCDQt9Cw0LTQsNGH0L3QuNC6

# file: ../test_data/javacc-tutorial.pdf
user.xynta.tags="java book pdf javacc it programming"

# file: ../test_data/n4606.pdf
user.xynta.tags="c++ book iso c++1z it programming"

# file: ../test_data/progit.pdf
user.xynta.tags="git it book pdf"

# file: ../test_data/ragel-guide-6.7.pdf
user.xynta.tags="ragel book it programming"
$ ls
arnold-taskbook.pdf  iso                  math         ragel
book                 it                   n4606.pdf    ragel-guide-6.7.pdf
c++                  java                 pdf          задачник
c++1z                javacc               progit.pdf
git                  javacc-tutorial.pdf  programming
$ ls c++
n4606.pdf
$ ls java
javacc-tutorial.pdf
$ ls programming
book  c++1z  it    javacc               n4606.pdf  ragel
c++   iso    java  javacc-tutorial.pdf  pdf        ragel-guide-6.7.pdf
$ ls programming/ragel
ragel-guide-6.7.pdf
```
