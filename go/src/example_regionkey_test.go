package variantkey_test

import (
	"fmt"
	"log"

	vk "github.com/tecnickcom/variantkey/go/src"
)

func ExampleEncodeRegionStrand() {
	v := vk.EncodeRegionStrand(-1)
	fmt.Println(v)

	// Output:
	// 2
}

func ExampleDecodeRegionStrand() {
	v := vk.DecodeRegionStrand(2)
	fmt.Println(v)

	// Output:
	// -1
}

func ExampleEncodeRegionKey() {
	v := vk.EncodeRegionKey(25, 1000, 2000, 2)
	fmt.Println(v)

	// Output:
	// 14411520955069251204
}

func ExampleExtractRegionKeyChrom() {
	v := vk.ExtractRegionKeyChrom(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// 25
}

func ExampleExtractRegionKeyStartPos() {
	v := vk.ExtractRegionKeyStartPos(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// 1000
}

func ExampleExtractRegionKeyEndPos() {
	v := vk.ExtractRegionKeyEndPos(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// 2000
}

func ExampleExtractRegionKeyStrand() {
	v := vk.ExtractRegionKeyStrand(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// 2
}

func ExampleDecodeRegionKey() {
	v := vk.DecodeRegionKey(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// {25 1000 2000 2}
}

func ExampleReverseRegionKey() {
	v := vk.ReverseRegionKey(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// {MT 1000 2000 -1}
}

func ExampleRegionKey() {
	v := vk.RegionKey("MT", 1000, 2000, -1)
	fmt.Println(v)

	// Output:
	// 14411520955069251204
}

func ExampleExtendRegionKey() {
	v := vk.ExtendRegionKey(14411520955069251204, 100)
	fmt.Println(v)

	// Output:
	// 14411520740320887204
}

func ExampleGetRegionKeyChromStartPos() {
	v := vk.GetRegionKeyChromStartPos(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// 6710887400
}

func ExampleGetRegionKeyChromEndPos() {
	v := vk.GetRegionKeyChromEndPos(0xc80001f400003e84)
	fmt.Println(v)

	// Output:
	// 6710888400
}

func ExampleAreOverlappingRegions() {
	v := vk.AreOverlappingRegions(5, 4, 6, 5, 3, 7)
	fmt.Println(v)

	// Output:
	// true
}

func ExampleAreOverlappingRegionRegionKey() {
	v := vk.AreOverlappingRegionRegionKey(5, 4, 6, 0x2800000180000038)
	fmt.Println(v)

	// Output:
	// true
}

func ExampleAreOverlappingRegionKeys() {
	v := vk.AreOverlappingRegionKeys(0x2800000200000030, 0x2800000180000038)
	fmt.Println(v)

	// Output:
	// true
}

func ExampleNRVKCols_AreOverlappingVariantKeyRegionKey() {
	nrvkmf, nrvk, err := vk.MmapNRVKFile(nrvkFile)
	if err != nil {
		log.Fatal(err)
	}

	defer nrvkmf.Close()

	v := nrvk.AreOverlappingVariantKeyRegionKey(0x2800000210920000, 0x2800000180000038)
	fmt.Println(v)

	// Output:
	// true
}

func ExampleNRVKCols_VariantToRegionkey() {
	nrvkmf, nrvk, err := vk.MmapNRVKFile(nrvkFile)
	if err != nil {
		log.Fatal(err)
	}

	defer nrvkmf.Close()

	v := nrvk.VariantToRegionkey(0x2800000210920000)
	fmt.Println(v)

	// Output:
	// 2882303770107052080
}
