// Package variantkey is a Go wrapper for the variantkey C library: a reversible
// 64 bit encoding of human genetic variants, genomic regions and string IDs.
package variantkey

/*
#cgo CFLAGS: -O3 -pedantic -std=c2x -Wextra -Wno-strict-prototypes -Wcast-align -Wundef -Wformat -Wformat-security -Wshadow
#include <stdlib.h>
#include <inttypes.h>
#include "../../c/src/variantkey/binsearch.h"
#include "../../c/src/variantkey/esid.h"
#include "../../c/src/variantkey/genoref.h"
#include "../../c/src/variantkey/hex.h"
#include "../../c/src/variantkey/nrvk.h"
#include "../../c/src/variantkey/regionkey.h"
#include "../../c/src/variantkey/rsidvar.h"
#include "../../c/src/variantkey/variantkey.h"
*/
import "C" //nolint:nolintlint,gci,typecheck

import (
	"fmt"
	"unsafe"
)

// maxcols is the maximum number of indexable columns as in binsearch.h file.
const maxcols = 255

// alleleMaxSize is the allele buffer size, as ALLELE_MAXSIZE in nrvk.h.
const alleleMaxSize = 256

// chromMaxSize is the CHROM buffer size required by decode_chrom in variantkey.h,
// including the terminating null byte.
const chromMaxSize = 4

// refaltMaxSize is the REF or ALT buffer size required by decode_refalt in
// variantkey.h, as VKMAX_ALLELE_LEN + 1.
const refaltMaxSize = 12

// hexMaxSize is the buffer size of a 16 digit hexadecimal string, as
// HEX_UINT64_LEN + 1 in hex.h.
const hexMaxSize = 17

// esidMaxSize is the buffer size required to decode any encoded string ID,
// as ESID_MAXSTRLEN in esid.h.
const esidMaxSize = 23

// TVariantKey contains a representation of a genetic variant key.
type TVariantKey struct {
	Chrom  uint8  `json:"chrom"`
	Pos    uint32 `json:"pos"`
	RefAlt uint32 `json:"refalt"`
}

// TVariantKeyRev contains the components of a genetic variant.
type TVariantKeyRev struct {
	Chrom   string `json:"chrom"`
	Pos     uint32 `json:"pos"`
	Ref     string `json:"ref"`
	Alt     string `json:"alt"`
	SizeRef uint8  `json:"size_ref"`
	SizeAlt uint8  `json:"size_alt"`
}

// TVKRange contains min and max VariantKey values for range searches.
type TVKRange struct {
	Min uint64 `json:"min"`
	Max uint64 `json:"max"`
}

// castCVariantKey converts a C variantkey_t to a Go TVariantKey.
func castCVariantKey(vk C.variantkey_t) TVariantKey {
	return TVariantKey{
		Chrom:  uint8(vk.chrom),
		Pos:    uint32(vk.pos),
		RefAlt: uint32(vk.refalt),
	}
}

// castCVariantKeyRev converts a C variantkey_rev_t to a Go TVariantKeyRev.
func castCVariantKeyRev(vk C.variantkey_rev_t) TVariantKeyRev {
	return TVariantKeyRev{
		Chrom:   C.GoString((*C.char)(unsafe.Pointer(&vk.chrom[0]))),
		Pos:     uint32(vk.pos),
		Ref:     C.GoString((*C.char)(unsafe.Pointer(&vk.ref[0]))),
		Alt:     C.GoString((*C.char)(unsafe.Pointer(&vk.alt[0]))),
		SizeRef: uint8(vk.sizeref),
		SizeAlt: uint8(vk.sizealt),
	}
}

// castCVKRrange convert C vkrange_t to GO TVKRange.
func castCVKRrange(vr C.vkrange_t) TVKRange {
	return TVKRange{
		Min: uint64(vr.min),
		Max: uint64(vr.max),
	}
}

// StringToNTBytes converts a string to a byte slice with an extra null
// terminator, as required by the CGO conversion to char*.
func StringToNTBytes(s string) []byte {
	b := make([]byte, len(s)+1)

	copy(b, s)

	return b
}

// StringToNTBytesN converts a string to a byte slice of "size" bytes.
func StringToNTBytesN(s string, size uint32) []byte {
	b := make([]byte, size)

	copy(b, s)

	return b
}

// EncodeChrom encodes a chromosome identifier into a numerical code.
func EncodeChrom(chrom string) uint8 {
	bchrom := StringToNTBytes(chrom)
	sizechrom := len(chrom)
	pchrom := unsafe.Pointer(&bchrom[0]) // #nosec

	return uint8(C.encode_chrom((*C.char)(pchrom), C.size_t(sizechrom)))
}

// DecodeChrom decodes a chromosome numerical code into its string representation.
func DecodeChrom(c uint8) string {
	var buf [chromMaxSize]byte

	ln := C.decode_chrom(C.uint8_t(c), (*C.char)(unsafe.Pointer(&buf[0]))) // #nosec

	return string(buf[:ln])
}

// EncodeRefAlt encodes a REF+ALT pair into a 31 bit code.
func EncodeRefAlt(ref string, alt string) uint32 {
	bref := StringToNTBytes(ref)
	balt := StringToNTBytes(alt)
	sizeref := len(ref)
	sizealt := len(alt)
	pref := unsafe.Pointer(&bref[0]) // #nosec
	palt := unsafe.Pointer(&balt[0]) // #nosec

	return uint32(C.encode_refalt((*C.char)(pref), C.size_t(sizeref), (*C.char)(palt), C.size_t(sizealt)))
}

// DecodeRefAlt decodes a 32 bit REF+ALT code if it was produced by the reversible encoding.
func DecodeRefAlt(c uint32) (string, string, uint8, uint8, uint8) {
	var (
		bref, balt         [refaltMaxSize]byte
		csizeref, csizealt C.size_t
	)

	pref := unsafe.Pointer(&bref[0]) // #nosec
	palt := unsafe.Pointer(&balt[0]) // #nosec
	ln := C.decode_refalt(C.uint32_t(c), (*C.char)(pref), &csizeref, (*C.char)(palt), &csizealt)

	return string(bref[:csizeref]), string(balt[:csizealt]), uint8(csizeref), uint8(csizealt), uint8(ln)
}

// EncodeVariantKey assembles a VariantKey from the pre-encoded CHROM, POS and REF+ALT.
func EncodeVariantKey(chrom uint8, pos, refalt uint32) uint64 {
	return uint64(C.encode_variantkey(C.uint8_t(chrom), C.uint32_t(pos), C.uint32_t(refalt)))
}

// ExtractVariantKeyChrom extracts the CHROM code from a VariantKey.
func ExtractVariantKeyChrom(v uint64) uint8 {
	return uint8(C.extract_variantkey_chrom(C.uint64_t(v)))
}

// ExtractVariantKeyPos extracts the POS value from a VariantKey.
func ExtractVariantKeyPos(v uint64) uint32 {
	return uint32(C.extract_variantkey_pos(C.uint64_t(v)))
}

// ExtractVariantKeyRefAlt extracts the REF+ALT code from a VariantKey.
func ExtractVariantKeyRefAlt(v uint64) uint32 {
	return uint32(C.extract_variantkey_refalt(C.uint64_t(v)))
}

// DecodeVariantKey splits a VariantKey into its CHROM, POS and REF+ALT components.
func DecodeVariantKey(v uint64) TVariantKey {
	var vk C.variantkey_t

	C.decode_variantkey(C.uint64_t(v), &vk)

	return castCVariantKey(vk)
}

// VariantKey returns a VariantKey for the given CHROM, POS (0-based), REF and ALT.
// The variant should be already normalized (see NormalizeVariant or use NormalizedVariantkey).
func VariantKey(chrom string, pos uint32, ref, alt string) uint64 {
	bchrom := StringToNTBytes(chrom)
	bref := StringToNTBytes(ref)
	balt := StringToNTBytes(alt)
	sizeref := len(ref)
	sizealt := len(alt)
	pchrom := unsafe.Pointer(&bchrom[0]) // #nosec
	pref := unsafe.Pointer(&bref[0])     // #nosec
	palt := unsafe.Pointer(&balt[0])     // #nosec

	return uint64(C.variantkey((*C.char)(pchrom), C.size_t(len(chrom)), C.uint32_t(pos), (*C.char)(pref), C.size_t(sizeref), (*C.char)(palt), C.size_t(sizealt)))
}

// Range returns the minimum and maximum VariantKey of a CHROM and POS range.
func Range(chrom uint8, posMin, posMax uint32) TVKRange {
	var r C.vkrange_t

	C.variantkey_range(C.uint8_t(chrom), C.uint32_t(posMin), C.uint32_t(posMax), &r)

	return castCVKRrange(r)
}

// CompareVariantKeyChrom compares two VariantKeys by chromosome only.
func CompareVariantKeyChrom(va, vb uint64) int {
	return int(C.compare_variantkey_chrom(C.uint64_t(va), C.uint64_t(vb)))
}

// CompareVariantKeyChromPos compares two VariantKeys by chromosome and position.
func CompareVariantKeyChromPos(va, vb uint64) int {
	return int(C.compare_variantkey_chrom_pos(C.uint64_t(va), C.uint64_t(vb)))
}

// Hex returns the 16 character hexadecimal representation of a 64 bit unsigned number.
func Hex(v uint64) string {
	var buf [hexMaxSize]byte

	ln := C.variantkey_hex(C.uint64_t(v), (*C.char)(unsafe.Pointer(&buf[0]))) // #nosec

	return string(buf[:ln])
}

// ParseHex parses a 16 character hexadecimal string into a 64 bit unsigned number.
func ParseHex(s string) uint64 {
	b := StringToNTBytes(s)
	p := unsafe.Pointer(&b[0]) // #nosec

	return uint64(C.parse_variantkey_hex((*C.char)(p)))
}

// ReverseVariantKey returns the CHROM, POS, REF and ALT components of a VariantKey.
// REF and ALT are empty for a non-reversible key: see NRVKCols.ReverseVariantKey.
func ReverseVariantKey(v uint64) (string, uint32, string, string, uint8, uint8) {
	vk := DecodeVariantKey(v)
	chrom := DecodeChrom(vk.Chrom)
	pos := vk.Pos

	var (
		ref, alt         string
		sizeref, sizealt uint8
	)

	if (vk.RefAlt & 0x1) == 0 {
		ref, alt, sizeref, sizealt, _ = DecodeRefAlt(vk.RefAlt)
	}

	return chrom, pos, ref, alt, sizeref, sizealt
}

// --- BINSEARCH ---

// TMMFile contains the memory mapped file info.
type TMMFile struct {
	Src     unsafe.Pointer // Pointer to the memory map.
	Fd      int            // File descriptor.
	Size    uint64         // File size in bytes.
	DOffset uint64         // Offset to the beginning of the data block (address of the first byte of the first item in the first column).
	DLength uint64         // Length in bytes of the data block.
	NRows   uint64         // Number of rows.
	NCols   uint8          // Number of columns.
	CTBytes []uint8        // Number of bytes per column type (i.e. 1 for uint8_t, 2 for uint16_t, 4 for uint32_t, 8 for uint64_t)
	Index   []uint64       // Index of the offsets to the beginning of each column.

	// cmf is the C copy of the fields above, built once when the file is
	// mapped. The C mmfile_t is more than 2 KB, so rebuilding it at every call
	// would cost more than the lookup itself. It is nil when the structure was
	// not returned by one of the Mmap* functions, in which case the exported
	// fields are converted on the fly.
	cmf *C.mmfile_t
}

// castCTMMFileToGo converts a C mmfile_t to a Go TMMFile.
func castCTMMFileToGo(mf C.mmfile_t) TMMFile {
	ncols := uint8(mf.ncols)
	ctbytes := make([]uint8, ncols, maxcols)
	index := make([]uint64, ncols, maxcols)

	var i uint8

	for i = range ncols {
		ctbytes[i] = uint8(mf.ctbytes[i])
		index[i] = uint64(mf.index[i])
	}

	cmf := new(C.mmfile_t)
	*cmf = mf

	return TMMFile{
		Src:     unsafe.Pointer(mf.src), // #nosec
		Fd:      int(mf.fd),
		Size:    uint64(mf.size),
		DOffset: uint64(mf.doffset),
		DLength: uint64(mf.dlength),
		NRows:   uint64(mf.nrows),
		NCols:   ncols,
		CTBytes: ctbytes,
		Index:   index,
		cmf:     cmf,
	}
}

// castGoTMMFileToC converts a Go TMMFile to a C mmfile_t.
func castGoTMMFileToC(mf TMMFile) C.mmfile_t {
	var cmf C.mmfile_t

	cmf.src = (*C.uint8_t)(mf.Src)
	cmf.fd = C.int(mf.Fd)
	cmf.size = C.uint64_t(mf.Size)
	cmf.doffset = C.uint64_t(mf.DOffset)
	cmf.dlength = C.uint64_t(mf.DLength)
	cmf.nrows = C.uint64_t(mf.NRows)
	cmf.ncols = C.uint8_t(mf.NCols)

	if len(mf.CTBytes) > 0 {
		cmf.ctbytes = *(*[maxcols]C.uint8_t)(unsafe.Pointer(&mf.CTBytes[0]))
		cmf.index = *(*[maxcols]C.uint64_t)(unsafe.Pointer(&mf.Index[0]))
	}

	return cmf
}

// cmmfile returns the C mmfile_t, reusing the copy built when the file was mapped.
func cmmfile(mf TMMFile) *C.mmfile_t {
	if mf.cmf != nil {
		return mf.cmf
	}

	cmf := castGoTMMFileToC(mf)

	return &cmf
}

// Close unmaps and closes the memory mapped file.
func (mf TMMFile) Close() error {
	e := int(C.munmap_binfile(cmmfile(mf)))
	if e != 0 {
		return fmt.Errorf("got %d error while unmapping the file", e)
	}

	return nil
}

// --- RSIDVAR ---

// RSIDVARCols contains the RSVK or VKRS memory mapped file column info.
type RSIDVARCols struct {
	Vk    unsafe.Pointer // Pointer to the VariantKey column.
	Rs    unsafe.Pointer // Pointer to the rsID column.
	NRows uint64         // Number of rows.
}

// castCRSIDVARColsToGo converts a C rsidvar_cols_t to a Go RSIDVARCols.
func castCRSIDVARColsToGo(crv C.rsidvar_cols_t) RSIDVARCols {
	return RSIDVARCols{
		Vk:    unsafe.Pointer(crv.vk), // #nosec
		Rs:    unsafe.Pointer(crv.rs), // #nosec
		NRows: uint64(crv.nrows),
	}
}

// castGoRSIDVARColsToC converts a Go RSIDVARCols to a C rsidvar_cols_t.
func castGoRSIDVARColsToC(rc RSIDVARCols) C.rsidvar_cols_t {
	var rvc C.rsidvar_cols_t

	rvc.vk = (*C.uint64_t)(rc.Vk)
	rvc.rs = (*C.uint32_t)(rc.Rs)
	rvc.nrows = C.uint64_t(rc.NRows)

	return rvc
}

// MmapVKRSFile memory maps the VKRS binary file.
func MmapVKRSFile(file string, ctbytes []uint8) (TMMFile, RSIDVARCols, error) {
	bfile := StringToNTBytes(file)
	flen := len(bfile)

	var p unsafe.Pointer

	if flen > 0 {
		p = unsafe.Pointer(&bfile[0]) // #nosec
	}

	var mf C.mmfile_t

	mf.ncols = C.uint8_t(len(ctbytes))

	for k, v := range ctbytes {
		mf.ctbytes[k] = C.uint8_t(v)
	}

	var rc C.rsidvar_cols_t

	C.mmap_vkrs_file((*C.char)(p), &mf, &rc) //nolint:gocritic

	if mf.fd < 0 || mf.size == 0 || mf.src == nil {
		return TMMFile{}, RSIDVARCols{}, fmt.Errorf("unable to map the file: %s", file)
	}

	return castCTMMFileToGo(mf), castCRSIDVARColsToGo(rc), nil
}

// MmapRSVKFile memory maps the RSVK binary file.
func MmapRSVKFile(file string, ctbytes []uint8) (TMMFile, RSIDVARCols, error) {
	bfile := StringToNTBytes(file)
	flen := len(bfile)

	var p unsafe.Pointer

	if flen > 0 {
		p = unsafe.Pointer(&bfile[0]) // #nosec
	}

	var mf C.mmfile_t

	mf.ncols = C.uint8_t(len(ctbytes))

	for k, v := range ctbytes {
		mf.ctbytes[k] = C.uint8_t(v)
	}

	var rc C.rsidvar_cols_t

	C.mmap_rsvk_file((*C.char)(p), &mf, &rc) //nolint:gocritic

	if mf.fd < 0 || mf.size == 0 || mf.src == nil {
		return TMMFile{}, RSIDVARCols{}, fmt.Errorf("unable to map the file: %s", file)
	}

	return castCTMMFileToGo(mf), castCRSIDVARColsToGo(rc), nil
}

// FindRVVariantKeyByRsid returns the first VariantKey associated with an rsID, and its position.
func (crv RSIDVARCols) FindRVVariantKeyByRsid(first, last uint64, rsid uint32) (uint64, uint64) {
	cfirst := C.uint64_t(first)
	vk := uint64(C.find_rv_variantkey_by_rsid(castGoRSIDVARColsToC(crv), &cfirst, C.uint64_t(last), C.uint32_t(rsid)))

	return vk, uint64(cfirst)
}

// GetNextRVVariantKeyByRsid returns the next VariantKey associated with an rsID, or 0,
// and its position.
func (crv RSIDVARCols) GetNextRVVariantKeyByRsid(pos, last uint64, rsid uint32) (uint64, uint64) {
	cpos := C.uint64_t(pos)
	vk := uint64(C.get_next_rv_variantkey_by_rsid(castGoRSIDVARColsToC(crv), &cpos, C.uint64_t(last), C.uint32_t(rsid)))

	return vk, uint64(cpos)
}

// FindAllRVVariantKeyByRsid returns all the VariantKeys associated with an rsID.
func (crv RSIDVARCols) FindAllRVVariantKeyByRsid(first, last uint64, rsid uint32) []uint64 {
	ccr := castGoRSIDVARColsToC(crv)
	cfirst := C.uint64_t(first)
	clast := C.uint64_t(last)
	crsid := C.uint32_t(rsid)
	vk := uint64(C.find_rv_variantkey_by_rsid(ccr, &cfirst, clast, crsid))

	var vks []uint64

	for vk > 0 {
		vks = append(vks, vk)
		vk = uint64(C.get_next_rv_variantkey_by_rsid(ccr, &cfirst, clast, crsid))
	}

	return vks
}

// FindVRRsidByVariantKey returns the first rsID associated with a VariantKey, and its position.
func (crv RSIDVARCols) FindVRRsidByVariantKey(first uint64, last uint64, vk uint64) (uint32, uint64) {
	cfirst := C.uint64_t(first)
	rsid := uint32(C.find_vr_rsid_by_variantkey(castGoRSIDVARColsToC(crv), &cfirst, C.uint64_t(last), C.uint64_t(vk)))

	return rsid, uint64(cfirst)
}

// GetNextVRRsidByVariantKey returns the next rsID associated with a VariantKey, or 0,
// and its position.
//
//nolint:revive
func (cvr RSIDVARCols) GetNextVRRsidByVariantKey(pos, last uint64, vk uint64) (uint32, uint64) {
	cpos := C.uint64_t(pos)
	rsid := uint32(C.get_next_vr_rsid_by_variantkey(castGoRSIDVARColsToC(cvr), &cpos, C.uint64_t(last), C.uint64_t(vk)))

	return rsid, uint64(cpos)
}

// FindAllVRRsidByVariantKey returns all the rsIDs associated with a VariantKey.
//
//nolint:revive
func (cvr RSIDVARCols) FindAllVRRsidByVariantKey(first, last uint64, vk uint64) []uint32 {
	ccr := castGoRSIDVARColsToC(cvr)
	cfirst := C.uint64_t(first)
	clast := C.uint64_t(last)
	cvk := C.uint64_t(vk)
	rsid := uint32(C.find_vr_rsid_by_variantkey(ccr, &cfirst, clast, cvk))

	var rsids []uint32

	for rsid > 0 {
		rsids = append(rsids, rsid)
		rsid = uint32(C.get_next_vr_rsid_by_variantkey(ccr, &cfirst, clast, cvk))
	}

	return rsids
}

// FindVRChromPosRange returns the first rsID of a CHROM and POS range,
// and the first and last position of the range.
func (crv RSIDVARCols) FindVRChromPosRange(first, last uint64, chrom uint8, posMin, posMax uint32) (uint32, uint64, uint64) {
	cfirst := C.uint64_t(first)
	clast := C.uint64_t(last)
	rsid := uint32(C.find_vr_chrompos_range(castGoRSIDVARColsToC(crv), &cfirst, &clast, C.uint8_t(chrom), C.uint32_t(posMin), C.uint32_t(posMax)))

	return rsid, uint64(cfirst), uint64(clast)
}

// --- NRVK ---

// NRVKCols contains the NRVK memory mapped file column info.
type NRVKCols struct {
	Vk     unsafe.Pointer // Pointer to the VariantKey column.
	Offset unsafe.Pointer // Pointer to the Offset column.
	Data   unsafe.Pointer // Pointer to the Data column.
	NRows  uint64         // Number of rows.
}

// castCNRVKColsToGo converts a C nrvk_cols_t to a Go NRVKCols.
func castCNRVKColsToGo(nr C.nrvk_cols_t) NRVKCols {
	return NRVKCols{
		Vk:     unsafe.Pointer(nr.vk),     // #nosec
		Offset: unsafe.Pointer(nr.offset), // #nosec
		Data:   unsafe.Pointer(nr.data),   // #nosec
		NRows:  uint64(nr.nrows),
	}
}

// castGoNRVKColsToC converts a Go NRVKCols to a C nrvk_cols_t.
func castGoNRVKColsToC(nr NRVKCols) C.nrvk_cols_t {
	var cnr C.nrvk_cols_t

	cnr.vk = (*C.uint64_t)(nr.Vk)
	cnr.offset = (*C.uint64_t)(nr.Offset)
	cnr.data = (*C.uint8_t)(nr.Data)
	cnr.nrows = C.uint64_t(nr.NRows)

	return cnr
}

// MmapNRVKFile memory maps the NRVK binary file.
func MmapNRVKFile(file string) (TMMFile, NRVKCols, error) {
	bfile := StringToNTBytes(file)
	flen := len(bfile)

	var p unsafe.Pointer

	if flen > 0 {
		p = unsafe.Pointer(&bfile[0]) // #nosec
	}

	var (
		mf C.mmfile_t
		rc C.nrvk_cols_t
	)

	C.mmap_nrvk_file((*C.char)(p), &mf, &rc) //nolint:gocritic

	if mf.fd < 0 || mf.size == 0 || mf.src == nil {
		return TMMFile{}, NRVKCols{}, fmt.Errorf("unable to map the file: %s", file)
	}

	return castCTMMFileToGo(mf), castCNRVKColsToGo(rc), nil
}

// FindRefAltByVariantKey looks up the REF and ALT strings of a VariantKey.
func (nr NRVKCols) FindRefAltByVariantKey(vk uint64) (string, string, uint8, uint8, uint32) {
	var (
		bref, balt         [alleleMaxSize]byte
		csizeref, csizealt C.size_t
	)

	pref := unsafe.Pointer(&bref[0]) // #nosec
	palt := unsafe.Pointer(&balt[0]) // #nosec
	ln := C.find_ref_alt_by_variantkey(castGoNRVKColsToC(nr), C.uint64_t(vk), (*C.char)(pref), &csizeref, (*C.char)(palt), &csizealt)

	return string(bref[:csizeref]), string(balt[:csizealt]), uint8(csizeref), uint8(csizealt), uint32(ln)
}

// ReverseVariantKey reverses a VariantKey into its CHROM, POS, REF and ALT components,
// reading REF and ALT from the lookup table when the key is not reversible.
func (nr NRVKCols) ReverseVariantKey(vk uint64) (TVariantKeyRev, uint32) {
	var rev C.variantkey_rev_t

	ln := C.reverse_variantkey(castGoNRVKColsToC(nr), C.uint64_t(vk), &rev)

	return castCVariantKeyRev(rev), uint32(ln)
}

// GetVariantKeyRefLength returns the REF length of a VariantKey.
func (nr NRVKCols) GetVariantKeyRefLength(vk uint64) uint8 {
	return uint8(C.get_variantkey_ref_length(castGoNRVKColsToC(nr), C.uint64_t(vk)))
}

// GetVariantKeyEndPos returns the end position of a VariantKey (POS + REF length).
func (nr NRVKCols) GetVariantKeyEndPos(vk uint64) uint32 {
	return uint32(C.get_variantkey_endpos(castGoNRVKColsToC(nr), C.uint64_t(vk)))
}

// GetVariantKeyChromStartPos returns the CHROM and START POS section of a VariantKey.
func GetVariantKeyChromStartPos(vk uint64) uint64 {
	return uint64(C.get_variantkey_chrom_startpos(C.uint64_t(vk)))
}

// GetVariantKeyChromEndPos returns the CHROM and END POS of a VariantKey.
func (nr NRVKCols) GetVariantKeyChromEndPos(vk uint64) uint64 {
	return uint64(C.get_variantkey_chrom_endpos(castGoNRVKColsToC(nr), C.uint64_t(vk)))
}

// VknrBinToTSV writes the content of the NRVK memory mapped file as a TSV file.
// For the reverse operation see the resources/tools/nrvk.sh script.
// It returns the number of bytes written, or an error if the file cannot be
// opened. The C function signals failure by returning 0.
func (nr NRVKCols) VknrBinToTSV(tsvfile string) (uint64, error) {
	file := StringToNTBytes(tsvfile)
	pfile := unsafe.Pointer(&file[0]) // #nosec

	n := uint64(C.nrvk_bin_to_tsv(castGoNRVKColsToC(nr), (*C.char)(pfile)))
	if n == 0 {
		return 0, fmt.Errorf("unable to write the TSV file: %s", tsvfile)
	}

	return n, nil
}

// --- GENOREF ---

// MmapGenorefFile memory maps the genoref binary file.
func MmapGenorefFile(file string) (TMMFile, error) {
	bfile := StringToNTBytes(file)
	flen := len(bfile)

	var p unsafe.Pointer

	if flen > 0 {
		p = unsafe.Pointer(&bfile[0]) // #nosec
	}

	var mf C.mmfile_t

	C.mmap_genoref_file((*C.char)(p), &mf) //nolint:gocritic

	if mf.fd < 0 || mf.size == 0 || mf.src == nil {
		return TMMFile{}, fmt.Errorf("unable to map the file: %s", file)
	}

	return castCTMMFileToGo(mf), nil
}

// FlipAllele replaces each nucleotide of an allele with its complement.
func FlipAllele(allele string) string {
	ballele := StringToNTBytes(allele)
	size := len(allele)
	pallele := unsafe.Pointer(&ballele[0]) // #nosec

	C.flip_allele((*C.char)(pallele), C.size_t(size))

	return C.GoString((*C.char)(pallele))
}

// GetGenorefSeq returns the genome reference nucleotide at the given chromosome and position.
func (mf TMMFile) GetGenorefSeq(chrom uint8, pos uint32) byte {
	return byte(C.get_genoref_seq(cmmfile(mf), C.uint8_t(chrom), C.uint32_t(pos)))
}

// CheckReference checks a reference allele against the genome reference data.
func (mf TMMFile) CheckReference(chrom uint8, pos uint32, ref string) int {
	bref := StringToNTBytes(ref)
	pref := unsafe.Pointer(&bref[0]) // #nosec

	return int(C.check_reference(cmmfile(mf), C.uint8_t(chrom), C.uint32_t(pos), (*C.char)(pref), C.size_t(len(ref))))
}

// NormalizeVariant normalizes a variant against the genome reference, flipping the
// alleles if required. See https://genome.sph.umich.edu/wiki/Variant_Normalization
func (mf TMMFile) NormalizeVariant(chrom uint8, pos uint32, ref string, alt string) (int, uint32, string, string, uint8, uint8) {
	bref := StringToNTBytesN(ref, alleleMaxSize)
	balt := StringToNTBytesN(alt, alleleMaxSize)
	sizeref := len(ref)
	sizealt := len(alt)
	pref := unsafe.Pointer(&bref[0]) // #nosec
	palt := unsafe.Pointer(&balt[0]) // #nosec
	cpos := C.uint32_t(pos)
	csizeref := C.size_t(sizeref)
	csizealt := C.size_t(sizealt)
	code := int(C.normalize_variant(cmmfile(mf), C.uint8_t(chrom), &cpos, (*C.char)(pref), &csizeref, (*C.char)(palt), &csizealt))
	npos := uint32(cpos)
	nref := string(bref[:csizeref])
	nalt := string(balt[:csizealt])
	nsizeref := uint8(csizeref)
	nsizealt := uint8(csizealt)

	return code, npos, nref, nalt, nsizeref, nsizealt
}

// NormalizedVariantKey normalizes a variant and returns its VariantKey, with the normalization return code.
func (mf TMMFile) NormalizedVariantKey(chrom string, pos uint32, posindex uint8, ref string, alt string) (uint64, int) {
	bchrom := StringToNTBytes(chrom)
	bref := StringToNTBytesN(ref, alleleMaxSize)
	balt := StringToNTBytesN(alt, alleleMaxSize)
	sizeref := len(ref)
	sizealt := len(alt)
	pchrom := unsafe.Pointer(&bchrom[0]) // #nosec
	pref := unsafe.Pointer(&bref[0])     // #nosec
	palt := unsafe.Pointer(&balt[0])     // #nosec
	cpos := C.uint32_t(pos)
	csizeref := C.size_t(sizeref)
	csizealt := C.size_t(sizealt)
	ccode := C.int(0)
	vk := uint64(C.normalized_variantkey(cmmfile(mf), (*C.char)(pchrom), C.size_t(len(chrom)), &cpos, C.uint8_t(posindex), (*C.char)(pref), &csizeref, (*C.char)(palt), &csizealt, &ccode))

	return vk, int(ccode)
}

// --- REGIONKEY ---

// TRegionKey contains a representation of a genomic region key.
type TRegionKey struct {
	Chrom    uint8  `json:"chrom"`
	StartPos uint32 `json:"startpos"`
	EndPos   uint32 `json:"endpos"`
	Strand   uint8  `json:"strand"`
}

// TRegionKeyRev contains the components of a genomic region.
type TRegionKeyRev struct {
	Chrom    string `json:"chrom"`
	StartPos uint32 `json:"startpos"`
	EndPos   uint32 `json:"endpos"`
	Strand   int8   `json:"strand"`
}

// castCRegionKey converts a C regionkey_t to a Go TRegionKey.
func castCRegionKey(rk C.regionkey_t) TRegionKey {
	return TRegionKey{
		Chrom:    uint8(rk.chrom),
		StartPos: uint32(rk.startpos),
		EndPos:   uint32(rk.endpos),
		Strand:   uint8(rk.strand),
	}
}

// castCRegionKeyRev converts a C regionkey_rev_t to a Go TRegionKeyRev.
func castCRegionKeyRev(rk C.regionkey_rev_t) TRegionKeyRev {
	return TRegionKeyRev{
		Chrom:    C.GoString((*C.char)(unsafe.Pointer(&rk.chrom[0]))),
		StartPos: uint32(rk.startpos),
		EndPos:   uint32(rk.endpos),
		Strand:   int8(rk.strand),
	}
}

// EncodeRegionStrand encodes a strand direction: -1 to 2, 0 to 0, +1 to 1.
func EncodeRegionStrand(strand int8) uint8 {
	return uint8(C.encode_region_strand(C.int8_t(strand)))
}

// DecodeRegionStrand decodes a strand code: 0 to 0, 1 to +1, 2 to -1.
func DecodeRegionStrand(strand uint8) int8 {
	return int8(C.decode_region_strand(C.uint8_t(strand)))
}

// EncodeRegionKey assembles a RegionKey from its pre-encoded components.
func EncodeRegionKey(chrom uint8, startpos, endpos uint32, strand uint8) uint64 {
	return uint64(C.encode_regionkey(C.uint8_t(chrom), C.uint32_t(startpos), C.uint32_t(endpos), C.uint8_t(strand)))
}

// ExtractRegionKeyChrom extracts the CHROM code from a RegionKey.
func ExtractRegionKeyChrom(rk uint64) uint8 {
	return uint8(C.extract_regionkey_chrom(C.uint64_t(rk)))
}

// ExtractRegionKeyStartPos extracts the START POS value from a RegionKey.
func ExtractRegionKeyStartPos(rk uint64) uint32 {
	return uint32(C.extract_regionkey_startpos(C.uint64_t(rk)))
}

// ExtractRegionKeyEndPos extracts the END POS value from a RegionKey.
func ExtractRegionKeyEndPos(rk uint64) uint32 {
	return uint32(C.extract_regionkey_endpos(C.uint64_t(rk)))
}

// ExtractRegionKeyStrand extracts the STRAND code from a RegionKey.
func ExtractRegionKeyStrand(rk uint64) uint8 {
	return uint8(C.extract_regionkey_strand(C.uint64_t(rk)))
}

// DecodeRegionKey splits a RegionKey into its encoded components.
func DecodeRegionKey(rk uint64) TRegionKey {
	var drk C.regionkey_t

	C.decode_regionkey(C.uint64_t(rk), &drk)

	return castCRegionKey(drk)
}

// ReverseRegionKey reverses a RegionKey into its decoded components.
func ReverseRegionKey(rk uint64) TRegionKeyRev {
	var rrk C.regionkey_rev_t

	C.reverse_regionkey(C.uint64_t(rk), &rrk)

	return castCRegionKeyRev(rrk)
}

// RegionKey returns a RegionKey for the given CHROM, START POS (0-based), END POS and STRAND.
func RegionKey(chrom string, startpos, endpos uint32, strand int8) uint64 {
	bchrom := StringToNTBytes(chrom)
	pchrom := unsafe.Pointer(&bchrom[0]) // #nosec

	return uint64(C.regionkey((*C.char)(pchrom), C.size_t(len(chrom)), C.uint32_t(startpos), C.uint32_t(endpos), C.int8_t(strand)))
}

// ExtendRegionKey extends a RegionKey region by a fixed amount at both ends.
func ExtendRegionKey(rk uint64, size uint32) uint64 {
	return uint64(C.extend_regionkey(C.uint64_t(rk), C.uint32_t(size)))
}

// GetRegionKeyChromStartPos returns the CHROM and START POS section of a RegionKey.
func GetRegionKeyChromStartPos(rk uint64) uint64 {
	return uint64(C.get_regionkey_chrom_startpos(C.uint64_t(rk)))
}

// GetRegionKeyChromEndPos returns the CHROM and END POS of a RegionKey.
func GetRegionKeyChromEndPos(rk uint64) uint64 {
	return uint64(C.get_regionkey_chrom_endpos(C.uint64_t(rk)))
}

// AreOverlappingRegions checks whether two regions overlap.
func AreOverlappingRegions(chromA uint8, startposA, endposA uint32, chromB uint8, startposB, endposB uint32) bool {
	return (uint8(C.are_overlapping_regions(C.uint8_t(chromA), C.uint32_t(startposA), C.uint32_t(endposA), C.uint8_t(chromB), C.uint32_t(startposB), C.uint32_t(endposB))) != 0)
}

// AreOverlappingRegionRegionKey checks whether a region and a RegionKey overlap.
func AreOverlappingRegionRegionKey(chrom uint8, startpos, endpos uint32, rk uint64) bool {
	return (uint8(C.are_overlapping_region_regionkey(C.uint8_t(chrom), C.uint32_t(startpos), C.uint32_t(endpos), C.uint64_t(rk))) != 0)
}

// AreOverlappingRegionKeys checks whether two RegionKeys overlap.
func AreOverlappingRegionKeys(rka, rkb uint64) bool {
	return (uint8(C.are_overlapping_regionkeys(C.uint64_t(rka), C.uint64_t(rkb))) != 0)
}

// AreOverlappingVariantKeyRegionKey checks whether a VariantKey and a RegionKey overlap.
func (nr NRVKCols) AreOverlappingVariantKeyRegionKey(vk, rk uint64) bool {
	return (uint8(C.are_overlapping_variantkey_regionkey(castGoNRVKColsToC(nr), C.uint64_t(vk), C.uint64_t(rk))) != 0)
}

// VariantToRegionkey converts a VariantKey into a RegionKey.
func (nr NRVKCols) VariantToRegionkey(vk uint64) uint64 {
	return uint64(C.variantkey_to_regionkey(castGoNRVKColsToC(nr), C.uint64_t(vk)))
}

// --- ESID ---

// EncodeStringID encodes up to 10 characters of a string into a 64 bit unsigned integer.
// The "start" argument is the index of the first character to encode.
func EncodeStringID(s string, start uint32) uint64 {
	bs := StringToNTBytes(s)
	ps := unsafe.Pointer(&bs[0]) // #nosec

	return uint64(C.encode_string_id((*C.char)(ps), C.size_t(len(s)), C.size_t(start)))
}

// EncodeStringNumID encodes a string made of a character section, a separator and a
// numerical section into a 64 bit unsigned integer. For example: "ABCDE:0001234".
// It encodes up to 5 characters in uppercase, a number up to 2^27, and up to 7 zero padding digits.
// Strings of 10 characters or less are encoded as by EncodeStringID.
func EncodeStringNumID(s string, sep byte) uint64 {
	bs := StringToNTBytes(s)
	ps := unsafe.Pointer(&bs[0]) // #nosec

	return uint64(C.encode_string_num_id((*C.char)(ps), C.size_t(len(s)), C.char(sep)))
}

// DecodeStringID decodes an encoded string ID.
func DecodeStringID(esid uint64) string {
	var buf [esidMaxSize]byte

	ln := C.decode_string_id(C.uint64_t(esid), (*C.char)(unsafe.Pointer(&buf[0]))) // #nosec

	return string(buf[:ln])
}

// HashStringID hashes a string into a non-reversible 64 bit string ID.
func HashStringID(s string) uint64 {
	bs := StringToNTBytes(s)
	ps := unsafe.Pointer(&bs[0]) // #nosec

	return uint64(C.hash_string_id((*C.char)(ps), C.size_t(len(s))))
}
