package variantkey_test

import (
	"fmt"
	"log"

	vk "github.com/tecnickcom/variantkey/go/src"
)

func ExampleRSIDVARCols_FindRVVariantKeyByRsid() {
	rvmf, rv, err := vk.MmapRSVKFile(rvFile, []uint8{})
	if err != nil {
		log.Fatal(err)
	}

	defer rvmf.Close()

	vk, first := rv.FindRVVariantKeyByRsid(0, 9, 0x00000061)
	fmt.Println(vk, first)

	// Output:
	// 9223656209074749440 3
}

func ExampleRSIDVARCols_GetNextRVVariantKeyByRsid() {
	rvmf, rv, err := vk.MmapRSVKFile(rvFile, []uint8{})
	if err != nil {
		log.Fatal(err)
	}

	defer rvmf.Close()

	vk, first := rv.GetNextRVVariantKeyByRsid(2, 9, 0x00000061)
	fmt.Println(vk, first)

	// Output:
	// 9223656209074749440 3
}

func ExampleRSIDVARCols_FindAllRVVariantKeyByRsid() {
	rvmmf, rvm, err := vk.MmapRSVKFile(rvmFile, []uint8{})
	if err != nil {
		log.Fatal(err)
	}

	defer rvmmf.Close()

	v := rvm.FindAllRVVariantKeyByRsid(0, 9, 0x00000003)
	fmt.Println(v)

	// Output:
	// [9223656209074749440 9223656316446408704 9223656367992733696]
}

func ExampleRSIDVARCols_FindVRRsidByVariantKey() {
	vrmf, vr, err := vk.MmapVKRSFile(vkrsFile, []uint8{})
	if err != nil {
		log.Fatal(err)
	}

	defer vrmf.Close()

	rsid, first := vr.FindVRRsidByVariantKey(0, 9, 0x80010274003A0000)
	fmt.Println(rsid, first)

	// Output:
	// 97 3
}

func ExampleRSIDVARCols_GetNextVRRsidByVariantKey() {
	vrmf, vr, err := vk.MmapVKRSFile(vkrsFile, []uint8{})
	if err != nil {
		log.Fatal(err)
	}

	defer vrmf.Close()

	rsid, first := vr.GetNextVRRsidByVariantKey(2, 9, 0x80010274003A0000)
	fmt.Println(rsid, first)

	// Output:
	// 97 3
}

func ExampleRSIDVARCols_FindAllVRRsidByVariantKey() {
	vrmf, vr, err := vk.MmapVKRSFile(vkrsFile, []uint8{})
	if err != nil {
		log.Fatal(err)
	}

	defer vrmf.Close()

	v := vr.FindAllVRRsidByVariantKey(0, 9, 0x80010274003A0000)
	fmt.Println(v)

	// Output:
	// [97]
}

func ExampleRSIDVARCols_FindVRChromPosRange() {
	vrmf, vr, err := vk.MmapVKRSFile(vkrsFile, []uint8{})
	if err != nil {
		log.Fatal(err)
	}

	defer vrmf.Close()

	rsid, first, last := vr.FindVRChromPosRange(0, 9, 0x14, 0x000256C5, 0x000256CB)
	fmt.Println(rsid, first, last)

	// Output:
	// 9973 7 9
}
