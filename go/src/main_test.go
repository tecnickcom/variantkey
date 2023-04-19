package variantkey

import (
	"os"
	"testing"

	"github.com/stretchr/testify/require"
)

const (
	// RSIDVAR
	// Lookup table for rsID to VariantKey.
	// The input binary file can be generated using the resources/tools/rsvk.sh script.
	// This example uses the "c/test/data/rsvk.10.bin".
	rvFile = "../../c/test/data/rsvk.10.bin"

	// RSIDVAR
	// Lookup table for rsID to VariantKey.
	// The input binary file can be generated using the resources/tools/rsvk.sh script.
	// This example uses the "c/test/data/rsvk.m.10.bin".
	rvmFile = "../../c/test/data/rsvk.m.10.bin"

	// RSIDVAR
	// Lookup table for VariantKey ro rsID
	// The input binary file can be generated using the resources/tools/vkrs.sh script.
	// This example uses the "c/test/data/vkrs.10.bin".
	vkrsFile = "../../c/test/data/vkrs.10.bin"

	// NRVK
	// Lookup table for non-reversible variantkeys.
	// The input binary files can be generated from a normalized VCF file using the resources/tools/nrvk.sh script.
	// The VCF file can be normalized using the `resources/tools/vcfnorm.sh` script.
	// This example uses the "c/test/data/nrvk.10.bin".
	nrvkFile = "../../c/test/data/nrvk.10.bin"

	// GENOREF
	// Reference genome binary file.
	// The input reference binary files can be generated from a FASTA file using the resources/tools/fastabin.sh script.
	genorefFile = "../../c/test/data/genoref.bin"
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

	// memory map the input files.

	// RSIDVAR
	// Load the lookup table for rsID to VariantKey.
	// The input binary file can be generated using the resources/tools/rsvk.sh script.
	rvmf, rv, err = MmapRSVKFile(rvFile, []uint8{})
	if err != nil {
		defer func() { os.Exit(1) }()
		return
	}

	defer closeTMMFile(rvmf)

	// RSIDVAR
	// Load the lookup table for rsID to VariantKey.
	// The input binary file can be generated using the resources/tools/rsvk.sh script.
	rvmmf, rvm, err = MmapRSVKFile(rvmFile, []uint8{})
	if err != nil {
		defer func() { os.Exit(2) }()
		return
	}

	defer closeTMMFile(rvmmf)

	// RSIDVAR
	// Load the lookup table for VariantKey ro rsID
	// The input binary file can be generated using the resources/tools/vkrs.sh script.
	vrmf, vr, err = MmapVKRSFile(vkrsFile, []uint8{})
	if err != nil {
		defer func() { os.Exit(3) }()
		return
	}

	defer closeTMMFile(vrmf)

	// NRVK
	// Load the lookup table for non-reversible variantkeys.
	// The input binary files can be generated from a normalized VCF file using the resources/tools/nrvk.sh script.
	// The VCF file can be normalized using the `resources/tools/vcfnorm.sh` script.
	nrvkmf, nrvk, err = MmapNRVKFile(nrvkFile)
	if err != nil {
		defer func() { os.Exit(4) }()
		return
	}

	defer closeTMMFile(nrvkmf)

	// GENOREF
	// Load the reference genome binary file.
	// The input reference binary files can be generated from a FASTA file using the resources/tools/fastabin.sh script.
	gref, err = MmapGenorefFile(genorefFile)
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
