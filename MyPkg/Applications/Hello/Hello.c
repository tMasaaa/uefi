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


// -----------------------------------

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
  
  // ---------------------------------
  // mclang.elf
  // ---------------------------------
  EFI_FILE_PROTOCOL *elf_file;
  status = root_dir->Open(
    root_dir,
    &elf_file,
    L"mclang.elf",
    EFI_FILE_MODE_READ,
    0
  );
  Print(L"Open ELF file status: %r\n", status);

  UINT64 elf_file_addr = 0x100000lu; // image base address
  UINTN num_pages = 5; // DYN->5, EXEC->4
  status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &elf_file_addr);
  Print(L"Allocate Memory status: %r\n", status);

  UINTN elf_buf = num_pages * 4096;
  status = elf_file->Read(elf_file, &elf_buf, (VOID*)elf_file_addr);
  Print(L"ELF Read status: %r\n", status); // ok

  // ---------------------------------------------------
  // relocation
  // ---------------------------------------------------

  // UINT64 entry_addr = *(UINT64*)(elf_file_addr + 1);
  // Print(L"elf_file_addr: %lx\n", elf_file_addr); // 100000 -> base address
  // Print(L"entry value: %lx\n", entry_addr); // 101020 ->MyMain

  // for (int i = 0;i<16;i++) {
  //   UINT32 param_addr = *(UINT32*)(elf_file_addr + i);
  //   Print(L"ident: %d ->%lx\n", i, param_addr);
  // }
  // UINT32 entry_addr,sh_offset;
  // entry_addr = *(UINT32*)(elf_file_addr + 16);
  // Print(L"e_type->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 18);
  // Print(L"e_machine->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 20);
  // Print(L"e_version->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 24);
  // Print(L"e_entry->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 32);
  // Print(L"e_phoff->%lx\n", entry_addr);
  // sh_offset = *(UINT32*)(elf_file_addr + 40);
  // Print(L"e_shoff->%lx\n", sh_offset);
  // entry_addr = *(UINT32*)(elf_file_addr + 48);
  // Print(L"e_flags->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 52);
  // Print(L"e_ehsize->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 54);
  // Print(L"e_phentsize->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 56);
  // Print(L"e_phnum->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 58);
  // Print(L"e_shentsize->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 60);
  // Print(L"e_shnum->%lx\n", entry_addr);
  // entry_addr = *(UINT32*)(elf_file_addr + 62);
  // Print(L"e_shstrndx->%lx\n", entry_addr);


  // entry_addr = *(UINT32*)(elf_file_addr + sh_offset);
  // Print(L"section header value->%lx\n", entry_addr);

  // typedef struct {
  //   CHAR8* e_ident[16]; // 1*16
  //   UINT16 e_type; // 16+2
  //   UINT16 e_machine; // 18+2
  //   UINT32 e_version; // 20+4
  //   UINT64 e_entry; // 24+8
  //   UINT64 e_phoff; // 28+8
  //   UINT64 e_shoff; // 32+8
  //   UINT32 e_flags; // 36+4
  //   UINT16 e_ehsize; // 40+2
  //   UINT16 e_phentsize; // 42+2
  //   UINT16 e_phnum; // 44+2
  //   UINT16 e_shentsize; // 46+2
  //   UINT16 e_shnum; // 48+2
  //   UINT16 e_shstrndx; // 50+2
  // } Elf64_Ehdr;

  // CHAR8* e_ident[16] = (CHAR8*)(elf_file_addr); // 1*16
  UINT16 e_type = *(UINT16*)(elf_file_addr + 16); // 16+2
  UINT16 e_machine = *(UINT16*)(elf_file_addr + 18); // 18+2
  UINT32 e_version = *(UINT32*)(elf_file_addr + 20); // 20+4
  UINT64 e_entry = *(UINT64*)(elf_file_addr + 24); // 24+8
  UINT64 e_phoff = *(UINT64*)(elf_file_addr + 28); // 28+8
  UINT64 e_shoff = *(UINT64*)(elf_file_addr + 32); // 32+8
  UINT32 e_flags = *(UINT32*)(elf_file_addr + 36); // 36+4
  UINT16 e_ehsize = *(UINT16*)(elf_file_addr + 40); // 40+2
  UINT16 e_phentsize = *(UINT16*)(elf_file_addr + 42); // 42+2
  UINT16 e_phnum = *(UINT16*)(elf_file_addr + 44); // 44+2
  UINT16 e_shentsize = *(UINT16*)(elf_file_addr + 46); // 46+2
  UINT16 e_shnum = *(UINT16*)(elf_file_addr + 48); // 48+2
  UINT16 e_shstrndx = *(UINT16*)(elf_file_addr + 50); // 50+2
  
  // Print(L"signature -> %s\n", *e_ident);
  Print(L"type -> %d\n", e_type);
  Print(L"machine -> %d\n", e_machine);
  Print(L"version -> %d\n", e_version);
  Print(L"entry -> %lx\n", e_entry);
  Print(L"program header offset -> %lx\n", e_phoff);
  Print(L"section header offset -> %lx\n", e_shoff);
  Print(L"flag(NONUSED) -> %d\n", e_flags);
  Print(L"ELF header bytes -> %d\n", e_ehsize);
  Print(L"program header size -> %d\n", e_phentsize);
  Print(L"number of entries in program header -> %d\n", e_phnum);
  Print(L"section header size -> %d\n", e_shentsize);
  Print(L"number of entries in section header -> %d\n", e_shnum);
  Print(L"section name string table index -> %d\n", e_shstrndx);

  // Print(L"%r\n", elf_file_addr); // base address
  // SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello UEFI!\n");
  // Print(L"elf_file_header? : %r\n", *(UINT64*)(elf_file_addr + 0));
  // for (int i = 1;i<10;++i) {
  //   SystemTable->ConOut->OutputString(SystemTable->ConOut, (VOID *)(elf_file_addr + i));
  //   // Print(L"NUM: %d -> %lu\n", i, *(UINT64*)(elf_file_addr + i));
  // }

  // ---------------------------------------------------

  // UINT64 entry_addr = *(UINT64*)(elf_file_addr + 24);

  // typedef int EntryPointType(void*);
  // EntryPointType* entry_point = (EntryPointType*)entry_addr;
  // int exit_code = entry_point(Print);

  // Print(L"Exit status: %d\n", exit_code);

  // ---------------------------------
  // memory map
  // ---------------------------------

  // // open file protocol 
  // EFI_FILE_PROTOCOL *memmap_file;
  // status = root_dir->Open(
  //   root_dir,
  //   &memmap_file,
  //   L"memmap",
  //   // EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
  //   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
  //   0
  // );
  // Print(L"Open File memmap status: %r\n", status);

  // // struct MemoryMap memmap = {
  // //   map_size,
  // //   (VOID *)map,
  // //   map_size,
  // //   map_key,
  // //   descriptor_size,
  // //   descriptor_version
  // // };


  // UINTN buf_size = 4096;
  // CHAR16 file_buf[4096] = L"Index, Type, PhysicalStart, VirtualStart, NumberofPages, Attribute\n"; // this is write object
  // // struct file_buf[4096];
  // status = memmap_file->Write(memmap_file, &buf_size, (void *)file_buf);
  // Print(L"Write File status: %r\n", status);
  
  // // Print(L"Start Iter: %r\n", (EFI_PHYSICAL_ADDRESS)map);
  // // Print(L"End Iter: %r\n", (EFI_PHYSICAL_ADDRESS)map+map_size);
  // // Print(L"Iter: %r\n", descriptor_size);
  // // CHAR8 body[256];
  // // AsciiSPrint(
  // //   body,
  // //   sizeof(body), 
  // //   "%lx, %lx, %lx, %lx, %lx\n",
  // //   map_size,
  // //   map,
  // //   map_key,
  // //   descriptor_size,
  // //   descriptor_version
  // // );

  // CHAR8 body[256];
  // EFI_PHYSICAL_ADDRESS iter;
  // int i;
  // UINTN len;
  // for (
  //   iter = (EFI_PHYSICAL_ADDRESS)map, i = 0;
  //   iter < (EFI_PHYSICAL_ADDRESS)map + map_size;
  //   iter += descriptor_size, i++
  // ) {
  //   EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
  //   len = AsciiSPrint(
  //     body,
  //     sizeof(body), 
  //     "Index: %02u, Type: %x, PhysicalStart: %lx, VirtualStart: %lx, NumberOfPages: %d, Attribute: %lx\n",
  //     i,
  //     desc->Type,
  //     desc->PhysicalStart,
  //     desc->VirtualStart,
  //     desc->NumberOfPages,
  //     desc->Attribute
  //   );
  //   Print(L"ok %u", i);
  //   status = memmap_file->Write(memmap_file, &len, (void *)body);
  // }

  // Print(L"Write File status: %r\n", status);

  // memmap_file->Flush(memmap_file);
  // status = memmap_file->Close(memmap_file);
  // Print(L"Close File status: %r\n", status);

  // // ----Success
  // // UINTN buf_size = 4096;
  // // CHAR8 file_buf[4096];
  // // status = root_dir->Read(
  // //   root_dir,
  // //   &buf_size,
  // //   (void *)file_buf
  // // );
  // // Print(L"File memmap Read status: %r\n", status);
  // // struct EFI_FILE_INFO *file_info;
  // // file_info = (struct EFI_FILE_INFO *)file_buf;

  Print(L"Hello EDK II.\n");
  while (1);
  return 0;
}