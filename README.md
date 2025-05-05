# MC Extractor
Simple minecraft asset extactor.  
The way that miencraft stores its larger assets makes it more dificult to access them. Mores specificaly instead of storing larger files like music inside of the game jar theya re instes stored speratly in the assets folder. In this folder they are named their hash valed insted of file name. This program aims to make the process of extraciong theese files extreamly simple.


## Useage
Run the program in a terminal from any location  
Type the name of the file you are looking for to reduce the number of displayed results  
Use the arrows keys to select the exact file you want
Press Enter to extract the file
The file will be located in the folder you launched this program in

## Compiling
For most situations it should be as simple as just passing `Mc\ Extractor.cpp` into your compiler  
Note for MSVC users:  
You will also need to include the following compiler flag: `/std:c++20` 
