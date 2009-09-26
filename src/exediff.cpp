/**@file exediff.cpp -- diff exe/dll files.
 * $Id: exediff.cpp,v 1.11 2004/06/30 06:59:44 hkuno Exp $
 * @author Hiroshi Kuno <hkuno-exediff-tool@micorhouse.co.jp>
 */
#include <windows.h>
#include <imagehlp.h>
#pragma comment(lib, "imagehlp.lib")
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mbstring.h>
#include <io.h>
#include <locale.h>
#include <time.h>
//using namespace std;

//------------------------------------------------------------------------
// 型、定数、グローバル変数の定義
//........................................................................
// typedef and constants
/** 無符号文字型の別名 */
typedef unsigned char uchar;

/** 無符号long型の別名 */
typedef unsigned long ulong;

//........................................................................
// global variables

/** -t: ignore time stamp */
bool gIgnoreTimeStamp = false;

/** -d: dump file image */
bool gDumpFileImage = false;

/** -q: quiet mode */
bool gQuiet = false;

/** -n#: max length of differ rawdatas */
size_t gDiffLength = 4;

/** directory diff mode */
bool gDirDiff = false;

//........................................................................
// messages
/** short help-message */
const char* gUsage  = "usage :exediff [-h?tdq][-n#] (FILE1 FILE2 | DIR1 DIR2 [WILD] | DIR1 DIR2\\WILD)\n";

/** detail help-message for options and version */
const char* gUsage2 =
	"  $Revision: 1.11 $\n"
	"  -h -?   this help\n"
	"  -t      ignore time stamp\n"
	"  -d      dump file image\n"
	"  -q      quiet mode\n"
	"  -n#     max length of differ rawdatas. default is 4\n"
	"  FILE1/2 compare exe/dll file\n"
	"  DIR1/2  compare folder\n"
	"  WILD    compare files pattern in DIR2. default is *\n"
	;

//------------------------------------------------------------------------
///@name 汎用エラー処理関数
//@{
/** usageとエラーメッセージを表示後に、exitする */
void error_abort(const char* msg = NULL)
{
	fputs(gUsage, stderr);
	if (msg)
		fputs(msg, stderr);
	exit(EXIT_FAILURE);
}

void error_abort(const char* prompt, const char* arg)
{
	fputs(gUsage, stderr);
	fprintf(stderr, "%s: %s\n", prompt, arg);
	exit(EXIT_FAILURE);
}

void errorf_abort(const char* fmt, ...)
{
	fputs(gUsage, stderr);
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

/** エラーメッセージと、Win32の詳細エラー情報を表示する */
void print_win32error(const char* msg)
{
	DWORD win32error = ::GetLastError();
	char buf[1000];
	::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, win32error, 0, buf, sizeof(buf), NULL);
	fprintf(stderr, "%s: Win32Error(%d) %s", msg, win32error, buf);
}
//@}

//------------------------------------------------------------------------
///@name 汎用文字列関数群
//@{
/** s1とs2は等しいか? */
inline bool strequ(const char* s1, const char* s2)
{
	return strcmp(s1, s2) == 0;
}

inline bool strequ(const uchar* s1, const uchar* s2)
{
	return strequ((const char*)s1, (const char*)s2);
}

/** 大文字小文字を無視すれば, s1とs2は等しいか? */
inline bool striequ(const char* s1, const char* s2)
{
	return _mbsicmp((const uchar*)s1, (const uchar*)s2) == 0;
}

/** s1はs2より小さいか? */
inline bool strless(const char* s1, const char* s2)
{
	return _mbscmp((const uchar*)s1, (const uchar*)s2) < 0;
}

/** 大文字小文字を無視すれば, s1はs2より小さいか? */
inline bool striless(const char* s1, const char* s2)
{
	return _mbsicmp((const uchar*)s1, (const uchar*)s2) < 0;
}
//@}

//------------------------------------------------------------------------
///@name ファイル名操作関数群
//@{
/** ワイルドカード記号を持っているか? */
inline bool has_wildcard(const char* pathname)
{
	return strpbrk(pathname, "*?") != NULL;
}

/** パス名を、フォルダ名とファイル名に分離する.
 * @param pathname	解析するパス名.
 * @param folder	分離したフォルダ名の格納先(不要ならNULL可). e.g. "a:\dir\dir\"
 * @param name		分離したファイル名の格納先(不要ならNULL可). e.g. "sample.cpp"
 */
void separate_pathname(const char* pathname, char* folder, char* name)
{
		if (strlen(pathname) >= _MAX_PATH)
			error_abort("too long pathname", pathname);
		char drv[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char base[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath(pathname, drv, dir, base, ext);
		if (folder != NULL)
			_makepath(folder, drv, dir, NULL, NULL);
		if (name != NULL)
			_makepath(name, NULL, NULL, base, ext);
}
//@}

//------------------------------------------------------------------------
/** ファイル検索クラス.
 * _findfirst/next処理のラッパークラスです.
 */
class FindFile : public _finddata_t {
	long mHandle;
public:
	//....................................................................
	/** コンストラクタ. */
	FindFile() : mHandle(-1) {}

	/** デストラクタ. 検索途中ならClose()を行う. */
	~FindFile() {
		if (mHandle != -1)
			Close();
	}

	//....................................................................
	//@{
	/** 検索開始. */
	void Open(const char* dir, const char* wild);
	void Open(const char* pathname);
	//@}

	//@{
	/** 次検索. 終端に達したら自動的にClose()を行う. */
	void Next() {
		if (_findnext(mHandle, this) != 0)
			Close();
	}
	void operator++() {
		Next();
	}
	//@}

	//@{
	/// 検索終了.
	void Close() {
		_findclose(mHandle); mHandle = -1;
	}
	//@}

	//....................................................................
	/** 検索中? */
	bool Opened() const {
		return mHandle != -1;
	}
	/** 検索中? */
	operator const void*() const {
		return Opened() ? this : NULL;
	}
	/** 検索終了? */
	bool operator!() const {
		return !Opened();
	}

	//....................................................................
	/** 検索ファイル名取得. */
	const char* Name() const {
		return name;
	}
	/** フォルダか? */
	bool IsFolder() const {
		return (attrib & _A_SUBDIR) != 0;
	}
	/** 相対フォルダ("." or "..")か? */
	bool IsDotFolder() const;
};

//........................................................................
void FindFile::Open(const char* dir, const char* wild)
{
	if (strlen(dir) >= _MAX_PATH)
		error_abort("too long folder name", dir);
	if (strlen(wild) >= _MAX_FNAME)
		error_abort("too long file pattern", wild);

	char path[_MAX_PATH + _MAX_FNAME + 10]; // パス区切り記号の追加に備えて +10の余裕をとる.
	_makepath(path, NULL, dir, wild, NULL);
	Open(path);
}

//........................................................................
void FindFile::Open(const char* pathname)
{
	mHandle = _findfirst(pathname, this);
}

//........................................................................
bool FindFile::IsDotFolder() const
{
	// "." or ".."?
	return IsFolder() && name[0] == '.'
		&& (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

//------------------------------------------------------------------------
/** 存在するフォルダであることを保証する. もし問題があれば終了する. */
void ValidateFolder(const char* dir)
{
	DWORD attr = ::GetFileAttributes(dir);
	if (attr == -1) {
		print_win32error(dir);
		error_abort();
	}
	if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		error_abort("not a folder", dir);
	}
}

/** 存在するフォルダであることを確認する. */
bool IsExistFolder(const char* dir)
{
	DWORD attr = ::GetFileAttributes(dir);
	return (attr != -1) && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

/** 存在するファイルであることを確認する. */
bool IsExistFile(const char* fname)
{
	DWORD attr = ::GetFileAttributes(fname);
	return (attr != -1) && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

//------------------------------------------------------------------------
/** PE format file image */
class ExeFileImage : public LOADED_IMAGE {
	BOOL mLoaded;
	ExeFileImage(const ExeFileImage&);		// don't copy
	void operator=(const ExeFileImage&);	// don't assign
public:
	ExeFileImage(const char* fname);

	~ExeFileImage();

	void print() const;

	bool IsLoaded() const {
		return mLoaded == TRUE;
	}
};

ExeFileImage::ExeFileImage(const char* fname)
{
	mLoaded = ::MapAndLoad(const_cast<char*>(fname), ".", this, TRUE, TRUE);
}

ExeFileImage::~ExeFileImage()
{
	if (mLoaded)
		::UnMapAndLoad(this);
}

/** 印刷可能文字を返す. 印刷不可能文字に対しては'.'を返す */
inline int ascii(int c)
{
	return (!iscntrl(c) && isprint(c)) ? c : '.';
}

const char* ImageCharacteristicsString(WORD flags, char* buf=NULL)
{
	static char mybuf[20*16];
	if (!buf) buf = mybuf;
	sprintf(buf, "%04X(", flags);

	#define M(f, id)	if (flags & f) strcat(buf, id ", ")
	M(IMAGE_FILE_RELOCS_STRIPPED, "relocations stripped");		// Relocation information is stripped from the file. 
	M(IMAGE_FILE_EXECUTABLE_IMAGE, "executable image");			// The file is executable (there are no unresolved external references). 
	M(IMAGE_FILE_LINE_NUMS_STRIPPED, "linenumbers stripped");	// Line numbers are stripped from the file. 
	M(IMAGE_FILE_LOCAL_SYMS_STRIPPED, "symbols stripped");		// Local symbols are stripped from file. 
	M(IMAGE_FILE_AGGRESIVE_WS_TRIM, "AGGRESIVE_WS_TRIM");		// Aggressively trim the working set. 
	M(IMAGE_FILE_LARGE_ADDRESS_AWARE, "LARGE_ADDRESS_AWARE");	// The application can handle addresses larger than 2 GB. 
	M(IMAGE_FILE_BYTES_REVERSED_LO, "BYTES_REVERSED_LO");		// Bytes of the word are reversed. 
	M(IMAGE_FILE_32BIT_MACHINE,  "32BIT_MACHINE");				// Computer supports 32-bit words. 
	M(IMAGE_FILE_DEBUG_STRIPPED, "debuginfo stripped");			// Debugging information is stored separately in a .dbg file. 
	M(IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, "REMOVABLE_RUN_FROM_SWAP");// If the image is on removable media, copy and run from the swap file. 
	M(IMAGE_FILE_NET_RUN_FROM_SWAP, "NET_RUN_FROM_SWAP");		// If the image is on the network, copy and run from the swap file. 
	M(IMAGE_FILE_SYSTEM, "system file");						// SYSTEM System file. 
	M(IMAGE_FILE_DLL, "DLL file");								//  DLL file. 
	M(IMAGE_FILE_UP_SYSTEM_ONLY, "UP_SYSTEM_ONLY");				// File should be run only on a uniprocessor computer. 
	M(IMAGE_FILE_BYTES_REVERSED_HI, "BYTES_REVERSED_HI");		// Bytes of the word are reversed. 
	#undef M

	char* last_comma = strrchr(buf, ',');
	if (last_comma) *last_comma = '\0';
	strcat(buf, ")");

	return buf;
}

const char* SectionCharacteristicsString(DWORD flags, char* buf=NULL)
{
	static char mybuf[20*32];
	if (!buf) buf = mybuf;
	sprintf(buf, "%08X(", flags);

	#define M(f, id)	if (flags & f) strcat(buf, id ", ")
//	M(IMAGE_SCN_TYPE_REG,    "*reg");
//	M(IMAGE_SCN_TYPE_DSECT,  "*dsect");
//	M(IMAGE_SCN_TYPE_NOLOAD, "*noload");
//	M(IMAGE_SCN_TYPE_GROUP,  "*group");
	M(IMAGE_SCN_TYPE_NO_PAD, "*no pad");
//	M(IMAGE_SCN_TYPE_COPY,   "*copy");

	M(IMAGE_SCN_CNT_CODE, "code");
	M(IMAGE_SCN_CNT_INITIALIZED_DATA, "data");
	M(IMAGE_SCN_CNT_UNINITIALIZED_DATA, "bss");

//	M(IMAGE_SCN_LNK_OTHER, "*other");
	M(IMAGE_SCN_LNK_INFO,  "*info");
//	M(IMAGE_SCN_TYPE_OVER, "*over");
	M(IMAGE_SCN_LNK_COMDAT, "comdat");

	M(IMAGE_SCN_MEM_FARDATA,   "*fardata");
	M(IMAGE_SCN_MEM_PURGEABLE, "*purgeable");
	M(IMAGE_SCN_MEM_16BIT,     "*16bit");
	M(IMAGE_SCN_MEM_LOCKED,    "*locked");
	M(IMAGE_SCN_MEM_PRELOAD,   "*preload");

	M(IMAGE_SCN_ALIGN_1BYTES,   "1-byte align");
	M(IMAGE_SCN_ALIGN_2BYTES,   "2-byte align");
	M(IMAGE_SCN_ALIGN_4BYTES,   "4-byte align");
	M(IMAGE_SCN_ALIGN_8BYTES,   "8-byte align");
	M(IMAGE_SCN_ALIGN_16BYTES, "16-byte align");
	M(IMAGE_SCN_ALIGN_32BYTES, "32-byte align");
	M(IMAGE_SCN_ALIGN_64BYTES, "64-byte align");

	M(IMAGE_SCN_LNK_NRELOC_OVFL, "extended relocations");
	M(IMAGE_SCN_MEM_DISCARDABLE, "discardable");
	M(IMAGE_SCN_MEM_NOT_CACHED, "cannot be cached");
	M(IMAGE_SCN_MEM_NOT_PAGED, "cannot be paged");
	M(IMAGE_SCN_MEM_SHARED,  "shared");
	M(IMAGE_SCN_MEM_EXECUTE, "execute");
	M(IMAGE_SCN_MEM_READ,    "read");
	M(IMAGE_SCN_MEM_WRITE,   "write");
	#undef M

	char* last_comma = strrchr(buf, ',');
	if (last_comma) *last_comma = '\0';
	strcat(buf, ")");

	return buf;
}

const char* SubsystemString(WORD value, char* buf=NULL)
{
	static char mybuf[100];
	if (!buf) buf = mybuf;

	const char* id = "?";
	switch (value) {
	#define C(v, name)	case v: id = name
	C(IMAGE_SUBSYSTEM_UNKNOWN,    "unknown");       break;	// Unknown subsystem.
	C(IMAGE_SUBSYSTEM_NATIVE,     "native");        break;	// No subsystem required. 
	C(IMAGE_SUBSYSTEM_WINDOWS_CUI,"WIN32 console"); break;	// Windows character-mode subsystem. 
	C(IMAGE_SUBSYSTEM_POSIX_CUI,  "POSIX console"); break;	// POSIX character-mode subsystem. 
	C(IMAGE_SUBSYSTEM_WINDOWS_CE_GUI,"Windows CE"); break;	// Windows CE. 
	#undef C
	}
	sprintf(buf, "%04X(%s)", value, id);
	return buf;
}

const char* MachineString(WORD value, char* buf=NULL)
{
	static char mybuf[100];
	if (!buf) buf = mybuf;

	const char* id = "?";
	switch (value) {
	#define C(v, name)	case v: id = name
	C(IMAGE_FILE_MACHINE_I386,   "32-bit Intel"); break;
	C(IMAGE_FILE_MACHINE_IA64,   "64-bit Intel"); break;
//	C(IMAGE_FILE_MACHINE_AMD64,  "64-bit AMD"); break;		// VC6のヘッダには,AMD64の定義がない.
	C(IMAGE_FILE_MACHINE_ALPHA,  "DEC Alpha"); break;
	C(IMAGE_FILE_MACHINE_POWERPC,"Power PC"); break;
	#undef C
	}
	sprintf(buf, "%04X(%s)", value, id);
	return buf;
}

const char* TimeDateString(time_t t, char* buf=NULL)
{
	static char mybuf[100];
	if (!buf) buf = mybuf;

	char* s = ctime(&t); if (!s) s = "? ";
	sprintf(buf, "%08X(%.*s)", t, strlen(s)-1, s);	// ctime が返す日付文字列は末尾に改行が付くので、最大長さを指定して抜く.

	return buf;
}

#define PRINTLONG(f,m)		printf("%24s : %08X\n", #m, (f).m)
#define PRINTWORD(f,m)		printf("%24s : %04X\n", #m, (f).m)
#define PRINTVER(f,m)		printf("%24s : %d.%d\n", #m, (f).Major##m, (f).Minor##m)
#define PRINTSTR(f,m)		printf("%24s : %s\n", #m, (f).m)
#define PRINTSTRF(f,m,fn)	printf("%24s : %s\n", #m, fn((f).m))

#define DIFFPRINTF(args)	(gQuiet ? (void)0 : (void)printf args)
#define DIFFLONG(f,m)		if ((f##1).m != (f##2).m) { ++differ; DIFFPRINTF(("\n%s.%s:\n<%08X\n>%08X\n", prompt, #m, (f##1).m, (f##2).m)); }
#define DIFFWORD(f,m)		if ((f##1).m != (f##2).m) { ++differ; DIFFPRINTF(("\n%s.%s:\n<%04X\n>%04X\n", prompt, #m, (f##1).m, (f##2).m)); }
#define DIFFVER(f,m)		if ((f##1).Major##m != (f##2).Major##m || (f##1).Minor##m != (f##2).Minor##m) { ++differ; \
								DIFFPRINTF(("\n%s.%s:\n<%d.%d\n>%d.%d\n", prompt, #m, \
										(f##1).Major##m, (f##1).Minor##m, \
										(f##2).Major##m, (f##2).Minor##m)); }
#define DIFFSTR(f,m)		if (!strequ((f##1).m, (f##2).m)) { ++differ; DIFFPRINTF(("\n%s.%s:\n<%s\n>%s\n", prompt, #m, (f##1).m, (f##2).m)); }
#define DIFFSTRF(f,m,fn)	if ((f##1).m != (f##2).m) { ++differ; DIFFPRINTF(("\n%s.%s:\n<%s\n", prompt, #m, fn((f##1).m))); DIFFPRINTF((">%s\n", fn((f##2).m))); }

void dump_header(const IMAGE_FILE_HEADER& file)
{
	PRINTSTRF(file, Machine, MachineString);
	PRINTWORD(file, NumberOfSections);
	PRINTSTRF(file, TimeDateStamp, TimeDateString);
	PRINTLONG(file, PointerToSymbolTable);
	PRINTLONG(file, NumberOfSymbols);
	PRINTWORD(file, SizeOfOptionalHeader);
	PRINTSTRF(file, Characteristics, ImageCharacteristicsString);
}

int diff_header(const char* prompt, const IMAGE_FILE_HEADER& file1, const IMAGE_FILE_HEADER& file2)
{
	int differ = 0;
	DIFFSTRF(file, Machine, MachineString);
	DIFFWORD(file, NumberOfSections);
	if (!gIgnoreTimeStamp) { DIFFSTRF(file, TimeDateStamp, TimeDateString); }
	DIFFLONG(file, PointerToSymbolTable);
	DIFFLONG(file, NumberOfSymbols);
	DIFFWORD(file, SizeOfOptionalHeader);
	DIFFSTRF(file, Characteristics, ImageCharacteristicsString);
	return differ;
}

void dump_header(const IMAGE_OPTIONAL_HEADER& opt)
{
	PRINTWORD(opt, Magic);
	PRINTVER (opt, LinkerVersion);
	PRINTLONG(opt, SizeOfCode);
	PRINTLONG(opt, SizeOfInitializedData);
	PRINTLONG(opt, SizeOfUninitializedData);
	PRINTLONG(opt, AddressOfEntryPoint);
	PRINTLONG(opt, BaseOfCode);
	PRINTLONG(opt, BaseOfData);
	PRINTLONG(opt, ImageBase);
	PRINTLONG(opt, SectionAlignment);
	PRINTLONG(opt, FileAlignment);
	PRINTVER (opt, OperatingSystemVersion);
	PRINTVER (opt, ImageVersion);
	PRINTVER (opt, SubsystemVersion);
	PRINTLONG(opt, Win32VersionValue);
	PRINTLONG(opt, SizeOfImage);
	PRINTLONG(opt, SizeOfHeaders);
	PRINTLONG(opt, CheckSum);
	PRINTSTRF(opt, Subsystem, SubsystemString);
	PRINTWORD(opt, DllCharacteristics);
	PRINTLONG(opt, SizeOfStackReserve);
	PRINTLONG(opt, SizeOfStackCommit);
	PRINTLONG(opt, SizeOfHeapReserve);
	PRINTLONG(opt, SizeOfHeapCommit);
	PRINTLONG(opt, LoaderFlags);
	PRINTLONG(opt, NumberOfRvaAndSizes);

	printf("----- Rva, Size -----\n");
	for (size_t i = 0; i < opt.NumberOfRvaAndSizes; ++i) {
		const IMAGE_DATA_DIRECTORY& d = opt.DataDirectory[i];
		printf("%20s[%2u] : %08X, %08X\n", "DataDirectory", i, d.VirtualAddress, d.Size);
	}
}

int diff_header(const char* prompt, const IMAGE_OPTIONAL_HEADER& opt1, const IMAGE_OPTIONAL_HEADER& opt2)
{
	int differ = 0;
	DIFFWORD(opt, Magic);
	DIFFVER (opt, LinkerVersion);
	DIFFLONG(opt, SizeOfCode);
	DIFFLONG(opt, SizeOfInitializedData);
	DIFFLONG(opt, SizeOfUninitializedData);
	DIFFLONG(opt, AddressOfEntryPoint);
	DIFFLONG(opt, BaseOfCode);
	DIFFLONG(opt, BaseOfData);
	DIFFLONG(opt, ImageBase);
	DIFFLONG(opt, SectionAlignment);
	DIFFLONG(opt, FileAlignment);
	DIFFVER (opt, OperatingSystemVersion);
	DIFFVER (opt, ImageVersion);
	DIFFVER (opt, SubsystemVersion);
	DIFFLONG(opt, Win32VersionValue);
	DIFFLONG(opt, SizeOfImage);
	DIFFLONG(opt, SizeOfHeaders);
	DIFFLONG(opt, CheckSum);
	DIFFSTRF(opt, Subsystem, SubsystemString);
	DIFFWORD(opt, DllCharacteristics);
	DIFFLONG(opt, SizeOfStackReserve);
	DIFFLONG(opt, SizeOfStackCommit);
	DIFFLONG(opt, SizeOfHeapReserve);
	DIFFLONG(opt, SizeOfHeapCommit);
	DIFFLONG(opt, LoaderFlags);
	DIFFLONG(opt, NumberOfRvaAndSizes);

/** @todo differ Rva,Size */
//	printf("----- Rva, Size -----\n");
//	for (size_t i = 0; i < opt.NumberOfRvaAndSizes; ++i) {
//		const IMAGE_DATA_DIRECTORY& d = opt.DataDirectory[i];
//		printf("%20s[%2u] : %08X, %08X\n", "DataDirectory", i, d.VirtualAddress, d.Size);
//	}
	return differ;
}

void dump_header(const IMAGE_SECTION_HEADER& sec)
{
	PRINTSTR(sec, Name);
	PRINTLONG(sec.Misc, VirtualSize);
	PRINTLONG(sec, VirtualAddress);
	PRINTLONG(sec, SizeOfRawData);
	PRINTLONG(sec, PointerToRawData);
	PRINTLONG(sec, PointerToRelocations);
	PRINTLONG(sec, PointerToLinenumbers);
	PRINTWORD(sec, NumberOfRelocations);
	PRINTWORD(sec, NumberOfLinenumbers);
	PRINTSTRF(sec, Characteristics, SectionCharacteristicsString);
}

int diff_header(const char* prompt, const IMAGE_SECTION_HEADER& sec1, const IMAGE_SECTION_HEADER& sec2)
{
	// DIFFマクロを使うための誤魔化し.
	struct {
		DWORD VirtualSize;
	} misc1, misc2;
	misc1.VirtualSize = sec1.Misc.VirtualSize;
	misc2.VirtualSize = sec2.Misc.VirtualSize;

	int differ = 0;
	DIFFSTR(sec, Name);
	DIFFLONG(misc, VirtualSize);
	DIFFLONG(sec, VirtualAddress);
	DIFFLONG(sec, SizeOfRawData);
	DIFFLONG(sec, PointerToRawData);
	DIFFLONG(sec, PointerToRelocations);
	DIFFLONG(sec, PointerToLinenumbers);
	DIFFWORD(sec, NumberOfRelocations);
	DIFFWORD(sec, NumberOfLinenumbers);
	DIFFSTRF(sec, Characteristics, SectionCharacteristicsString);
	return differ;
}

void dump_rawdata(const UCHAR* prompt, const UCHAR* p, size_t n)
{
	const UCHAR* b = p;
	char dump[16*3+1];
	char asc[16+1];
	size_t i = 0, j = 0;
	while (j < n) {
		int c = p[j++];
		sprintf(dump + i*3, "%02X%c", c, i==7 ? '-' : ' ');
		asc[i] = ascii(c);
		if (++i >= 16) {
			asc[16] = 0;
			printf("%14s +%p : %-48s:%-16s\n", prompt, j-i, dump, asc);
			i = 0;
		}
	}//.endwhile
	if (i != 0) {
		asc[i] = 0;
		printf("%14s +%p : %-48s:%-16s\n", prompt, j-i, dump, asc);
	}
}

int diff_rawdata(const char* prompt, const UCHAR* p1, size_t n1, const UCHAR* p2, size_t n2)
{
	size_t differ = 0;
	for (size_t i = 0; i < n1 || i < n2; ++i) {
		int c1 = (i < n1) ? p1[i] : -1;
		int c2 = (i < n2) ? p2[i] : -1;

		if (c1 == c2) continue;

		if (differ == 0)
			DIFFPRINTF(("\n%s\n", prompt));

		if (++differ > gDiffLength) {
			DIFFPRINTF(("\t<snip> differ more than %d bytes.\n", gDiffLength));
			break;
		}

		if (c1 == -1)
			DIFFPRINTF(("+%p: ----- <=> %02X(%c)\n", i, c2, ascii(c2)));
		else if (c2 == -1)
			DIFFPRINTF(("+%p: %02X(%c) <=> -----\n", i, c1, ascii(c1)));
		else
			DIFFPRINTF(("+%p: %02X(%c) <=> %02X(%c)\n", i, c1, ascii(c1), c2, ascii(c2)));
	}//.endfor
	return differ != 0;
}

DWORD size_of_rawdata(const IMAGE_SECTION_HEADER& sec)
{
	return min(sec.Misc.VirtualSize, sec.SizeOfRawData);
}

void ExeFileImage::print() const
{
	printf("===== dump of \"%s\" =====\n", ModuleName);
	PRINTLONG(*FileHeader, Signature);

	printf("----- FileHeader -----\n");
	dump_header(FileHeader->FileHeader);

	printf("----- OptionalHeader -----\n");
	dump_header(FileHeader->OptionalHeader);

	for (size_t i = 0; i < NumberOfSections; ++i) {
		const IMAGE_SECTION_HEADER& sec = Sections[i];

		printf("----- Section Header[%u] -----\n", i+1);
		dump_header(sec);

		printf("----- Section RawData[%u] (BaseAddress:%p, Size:%d bytes) -----\n", i+1,
			FileHeader->OptionalHeader.ImageBase + sec.VirtualAddress, sec.Misc.VirtualSize);
		dump_rawdata(sec.Name,   MappedAddress + sec.PointerToRawData, size_of_rawdata(sec));
	}
}

int diff(const ExeFileImage& exe1, const ExeFileImage& exe2)
{
	int differ = 0;

	if (gDirDiff && !gQuiet)
		printf("===== compare \"%s\" and \"%s\" =====\n", exe1.ModuleName, exe2.ModuleName);

	differ += diff_header("FileHeader", exe1.FileHeader->FileHeader, exe2.FileHeader->FileHeader);

	differ += diff_header("OptionalHeader", exe1.FileHeader->OptionalHeader, exe2.FileHeader->OptionalHeader);

	for (size_t i = 0; i < exe1.NumberOfSections || i < exe2.NumberOfSections; ++i) {
		const IMAGE_SECTION_HEADER& sec1 = exe1.Sections[i];
		const IMAGE_SECTION_HEADER& sec2 = exe2.Sections[i];
		char prompt[100];

		if (i >= exe1.NumberOfSections) {
			printf("%s section is only in \"%s\"\n", sec2.Name, exe2.ModuleName); ++differ; continue;
		}
		if (i >= exe2.NumberOfSections) {
			printf("%s section is only in \"%s\"\n", sec1.Name, exe1.ModuleName); ++differ; continue;
		}
		sprintf(prompt, "Section Header[%u]", i+1);
		differ += diff_header(prompt, sec1, sec2);

		sprintf(prompt, "Section RawData[%u] %s <=> %s:", i+1, sec1.Name, sec2.Name);
		differ += diff_rawdata(prompt,
			exe1.MappedAddress + sec1.PointerToRawData, size_of_rawdata(sec1),
			exe2.MappedAddress + sec2.PointerToRawData, size_of_rawdata(sec2));
	}//.endfor

	if (differ != 0)
		printf("\"%s\" and \"%s\" differ\n",        exe1.ModuleName, exe2.ModuleName);
	else
		printf("\"%s\" and \"%s\" are identical\n", exe1.ModuleName, exe2.ModuleName);

	return differ;
}


//------------------------------------------------------------------------
/** ロードイメージ比較を実行する.
 * @retval 0 一致
 * @retval 1 不一致
 */
int Compare(const ExeFileImage& f1, const ExeFileImage& f2)
{
	if (gDumpFileImage) {
		f1.print();
		f2.print();
	}
	return diff(f1, f2) != 0;
}

/** ロードイメージ比較を実行する.
 * @retval 0 一致
 * @retval 1 不一致
 * @retval 2 ファイル読み込み失敗
 */
int Compare(const char* fname1, const char* fname2)
{
	ExeFileImage f1(fname1); if (!f1.IsLoaded()) { print_win32error(fname1); return 2; }
	ExeFileImage f2(fname2); if (!f2.IsLoaded()) { print_win32error(fname2); return 2; }
	return Compare(f1, f2);
}

//------------------------------------------------------------------------
/** メイン関数 */
int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	//--- コマンドライン上のオプションを解析する.
	while (argc > 1 && argv[1][0] == '-') {
		char* sw = &argv[1][1];
		int i;
		if (strcmp(sw, "help") == 0)
			goto show_help;
		else if (sscanf(sw, "n%i", &i) == 1)
			gDiffLength = i;
		else {
			do {
				switch (*sw) {
				case 'h': case '?':
show_help:			error_abort(gUsage2);
					break;
				case 't':
					gIgnoreTimeStamp = true;
					break;
				case 'd':
					gDumpFileImage = true;
					break;
				case 'q':
					gQuiet = true;
					break;
				default:
					errorf_abort("%s: unknown option '%c'.\n", argv[1], *sw);
					break;
				}
			} while (*++sw);
		}
//next_arg:
		++argv;
		--argc;
	}
	if (argc < 3) {
		error_abort("please specify FILE or DIR\n");
	}

	int ret = EXIT_SUCCESS;

	if (argc == 3 && IsExistFile(argv[1])) {
		//--- コマンドライン上には FILE1 FILE2 を取り出し、両ファイルを比較する.
		ret = Compare(argv[1], argv[2]);
	}
	else {
		//--- コマンドライン上の DIR1 DIR2 WILD を取り出す.
		char dir1[_MAX_PATH];
		char dir2[_MAX_PATH];
		char wild[_MAX_PATH];
		strcpy(dir1, argv[1]);
		strcpy(dir2, argv[2]);
		strcpy(wild, argc <= 3 ? "*" : argv[3]);
		if (argc <= 3 && has_wildcard(dir2))
			separate_pathname(dir2, dir2, wild);

		//--- DIR2 下の WILD に合致するファイルを取り出し、DIR1下の同名ファイルと比較する.
		gDirDiff = true;
		ValidateFolder(dir1);
		ValidateFolder(dir2);
		FindFile find;
		for (find.Open(dir2, wild); find; find.Next()) {
			if (find.IsFolder())
				continue;
			char path1[_MAX_PATH];
			char path2[_MAX_PATH];
			_makepath(path1, NULL, dir1, find.name, NULL); // file1は存在しない可能性あり.
			_makepath(path2, NULL, dir2, find.name, NULL);
			ret |= Compare(path1, path2);
		}//.endfor
	}
	return ret;
}

//------------------------------------------------------------------------
/**@mainpage exediff - find differences between two exe/dll files

@version $Revision: 1.11 $

@author Hiroshi Kuno <hkuno-exediff-tool@micorhouse.co.jp>

@par License:
	Copyright &copy; 2004 by Hiroshi Kuno
	<br>本ソフトウェアは無保証かつ無償で提供します。利用、再配布、改変は自由です。

<hr>
@section intro はじめに
	exediffは、Win32実行ファイル(exe/dll)のロードイメージを比較し、差異を表示するコンソールアプリケーションです。
	二つのプログラムファイルの、同一性や違いを確認するのに便利です。

@section func 特徴
	- ロードイメージのヘッダ構造を認識し、構造単位での比較を行います。
	- ロードイメージのセクションデータ(RAWDATA)の比較では、差異が一定量に達したら比較を打ち切ります。
	- ディレクトリ間で複数ファイルの比較ができます。
	- ロードイメージに埋め込まれたタイムスタンプ部分を除外して比較できます。
	- 比較ファイルのテキスト形式ダンプ(dumpbin /all 相当)を出力できます。

@section env 動作環境
	WindowsNT3.1/Windows95以降。
	Windows98SE/Windows2000/WindowsXP にて動作確認済み。

@section install インストール方法
	配布ファイル windiff.exe を、PATHが通ったフォルダにコピーしてください。
	アインインストールするには、そのコピーしたファイルを削除してください。

@section usage 使い方
	@verbinclude usage.tmp

@section example 出力例
	@verbinclude example.tmp

@section pending 欠けている機能
	- RVAテーブルの比較を行っていません。
		- このテーブルの中身は補助構造へのオフセットとサイズなので、それらの値を比較しても意味がありません。
		比較すべきはオフセット先の補助構造ですが、その構造の形式が不明なため実現できていません。
		- 補助構造は .rdata セクションのRAWDATA中に埋め込まれており、バイト列としての比較は出来ています。

@section links リンク
	- http://hp.vector.co.jp/authors/VA002803/exediff.html - exediff公開サイト

@section download ダウンロード
	- http://hp.vector.co.jp/authors/VA002803/arc/exediff111.zip - 最新版 [June 30, 2004]

@section changelog 改訂履歴
	@subsection Rel111 Release 1.11 [June 30, 2004] 公開初版
*/

// exediff.cpp - end.
