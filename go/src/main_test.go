package variantkey

import (
	"os"
	"testing"

	"github.com/stretchr/testify/require"
)

//nolint:gochecknoglobals
var (
	gref, rvmf, rvmmf, vrmf, nrvkmf TMMFile
	rv, rvm, vr                     RSIDVARCols
	nrvk                            NRVKCols
	retCode                         int
)

func closeTMMFile(mmf TMMFile) {
	err := mmf.Close()
	if err != nil {
		retCode++
	}
}

func TestMain(m *testing.M) {
	var err error

	// memory map the input files

	rvmf, rv, err = MmapRSVKFile("../../c/test/data/rsvk.10.bin", []uint8{})
	if err != nil {
		defer func() { os.Exit(1) }()
		return
	}

	defer closeTMMFile(rvmf)

	rvmmf, rvm, err = MmapRSVKFile("../../c/test/data/rsvk.m.10.bin", []uint8{})
	if err != nil {
		defer func() { os.Exit(2) }()
		return
	}

	defer closeTMMFile(rvmmf)

	vrmf, vr, err = MmapVKRSFile("../../c/test/data/vkrs.10.bin", []uint8{})
	if err != nil {
		defer func() { os.Exit(3) }()
		return
	}

	defer closeTMMFile(vrmf)

	nrvkmf, nrvk, err = MmapNRVKFile("../../c/test/data/nrvk.10.bin")
	if err != nil {
		defer func() { os.Exit(4) }()
		return
	}

	defer closeTMMFile(nrvkmf)

	gref, err = MmapGenorefFile("../../c/test/data/genoref.bin")
	if err != nil {
		defer func() { os.Exit(5) }()
		return
	}

	defer closeTMMFile(gref)

	retCode += m.Run()

	defer func() { os.Exit(retCode) }()
}

func TestClose(t *testing.T) {
	t.Parallel()

	lmf, err := MmapGenorefFile("../../c/test/data/test_data.bin")
	require.NoError(t, err)

	err = lmf.Close()
	require.NoError(t, err)
}

func TestCloseError(t *testing.T) {
	t.Parallel()

	lmf := TMMFile{}
	err := lmf.Close()
	require.Error(t, err)
}

func TestMmapRSVKFileError(t *testing.T) {
	t.Parallel()

	_, _, err := MmapRSVKFile("error", []uint8{1})
	require.Error(t, err)
}

func TestMmapVKRSFileError(t *testing.T) {
	t.Parallel()

	_, _, err := MmapVKRSFile("error", []uint8{1})
	require.Error(t, err)
}

func TestMmapNRVKFileError(t *testing.T) {
	t.Parallel()

	_, _, err := MmapNRVKFile("error")
	require.Error(t, err)
}

func TestMmapGenorefFileError(t *testing.T) {
	t.Parallel()

	_, err := MmapGenorefFile("error")
	require.Error(t, err)
}
