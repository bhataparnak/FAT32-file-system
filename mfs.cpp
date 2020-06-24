/*
Student Name:- Aparna Krishna Bhat

Description:-You will implement a user space shell application that is capable of interpreting a FAT32 file system image. The utility must not corrupt
the file system image and should be robust. This project has been coded using C++.

Compilation and Execution:-
1) g++ mfs.cpp -o mfs.o
2) ./mfs.o

*/



//#include <fat.h>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>

using namespace std;

FILE * imageFile;
//The actual fat32 variables
char BS_OEMName[8]; 
int BPB_BytesPerSec; // bytes per sector
int BPB_SecPerClus; // sectors per cluster
int BPB_RsvdSecCnt; 
int BPB_NumFATs; // number of FAT data structures
int BPB_RootEntCnt; // number of 32 byte directories in the root
int BPB_FATSz32; 
int BPB_RootClus;
int RootDirClusterAddr = 0; // offset location of the root directory
int CurrentDirClusterAddr = 0;
char formattedDirectory[12];
char filename[100];
char *args[100];
char *arg;
char *base_command;
int directls = 0;
int fileSystemOpened = 0;

int32_t currentDirectory;

struct __attribute__((__packed__)) DirectoryEntry {
    char DIR_Name[12];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

void populate_dir(int DirectoryAddress, struct DirectoryEntry* direct) {
	int i;
    fseek(imageFile, DirectoryAddress, SEEK_SET);
    for(i = 0; i < 16; i ++){
        fread(direct[i].DIR_Name, 1, 11, imageFile);
        direct[i].DIR_Name[11] = 0;
        fread(&direct[i].DIR_Attr, 1, 1, imageFile);
        fread(&direct[i].Unused1, 1, 8, imageFile);
        fread(&direct[i].DIR_FirstClusterHigh, 2, 1, imageFile);
        fread(&direct[i].Unused2, 1, 4, imageFile);
        fread(&direct[i].DIR_FirstClusterLow, 2, 1, imageFile);
        fread(&direct[i].DIR_FileSize, 4, 1, imageFile);
    }
}

void decToHex(int dec)
{
    char hex[100];
    int i = 1;
    int j;
    int temp;
    while (dec != 0)
    {
        temp = dec % 16;
        if (temp < 10)
        {
            temp += 48;
        }
        else
        {
            temp += 55;
        }
        hex[i++] = temp;
        dec /= 16;
    }
    cout << "Hex : 0X";
    for (j = i - 1; j > 0; j--)
    {
        cout << hex[j] ;
    }
    cout << endl;
}

int fileExists(char* filename)
{

    struct stat buffer;
    int exist = stat(filename,&buffer);
    if(exist == 0)
        return 1;
    else // -1
        return 0;
}

int16_t next_lb(int16_t sector){
    uint32_t FATAddr = (BPB_RsvdSecCnt * BPB_BytesPerSec) + (sector * 4);
    int16_t val;
    fseek(imageFile, FATAddr, SEEK_SET);
    fread(&val, 2, 1, imageFile);
    return val;
}

void make_name(char* dir_name){
    int whitespace = 11 - strlen(dir_name);
    int counter ;
    
    for(counter = 0; counter < 11; counter ++){
        dir_name[counter] = toupper(dir_name[counter]);
    }
    
    for(counter = 0; counter < whitespace; counter ++){
        strcat(dir_name, " ");
    }
}

int LBtoAddr(int32_t sector){
    if(!sector)
        return RootDirClusterAddr;
    return (BPB_BytesPerSec * BPB_RsvdSecCnt) + ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_NumFATs * BPB_FATSz32);
}

void formatDirectory(char *dirname)
{
    char expanded_name[12];
    memset(expanded_name, ' ', 12);

    char *token = strtok(dirname, ".");

    if (token)
    {
        strncpy(expanded_name, token, strlen(token));

        token = strtok(NULL, ".");

        if (token)
        {
            strncpy((char *)(expanded_name + 8), token, strlen(token));
        }

        expanded_name[11] = '\0';

        int i;
        for (i = 0; i < 11; i++)
        {
            expanded_name[i] = toupper(expanded_name[i]);
        }
    }
    else
    {
        strncpy(expanded_name, dirname, strlen(dirname));
        expanded_name[11] = '\0';
    }
    strncpy(formattedDirectory, expanded_name, 12);
}

int32_t getCluster(char *dirname)
{
   
    formatDirectory(dirname);

    int i;
    for (i = 0; i < 16; i++)
    {
        char *directory = (char*)malloc(11);
        memset(directory, '\0', 11);
        memcpy(directory, dir[i].DIR_Name, 11);

        if (strncmp(directory, formattedDirectory, 11) == 0)
        {
            int32_t cluster = dir[i].DIR_FirstClusterLow;
            return cluster;
        }
    }

    return -1;
}

int32_t getSizeOfCluster(int32_t cluster)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        if (cluster == dir[i].DIR_FirstClusterLow)
        {
            int32_t size = dir[i].DIR_FileSize;
            return size;
        }
    }
    return -1;
}

void stat(char *dirname)
{
	if(!fileSystemOpened){
        cout << "File system not open!" <<endl;
        return;
    }

    /*if(!fileExists(dirname))
    {
    	cout << "File not found!" << endl;
    	return;
    }*/
    int cluster = getCluster(dirname);
   // cout << "cluster" << cluster << endl;
    int size = getSizeOfCluster(cluster);
    //cout << size;
    int i;
    for (i = 0; i < 16; i++)
    {
        if (cluster == dir[i].DIR_FirstClusterLow)
        {
            cout << "Attr: "<< dir[i].DIR_Attr<<endl;
           // cout << "Dir Name : "<< dir[i].DIR_Name;
            cout << "Starting Cluster: " << cluster<<endl;
            //cout << "Cluster High:  " << dir[i].DIR_FirstClusterHigh<<endl;
        }
    }
}

//Code for Stat begins
void show_stat(){
    char namebuffer[20]; // string for the name
    char extbuffer[4]; //for the extension
    char statbuffer[20]; // string will become the concatenation of the previous two
    char *token; 
    int whitespace; 
    int found = 0; 
    int counter;

   
    
    if(fileSystemOpened){
       
        if(strlen(args[0]) < 13){
           
            token = strtok(args[0], ".");
            if(token == NULL){
                cout << "Not a valid filename" <<endl;
                return;
            }
            strcpy(namebuffer, token);
            
            token = strtok(NULL, "");
           
            if(token == NULL && strlen(namebuffer) < 12){
               
                for(counter = 0; counter < 11; counter ++){
                    namebuffer[counter] = toupper(namebuffer[counter]);
                }
                
                whitespace = 11 - strlen(namebuffer);
                
                for(counter = 0; counter < whitespace; counter ++){
                    strcat(namebuffer, " ");
                }
               
                found = 0;
                for(counter = 0; counter < 16; counter ++){
                    
                    if(!strcmp(dir[counter].DIR_Name, namebuffer)){
                        //print attribute, starting cluster number, and size
                        cout << "Attribute: ";
						decToHex(dir[counter].DIR_Attr);
						cout << "Starting cluster number: " << dir[counter].DIR_FirstClusterLow ;
						decToHex(dir[counter].DIR_FirstClusterLow);
                        
                       
                        cout << "Size: 0 Bytes\n"; 
                        found = 1;
                        break;
                    }
                }    
                if(!found)
                    cout << "Couldn't find the directory\n";
            }
            else if(strlen(token) > 3){
                cout << "File extension too long (3 characters)\n";
            }
            else if(strlen(namebuffer) > 8){
                cout << "File name too long (8 characters).\n";
            }
            else{
               
                whitespace = 8 - strlen(namebuffer);
                
                for(counter = 0; counter < whitespace; counter ++){
                    strcat(namebuffer, " ");
                }
                strcpy(extbuffer, token);
                
                whitespace = 3 - strlen(extbuffer);
               
                for(counter = 0; counter < whitespace; counter ++){
                    strcat(extbuffer, " ");
                }
                
                sprintf(statbuffer, "%s%s", namebuffer, extbuffer);
               
                for(counter = 0; counter < 11; counter ++){
                    statbuffer[counter] = toupper(statbuffer[counter]);
                }
                //check through directory   
                found = 0;
                for(counter = 0; counter < 16; counter ++){
                    
                    if(!strcmp(dir[counter].DIR_Name, statbuffer)){
                        //print attribute, starting cluster number, and size
                        cout << "Attribute ";
						decToHex(dir[counter].DIR_Attr);
						cout << "Starting cluster number: " << dir[counter].DIR_FirstClusterLow;
						decToHex(dir[counter].DIR_FirstClusterLow);
                        cout << "Size: " << dir[counter].DIR_FileSize << "bytes"<<endl; 
                        found = 1;
                        break;
                    }
                }
                if(!found)
                    cout << "File not found\n";    
            }
        }
        else
            cout << "That is an invalid file or directory name\n";
    }
    else
        cout << "There is no filesystem open.\n";
}

int LBAToOffset(int32_t sector)
{
    if (sector == 0)
        sector = 2;
    return ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}


void change_dir(){
    char tempname[15]; // to hold the directory name
    char * token; // token to temporarily hold the next task
    int found;
    int counter; 
    
    if(!fileSystemOpened){
        cout << "File system not open!" <<endl;
        return;
    }
    
    if(args[0] == NULL){
        CurrentDirClusterAddr = RootDirClusterAddr;
        populate_dir(CurrentDirClusterAddr, dir);
        return;
    }
    
    token = strtok(args[0], "/");
    while(token != NULL){
        //check to see if it is valid
        if(strlen(token) > 12){
            cout << "Directory does not exist" << endl;
            return;
        }
        //take the name and make it uppercase.						       
        strcpy(tempname, token);
        make_name(tempname);      
        //check it against the current directory
        found = 0;
        for(counter = 0; counter < 16; counter ++){
            if(!strcmp(dir[counter].DIR_Name, tempname)){
                CurrentDirClusterAddr = LBtoAddr(dir[counter].DIR_FirstClusterLow);
                populate_dir(CurrentDirClusterAddr, dir);
                found ++;
                break;
            }
        }
           
        if(!found){
            cout <<"Could not find directory "<<token <<" stopped at 1 directory before" << endl;
            break;
        }
        //take the next token
        token = strtok(NULL, "/");
        if(token == NULL){
            break;
        }
    }
}


void make_file(char* file_name){
	int counter;
    char * token; 
    char result[15]; 
    int whitespace = 8; 
    token = strtok(file_name, ".");
    if(strlen(token) > 8 || token == NULL){
        cout << "Invalid filename" <<endl;
        return;
    }
    
    strcpy(result, token);
    
    token = strtok(NULL, ""); 
    if(strlen(token) > 3 || token == NULL){
        cout <<"Invalid extension for filename" <<endl;
        return;
    }
    
    whitespace -= strlen(result);
    for(counter = 0; counter < whitespace; counter ++){
       strcat(result, " ");
    }
    
    whitespace = 3 - strlen(token);
    strcat(result, token);
    for(counter = 0; counter < whitespace; counter ++){
        strcat(result, " ");
    }
    result[11] = 0;
    
    for(counter = 0; counter < 11; counter ++){
        result[counter] = toupper(result[counter]);
    }
   
    strcpy(file_name, result);
}

void read_file(){
	int counter;
    char * token;
    char tempname[15];
    int found; 
    int32_t tempAddr = CurrentDirClusterAddr; 
    struct DirectoryEntry tempdir[16];     
    int32_t size;
    int16_t LB; 
    int data = atoi(args[2]); 
    int BlockOff = atoi(args[1])/512;
    int ByteOff = atoi(args[1])%512; 
    int grab; 
    char buffer[513]; 
    if(!fileSystemOpened){
        cout << "File system not open" <<endl;
        return;
    }

    token = strtok(args[0], "/");
    
    while(1){
        
        if(strlen(token) > 12){
            cout << "Invalid argument!" <<endl;
            return;
        } 
       
        strcpy(tempname, token);
        token = strtok(NULL, "/");
        if(token == NULL){
            break;
        } 
        
        make_name(tempname);
        populate_dir(tempAddr, tempdir);
        found = 0;
        for(counter = 0; counter < 16; counter ++){
            
            if(!strcmp(tempdir[counter].DIR_Name, tempname)){
                tempAddr = LBtoAddr(tempdir[counter].DIR_FirstClusterLow);
                found ++;
                break;
            }
        }
        
        if(!found){
            cout << "Invalid Directory Name"<<endl;
            return;
        }
    }
   
    make_file(tempname);
    populate_dir(tempAddr, tempdir);
    found = 0;
    for(counter = 0; counter < 16; counter ++){
        
        if(!strcmp(tempdir[counter].DIR_Name, tempname)){
            size = tempdir[counter].DIR_FileSize;
            LB = tempdir[counter].DIR_FirstClusterLow;
            found ++;
            break;
        }
    }
    
    if(!found){
        cout <<"File Not Found!"<<endl;
        return;
    }
   
    if(size < data + atoi(args[1])){
        cout <<"More data requested than can be given!"<<endl;
        return;
    }
    
    for(counter = 0; counter < BlockOff; counter ++){
        LB = next_lb(LB);
    }
    
    tempAddr = LBtoAddr(LB);
    
    fseek(imageFile, tempAddr + ByteOff, SEEK_SET);
    
    while(1){
        
        if(data < 512)
            grab = data;
        else
            grab = 512;
        
        fread(buffer, 1, 512, imageFile);
        buffer[grab] = 0;
        
        for(counter = 0; counter < grab; counter ++){
            
            cout << buffer[counter];
        }
        
        data -= grab;
        if(data == 0){
            cout << endl;
            return;
        }
        
        LB = next_lb(LB);
        tempAddr = LBtoAddr(LB);
        
        fseek(imageFile, tempAddr, SEEK_SET); 
    }
}



void list_dir() {
	int counter;
    char *token; 
    char tempname[15]; 
    char dirname[15]; 
    int32_t tempAddr = CurrentDirClusterAddr; 
    struct DirectoryEntry tempdir[16];    
    int found; 
    
   if(!fileSystemOpened){
        cout << "File system not open!" <<endl;
        return;
    }

    
    if(directls == 1 || args[0] == NULL || !strcmp(args[0],".")){
        for(counter = 0; counter < 16; counter ++){
        	
            if(dir[counter].DIR_Attr == 1  ||
               dir[counter].DIR_Attr == 16 ||
               dir[counter].DIR_Attr == 32 ){
               	 
                cout  << dir[counter].DIR_Name << "  " << dir[counter].DIR_FileSize << "bytes" << endl;   
            }   
        }
        return;
    }
    
    token = strtok(args[0], "/");
    while(token != NULL){
        //check to see if it is valid
        if(strlen(token) > 12){
            cout << "Directory does not exist";
            return;
        }
        							       
        strcpy(tempname, token);
        make_name(tempname);      
        
        populate_dir(tempAddr, tempdir);
        
        found = 0;
        for(counter = 0; counter < 16; counter ++){
            if(!strcmp(tempdir[counter].DIR_Name, tempname)){
                tempAddr = LBtoAddr(tempdir[counter].DIR_FirstClusterLow);
                populate_dir(tempAddr, tempdir);
                found ++;
                break;
            }
        }
        
        if(!found){
            cout << "Could not find directory"<<endl ;
            break;
        }
        
        token = strtok(NULL,"/");
        
        if(token == NULL){
               
            for(counter = 0; counter < 16; counter ++){
               
                if(tempdir[counter].DIR_Attr == 1  ||
                   tempdir[counter].DIR_Attr == 16 ||
                   tempdir[counter].DIR_Attr == 32 ){
                    cout << tempdir[counter].DIR_Name <<"   " << tempdir[counter].DIR_FileSize <<"bytes"<<endl;   
                }   
            }
            break;
        }
    }
    
}


int openFat32Img(char* filename)
{
	
	if( access( filename, F_OK ) == -1 ) 
	{
			cout << filename <<"File System Image Not found" << endl;
			exit(1);
	}
	else
	{
		imageFile = fopen(filename, "rb");
		cout << "File opened" << endl;
		fseek(imageFile, 3, SEEK_SET);
        fread(BS_OEMName, 1, 8, imageFile);
        fseek(imageFile, 11, SEEK_SET);
        fread(&BPB_BytesPerSec, 1, 2, imageFile);
        fread(&BPB_SecPerClus, 1, 1, imageFile);
        fread(&BPB_RsvdSecCnt, 1, 2, imageFile);
        fread(&BPB_NumFATs, 1, 1, imageFile);
        fread(&BPB_RootEntCnt, 1, 2, imageFile);
        fseek(imageFile, 36, SEEK_SET);
        fread(&BPB_FATSz32, 1, 4, imageFile);
        fseek(imageFile, 44, SEEK_SET);
        fread(&BPB_RootClus, 1, 4, imageFile);
        currentDirectory = BPB_RootClus;

        RootDirClusterAddr = (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) +
                             (BPB_RsvdSecCnt * BPB_BytesPerSec);

        
        CurrentDirClusterAddr = RootDirClusterAddr;
        populate_dir(CurrentDirClusterAddr, dir);
		return 1;
	}

	
}


void info()
{

    if(!fileSystemOpened){
        cout << "File system not open!" <<endl;
        return;
    }

	cout << "BPB_BytesPerSec: " <<  BPB_BytesPerSec <<endl;
        decToHex(BPB_BytesPerSec);
        cout << endl;
        cout << "BPB_SecPerClus:  " << BPB_SecPerClus <<endl;
        decToHex(BPB_SecPerClus);
        cout << endl;
        cout <<"BPB_RsvdSecCnt: "<< BPB_RsvdSecCnt << endl;
        decToHex(BPB_RsvdSecCnt);
        cout << endl;
        cout << "BPB_NumFATs: " << BPB_NumFATs << endl;
        decToHex(BPB_NumFATs);
        cout << endl;
        cout <<"BPB_FATSz32: " << BPB_FATSz32 << endl;
        decToHex(BPB_FATSz32);
        cout << endl;
}
      



vector<string> split(const char *str, char c = ' ')
{
    vector<string> result;
    int i;

    do
    {
        const char *begin = str;

        while(*str != c && *str)
            str++;

        result.push_back(string(begin, str));
    } while (0 != *str++);

    for(i = 0; i < result.size() ; i++)
    {
    	cout << result[i];
    }

    return result;
}



void get(char *dirname)
{
	if(!fileSystemOpened){
        cout << "File system not open!" <<endl;
        return;
    }


    char *dirstring = (char *)malloc(strlen(dirname));
    strncpy(dirstring, dirname, strlen(dirname));
    int cluster = getCluster(dirstring);
    int size = getSizeOfCluster(cluster);
    FILE *newfp = fopen(args[0], "w");
    fseek(imageFile, LBAToOffset(cluster), SEEK_SET);
    unsigned char *ptr = (unsigned char*)malloc(size);
    fread(ptr, size, 1, imageFile);
    fwrite(ptr, size, 1, newfp);
    fclose(newfp);
}

int main(int argc , char **argv)
{

	char ch;
	string commandline;
	
	char command[100];
	int textSize = 0;
	char delimiter = ' ';
	string commandstr;
	string filenamestr = "";
	int counter;
	
	while(true)
	{
		cout << "mfs>";

		
		std::getline(std::cin,commandline);
		string commandstr = commandline.substr(0 , commandline.find(delimiter));
		

		if(commandstr == "quit")
		{
			if(fileSystemOpened == 1)
			{
				fclose(imageFile);
			}
			exit(0);
		}

		if(commandstr == "open")
		{
			if(fileSystemOpened == 0)
			{
				
				filenamestr = commandline.substr((commandline.find(delimiter) + 1) , commandline.length());
				strcpy(filename, filenamestr.c_str());
				args[0] = filename;
			
				openFat32Img(filename);
				fileSystemOpened = 1;
			}
			else
			{
				cout << "ERROR: File System Image already opened" << endl;
				fileSystemOpened = 1;
				continue;
			}
			
		}

		if(commandstr == "close")
		{
			if(fileSystemOpened == 0)
			{
				cout << "ERROR: File System not open" << endl;
				continue;
			}
			else
			{
				fclose(imageFile);
				cout << "file system closed" << endl;
				fileSystemOpened = 0;
			}
		}

		if(commandstr == "info")
		{
			info();
		}
		if(commandstr == "stat")
		{
			filenamestr = commandline.substr((commandline.find(delimiter) + 1) , commandline.length());
			strcpy(filename, filenamestr.c_str());
			args[0] = filename;
			show_stat();
		}
		if(commandstr == "ls")
		{
			if(commandline.find(delimiter) == std::string::npos)
			{
				directls = 1;
				//cout << directls << endl;
			}
			else
			{
				directls = 0;
				filenamestr = commandline.substr((commandline.find(delimiter) + 1) , commandline.length());				
				strcpy(filename, filenamestr.c_str());
				args[0] = filename;
			}
			
			list_dir();
		}
		if(commandstr == "cd")
		{
			
			filenamestr = commandline.substr((commandline.find(delimiter) + 1) , commandline.length());
			strcpy(filename, filenamestr.c_str());
			args[0] = filename;
			change_dir();
		}
		if(commandstr == "read")
		{
			int i;
			char commandlinechr[100];
			strcpy(commandlinechr, commandline.c_str());
			arg = strtok(commandlinechr, " ");
		

 
        // allocate memory for base command
	        base_command = (char*) malloc(sizeof(arg)+1);
	        base_command = arg;
	        
	        i = 0;
	        do { 
	            arg = strtok(NULL, " ");

	            args[i] = (char*) malloc(sizeof(arg)+1);
	            args[i] = arg;
	            i++;
	        
	        } while(i < 10);
	        
	        read_file();
		}

		if(commandstr == "get")
		{
			filenamestr = commandline.substr((commandline.find(delimiter) + 1) , commandline.length());
			strcpy(filename, filenamestr.c_str());
			args[0] = filename;
			get(filename);
		}
	
		
	}

	return 0;

}


