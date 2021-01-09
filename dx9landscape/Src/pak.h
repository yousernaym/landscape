#pragma once

struct SpakHeader
{
	UINT ftOffset;
	UINT nFiles;
	UINT ftComprSize;
};

struct SftEntry
{
	bool bReadFromMem;
	char fName[100];
	void *memFile;
	UINT fOffset;
	UINT fSize;
	UINT fComprSize;
	SftEntry *next;

	//Constructor/Destructor
	SftEntry()
	{
		next=0;
	}
	~SftEntry()
	{
		safeDelete(next);
	}
};

class Cpak
{
private:
	int state;
	string pakFilename;
	FILE *pakFile;
	string dataFolder;
    SpakHeader pakHeader;
	SftEntry *ftEntry;
	SftEntry *current;
	
	BOOL addEntry2(string folder, string writePath);
public:
	Cpak();
	~Cpak();
	BOOL create(const string &srcFolder, const string &destFile, UINT level=6);
	BOOL init(const string &file);
	BOOL addEntry(const string &folder, const string &writePath="");
	BOOL addEntry(void *data, UINT size, const string &name);
	BOOL writePakFile(UINT level=6);
	
	BOOL load(const string &file);
	BOOL extractEntry(const string &name, const string &saveFolder="");
	BOOL extractEntry(const string &name, void **data, UINT *size);
	BOOL extractAll(const string &saveFolder="");
	
	BOOL close();

	bool isInit();
	bool isFile();
	bool isOpen();
};
