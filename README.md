# XYNTA FS
## Overview
Tired with tons of disorganized exploitable mems pictures on your hard drive, but can't remember where you saved appropriate ones when you're so desperately need them?
Forget about this pain. XYNTA FS will help you to organize you pictures, photos, books and documents, so you could find desired one in no time.
## How does it work?
If your eyes a red enough, you probably heard about extends file attributes in Linux. If they are crimson read, then you know what is File System in Userspace. So, this is what you going to do with your files:
1. For each file, set extended file attribute `user.xynta.tags` to tag list separated with space. For example: `setfattr -n user.xynta.tags -v "pepe sad meme exploitable pic" data/pepe-the-frog.jpg`
2. Tag two files or more. We are all so diligent, so most of us will stop after dozen of files. Don't worry, this is mostly about tagging incoming files, and as the time passes, you will find that most of your collection is organized, handy and useful.
3. OK! It's time to turn directory with tagged files (let is be `data`) into useful file system: `make && mkdir fsroot && xynta $(pwd)/data fsroot`.
4. Done. Try list files in `fsroot` folders. You will find that in every subfolder of `fsroot` there are only files which match tags presented in path.
