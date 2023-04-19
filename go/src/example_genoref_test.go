package variantkey_test

import (
	"fmt"
	"log"

	vk "github.com/tecnickcom/variantkey/go/src"
)

func ExampleTMMFile_GetGenorefSeq() {
	gref, err := vk.MmapGenorefFile(genorefFile)
	if err != nil {
		log.Fatal(err)
	}

	defer gref.Close()

	v := gref.GetGenorefSeq(23, 0)
	fmt.Println(v)

	// Output:
	// 65
}

func ExampleTMMFile_CheckReference() {
	gref, err := vk.MmapGenorefFile(genorefFile)
	if err != nil {
		log.Fatal(err)
	}

	defer gref.Close()

	v := gref.CheckReference(23, 0, "A")
	fmt.Println(v)

	// Output:
	// 0
}

func ExampleFlipAllele() {
	v := vk.FlipAllele("ATCGMKRYBVDHWSNatcgmkrybvdhwsn")
	fmt.Println(v)

	// Output:
	// TAGCKMYRVBHDWSNTAGCKMYRVBHDWSN
}

func ExampleTMMFile_NormalizeVariant() {
	gref, err := vk.MmapGenorefFile(genorefFile)
	if err != nil {
		log.Fatal(err)
	}

	defer gref.Close()

	code, npos, nref, nalt, nsizeref, nsizealt := gref.NormalizeVariant(13, 2, "CDE", "CFE")
	fmt.Println(code, npos, nref, nalt, nsizeref, nsizealt)

	// Output:
	// 48 3 D F 1 1
}

func ExampleTMMFile_NormalizedVariantKey() {
	gref, err := vk.MmapGenorefFile(genorefFile)
	if err != nil {
		log.Fatal(err)
	}

	defer gref.Close()

	vk, code := gref.NormalizedVariantKey("13", 2, 0, "CDE", "CFE")
	fmt.Println(vk, code)

	// Output:
	// 7493989787586955617 48
}
