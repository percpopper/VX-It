#include <UnrealEngine/UE.h>

void FNameEntry::GetAnsiName(ANSICHAR(&Out)[NAME_SIZE]) const
{
	if (!IsWide()) {
		CopyUnterminatedName(Out);
		Out[Header.Len] = '\0';
	}
	else {
		printf("[FAIL] GetAnsiName was called while the name is wide.\n");
	}
}

void FNameEntry::GetWideName(WIDECHAR(&Out)[NAME_SIZE]) const
{
	if (IsWide()) {
		CopyUnterminatedName(Out);
		Out[Header.Len] = '\0';
	}
	else {
		printf("[FAIL] GetWideName was called while the name is not wide.\n");
	}
}

void DecryptWide(FNameEntry* Entry, wchar_t* OutName)
{
	wchar_t* v2; // rdi
	int NameLength; // ebx
	__int64 result; // rax
	int v6; // r8d
	unsigned int v7; // ecx
	char v8; // dl

	v2 = OutName;

	NameLength = Entry->GetNameLength();

	int32 CachedLength = NameLength;

	memcpy(OutName, (void*)(reinterpret_cast<uint64>(Entry) + Entry->GetDataOffset()), NameLength * 2);

	static uint64 BaseAddress = 0;
	if (!BaseAddress) BaseAddress = reinterpret_cast<uint64>(GetModuleHandleA(NULL));

	unsigned __int8 Key = *(unsigned __int8*)(BaseAddress + 0x62706D0);

	if (NameLength)
	{
		do
		{
			*v2++ ^= Key;
			--NameLength;
		} while (NameLength);
	}

	OutName[CachedLength] = '\0';
}


void DecryptNonWide(FNameEntry* Entry, char* OutName)
{
	char* v2; // rdi
	int NameLength; // ebx
	__int64 result; // rax
	int v6; // r8d
	unsigned int v7; // ecx
	char v8; // dl

	v2 = OutName;

	NameLength = Entry->GetNameLength();

	int32 CachedLength = NameLength;

	memcpy(OutName, (void*)(reinterpret_cast<uint64>(Entry) + Entry->GetDataOffset()), NameLength);

	static uint64 BaseAddress = 0;
	if (!BaseAddress) BaseAddress = reinterpret_cast<uint64>(GetModuleHandleA(NULL));

	unsigned __int8 Key = *(unsigned __int8*)(BaseAddress + 0x62706D0);

	if (NameLength)
	{
		do
		{
			*v2++ ^= Key;
			--NameLength;
		} while (NameLength);
	}

	OutName[CachedLength] = '\0';
}


std::string FNameEntry::String()
{
	if (IsWide()) 
	{
		wchar_t OutName[1024];
		DecryptWide(this, OutName);
		std::wstring Wide(OutName, Header.Len);
		return std::string(Wide.begin(), Wide.end());
	}

	char OutName[1024];
	DecryptNonWide(this, OutName);

	return std::string(OutName, Header.Len);
}

std::string FName::GetName()
{
	FNameEntry Entry = NamePoolData->Entries.Resolve(GetDisplayIndex());

	std::string Name = Entry.String();

	if (Number > 0)
	{
		/** Not the first instance of this name so add instance number. */
		Name += '_' + std::to_string(Number);
	}

	/** Find the last '/' and start the string from there + 1 so the name doesnt contain extra length and info and look ugly. */
	std::size_t Pos = Name.rfind('/');

	if (Pos != std::string::npos) Name = Name.substr(Pos + 1);

	return Name;
}

const FNameEntry* FName::GetDisplayNameEntry() const
{
	return &NamePoolData->Entries.Resolve(GetDisplayIndex());
}

int32 FNameEntry::GetSize(int32 Length, bool bIsPureAnsi)
{
	int32 Bytes = GetDataOffset() + Length * (bIsPureAnsi ? sizeof(ANSICHAR) : sizeof(WIDECHAR));
	return Align(Bytes, alignof(FNameEntry));
}

void FNameEntryAllocator::LogBlock(uint8* It, uint32 BlockSize, Logger& LOG)
{
	uint8* End = It + BlockSize - FNameEntry::GetDataOffset();
	while (It < End)
	{
		FNameEntry* Entry = reinterpret_cast<FNameEntry*>(It);
		if (uint32 Len = Entry->GetEntryHeader().Len)
		{
			LOG.LogToFile(Entry->String());
			It += FNameEntry::GetSize(Len, !Entry->IsWide());
		}
		else
		{
			break;
		}
	}
}

void FNameEntryAllocator::Log()
{
	Logger LOG("\\FNamePool_Entries_Log.txt");

	printf("[INFO] Logging FNamePool->Entries...\n\n");

	for (uint32 BlockIdx = 0; BlockIdx < CurrentBlock; ++BlockIdx)
	{
		LogBlock(Blocks[BlockIdx], BlockSizeBytes, LOG);
	}

	LogBlock(Blocks[CurrentBlock], CurrentByteCursor, LOG);

	printf("[SUCCESS] FNamePool->Entries log has completed.\n\n");

	LOG.CloseLog();
}