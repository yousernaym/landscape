#include "general.h"
#include <zlib.h>
#include <zconf.h>

enum {STATE_INIT=1, STATE_FILE, STATE_OPEN};

//---------------------------------------------------
//Constructor/Destructor
Cpak::Cpak()
{
	state = 0;
	ftEntry = NULL;
	init("");
}

Cpak::~Cpak()
{
	close();
	safeDelete(ftEntry);
}

//write functions---------
BOOL Cpak::create(const string &srcFolder, const string &destFile, UINT level)
{
	if (!init(destFile))
		return FALSE;
	if (!addEntry(srcFolder))
		return FALSE;
	if (!writePakFile(level))
		return FALSE;
	return TRUE;
}

BOOL Cpak::init(const string &file)
{
	if (file.empty())
		return FALSE;

	if (!close())
		return FALSE;
	safeDelete(ftEntry);
	pakFile = NULL;
	dataFolder = "";
	current = NULL;
	ZeroMemory(&pakHeader, sizeof(pakHeader));
	pakHeader.ftOffset = sizeof(SpakHeader);
	pakFilename = file;
	state = STATE_INIT;
	
	return TRUE;
}

//private
BOOL Cpak::addEntry2(string folder, string writePath)
{
	WIN32_FIND_DATA findData;
	addBackslash(folder);
	if (!writePath.empty())
		addBackslash(writePath);
	string folder2=folder + "*.*";
	HANDLE hFind = FindFirstFile(folder2.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	while (true)
	{
		if (!findData.nFileSizeLow)
		{
			//Found a folder
			if (!(strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0))
			{
				//Found a folder other than "." and ".."
				if (!addEntry2(folder + findData.cFileName, writePath + findData.cFileName))
					return FALSE;
			}
			if (!FindNextFile(hFind, &findData))
				break;
			continue;
		}

		SftEntry *tempEntry = new SftEntry;
		tempEntry->fSize=findData.nFileSizeLow;
		
		//The file will start right after the header
		//(where the filetable's offset currently is)
			//tempEntry->fOffset = pakHeader.ftOffset;
		
		//The filetable will start right after the file
			//pakHeader.ftOffset+=tempEntry->fSize;
		
		string fullName = writePath + findData.cFileName;
		strcpy_s(tempEntry->fName, fullName.c_str());
		tempEntry->bReadFromMem=false;

		pakHeader.nFiles++;
		
		tempEntry->next=ftEntry;
		ftEntry=tempEntry;
		
		if (!FindNextFile(hFind, &findData))
			break;
	}
	FindClose(hFind);
	if (GetLastError() != ERROR_NO_MORE_FILES)
		return FALSE;  //Unexpected error
	else
		return TRUE;
}

BOOL Cpak::addEntry(const string &folder, const string &writePath)
{
	if (!isInit() || folder.empty())
		return FALSE;

	dataFolder = folder;
	removeBackslash(dataFolder);

	if (!addEntry2(folder, writePath))
		return FALSE;

	return TRUE;
}

BOOL Cpak::addEntry(void *data, UINT size, const string &name)
{
	if (!isInit() || !data)
		return FALSE;
	
	SftEntry *tempEntry = new SftEntry;
	tempEntry->fSize=size;
	//tempEntry->fOffset=pakHeader.ftOffset;
	//pakHeader.ftOffset+=size;
	strcpy_s(tempEntry->fName, name.c_str());
	tempEntry->bReadFromMem=true;
	tempEntry->memFile=data;

	pakHeader.nFiles++;

	tempEntry->next=ftEntry;
	ftEntry=tempEntry;

	return TRUE;
}

BOOL Cpak::writePakFile(UINT level)
{
	if (!ftEntry || !isInit())
		return FALSE;

	if ((fopen_s(&pakFile, pakFilename.c_str(), "wb")))
		return FALSE;
	
	//Write files-------------------------------
	pakHeader.ftOffset = sizeof(SpakHeader);
	if (0 != fseek(pakFile, sizeof(SpakHeader), SEEK_SET))
		return FALSE;
	current = ftEntry;
	while(current)
	{
		void *buf=0;
		if (!current->bReadFromMem)
		{
			FILE *file;
			string path=dataFolder + "\\" + current->fName;
			if ((fopen_s(&file, path.c_str(), "rb")))
				return FALSE;
			buf = new BYTE[current->fSize];
			if (fread(buf, current->fSize, 1, file)<1)
				return FALSE;
			if (fclose(file)==EOF)
				return FALSE;
		}
		else
			buf = current->memFile;

		UINT comprSize = UINT(current->fSize * 1.1f + 12);
		BYTE *compr = new BYTE[comprSize];
		
		int err = compress2(compr, (uLongf*)&comprSize, (BYTE*)buf, current->fSize, level);
		
		if (err != Z_OK)
			return FALSE;

		if (fwrite(compr, comprSize, 1, pakFile) < 1)
			return FALSE;

		if (!current->bReadFromMem)
			safeDeleteArray(buf);
		
		safeDeleteArray(compr);

		current->fComprSize = comprSize;
		current->fOffset = pakHeader.ftOffset;
		pakHeader.ftOffset += comprSize;
		
		current=current->next;
			
	}
	//------------------------------------------

	//Write filetable-----------------------------
	UINT comprSize = UINT(sizeof(SftEntry) * pakHeader.nFiles * 1.1f + 12);
	BYTE *compr = new BYTE[comprSize];
	BYTE *uncompr = new BYTE[pakHeader.nFiles * sizeof(SftEntry)];
	//Copy linked list to uncompressed data chunk
	current = ftEntry;
	int i=0;
	while(current)
	{
		memcpy(uncompr + i * sizeof(SftEntry), current, sizeof(SftEntry));
		current=current->next;
		i++;
	}
	//Compress and write to disk
	int err = compress2(compr, (uLongf*)&comprSize, (BYTE*)uncompr, sizeof(SftEntry) * pakHeader.nFiles, 9);
	safeDeleteArray(uncompr);
	if (err != Z_OK)
		return FALSE;
	if (fwrite(compr, comprSize, 1, pakFile) < 1)
		return FALSE;
	safeDeleteArray(compr);
	
	pakHeader.ftComprSize = comprSize;
	//------------------------------------
	
	//Write header------------------------
	if (0 != fseek(pakFile, 0, SEEK_SET))
		return FALSE;
	if (fwrite(&pakHeader, sizeof(pakHeader), 1, pakFile)<1)
		return FALSE;
	//----------------------------------

	if (fclose(pakFile) == EOF)
		return FALSE;

	return TRUE;
}
		
//load functions-----------------
BOOL Cpak::load(const string &file)
{
	if (file.empty())
		return FALSE;
	init(file);
	
	if ((fopen_s(&pakFile, file.c_str(), "rb")))
		return FALSE;
	
	//Read header
	if (fread(&pakHeader, sizeof(pakHeader), 1, pakFile)<1)
		return FALSE;
	
	//Read filetable-------------------
	if (0 != fseek(pakFile, pakHeader.ftOffset, SEEK_SET))
		return FALSE;
	
	//Read compressed data chunk
	BYTE *compr = new BYTE[pakHeader.ftComprSize];
	if (fread(compr, pakHeader.ftComprSize, 1, pakFile) < 1)
		return FALSE;

	//Uncompress
	BYTE*uncompr = new BYTE[pakHeader.nFiles * sizeof(SftEntry)];
	uLongf size = pakHeader.nFiles * sizeof(SftEntry);
	int err = uncompress((BYTE*)uncompr, &size, compr, pakHeader.ftComprSize);
	safeDeleteArray(compr);
	if (err != Z_OK)
		return FALSE;

	//Copy data from uncompressed data chunk to linked list
	for (UINT i=0;i<pakHeader.nFiles;i++)
	{
		current = new SftEntry;
		memcpy(current, uncompr + i * sizeof(SftEntry), sizeof(SftEntry));
		
		current->next=ftEntry;
		ftEntry=current;
	}
	safeDeleteArray(uncompr);
	//--------------------------------------
	
	state=STATE_OPEN;
	return TRUE;
}

BOOL Cpak::extractEntry(const string &name, const string &saveFolder)
{
	if (!isOpen())
		return FALSE;

	void *data;
	UINT size;
	if (!extractEntry(name, &data, &size))
		return FALSE;
    
	//Save found file
	string path;
	if (saveFolder.empty())
	{ 
		//Default to same folder as pakfile is in + extract folder
		//path.assign(pakFilename
		path=pakFilename;
		int slashpos=pakFilename.rfind("\\");
		path.erase(slashpos+1);
		path=path + "PAK_Extract";
		if (!CreateDirectory(path.c_str(), 0))
		{
			//Unexpected error (not "already exists")?
			if (GetLastError() != 183)
				return FALSE;
		}
	}
	else
	{
		path = saveFolder;
		removeBackslash(path);
	}
	
	int spos;
	string name2 = name;
	while( (spos = name2.find("\\")) != string::npos )
	{
        //Add folder to path
		string s = "\\" + name2;
		path.append(s.c_str(), spos + 1);
		//Erase folder so it won't be found more than once
		name2.erase(0, spos + 1);
		//Create folder
		if (!CreateDirectory(path.c_str(), 0))
		{
			//Unexpected error (not "already exists")?
			if (GetLastError() != 183)
				return FALSE;
        }
	}
	path += "\\" + name2;

	FILE *file;
	//Does file already exist?
	if (fopen_s(&file, path.c_str(), "rb"))
	{
		fclose(file);
		return FALSE;
	}

	if ((fopen_s(&file, path.c_str(), "wb")))
		return FALSE;
	
	if (fwrite(data, size, 1, file)<1)
		return FALSE;
	
	safeDeleteArray(data);

	if (fclose(file)==EOF)
		return FALSE;

    return TRUE;
}

BOOL Cpak::extractEntry(const string &name, void **data, UINT *size)
{
	if (!isOpen())
		return FALSE;

	current = ftEntry;
	while (current)
	{
		if (lcase(current->fName) == lcase(name))
			break;
		current=current->next;
	}
	//File not found?
	if (!current)
		return FALSE;
	
	if (0 != fseek(pakFile, current->fOffset, SEEK_SET))
		return FALSE;
	
	BYTE *compr = new BYTE[current->fComprSize];
	if (fread(compr, current->fComprSize, 1, pakFile)<1)
		return FALSE;

	*size = current->fSize;
	*data = new BYTE[*size];
	int err = uncompress((BYTE*)*data, (uLongf*)size, compr, current->fComprSize);
	safeDeleteArray(compr);
	if (err != Z_OK)
		return FALSE;
	if (*size != current->fSize)
		return FALSE;
		
	return TRUE;
}

BOOL Cpak::extractAll(const string &saveFolder)
{
	if (!isOpen())
		return FALSE;

	//overload member with local copy
	SftEntry *current = ftEntry;  
	while (current)
	{
		if (!extractEntry(current->fName, saveFolder))
			return FALSE;
		current=current->next;
	}
	if (!close())
		return FALSE;
	return TRUE;
}
//-----------------------
BOOL Cpak::close()
{
	if (isOpen())
	{
		if (fclose(pakFile) == EOF)
			return FALSE;
		state-=1;
	}
	return TRUE;
}

bool Cpak::isInit()
{
	return (state>=STATE_INIT);
}

bool Cpak::isFile()
{
	return (state>=STATE_FILE);
}

bool Cpak::isOpen()
{
	return (state>=STATE_OPEN);
}
