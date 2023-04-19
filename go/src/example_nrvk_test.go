package variantkey_test

import (
	"fmt"
	"log"

	vk "github.com/tecnickcom/variantkey/go/src"
)

func ExampleNRVKCols_FindRefAltByVariantKey() {
	nrvkmf, nrvk, err := vk.MmapNRVKFile(nrvkFile)
	if err != nil {
		log.Fatal(err)
	}

	defer nrvkmf.Close()

	ref, alt, sizeref, sizealt, ln := nrvk.FindRefAltByVariantKey(0x2000c3521f1c15ab)
	fmt.Println(ref, alt, sizeref, sizealt, ln)

	// Output:
	// ACGTACGT ACGT 8 4 12
}

func ExampleNRVKCols_ReverseVariantKey() {
	nrvkmf, nrvk, err := vk.MmapNRVKFile(nrvkFile)
	if err != nil {
		log.Fatal(err)
	}

	defer nrvkmf.Close()

	v, ln := nrvk.ReverseVariantKey(0x2000c3521f1c15ab)
	fmt.Println(v, ln)

	// Output:
	// {4 100004 ACGTACGT ACGT 8 4} 12
}

func ExampleNRVKCols_GetVariantKeyRefLength() {
	nrvkmf, nrvk, err := vk.MmapNRVKFile(nrvkFile)
	if err != nil {
		log.Fatal(err)
	}

	defer nrvkmf.Close()

	v := nrvk.GetVariantKeyRefLength(0x2000c3521f1c15ab)
	fmt.Println(v)

	// Output:
	// 8
}

func ExampleNRVKCols_GetVariantKeyEndPos() {
	nrvkmf, nrvk, err := vk.MmapNRVKFile(nrvkFile)
	if err != nil {
		log.Fatal(err)
	}

	defer nrvkmf.Close()

	v := nrvk.GetVariantKeyEndPos(0x2000c3521f1c15ab)
	fmt.Println(v)

	// Output:
	// 100012
}

func ExampleGetVariantKeyChromStartPos() {
	v := vk.GetVariantKeyChromStartPos(0x2000c3521f1c15ab)
	fmt.Println(v)

	// Output:
	// 1073841828
}

func ExampleNRVKCols_GetVariantKeyChromEndPos() {
	nrvkmf, nrvk, err := vk.MmapNRVKFile(nrvkFile)
	if err != nil {
		log.Fatal(err)
	}

	defer nrvkmf.Close()

	v := nrvk.GetVariantKeyChromEndPos(0x2000c3521f1c15ab)
	fmt.Println(v)

	// Output:
	// 1073841836
}
