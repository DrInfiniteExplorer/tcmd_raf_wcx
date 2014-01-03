#include <Windows.h>
#include <string>
#include <algorithm>


#define STDCALL __stdcall

#include "wcxhead.h"

extern "C" {
    // Windows Commander Interface
    HANDLE	STDCALL OpenArchive(tOpenArchiveData *ArchiveData);
    int		STDCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData);
    int		STDCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName);
    int		STDCALL CloseArchive(HANDLE hArcData);
    int		STDCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags);
    int		STDCALL DeleteFiles(char *PackedFile, char *DeleteList);
    int		STDCALL GetPackerCaps(void);
    void	STDCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance);
    void	STDCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1);
    void	STDCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc);
}

HINSTANCE g_hInst;
BOOL  STDCALL  DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }
    return true;
}



#include <RiotFiles\RiotFiles.h>

struct ArchiveData {
    RiotArchiveFile* file;
    int currentFile;
};

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "RiotFiles.lib")

void displayError(const std::exception& t, const char* u) {
    MessageBoxA(nullptr, t.what(), u, 0);
}
void displayError(const std::exception& t) {
    displayError(t, "Exception caught");
}

HANDLE  STDCALL MyOpenArchive(tOpenArchiveData *archiveData) {
    try {
        auto file = new RiotArchiveFile(archiveData->ArcName);
        auto data = new ArchiveData{ file, -1 };
        //        data.file = file;
        //      data.currentFile = 0;
        return (void*)data;

    }
    catch (const std::exception& e) {
        displayError(e);
        archiveData->OpenResult = E_BAD_ARCHIVE;
        return nullptr;
    }
}

int  STDCALL  MyReadHeader(HANDLE hArcData, tHeaderData *headerData) {
    try{
        auto data = (ArchiveData*)hArcData;
        auto file = data->file;
        data->currentFile++;

        if (data->currentFile == file->getFileCount()) {
            return E_END_ARCHIVE;
        }

        auto fileSize = file->getFileSize(data->currentFile);
        auto fileName = file->getFileName(data->currentFile);

        
        headerData->HostOS = 0;
        headerData->FileCRC = 0;

        headerData->CmtBuf = nullptr;
        headerData->CmtBufSize = 0;
        headerData->CmtSize = 0;
        headerData->CmtState = 0;
        headerData->FileAttr = 0;


        //PackSize, UnpSize, FileTime, and FileName necessarry.
        headerData->PackSize = (int)fileSize;
        headerData->UnpSize = (int)fileSize;
        auto name = fileName;
        
        std::replace(name.begin(), name.end(), '/', '\\');
        if (name[0] == '\\') {
            name = name.substr(1);
        }

        strcpy_s(headerData->FileName, sizeof(headerData->FileName), name.c_str());

        auto year = 0; // recentTime.year;
        auto month = 0; // recentTime.month;
        auto day = 0; // recentTime.day;
        auto hour = 0; // recentTime.hour;
        auto minute = 0; // recentTime.minute;
        auto second = 0; // recentTime.second;
        headerData->FileTime = (year - 1980) << 25 | month << 21 | day << 16 | hour << 11 | minute << 5 | second / 2;;

        // ?
        //char ArcName[260];
        //int Flags;
        //int UnpVer;
        //int Method;

        return 0;
    }
    catch (const std::exception& e) {
        displayError(e);
        return E_EREAD;
    }
}


int  STDCALL  MyProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName) {
    //MessageBoxA(null, "MyProcessFil2", "", 0);
    try{
        auto data = (ArchiveData*)hArcData;
        auto file = data->file;

        if (Operation == PK_SKIP) {
            return 0;
        }
        else if (Operation == PK_TEST) {
            return 0;
        }

        std::string fullDstPath;
        if (DestPath) {
            fullDstPath = std::string(DestPath) + "\\" + std::string(DestName);
        }
        else {
            fullDstPath = std::string(DestName);
        }

        file->extractFile(data->currentFile, fullDstPath);
        return 0;
    }
    catch (const std::exception& e) {
        displayError(e);
        return E_EREAD;
    }

}

int  STDCALL  MyCloseArchive(HANDLE hArcData) {
    //MessageBoxA(null, "myCloseArchive", "", 0);
    try{
        auto data = (ArchiveData*)hArcData;
        auto file = data->file;
        file->dispose();
        delete file;
        return 0;
    }
    catch (const std::exception& e) {
        displayError(e);
        return E_EREAD;
    }
}

void STDCALL  SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1) {
    // Call pChangeVolProc1 if we ever need to change to the next archive :P
}
void STDCALL  SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc) {
    // Call pProcessDataProc to derp the herp.
}

int STDCALL  GetPackerCaps() {
    return
        PK_CAPS_NEW |
        PK_CAPS_MODIFY |
        PK_CAPS_MULTIPLE |
        PK_CAPS_DELETE |
        PK_CAPS_SEARCHTEXT |
        PK_CAPS_BY_CONTENT;
}
BOOL STDCALL  CanYouHandleThisFile(char* filename) {
    try{
        return RiotArchiveFile::couldBeRAF(filename);
    }
    catch (const std::exception& e) {
        displayError(e);
        return 0;
    }
}

/*
extern(Windows) void  ConfigurePacker (HWND Parent, HINSTANCE DllInstance);
*/
int  STDCALL  PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags) {
    //_CrtDbgBreak();
    RiotArchiveFile* file = nullptr;
    try {

        if (!RiotArchiveFile::couldBeRAF(PackedFile)) {
            RiotArchiveFile::createEmptyFile(PackedFile);
        }
        file = new RiotArchiveFile(PackedFile);

        std::string subPath;
        if (SubPath) {
            subPath = std::string(SubPath) + "\\";
        }
        else {
            subPath = "";
        }

        do {
            std::string targetName = AddList ? AddList : "";
            std::string sourcePath = SrcPath ? SrcPath : "";

            AddList += targetName.length() + 1;
            //SrcPath += sourcePath.length() + 1;

            std::string finalPath = (subPath + targetName);
            std::replace(finalPath.begin(), finalPath.end(), '\\', '/');
            auto isDirectory = finalPath[finalPath.size() - 1] == '/';
            if (isDirectory) {
                continue;
            }

            //displayError([sourceName, finalPath]);
            file->addFile(finalPath, sourcePath + targetName);
        } while (*AddList);
        //displayError(extras, "Files to remove");
        file->apply();
    }
    catch (const std::exception& e) {
        displayError(e);
        if (file) {
            file->dispose();
            delete file;
        }
        return E_BAD_DATA;
    }
    file->dispose();
    delete file;

    return 0;
}

int  STDCALL  DeleteFiles(char *PackedFile, char *DeleteList) {
    //_CrtDbgBreak();
    RiotArchiveFile* file = nullptr;
    try {
        file = new RiotArchiveFile(PackedFile);

        std::vector<std::string> skipped;
        std::vector<std::string> extras;
        do {
            std::string name = DeleteList ? DeleteList : "<ugh bug!>";
            DeleteList += name.length() + 1;
            //displayError(name, "Files to remove:");
            if (name.length() > 3 && name.substr(name.size()-3) == "*.*") {
                auto start = name.substr(0, name.size() - 3);
                std::transform(start.begin(), start.end(), start.begin(), tolower);
                std::replace(start.begin(), start.end(), '\\', '/');

                for (unsigned int fileIdx = 0; fileIdx < file->getFileCount(); fileIdx++) {
                    auto fileName = file->getFileName(fileIdx);
                    auto itemName = fileName;
                    std::transform(itemName.begin(), itemName.end(), itemName.begin(), tolower);
                    std::replace(itemName.begin(), itemName.end(), '\\', '/');

                    if (itemName.find(start) == 0) {
                        extras.push_back(fileName);
                    }
                }
            }
            else {
                if (!file->hasFile(name)) {
                    std::replace(name.begin(), name.end(), '/', '\\');
                    if (!file->hasFile(name)) {
                        skipped.push_back(name);
                        continue;
                    }
                }
                file->removeFile(name);
            }
        } while (*DeleteList);
        //displayError(extras, "Files to remove");
        for (const auto& extra : extras) {
            file->removeFile(extra);
        }
        file->apply();
        if (skipped.size()) {
            //displayError(skipped, "Files not found in archive");
            std::string a = "";
            for (const auto& asd : skipped) {
                a += asd + "\n";
            }
            MessageBoxA(nullptr, a.c_str(), "Files not found in archive", MB_ICONWARNING);
        }
    }
    catch (const std::exception& e) {
        displayError(e);
        if (file) {
            file->dispose();
            delete file;
        }
        return E_BAD_DATA;
    }
    file->dispose();
    delete file;
    return 0;
}
