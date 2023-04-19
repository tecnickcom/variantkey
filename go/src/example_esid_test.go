package variantkey_test

import (
	"fmt"

	vk "github.com/tecnickcom/variantkey/go/src"
)

func ExampleEncodeStringID() {
	v := vk.EncodeStringID("A0A022YWF9", 0)
	fmt.Println(v)

	// Output:
	// 12128340051199752601
}

func ExampleDecodeStringID() {
	v := vk.DecodeStringID(0xa850850492e77999)
	fmt.Println(v)

	// Output:
	// A0A022YWF9
}

func ExampleEncodeStringNumID() {
	v := vk.EncodeStringNumID("ABC:0000123456", ':')
	fmt.Println(v)

	// Output:
	// 15592178792074961472
}

func ExampleHashStringID() {
	v := vk.HashStringID("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")
	fmt.Println(v)

	// Output:
	// 12945031672818874332
}
