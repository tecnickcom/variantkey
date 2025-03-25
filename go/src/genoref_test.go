package variantkey

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestGenorefIndex(t *testing.T) {
	t.Parallel()

	exp := []uint64{248, 248, 274, 299, 323, 346, 368, 389, 409, 428, 446, 463, 479, 494, 508, 521, 533, 544, 554, 563, 571, 578, 584, 589, 593, 596, 598}
	require.Len(t, gref.Index, len(exp), "Expected size %d, got %d", len(exp), len(gref.Index))

	for k, v := range exp {
		require.Equal(t, gref.Index[k], v, "(%d) Expected value %d, got %d", k, v, gref.Index[k])
	}
}

func TestGetGenorefSeq(t *testing.T) {
	t.Parallel()

	var (
		chrom    uint8
		ref, exp byte
	)

	for chrom = 1; chrom <= 25; chrom++ {
		ref = gref.GetGenorefSeq(chrom, 0) // first
		require.Equal(t, uint8('A'), ref, "%d (first): Expected reference 'A', got '%c'", chrom, ref)

		ref = gref.GetGenorefSeq(chrom, (26 - uint32(chrom))) // last
		exp = 'Z' + 1 - chrom
		require.Equal(t, exp, ref, "%d (last): Expected reference '%c', got '%c'", chrom, exp, ref)

		ref = gref.GetGenorefSeq(chrom, (27 - uint32(chrom))) // invalid
		require.Equal(t, uint8(0), ref, "%d (invalid): Expected reference 0, got '%c'", chrom, ref)
	}
}

func BenchmarkGetGenorefSeq(b *testing.B) {
	b.ResetTimer()

	for range b.N {
		gref.GetGenorefSeq(13, 1)
	}
}

func TestCheckReference(t *testing.T) {
	t.Parallel()

	type TCheckRefData struct {
		exp     int
		chrom   uint8
		pos     uint32
		sizeref uint8
		ref     string
	}

	tdata := []TCheckRefData{
		{0, 1, 0, 1, "A"},
		{0, 1, 25, 1, "Z"},
		{0, 25, 0, 1, "A"},
		{0, 25, 1, 1, "B"},
		{0, 2, 0, 25, "ABCDEFGHIJKLmnopqrstuvwxy"},
		{-2, 1, 26, 4, "ZABC"},
		{-1, 1, 0, 26, "ABCDEFGHIJKLmnopqrstuvwxyJ"},
		{-1, 14, 2, 3, "ZZZ"},
		{1, 1, 0, 1, "N"},
		{1, 10, 13, 1, "A"},
		{1, 1, 3, 1, "B"},
		{1, 1, 1, 1, "C"},
		{1, 1, 0, 1, "D"},
		{1, 1, 3, 1, "A"},
		{1, 1, 0, 1, "H"},
		{1, 1, 7, 1, "A"},
		{1, 1, 0, 1, "V"},
		{1, 1, 21, 1, "A"},
		{1, 1, 0, 1, "W"},
		{1, 1, 19, 1, "W"},
		{1, 1, 22, 1, "A"},
		{1, 1, 22, 1, "T"},
		{1, 1, 2, 1, "S"},
		{1, 1, 6, 1, "S"},
		{1, 1, 18, 1, "C"},
		{1, 1, 18, 1, "G"},
		{1, 1, 0, 1, "M"},
		{1, 1, 2, 1, "M"},
		{1, 1, 12, 1, "A"},
		{1, 1, 12, 1, "C"},
		{1, 1, 6, 1, "K"},
		{1, 1, 19, 1, "K"},
		{1, 1, 10, 1, "G"},
		{1, 1, 10, 1, "T"},
		{1, 1, 0, 1, "R"},
		{1, 1, 6, 1, "R"},
		{1, 1, 17, 1, "A"},
		{1, 1, 17, 1, "G"},
		{1, 1, 2, 1, "Y"},
		{1, 1, 19, 1, "Y"},
		{1, 1, 24, 1, "C"},
		{1, 1, 24, 1, "T"},
	}

	for _, v := range tdata {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			h := gref.CheckReference(v.chrom, v.pos, v.ref)
			require.Equal(t, v.exp, h, "The return code is different, got: %#v expected %#v", h, v.exp)
		})
	}
}

func TestFlipAlllele(t *testing.T) {
	t.Parallel()

	allele := "ATCGMKRYBVDHWSNatcgmkrybvdhwsn"
	expected := "TAGCKMYRVBHDWSNTAGCKMYRVBHDWSN"
	res := FlipAllele(allele)

	require.Equal(t, expected, res, "The returned value is incorrect, got: %#v expected %#v", res, expected)
}

func TestNormalizeVariant(t *testing.T) {
	t.Parallel()

	type TNormData struct {
		ecode    int
		chrom    uint8
		pos      uint32
		epos     uint32
		sizeref  uint8
		sizealt  uint8
		esizeref uint8
		esizealt uint8
		eref     string
		ealt     string
		ref      string
		alt      string
	}

	ndata := []TNormData{
		{-2, 1, 26, 26, 1, 1, 1, 1, "A", "C", "A", "C"},         // invalid position
		{-1, 1, 0, 0, 1, 1, 1, 1, "J", "C", "J", "C"},           // invalid reference
		{4, 1, 0, 0, 1, 1, 1, 1, "A", "C", "T", "G"},            // flip
		{0, 1, 0, 0, 1, 1, 1, 1, "A", "C", "A", "C"},            // OK
		{32, 13, 2, 3, 3, 2, 2, 1, "DE", "D", "CDE", "CD"},      // left trim
		{48, 13, 2, 3, 3, 3, 1, 1, "D", "F", "CDE", "CFE"},      // left trim + right trim
		{48, 1, 0, 2, 6, 6, 1, 1, "C", "K", "aBCDEF", "aBKDEF"}, // left trim + right trim
		{0, 1, 0, 0, 1, 0, 1, 0, "A", "", "A", ""},              // OK
		{8, 1, 3, 2, 1, 0, 2, 1, "CD", "C", "D", ""},            // left extend
		{0, 1, 24, 24, 1, 2, 1, 2, "Y", "CK", "Y", "CK"},        // OK
		{2, 1, 0, 0, 1, 1, 1, 1, "A", "G", "G", "A"},            // swap
		{6, 1, 0, 0, 1, 1, 1, 1, "A", "C", "G", "T"},            // swap + flip
	}

	for _, v := range ndata {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			code, npos, nref, nalt, nsizeref, nsizealt := gref.NormalizeVariant(v.chrom, v.pos, v.ref, v.alt)
			require.Equal(t, v.ecode, code, "The return code is different, got: %#v expected %#v", code, v.ecode)
			require.Equal(t, v.epos, npos, "The POS value is different, got: %#v expected %#v", npos, v.epos)
			require.Equal(t, v.esizeref, nsizeref, "The REF size is different, got: %#v expected %#v", nsizeref, v.esizeref)
			require.Equal(t, v.esizealt, nsizealt, "The ALT size is different, got: %#v expected %#v", nsizealt, v.esizealt)
			require.Equal(t, v.eref, nref, "The REF is different, got: %#v expected %#v", nref, v.eref)
			require.Equal(t, v.ealt, nalt, "The ALT is different, got: %#v expected %#v", nalt, v.ealt)
		})
	}
}

func TestNormalizedVariantKey(t *testing.T) {
	t.Parallel()

	type TNormData struct {
		code     int
		chrom    string
		posindex uint8
		pos      uint32
		epos     uint32
		sizeref  uint8
		sizealt  uint8
		esizeref uint8
		esizealt uint8
		vk       uint64
		eref     string
		ealt     string
		ref      string
		alt      string
	}

	ndata := []TNormData{
		{-2, "1", 0, 26, 26, 1, 1, 1, 1, 0x0800000d08880000, "A", "C", "A", "C"},         // invalid position
		{-1, "1", 1, 1, 0, 1, 1, 1, 1, 0x08000000736a947f, "J", "C", "J", "C"},           // invalid reference
		{4, "1", 0, 0, 0, 1, 1, 1, 1, 0x0800000008880000, "A", "C", "T", "G"},            // flip
		{0, "1", 0, 0, 0, 1, 1, 1, 1, 0x0800000008880000, "A", "C", "A", "C"},            // OK
		{32, "13", 1, 3, 3, 3, 2, 2, 1, 0x68000001fed6a22d, "DE", "D", "CDE", "CD"},      // left trim
		{48, "13", 0, 2, 3, 3, 3, 1, 1, 0x68000001c7868961, "D", "F", "CDE", "CFE"},      // left trim + right trim
		{48, "1", 0, 0, 2, 6, 6, 1, 1, 0x0800000147df7d13, "C", "K", "aBCDEF", "aBKDEF"}, // left trim + right trim
		{0, "1", 0, 0, 0, 1, 0, 1, 0, 0x0800000008000000, "A", "", "A", ""},              // OK
		{8, "1", 0, 3, 2, 1, 0, 2, 1, 0x0800000150b13d0f, "CD", "C", "D", ""},            // left extend
		{0, "1", 1, 25, 24, 1, 2, 1, 2, 0x0800000c111ea6eb, "Y", "CK", "Y", "CK"},        // OK
		{2, "1", 0, 0, 0, 1, 1, 1, 1, 0x0800000008900000, "A", "G", "G", "A"},            // swap
		{6, "1", 1, 1, 0, 1, 1, 1, 1, 0x0800000008880000, "A", "C", "G", "T"},            // swap + flip
	}

	for _, v := range ndata {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			vk, code := gref.NormalizedVariantKey(v.chrom, v.pos, v.posindex, v.ref, v.alt)
			require.Equal(t, v.vk, vk, "The VK is different, got: %#v expected %#v", vk, v.vk)
			require.Equal(t, v.code, code, "The return code is different, got: %#v expected %#v", code, v.code)
		})
	}
}
