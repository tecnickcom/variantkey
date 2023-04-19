package variantkey_test

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
