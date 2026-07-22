#pragma once
typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef long long INT64;
typedef UINT64 UINTN;
typedef UINT16 CHAR16;
typedef void* EFI_HANDLE;
typedef UINT64 EFI_STATUS;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;
#define EFI_SUCCESS 0ULL
#define EFI_ERROR(x) ((x) != EFI_SUCCESS)
#define IN
#define OUT
#define OPTIONAL
#define EFIAPI __attribute__((ms_abi))
typedef struct { UINT32 Data1; UINT16 Data2; UINT16 Data3; UINT8 Data4[8]; } EFI_GUID;
typedef struct _EFI_SYSTEM_TABLE {
    UINT64 Hdr[2]; CHAR16* FirmwareVendor; UINT32 FirmwareRevision; UINT32 _pad;
    EFI_HANDLE ConsoleInHandle; void* ConIn; EFI_HANDLE ConsoleOutHandle; void* ConOut;
    EFI_HANDLE StandardErrorHandle; void* StdErr; void* RuntimeServices; void* BootServices;
} EFI_SYSTEM_TABLE;
EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable);
