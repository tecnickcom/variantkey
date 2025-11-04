package variantkey

import (
	"testing"

	"github.com/stretchr/testify/require"
)

type testNonRevVKData struct {
	vk            uint64
	chrom         string
	pos           uint32
	len           uint32
	sizeRef       uint8
	sizeAlt       uint8
	chromStartPos uint64
	chromEndPos   uint64
	ref           string
	alt           string
}

func getTestNonRevVKData() []testNonRevVKData {
	return []testNonRevVKData{
		{0x0800c35093ace339, "1", 100001, 0x00000004, 0x01, 0x01, 0x00000000100186a1, 0x00000000100186a2, "N", "A"},
		{0x1000c3517f91cdb1, "2", 100002, 0x0000000e, 0x0b, 0x01, 0x00000000200186a2, 0x00000000200186ad, "AAGAAAGAAAG", "A"},
		{0x1800c351f61f65d3, "3", 100003, 0x0000000e, 0x01, 0x0b, 0x00000000300186a3, 0x00000000300186a4, "A", "AAGAAAGAAAG"},
		{0x2000c3521f1c15ab, "4", 100004, 0x0000000e, 0x08, 0x04, 0x00000000400186a4, 0x00000000400186ac, "ACGTACGT", "ACGT"},
		{0x2800c352d8f2d5b5, "5", 100005, 0x0000000e, 0x04, 0x08, 0x00000000500186a5, 0x00000000500186a9, "ACGT", "ACGTACGT"},
		{0x5000c3553bbf9c19, "10", 100010, 0x00000012, 0x08, 0x08, 0x00000000a00186aa, 0x00000000a00186b2, "ACGTACGT", "CGTACGTA"},
		{0xb000c35b64690b25, "22", 100022, 0x0000000b, 0x08, 0x01, 0x00000001600186b6, 0x00000001600186be, "ACGTACGT", "N"},
		{0xb800c35bbcece603, "X", 100023, 0x0000000e, 0x0a, 0x02, 0x00000001700186b7, 0x00000001700186c1, "AAAAAAAAGG", "AG"},
		{0xc000c35c63741ee7, "Y", 100024, 0x0000000e, 0x02, 0x0a, 0x00000001800186b8, 0x00000001800186ba, "AG", "AAAAAAAAGG"},
		{0xc800c35c96c18499, "MT", 100025, 0x00000012, 0x04, 0x0c, 0x00000001900186b9, 0x00000001900186bd, "ACGT", "AAACCCGGGTTT"},
	}
}

func TestFindRefAltByVariantKey(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestNonRevVKData() {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			ref, alt, sizeRef, sizeAlt, l := nrvk.FindRefAltByVariantKey(tt.vk)
			require.Equal(t, tt.ref, ref, "%d. Expected REF %s, got %s", i, tt.ref, ref)
			require.Equal(t, tt.alt, alt, "%d. Expected ALT %s, got %s", i, tt.alt, alt)
			require.Equal(t, tt.sizeRef, sizeRef, "%d. Expected REF size %d, got %d", i, tt.sizeRef, sizeRef)
			require.Equal(t, tt.sizeAlt, sizeAlt, "%d. Expected ALT size %d, got %d", i, tt.sizeAlt, sizeAlt)
			require.Equal(t, (tt.len - 2), l, "%d. Expected len %d, got %d", i, (tt.len - 2), l)
		})
	}
}

func BenchmarkFindRefAltByVariantKey(b *testing.B) {
	for b.Loop() {
		nrvk.FindRefAltByVariantKey(0xb000c35b64690b25)
	}
}

func TestNRReverseVariantKey(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestNonRevVKData() {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			rev, l := nrvk.ReverseVariantKey(tt.vk)
			require.Equal(t, tt.chrom, rev.Chrom, "%d. Expected CHROM %s, got %s", i, tt.chrom, rev.Chrom)
			require.Equal(t, tt.pos, rev.Pos, "%d. Expected POS size %d, got %d", i, tt.pos, rev.Pos)
			require.Equal(t, tt.ref, rev.Ref, "%d. Expected REF %s, got %s", i, tt.ref, rev.Ref)
			require.Equal(t, tt.alt, rev.Alt, "%d. Expected ALT %s, got %s", i, tt.alt, rev.Alt)
			require.Equal(t, tt.sizeRef, rev.SizeRef, "%d. Expected REF size %d, got %d", i, tt.sizeRef, rev.SizeRef)
			require.Equal(t, tt.sizeAlt, rev.SizeAlt, "%d. Expected ALT size %d, got %d", i, tt.sizeAlt, rev.SizeAlt)
			require.Equal(t, (tt.len - 2), l, "%d. Expected len %d, got %d", i, (tt.len - 2), l)
		})
	}
}

func BenchmarkNRReverseVariantKey(b *testing.B) {
	for b.Loop() {
		nrvk.ReverseVariantKey(0xb000c35b64690b25)
	}
}

func TestGetVariantKeyRefLength(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestNonRevVKData() {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			sizeRef := nrvk.GetVariantKeyRefLength(tt.vk)
			require.Equal(t, tt.sizeRef, sizeRef, "%d. Expected REF size %d, got %d", i, tt.sizeRef, sizeRef)
		})
	}
}

func TestGetVariantKeyRefLengthReversible(t *testing.T) {
	t.Parallel()

	sizeRef := nrvk.GetVariantKeyRefLength(0x1800925199160000)
	require.Equal(t, uint8(3), sizeRef, "Expected REF size 3, got %d", sizeRef)
}

func TestGetVariantKeyRefLengthNotFound(t *testing.T) {
	t.Parallel()

	sizeRef := nrvk.GetVariantKeyRefLength(0xffffffffffffffff)
	require.Equal(t, uint8(0), sizeRef, "Expected REF size 0, got %d", sizeRef)
}

func TestGetVariantKeyEndPos(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestNonRevVKData() {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			endpos := nrvk.GetVariantKeyEndPos(tt.vk)
			exp := tt.pos + uint32(tt.sizeRef)
			require.Equal(t, exp, endpos, "%d. Expected END POS %d, got %d", i, exp, endpos)
		})
	}
}

func TestGetVariantKeyChromStartPos(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestNonRevVKData() {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			res := GetVariantKeyChromStartPos(tt.vk)
			require.Equal(t, tt.chromStartPos, res, "%d. Expected CHROM + START POS %d, got %d", i, tt.chromStartPos, res)
		})
	}
}

func TestGetVariantKeyChromEndPos(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestNonRevVKData() {
		t.Run("", func(t *testing.T) {
			t.Parallel()

			res := nrvk.GetVariantKeyChromEndPos(tt.vk)
			require.Equal(t, tt.chromEndPos, res, "%d. Expected CHROM + END POS %d, got %d", i, tt.chromEndPos, res)
		})
	}
}

func TestVknrBinToTSV(t *testing.T) {
	t.Parallel()

	l := nrvk.VknrBinToTSV("nrvk.test")
	require.Equal(t, uint64(305), l, "Expected file length 305, got %d", l)
}
