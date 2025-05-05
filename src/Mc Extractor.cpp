//============================================================================
// Name        : Mc Extractor.cpp
// Author      : jSdCool
// Version     :
// Copyright   : shut up!
// Description :
//============================================================================

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <ctime>
#include <chrono>
#include <cctype>
#include <fstream>
#include "rogueutil.h"
#include "json.hpp"
#include "stopHandler.h"


using namespace std;
using namespace rogueutil;
using json = nlohmann::json;
namespace fs = std::filesystem;

struct TerminalConditioner{
	CursorHider hideTheCursor;
	TerminalConditioner(){
		saveDefaultColor();
	}
	~TerminalConditioner(){
		resetColor();
		gotoxy(0,trows());
	}
};

bool stringContains(string &source, string &contains){
	return source.find(contains) != string::npos;
}

string getEnvVar(string const & key ){
    char * val = getenv( key.c_str() );
    return val == NULL ? string("") : string(val);
}

string getMinecraftFolder(){
#ifdef _WIN32
	return getEnvVar("appdata") + "/.minecraft/";
#else
	//no idea if this will work for macOS
	return getEnvVar("HOME") + "/.minecraft/";
#endif
}

vector<string> get_all_files_in_directory(const string& path) {
    vector<string> files;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (fs::is_regular_file(entry)) {
            files.push_back(entry.path().string());
        }
    }
    return files;
}

//stolen from https://stackoverflow.com/questions/61030383/how-to-convert-stdfilesystemfile-time-type-to-time-t
template <typename TP>
time_t to_time_t(TP tp){
    using namespace chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}


string findMostRecentFile(vector<string> &files, string extention){
	time_t recent;
	string file = "";
	bool first = true;
	for(string s: files){
		if(!stringContains(s,extention)){
			continue;
		}

		fs::file_time_type currentFileTime = fs::last_write_time(s);
		time_t fileUnixTime =  to_time_t(currentFileTime);
		if(first){
			recent = fileUnixTime;
			file = s;
			first = false;
		}else{
			if(fileUnixTime > recent){
				recent = fileUnixTime;
				file = s;
			}
		}
	}

	return file;
}

void renderMainText(int width,int height, int selected, vector<string> &options){
	int scrollvalue = -2;

	if(selected > height - 6){
		scrollvalue += selected - height+6;
	}
	//starting at row 1 untill 2 less then height
	for(int i=2;i<height-2;i++){
		int currentIndex = i+scrollvalue;
		//print the content of optins modidied by any nessarry scrolling
		//if this line is currently selected then make the line gray
		gotoxy(3,i);
		if(currentIndex == selected){
			setBackgroundColor(GREY);
			setColor(BLACK);
		}
		int textWidth = 0;
		if(currentIndex < (int)options.size()){
			cout << options[currentIndex];
			textWidth = (int)options[currentIndex].size();
		}
		//padding
		for(int j=0;j<width-textWidth-3;j++){
			cout << " ";
		}
		if(currentIndex == selected){
			resetColor();
		}
	}
}

void renderSearch(int width,int height, string searchText,int cursorPos, bool hilighColor){
	gotoxy(1,height-1);
	if(hilighColor){
		setBackgroundColor(GREEN);
	}else{
		setBackgroundColor(CYAN);
	}
	setColor(BLACK);
	if((int)searchText.size() < width){
		cout << searchText;
		for(int i=0;i<width-(int)searchText.size();i++){
			cout << ' ';
		}
	}else{
		string sub = searchText.substr(searchText.size()-width, searchText.size());
		cout << sub;
	}

	resetColor();
	cout.flush();
}

void filterResults(string &filter, vector<string> &output, vector<string> &input){
	output.clear();
	for(string s:input){
		if(stringContains(s, filter)){
			output.push_back(s);
		}
	}
}

string getFileName(string &path){

	for(int i= path.size()-1;i>=0;i--){
		if(path[i] == '/' || path[i] == '\\'){
			return path.substr(i+1, path.size());
		}
	}

	return path;
}

void extract(string &fileName,json indeices, string &assetsFolder){
	//get relevant info about the file entry
	json fileInfo = indeices[fileName];
	string hash = fileInfo["hash"];
	int size = fileInfo["size"];

	//get the actual name of the file without the rest of the path
	string realFileName = getFileName(fileName);

	//copy the content of the origonal file to a new file
	ifstream rawFile(assetsFolder+"objects/"+hash[0]+hash[1]+"/"+hash,ios::binary);
	ofstream saveFile(realFileName,ios::binary);
	char byteIn;
	for(int i=0;i<size;i++){
		rawFile.read(&byteIn, 1);
		saveFile.write(&byteIn, 1);
		//rawFile >> byteIn;
		//saveFile << byteIn;
	}
	//close the things
	rawFile.close();
	saveFile.flush();
	saveFile.close();

	renderSearch(tcols(),trows()," Extracted File: "+realFileName,0,true);
	msleep(5000);
}

void handleCtrlC(){
	resetColor();
	showcursor();
	gotoxy(0,trows());
	cout << "STOP"<< endl;
}


int main() {
	//get the minecraft asset folder location
	string mcAssetsFolder = getMinecraftFolder() + "assets/";

	//get a list of all the asset index files
	vector<string> indeciesFiles = get_all_files_in_directory(mcAssetsFolder+"indexes");
	if(indeciesFiles.size()==0){
		cerr << "No Index Files Found!" <<endl;
		return EXIT_FAILURE;
	}
	//find the most recently written index file
	string indexFilePath = findMostRecentFile(indeciesFiles,".json");

	if(indexFilePath == ""){
		cerr << "No Index Files Found!" << endl;
		return EXIT_FAILURE;
	}

	//check terminal sizes
	int terminalWidth = tcols();
	int terminalHeight = trows();

	if(terminalWidth < 80 || terminalHeight < 25){
		cerr << "Terminal Window too small. must be at leased 80X25" << endl;
		return EXIT_FAILURE;
	}

	//load the index file
	json indexFile;
	ifstream indexFileInputStream(indexFilePath);
	indexFileInputStream >> indexFile;
	indexFileInputStream.close();

	//get the "objects" object from the loaded json
	json indexedObjects = indexFile["objects"];

	vector<string> indexedFileKeys;
	//extract the true file names the exist as keys in the json object
	for(auto a : indexedObjects.items()){
		indexedFileKeys.push_back(a.key());
	}
	//register something to hanle ctrl c press
	stopHandler::setContrlCHandler(handleCtrlC);
	//setup the terminal for our graphics

	TerminalConditioner condition;
	cls();
	setConsoleTitle("MC Extractor");

	cout.flush();
	//prepair the initial graphics
	setBackgroundColor(GREEN);
	setColor(BLACK);
	gotoxy(1,1);
	cout << "MC EXTRACTOR";
	setColor(WHITE);
	cout << " ESC: Quit | ENTER: Extract | UP/DOWN: Select | TYPING: Search";
	for(int i=0;i<terminalWidth-(int)(string("MC Extractor ESC: Quit | ENTER: Extract | UP/DOWN: Select | TYPING: Search").size());i++){
		cout << ' ';
	}
	resetColor();

	renderMainText(tcols(),trows(),0,indexedFileKeys);
	renderSearch(tcols(),trows(),"Search: ",8,false);

	string searchString = "";
	vector<string> visableoptions;//copy keys
	int scrollValue = 0 ;

	for(string s: indexedFileKeys){
		visableoptions.push_back(s);
	}

	bool quit = false;
	while(!quit){
		char input = getkey();

		if(input == KEY_DOWN){
			scrollValue++;
			if(scrollValue >= (int)visableoptions.size()){
				scrollValue = (int)visableoptions.size()-1;
			}
		}else if(input == KEY_UP){
			scrollValue--;
			if(scrollValue < 0){
				scrollValue = 0;
			}
		} else if(input == KEY_ESCAPE){
			quit = true;
		} else if(input == KEY_ENTER){
			if(visableoptions.size()>0){
				extract(visableoptions[scrollValue],indexedObjects,mcAssetsFolder);
			}
		} else if(input == 8 || input == 127){//backspace (8 on windows 127 on linux)
			if(searchString.size()>0){
				searchString = searchString.substr(0, searchString.size()-1);
				filterResults(searchString,visableoptions,indexedFileKeys);
				if(scrollValue >= (int)visableoptions.size()){
					scrollValue = max((int)visableoptions.size()-1,0);
				}
			}else{
				continue;
			}
		}else if(isprint(input)){
			searchString+=input;
			filterResults(searchString,visableoptions,indexedFileKeys);
			if(scrollValue >= (int)visableoptions.size()){
				scrollValue = max((int)visableoptions.size()-1,0);
			}
		}else{
			continue;
		}

		renderMainText(tcols(),trows(),scrollValue,visableoptions);
		renderSearch(tcols(),trows(),"Search: "+searchString,8,false);
	}
	return EXIT_SUCCESS;
}
