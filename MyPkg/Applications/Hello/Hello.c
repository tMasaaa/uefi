#include  <Uefi.h>
#include  <Library/UefiLib.h>

#include  <Library/UefiBootServicesTableLib.h> // gBS declaration
#include  <Library/PrintLib.h> // Print

EFI_STATUS
EFIAPI
UefiMain (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
  EFI_STATUS status;
  
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

  Print(L"Hello EDK II.\n");
  while (1);
  return 0;
}