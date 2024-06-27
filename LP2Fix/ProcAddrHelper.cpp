#include "stdafx.h"
#include <sstream>
#include <vector>
#include <windows.h>
#include <string>
#include <winsock2.h>
#include <algorithm>
#include <DbgHelp.h>
#include <Psapi.h>
using namespace std;

// Use only with SEC_COMMIT mappings, not SEC_IMAGE! (in that case, just do VA = base + rva...)
ULONG64 ModRvaToOffset(ULONG64 base, PIMAGE_NT_HEADERS ntHeaders, ULONG64 rva)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i)
    {
        if (rva >= section->VirtualAddress &&
            rva < section->VirtualAddress + section->SizeOfRawData)
        {
            //ASSERT_TRUE(rva != 0); // Following garbage in is garbage out, RVA 0 should always yield VA 0
            return base + (rva - section->VirtualAddress) + section->PointerToRawData;
        }
        section++;
    }
    return 0;
}

void ReadExportDirectory(MODINFO& Info, ULONG_PTR FileMapVA)
{
    // Get the export directory and its size
    ULONG exportDirSize = 0;
    PIMAGE_EXPORT_DIRECTORY exportDir = static_cast<PIMAGE_EXPORT_DIRECTORY>(
        ImageDirectoryEntryToData((PVOID)FileMapVA, Info.isVirtual, IMAGE_DIRECTORY_ENTRY_EXPORT, &exportDirSize));
    /*ULONG exportDirSize;
    auto exportDir = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData((PVOID)FileMapVA,
        Info.isVirtual,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &exportDirSize);*/
    if (exportDirSize == 0)
        return;
    if (exportDir == nullptr)
        return;
    if ((ULONG_PTR)exportDir + exportDirSize > FileMapVA + Info.loadedSize) // Check if exportDir fits into the mapped area
        return;
    if (exportDir->NumberOfNames == 0)
        return;
    if ((ULONG_PTR)exportDir + exportDirSize < (ULONG_PTR)exportDir) // Check for ULONG_PTR wraparound (e.g. when exportDirSize == 0xfffff000)
        return;
    if (exportDir->NumberOfFunctions == 0)
        return;
    DWORD64 totalFunctionSize = exportDir->NumberOfFunctions * sizeof(ULONG_PTR);
    if (totalFunctionSize / exportDir->NumberOfFunctions != sizeof(ULONG_PTR) || // Check for overflow
        totalFunctionSize > Info.loadedSize) // Check for impossible number of exports
        return;

    auto rva2offset = [&Info](ULONG64 rva)
        {
            return Info.isVirtual ? rva : ModRvaToOffset(0, Info.headers, rva);
        };

    auto addressOfFunctionsOffset = rva2offset(exportDir->AddressOfFunctions);
    if (!addressOfFunctionsOffset)
        return;

    auto addressOfFunctions = PDWORD(addressOfFunctionsOffset + FileMapVA);

    auto addressOfNamesOffset = rva2offset(exportDir->AddressOfNames);
    auto addressOfNames = PDWORD(addressOfNamesOffset ? addressOfNamesOffset + FileMapVA : 0);

    auto addressOfNameOrdinalsOffset = rva2offset(exportDir->AddressOfNameOrdinals);
    auto addressOfNameOrdinals = PWORD(addressOfNameOrdinalsOffset ? addressOfNameOrdinalsOffset + FileMapVA : 0);

    // Do not reserve memory based on untrusted input
    //Info.exports.reserve(exportDir->NumberOfFunctions);
    Info.exportOrdinalBase = exportDir->Base;

    // TODO: 'invalid address' below means an RVA that is obviously invalid, like being greater than SizeOfImage.
    // In that case rva2offset will return a VA of 0 and we can ignore it. However the ntdll loader (and this code)
    // will still crash on corrupt or malicious inputs that are seemingly valid. Find out how common this is
    // (i.e. does it warrant wrapping everything in try/except?) and whether there are better solutions.
    // Note that we're loading this file because the debuggee did; that makes it at least somewhat plausible that we will also survive
    for (DWORD i = 0; i < exportDir->NumberOfFunctions; i++)
    {
        // Check if addressOfFunctions[i] is valid
        ULONG_PTR target = (ULONG_PTR)addressOfFunctions + i * sizeof(DWORD);
        if (target > FileMapVA + Info.loadedSize || target < (ULONG_PTR)addressOfFunctions)
        {
            continue;
        }

        // It is possible the AddressOfFunctions contain zero RVAs. GetProcAddress for these ordinals returns zero.
        // "The reason for it is to assign a particular ordinal to a function." - NTCore
        if (!addressOfFunctions[i])
            continue;

        Info.exports.emplace_back();
        auto& entry = Info.exports.back();
        entry.ordinal = i + exportDir->Base;
        entry.rva = addressOfFunctions[i];
        const auto entryVa = Info.isVirtual ? entry.rva : ModRvaToOffset(FileMapVA, Info.headers, entry.rva);
        entry.forwarded = entryVa >= (ULONG64)exportDir && entryVa < (ULONG64)exportDir + exportDirSize;
        if (entry.forwarded)
        {
            auto forwardNameOffset = rva2offset(entry.rva);
            if (forwardNameOffset) // Silent ignore (1) by ntdll loader: invalid forward names or addresses of forward names
                entry.forwardName = std::string((const char*)(forwardNameOffset + FileMapVA));
        }
    }

    for (DWORD i = 0; i < exportDir->NumberOfNames; i++)
    {
        // Check if addressOfNameOrdinals[i] is valid
        ULONG_PTR target = (ULONG_PTR)addressOfNameOrdinals + i * sizeof(WORD);
        if (target > FileMapVA + Info.loadedSize || target < (ULONG_PTR)addressOfNameOrdinals)
        {
            continue;
        }

        DWORD index = addressOfNameOrdinals[i];
        if (index < exportDir->NumberOfFunctions) // Silent ignore (2) by ntdll loader: bogus AddressOfNameOrdinals indices
        {
            // Check if addressOfNames[i] is valid
            target = (ULONG_PTR)addressOfNames + i * sizeof(DWORD);
            if (target > FileMapVA + Info.loadedSize || target < (ULONG_PTR)addressOfNames)
            {
                continue;
            }

            auto nameOffset = rva2offset(addressOfNames[i]);
            if (nameOffset) // Silent ignore (3) by ntdll loader: invalid names or addresses of names
            {
                // Info.exports has excluded some invalid exports, so addressOfNameOrdinals[i] is not equal to
                // the index of Info.exports. We need to iterate over Info.exports.
                for (size_t j = 0; j < Info.exports.size(); j++)
                {
                    if (index + exportDir->Base == Info.exports[j].ordinal)
                    {
                        Info.exports[j].name = std::string((const char*)(nameOffset + FileMapVA));
                        //Log("Exported function: " + Info.exports[j].name); // Log the function name
                        break;
                    }
                }
            }
        }
    }

    // give some kind of name to ordinal functions
    for (size_t i = 0; i < Info.exports.size(); i++)
    {
        if (Info.exports[i].name.empty())
            Info.exports[i].name = "Ordinal#" + std::to_string(Info.exports[i].ordinal);
    }

    // prepare sorted vectors
    Info.exportsByName.resize(Info.exports.size());
    Info.exportsByRva.resize(Info.exports.size());
    for (size_t i = 0; i < Info.exports.size(); i++)
    {
        Info.exportsByName[i].index = i;
        Info.exportsByName[i].name = Info.exports[i].name.c_str(); //NOTE: DO NOT MODIFY name is any way!
        Info.exportsByRva[i] = i;
    }

    std::sort(Info.exportsByName.begin(), Info.exportsByName.end());

    std::sort(Info.exportsByRva.begin(), Info.exportsByRva.end(), [&Info](size_t a, size_t b)
        {
            return Info.exports.at(a).rva < Info.exports.at(b).rva;
        });

    // undecorate names
    /*for(auto & x : Info.exports)
    {
        if(!x.name.empty())
        {
            auto demangled = LLVMDemangle(x.name.c_str());
            if(demangled && x.name.compare(demangled) != 0)
                x.undecoratedName = demangled;
            LLVMDemangleFree(demangled);
        }
    }*/
}

void* GetProcAddressManual(HMODULE hModule, LPCSTR lpProcName)
{
    // Get the base address of the module
    ULONG_PTR baseAddress = (ULONG_PTR)hModule;

    // Initialize the MODINFO structure
    MODINFO myModInfo;
    myModInfo.isVirtual = true; // Set to true if you're working with a virtual address space
    myModInfo.headers = PIMAGE_NT_HEADERS(baseAddress + ((PIMAGE_DOS_HEADER)baseAddress)->e_lfanew); // Get the NT headers

    MODULEINFO modInfo = { 0 };
    if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO)))
    {
        myModInfo.loadedSize = modInfo.SizeOfImage;
    }

    // Read the export directory
    ReadExportDirectory(myModInfo, baseAddress);

    // Search for the CreateProcessW function
    for (const auto& exportEntry : myModInfo.exports) {
        //Log(exportEntry.name);
        if (exportEntry.name == std::string(lpProcName)) {
            //Log("Found");
            // The RVA of the function is stored in exportEntry.rva
            // The actual address of the function can be calculated by adding the base address of the module
            //ULONG_PTR functionAddress = baseAddress + exportEntry.rva;

            //Log("Function address: " + hexAddress);

            return reinterpret_cast<void*>(baseAddress + exportEntry.rva);
        }
    }

    return nullptr;
}