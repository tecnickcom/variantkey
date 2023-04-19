package variantkey_test

import (
	"fmt"

	vk "github.com/tecnickcom/variantkey/go/src"
)

func ExampleEncodeChrom() {
	v := vk.EncodeChrom("X")
	fmt.Println(v)

	// Output:
	// 23
}

func ExampleDecodeChrom() {
	v := vk.DecodeChrom(23)
	fmt.Println(v)

	// Output:
	// X
}

func ExampleEncodeRefAlt() {
	v := vk.EncodeRefAlt("AC", "GT")
	fmt.Println(v)

	// Output:
	// 286097408
}

func ExampleDecodeRefAlt() {
	ref, alt, sizeref, csizealt, ln := vk.DecodeRefAlt(286097408)
	fmt.Println(ref, alt, sizeref, csizealt, ln)

	// Output:
	// AC GT 2 2 4
}

func ExampleEncodeVariantKey() {
	v := vk.EncodeVariantKey(23, 12345, 286097408)
	fmt.Println(v)

	// Output:
	// 13258623813950472192
}

func ExampleExtractVariantKeyChrom() {
	v := vk.ExtractVariantKeyChrom(13258623813950472192)
	fmt.Println(v)

	// Output:
	// 23
}

func ExampleExtractVariantKeyPos() {
	v := vk.ExtractVariantKeyPos(13258623813950472192)
	fmt.Println(v)

	// Output:
	// 12345
}

func ExampleExtractVariantKeyRefAlt() {
	v := vk.ExtractVariantKeyRefAlt(13258623813950472192)
	fmt.Println(v)

	// Output:
	// 286097408
}

func ExampleDecodeVariantKey() {
	v := vk.DecodeVariantKey(13258623813950472192)
	fmt.Println(v)

	// Output:
	// {23 12345 286097408}
}

func ExampleVariantKey() {
	v := vk.VariantKey("X", 12345, "AC", "GT")
	fmt.Println(v)

	// Output:
	// 13258623813950472192
}

func ExampleRange() {
	v := vk.Range(23, 1234, 5678)
	fmt.Println(v)

	// Output:
	// {13258599952973561856 13258609498538377215}
}

func ExampleCompareVariantKeyChrom() {
	v := vk.CompareVariantKeyChrom(13258599952973561856, 13258609498538377215)
	fmt.Println(v)

	// Output:
	// 0
}

func ExampleCompareVariantKeyChromPos() {
	v := vk.CompareVariantKeyChromPos(13258599952973561856, 13258609498538377215)
	fmt.Println(v)

	// Output:
	// -1
}

func ExampleHex() {
	v := vk.Hex(13258623813950472192)
	fmt.Println(v)

	// Output:
	// b800181c910d8000
}

func ExampleParseHex() {
	v := vk.ParseHex("b800181c910d8000")
	fmt.Println(v)

	// Output:
	// 13258623813950472192
}
