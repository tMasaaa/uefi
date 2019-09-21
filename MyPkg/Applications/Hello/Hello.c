#include  <Uefi.h>
#include  <Library/UefiLib.h>
// #include <stdint.h>

#include  <Library/UefiBootServicesTableLib.h> // gBS declaration
#include  <Library/PrintLib.h>
// #include  <Library/MemoryAllocationLib.h>
// #include  <Library/BaseMemoryLib.h>
// #include  <Protocol/LoadedImage.h>
// #include  <Protocol/SimpleFileSystem.h>
// #include  <Protocol/DiskIo2.h>
// #include  <Protocol/BlockIo.h>
// #include  <Guid/FileInfo.h>


// #include <Protocol/Device  Path.h>

// struct MemoryMap {
//   UINTN buffer_size; // map_size
//   VOID *buffer;
//   UINTN map_size;
//   UINTN map_key;
//   UINTN descriptor_size;
//   UINT32 descriptor_version;
// };

// EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
//   EFI_STATUS status;
//   CHAR8 buf[256];
//   UINTN len;

//   CHAR8* header =
//     "Index, Type, PhysicalStart, NumberOfPages, Attribute\n";
//   len = AsciiStrLen(header);
//   status = file->Write(file, &len, header);
//   if (EFI_ERROR(status)) {
//     return status;
//   }

//   Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
//       map->buffer, map->map_size);

//   EFI_PHYSICAL_ADDRESS iter;
//   int i;
//   for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
//        iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
//        iter += map->descriptor_size, i++) {
//     EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
//     len = AsciiSPrint(
//         buf, sizeof(buf),
//         "%u, %x, %08lx, %lx, %lx\n",
//         i, desc->Type,
//         desc->PhysicalStart, desc->NumberOfPages,
//         desc->Attribute & 0xffffflu);
//     status = file->Write(file, &len, buf);
//     if (EFI_ERROR(status)) {
//       return status;
//     }
//   }

//   return EFI_SUCCESS;
// }

EFI_STATUS
EFIAPI
UefiMain (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
  // CHAR8 memmap_buf[4096 * 4]; // ???
  EFI_STATUS status;
  // struct MemoryMap memmap = { 4096, NULL, 4096, 0, 0, 0 };
  // status = GetMemoryMap(&memmap);
  
  // GetMemoryMap
  EFI_MEMORY_DESCRIPTOR *map = NULL;
  UINTN map_size = 0;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
  
  status = gBS->GetMemoryMap(
    &map_size,
    map, // map->buffer
    &map_key,
    &descriptor_size, 
    &descriptor_version
  );
  if (status == EFI_BUFFER_TOO_SMALL) {
    if (map) {
      gBS->FreePool(map);
    }
    map_size = 4096;
    status = gBS->AllocatePool(EfiLoaderData, map_size, (void **)&map); // http://yuma.ohgami.jp/UEFI-Bare-Metal-Programming-2/05_other_bs_rs.html
    status = gBS->GetMemoryMap(&map_size, map, &map_key, &descriptor_size, &descriptor_version);
  }
  Print(L"GetMemoryMap status: %r\n", status);
  Print(L"Memory: %r\n", map_size);

  // SaveMamoryMap to file
  // open root directory
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
  EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
  status = gBS->LocateProtocol(&fs_guid, NULL, (void **)&fs);
  Print(L"Simple file system protocol status: %r\n", status);

  EFI_FILE_PROTOCOL *root_dir;
  status = fs->OpenVolume(fs, &root_dir);
  Print(L"Root directory status: %r\n", status);

  // open file protocol 
  EFI_FILE_PROTOCOL *memmap_file;
  status = root_dir->Open(
    root_dir,
    &memmap_file,
    L"memmap",
    // EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
    0
  );
  Print(L"Open File memmap status: %r\n", status);

  // struct MemoryMap memmap = {
  //   map_size,
  //   (VOID *)map,
  //   map_size,
  //   map_key,
  //   descriptor_size,
  //   descriptor_version
  // };


  UINTN buf_size = 4096;
  CHAR16 file_buf[4096] = L"Index, Type, PhysicalStart, VirtualStart, NumberofPages, Attribute\n"; // this is write object
  // struct file_buf[4096];
  status = memmap_file->Write(memmap_file, &buf_size, (void *)file_buf);
  Print(L"Write File status: %r\n", status);
  
  // Print(L"Start Iter: %r\n", (EFI_PHYSICAL_ADDRESS)map);
  // Print(L"End Iter: %r\n", (EFI_PHYSICAL_ADDRESS)map+map_size);
  // Print(L"Iter: %r\n", descriptor_size);
  // CHAR8 body[256];
  // AsciiSPrint(
  //   body,
  //   sizeof(body), 
  //   "%lx, %lx, %lx, %lx, %lx\n",
  //   map_size,
  //   map,
  //   map_key,
  //   descriptor_size,
  //   descriptor_version
  // );

  CHAR8 body[256];
  EFI_PHYSICAL_ADDRESS iter;
  int i;
  UINTN len;
  for (
    iter = (EFI_PHYSICAL_ADDRESS)map, i = 0;
    iter < (EFI_PHYSICAL_ADDRESS)map + map_size;
    iter += descriptor_size, i++
  ) {
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
    len = AsciiSPrint(
      body,
      sizeof(body), 
      "Index: %02u, Type: %x, PhysicalStart: %lx, VirtualStart: %lx, NumberOfPages: %d, Attribute: %lx\n",
      i,
      desc->Type,
      desc->PhysicalStart,
      desc->VirtualStart,
      desc->NumberOfPages,
      desc->Attribute
    );
    Print(L"ok %u", i);
    status = memmap_file->Write(memmap_file, &len, (void *)body);
  }

  Print(L"Write File status: %r\n", status);

  memmap_file->Flush(memmap_file);
  status = memmap_file->Close(memmap_file);
  Print(L"Close File status: %r\n", status);

  // ----Success
  // UINTN buf_size = 4096;
  // CHAR8 file_buf[4096];
  // status = root_dir->Read(
  //   root_dir,
  //   &buf_size,
  //   (void *)file_buf
  // );
  // Print(L"File memmap Read status: %r\n", status);
  // struct EFI_FILE_INFO *file_info;
  // file_info = (struct EFI_FILE_INFO *)file_buf;

  Print(L"Hello EDK II.\n");
  while (1);
  return 0;
}