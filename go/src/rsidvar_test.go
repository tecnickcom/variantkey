package variantkey

import (
	"testing"

	"github.com/stretchr/testify/require"
)

type testData = struct {
	chrom  uint8
	pos    uint32
	refalt uint32
	rsid   uint32
	vk     uint64
}

func getTestData() []testData {
	return []testData{
		{0x01, 0x0004F44B, 0x00338000, 0x00000001, 0x08027A2580338000},
		{0x09, 0x000143FC, 0x439E3918, 0x00000007, 0x4800A1FE439E3918},
		{0x09, 0x000143FC, 0x7555EB16, 0x0000000B, 0x4800A1FE7555EB16},
		{0x10, 0x000204E8, 0x003A0000, 0x00000061, 0x80010274003A0000},
		{0x10, 0x0002051A, 0x00138000, 0x00000065, 0x8001028D00138000},
		{0x10, 0x00020532, 0x007A0000, 0x000003E5, 0x80010299007A0000},
		{0x14, 0x000256C4, 0x003A0000, 0x000003F1, 0xA0012B62003A0000},
		{0x14, 0x000256C5, 0x00708000, 0x000026F5, 0xA0012B6280708000},
		{0x14, 0x000256CB, 0x63256692, 0x000186A3, 0xA0012B65E3256692},
		{0x14, 0x000256CF, 0x55439803, 0x00019919, 0xA0012B67D5439803},
	}
}

func TestFindRVVariantKeyByRsid(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestData() {
		i := i
		tt := tt

		t.Run("", func(t *testing.T) {
			t.Parallel()

			vk, first := rv.FindRVVariantKeyByRsid(0, rv.NRows, tt.rsid)
			require.Equal(t, uint64(i), first, "Expected first %d, got %d", i, first)
			require.Equal(t, tt.vk, vk, "Expected VariantKey %x, got %x", tt.vk, vk)
		})
	}
}

func TestFindRVVariantKeyByRsidNotFound(t *testing.T) {
	t.Parallel()

	vk, first := rv.FindRVVariantKeyByRsid(0, rv.NRows, 0xfffffff0)
	require.Equal(t, uint64(9), first, "Expected first 10, got %d", first)
	require.Equal(t, uint64(0), vk, "ExpectedVariantKey 0, got %x", vk)
}

func BenchmarkFindRVVariantKeyByRsid(b *testing.B) {
	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		rv.FindRVVariantKeyByRsid(0, 9, 0x2F81F575)
	}
}

func TestGetNextRVVariantKeyByRsid(t *testing.T) {
	t.Parallel()

	var vk uint64

	pos := uint64(2)

	vk, pos = rv.GetNextRVVariantKeyByRsid(pos, rv.NRows, 0x00000061)
	require.Equal(t, uint64(3), pos, "(1) Expected pos 3, got %d", pos)
	require.Equal(t, uint64(0x80010274003A0000), vk, "(1) Expected VariantKey 0x80010274003A0000, got %x", vk)
	vk, pos = rv.GetNextRVVariantKeyByRsid(pos, rv.NRows, 0x00000061)
	require.Equal(t, uint64(4), pos, "(2) Expected pos 4, got %d", pos)
	require.Equal(t, uint64(0), vk, "(2) Expected VariantKey 0, got %x", vk)
}

func TestFindAllRVVariantKeyByRsid(t *testing.T) {
	t.Parallel()

	vks := rvm.FindAllRVVariantKeyByRsid(0, rvm.NRows, 0x00000003)
	require.Len(t, vks, 3)
	require.Equal(t, uint64(0x80010274003A0000), vks[0], "Expected VariantKey 0x80010274003A0000, got %x", vks[0])
	require.Equal(t, uint64(0x8001028D00138000), vks[1], "Expected VariantKey 0x8001028D00138000, got %x", vks[1])
	require.Equal(t, uint64(0x80010299007A0000), vks[2], "Expected VariantKey 0x80010299007A0000, got %x", vks[2])
}

func TestFindAllRVVariantKeyByRsidNotFound(t *testing.T) {
	t.Parallel()

	vks := rvm.FindAllRVVariantKeyByRsid(0, rvm.NRows, 0x12345678)
	require.Empty(t, vks)
}

func TestFindVRRsidByVariantKey(t *testing.T) {
	t.Parallel()

	for i, tt := range getTestData() {
		i := i
		tt := tt

		t.Run("", func(t *testing.T) {
			t.Parallel()

			rsid, first := vr.FindVRRsidByVariantKey(0, vr.NRows, tt.vk)
			require.Equal(t, tt.rsid, rsid, "%d. Expected rsid %x, got %x", i, tt.rsid, rsid)
			require.Equal(t, uint64(i), first, "%d. Expected first %d, got %d", i, i, first)
		})
	}
}

func TestFindVRRsidByVariantKeyNotFound(t *testing.T) {
	t.Parallel()

	rsid, first := vr.FindVRRsidByVariantKey(0, vr.NRows, 0xfffffffffffffff0)
	require.Equal(t, uint32(0), rsid, "Expected rsid 0, got %x", rsid)
	require.Equal(t, uint64(9), first, "Expected first 9, got %d", first)
}

func BenchmarkFindVRRsidByVariantKey(b *testing.B) {
	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		vr.FindVRRsidByVariantKey(0, vr.NRows, 0x160017CCA313D0E0)
	}
}

func TestGetNextVRRsidByVariantKey(t *testing.T) {
	t.Parallel()

	var rsid uint32

	pos := uint64(2)

	rsid, pos = vr.GetNextVRRsidByVariantKey(pos, vr.NRows, 0x80010274003A0000)
	require.Equal(t, uint64(3), pos, "(1) Expected pos 3, got %d", pos)
	require.Equal(t, uint32(97), rsid, "(1) Expected rsID 97, got %d", rsid)

	rsid, pos = vr.GetNextVRRsidByVariantKey(pos, vr.NRows, 0x80010274003A0000)
	require.Equal(t, uint64(4), pos, "(2) Expected pos 4, got %d", pos)
	require.Equal(t, uint32(0), rsid, "(2) Expected rsID 0, got %d", rsid)
}

func TestFindAllVRRsidByVariantKey(t *testing.T) {
	t.Parallel()

	rsid := vr.FindAllVRRsidByVariantKey(0, vr.NRows, 0x80010274003A0000)
	require.Len(t, rsid, 1)
	require.Equal(t, uint32(97), rsid[0], "Expected rsID 97, got %d", rsid[0])
}

func TestFindVRChromPosRange(t *testing.T) {
	t.Parallel()

	data := getTestData()

	rsid, first, last := vr.FindVRChromPosRange(0, vr.NRows, data[6].chrom, data[7].pos, data[8].pos)
	require.Equal(t, data[7].rsid, rsid, "Expected rsid %x, got %x", data[7].rsid, rsid)
	require.Equal(t, uint64(7), first, "Expected first 4, got %d", first)
	require.Equal(t, uint64(9), last, "Expected last 4, got %d", last)
}

func TestFindVRChromPosRangeNotFound(t *testing.T) {
	t.Parallel()

	rsid, first, last := vr.FindVRChromPosRange(0, vr.NRows, 0xff, 0xffffff00, 0xfffffff0)
	require.Equal(t, uint32(0), rsid, "Expected rsid 0, got %x", rsid)
	require.Equal(t, uint64(10), first, "Expected first 10, got %d", first)
	require.Equal(t, uint64(10), last, "Expected last 10, got %d", last)
}

func BenchmarkFindVRChromPosRange(b *testing.B) {
	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		vr.FindVRChromPosRange(0, vr.NRows, 0x19, 0x001AF8FD, 0x001C8F2A)
	}
}
