/** VariantKey Javascript Library
 * 
 * variantkey.js
 * 
 * @category   Tools
 * @author     Nicola Asuni <info@tecnick.com>
 * @link       https://github.com/tecnickcom/variantkey
 * @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)
 */

function encodeChrom(chrom) {
    chrom = chrom.replace(/^chr/i, '');
    const clen = chrom.length;
    if (clen == 0) {
        return 0;
    }
    // X > 23 ; Y > 24 ; M > 25
    const onecharmap = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*                                     M                                 X   Y */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 24, 0, 0, 0, 0, 0, 0,
        /*                                     m                                 x   y */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 24, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    var c = chrom.charAt(0);
    if ((c <= '9') && (c >= '0')) {
        var i;
        var v = (c - '0');
        for (i = 1; i < clen; i++) {
            c = chrom.charAt(i);
            if ((c > '9') || (c < '0')) {
                return 0; // NA
            }
            // The C encode_numeric_chrom() accumulates in a uint8_t, so the
            // value wraps at 256: the mask reproduces the same wrapping.
            v = (((v * 10) + (c - '0')) & 0xFF);
        }
        return v;
    }
    if ((clen == 1) || ((clen == 2) && ((chrom.charAt(1) == 'T') || (chrom.charAt(1) == 't')))) {
        return onecharmap[chrom.charCodeAt(0)];
    }
    return 0; // NA
}

function decodeChrom(code) {
    if ((code < 1) || (code > 25)) {
        return 'NA';
    }
    if (code < 23) {
        return code.toString();
    }
    const map = ['X', 'Y', 'MT'];
    return map[(code - 23)];
}

function encodeBase(c) {
    /*
      Encode base:

      A > 0
      C > 1
      G > 2
      T > 3
    */
    const map = [
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* A     C           G                                      T */
        4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        /* a     c           g                                      t */
        4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    ];
    return map[c.charCodeAt(0)];
}

function encodeAllele(h, bitpos, str, size) {
    var v;
    for (var i = 0; i < size; i++) {
        v = encodeBase(str.charAt(i));
        if (v > 3) {
            return -1;
        }
        bitpos -= 2;
        h |= (v << bitpos);
    }
    return h;
}

function encodeRefAltRev(ref, sizeref, alt, sizealt) {
    var h = 0 >>> 0;
    h |= (sizeref << 27); // RRRR: length of REF
    h |= (sizealt << 23); // AAAA: length of ALT
    h = encodeAllele(h, 23, ref, sizeref);
    if (h < 0) {
        return 0xffffffff;
    }
    h = encodeAllele(h, (23 - (2 * sizeref)), alt, sizealt);
    if (h < 0) {
        return 0xffffffff;
    }
    return h >>> 0;
}

// Mix two 32 bit hash numbers using the MurmurHash3 algorithm
function muxHash(k, h) {
    k = ((((k & 0xffff) * 0xcc9e2d51) + ((((k >>> 16) * 0xcc9e2d51) & 0xffff) << 16))) & 0xffffffff;
    k = ((k << 15) | (k >>> 17));
    k = ((((k & 0xffff) * 0x1b873593) + ((((k >>> 16) * 0x1b873593) & 0xffff) << 16))) & 0xffffffff;
    h ^= k;
    h = ((h << 13) | (h >>> 19));
    var hb = ((((h & 0xffff) * 5) + ((((h >>> 16) * 5) & 0xffff) << 16))) & 0xffffffff;
    h = (((hb & 0xffff) + 0x6b64) + ((((hb >>> 16) + 0xe654) & 0xffff) << 16));
    return h >>> 0;
}

function encodePackChar(c) {
    if ((c < 65) || (c > 127)) {
        return (27 >>> 0);
    }
    if (c >= 96) {
        return ((c - 96) >>> 0);
    }
    return ((c - 64) >>> 0);
}

function packCharsTail(str, size, offset) {
    var h = (0 >>> 0);
    offset += (size - 1);
    switch (size) {
        case 5:
            h ^= encodePackChar(str.charCodeAt(offset--)) << 6;
            // fall through
        case 4:
            h ^= encodePackChar(str.charCodeAt(offset--)) << 11;
            // fall through
        case 3:
            h ^= encodePackChar(str.charCodeAt(offset--)) << 16;
            // fall through
        case 2:
            h ^= encodePackChar(str.charCodeAt(offset--)) << 21;
            // fall through
        case 1:
            h ^= encodePackChar(str.charCodeAt(offset)) << 26;
    }
    return h;
}

function packChars(str, offset) {
    return ((encodePackChar(str.charCodeAt(offset + 5)) << 1) ^
        (encodePackChar(str.charCodeAt(offset + 4)) << 6) ^
        (encodePackChar(str.charCodeAt(offset + 3)) << 11) ^
        (encodePackChar(str.charCodeAt(offset + 2)) << 16) ^
        (encodePackChar(str.charCodeAt(offset + 1)) << 21) ^
        (encodePackChar(str.charCodeAt(offset)) << 26));
}

// Return a 32 bit hash of a nucleotide string
function hash32(str, size) {
    var h = 0;
    var len = 6;
    var offset = 0;
    while (size >= len) {
        h = muxHash(packChars(str, offset), h);
        size -= len;
        offset += len;
    }
    if (size > 0) {
        h = muxHash(packCharsTail(str, size, offset), h);
    }
    return h;
}

function encodeRefAltHash(ref, sizeref, alt, sizealt) {
    // 0x3 is the separator character between REF and ALT [00000000 00000000 00000000 00000011]
    var h = muxHash(hash32(alt, sizealt), muxHash(0x3, hash32(ref, sizeref)));
    // finalization mix - MurmurHash3 algorithm
    h ^= h >>> 16;
    h = (((h & 0xffff) * 0x85ebca6b) + ((((h >>> 16) * 0x85ebca6b) & 0xffff) << 16)) & 0xffffffff;
    h ^= h >>> 13;
    h = ((((h & 0xffff) * 0xc2b2ae35) + ((((h >>> 16) * 0xc2b2ae35) & 0xffff) << 16))) & 0xffffffff;
    h ^= h >>> 16;
    return ((h >>> 1) | 0x1); // 0x1 is the set bit to indicate HASH mode [00000000 00000000 00000000 00000001]
}

function encodeRefAlt(ref, alt) {
    const sizeref = ref.length >>> 0;
    const sizealt = alt.length >>> 0;
    // The length of a single allele is checked as well: the length fields hold
    // up to 11, but decodeRefAlt only reverses the codes whose alleles are
    // within 10 bases.
    if (((sizeref + sizealt) <= 11) && (sizeref <= 10) && (sizealt <= 10)) {
        var h = encodeRefAltRev(ref, sizeref, alt, sizealt);
        if (h != 0xffffffff) {
            return h >>> 0;
        }
    }
    return encodeRefAltHash(ref, sizeref, alt, sizealt) >>> 0;
}

function decodeBase(code, bitpos) {
    const base = ['A', 'C', 'G', 'T'];
    return base[((code >> bitpos) & 0x3)]; // 0x3 is the 2 bit mask [00000011]
}

function decodeRefAltRev(code) {
    code >>>= 0;
    const sizeref = ((code & 0x78000000) >>> 27); // [01111000 00000000 00000000 00000000]
    const sizealt = ((code & 0x07800000) >>> 23); // [00000111 10000000 00000000 00000000]
    var bitpos = 23;
    var ref = "";
    var alt = "";
    for (var i = 0; i < sizeref; i++) {
        bitpos -= 2;
        ref += decodeBase(code, bitpos);
    }
    for (var i = 0; i < sizealt; i++) {
        bitpos -= 2;
        alt += decodeBase(code, bitpos);
    }
    return {
        "ref": ref,
        "alt": alt
    };
}

function decodeRefAlt(code) {
    code >>>= 0;
    if (code & 0x1) // check last bit
    {
        return {
            "ref": "",
            "alt": ""
        }; // non-reversible encoding
    }
    // Codes whose length fields are outside the reversible range are rejected,
    // as in the C decode_refalt().
    const lenref = ((code & 0x78000000) >>> 27);
    const lenalt = ((code & 0x07800000) >>> 23);
    if ((lenref > 10) || (lenalt > 10) || ((lenref + lenalt) > 11)) {
        return {
            "ref": "",
            "alt": ""
        }; // the length fields are out of the reversible range
    }
    return decodeRefAltRev(code);
}

function encodeVariantKey(chrom, pos, refalt) {
    return {
        "hi": ((((chrom >>> 0) << 27) | (pos >>> 1)) >>> 0),
        "lo": ((((pos >>> 0) << 31) | refalt) >>> 0)
    };
}

function extractVariantKeyChrom(vk) {
    return ((vk.hi & 0xF8000000) >>> 27);
}

function extractVariantKeyPos(vk) {
    return (((vk.hi & 0x07FFFFFF) << 1) | (vk.lo >>> 31)) >>> 0;
}

function extractVariantKeyRefAlt(vk) {
    return (vk.lo & 0x7FFFFFFF) >>> 0;
}

function decodeVariantKey(vk) {
    return {
        "chrom": extractVariantKeyChrom(vk),
        "pos": extractVariantKeyPos(vk),
        "refalt": extractVariantKeyRefAlt(vk)
    };
}

function reverseVariantKey(vk) {
    var ra = decodeRefAlt(extractVariantKeyRefAlt(vk));
    return {
        "chrom": decodeChrom(extractVariantKeyChrom(vk)),
        "pos": extractVariantKeyPos(vk),
        "ref": ra.ref,
        "alt": ra.alt
    }
}

function variantKey(chrom, pos, ref, alt) {
    return encodeVariantKey(encodeChrom(chrom), pos, encodeRefAlt(ref, alt));
}

function variantKeyRange(chrom, pos_min, pos_max) {
    return {
        "min": {
            "hi": ((((chrom >>> 0) << 27) | (pos_min >>> 1)) >>> 0),
            "lo": (((pos_min >>> 0) << 31) >>> 0)
        },
        "max": {
            "hi": ((((chrom >>> 0) << 27) | (pos_max >>> 1)) >>> 0),
            "lo": ((((pos_max >>> 0) << 31) | 0x7FFFFFFF) >>> 0)
        }
    };
}

function compare(a, b) {
    return ((a < b) ? -1 : ((a > b) ? 1 : 0));
}

function compareVariantKeyChrom(vka, vkb) {
    return compare((vka.hi >>> 27), (vkb.hi >>> 27));
}

function compareVariantKeyChromPos(vka, vkb) {
    var cmp = compare(vka.hi, vkb.hi);
    if (cmp == 0) {
        return compare((vka.lo >>> 31), (vkb.lo >>> 31));
    }
    return cmp;
}

function padL08(s) {
    return ("00000000" + s).slice(-8);
}

function variantKeyString(vk) {
    return padL08(vk.hi.toString(16)) + padL08(vk.lo.toString(16));
}

function parseHex(vs) {
    return {
        "hi": parseInt(vs.substring(0, 8), 16) >>> 0,
        "lo": parseInt(vs.substring(8, 16), 16) >>> 0,
    };
}

function encodeRegionStrand(strand) {
    var map = [2, 0, 1, 0];
    return map[((++strand) & 3)];
}

function decodeRegionStrand(strand) {
    var map = [0, 1, -1, 0];
    return map[(strand & 3)];
}

function encodeRegionKey(chrom, startpos, endpos, strand) {
    return {
        "hi": ((((chrom >>> 0) << 27) | (startpos >>> 1)) >>> 0),
        "lo": ((((startpos >>> 0) << 31) | ((endpos >>> 0) << 3) | ((strand >>> 0) << 1)) >>> 0)
    };
}

function extractRegionKeyChrom(rk) {
    return ((rk.hi & 0xF8000000) >>> 27);
}

function extractRegionKeyStartPos(rk) {
    return (((rk.hi & 0x07FFFFFF) << 1) | (rk.lo >>> 31)) >>> 0;
}

function extractRegionKeyEndPos(rk) {
    return (rk.lo & 0x7FFFFFF8) >>> 3
}

function extractRegionKeyStrand(rk) {
    return (rk.lo & 0x00000006) >>> 1;
}

function decodeRegionKey(rk) {
    return {
        "chrom": extractRegionKeyChrom(rk),
        "startpos": extractRegionKeyStartPos(rk),
        "endpos": extractRegionKeyEndPos(rk),
        "strand": extractRegionKeyStrand(rk)
    };
}

function reverseRegionKey(rk) {
    return {
        "chrom": decodeChrom(extractRegionKeyChrom(rk)),
        "startpos": extractRegionKeyStartPos(rk),
        "endpos": extractRegionKeyEndPos(rk),
        "strand": decodeRegionStrand(extractRegionKeyStrand(rk))
    };
}

function regionKey(chrom, startpos, endpos, strand) {
    return encodeRegionKey(encodeChrom(chrom), startpos, endpos, encodeRegionStrand(strand));
}

function extendRegionKey(rk, size) {
    const drk = decodeRegionKey(rk);
    drk.startpos = ((size >= drk.startpos) ? 0 : (drk.startpos - size));
    drk.endpos = (((0x0FFFFFFF - drk.endpos) <= size) ? 0x0FFFFFFF : (drk.endpos + size));
    const out = encodeRegionKey(drk.chrom, drk.startpos, drk.endpos, drk.strand);
    // C masks with RKMASK_NOPOS = 0xF800000000000007, which keeps bit 0.
    // Decoding and re-encoding drops it, so it is carried over explicitly.
    out.lo = ((out.lo | (rk.lo & 0x1)) >>> 0);
    return out;
}

function regionKeyString(rk) {
    return padL08(rk.hi.toString(16)) + padL08(rk.lo.toString(16));
}

// Return the REF length of a VariantKey. Mirrors the C
// get_variantkey_ref_length().
// The length is stored in the key only when the REF+ALT pair uses the reversible
// encoding (bit 0 clear). For a hash-mode key the C version looks the length up
// in the NRVK binary table; that table is not ported to Javascript, so 0 is
// always returned here.
function getVariantKeyRefLength(vk) {
    if ((vk.lo & 0x1) !== 0) {
        return 0; // non-reversible encoding: the REF length is not recoverable from the key
    }
    return ((vk.lo & 0x78000000) >>> 27);
}

function getVariantKeyEndPos(vk) {
    return extractVariantKeyPos(vk) + getVariantKeyRefLength(vk);
}

function areOverlappingRegions(a_chrom, a_startpos, a_endpos, b_chrom, b_startpos, b_endpos) {
    return ((a_chrom == b_chrom) && (a_startpos < b_endpos) && (a_endpos > b_startpos));
}

function areOverlappingRegionRegionKey(chrom, startpos, endpos, rk) {
    return ((chrom == extractRegionKeyChrom(rk)) && (startpos < extractRegionKeyEndPos(rk)) && (endpos > extractRegionKeyStartPos(rk)));
}

function areOverlappingRegionKeys(rka, rkb) {
    return ((extractRegionKeyChrom(rka) == extractRegionKeyChrom(rkb)) && (extractRegionKeyStartPos(rka) < extractRegionKeyEndPos(rkb)) && (extractRegionKeyEndPos(rka) > extractRegionKeyStartPos(rkb)));
}

function areOverlappingVariantKeyRegionKey(vk, rk) {
    return ((extractVariantKeyChrom(vk) == extractRegionKeyChrom(rk)) && (extractVariantKeyPos(vk) < extractRegionKeyEndPos(rk)) && (getVariantKeyEndPos(vk) > extractRegionKeyStartPos(rk)));
}

function variantKeyToRegionKey(vk) {
    return {
        "hi": vk.hi,
        "lo": ((vk.lo & 0x80000000) | ((getVariantKeyEndPos(vk) << 3) >>> 0)) >>> 0
    };
}

function esidEncodeChar(c) {
    if ((c < 33) || (c > 127)) {
        return (63 >>> 0);
    }
    if (c > 95) {
        return ((c - 64) >>> 0);
    }
    return ((c - 32) >>> 0);
}

function esidDecodeChar(esid, pos) {
    return String.fromCharCode(((esid >>> pos) & 63) + 32); // 63 dec = 00111111 bin
}

function encodeStringID(str, start) {
    if (start > str.length) {
        return {
            "hi": (0 >>> 0),
            "lo": (0 >>> 0),
        };
    }
    var size = str.length - start;
    if (size > 10) {
        size = 10;
    }
    var hi = (0 >>> 0);
    var lo = (0 >>> 0);
    var offset = (size + start - 1);
    switch (size) {
        case 10:
            lo ^= esidEncodeChar(str.charCodeAt(offset--));
            // fall through
        case 9:
            lo ^= esidEncodeChar(str.charCodeAt(offset--)) << 6;
            // fall through
        case 8:
            lo ^= esidEncodeChar(str.charCodeAt(offset--)) << 12;
            // fall through
        case 7:
            lo ^= esidEncodeChar(str.charCodeAt(offset--)) << 18;
            // fall through
        case 6:
            lo ^= esidEncodeChar(str.charCodeAt(offset--)) << 24;
            // fall through
        case 5:
            hi ^= esidEncodeChar(str.charCodeAt(offset--));
            // fall through
        case 4:
            hi ^= esidEncodeChar(str.charCodeAt(offset--)) << 6;
            // fall through
        case 3:
            hi ^= esidEncodeChar(str.charCodeAt(offset--)) << 12;
            // fall through
        case 2:
            hi ^= esidEncodeChar(str.charCodeAt(offset--)) << 18;
            // fall through
        case 1:
            hi ^= esidEncodeChar(str.charCodeAt(offset)) << 24;
    }
    return {
        "hi": ((hi >>> 2) | ((size >>> 0) << 28)) >>> 0,
        "lo": (lo | ((hi & 3) << 30)) >>> 0,
    };
}

function encodeStringNumID(str, sep) {
    var size = str.length;
    if (size <= 10) {
        return encodeStringID(str, 0);
    }
    var hi = (0 >>> 0);
    var lo = (0 >>> 0);
    var num = (0 >>> 0);
    var nchr = (0 >>> 0);
    var npad = (0 >>> 0);
    var bitpos = 30;
    var c;
    var i = 0;
    while (size--) {
        c = str.charCodeAt(i++);
        if (c == sep) {
            break;
        }
        if (nchr < 5) {
            bitpos -= 6;
            hi |= (esidEncodeChar(c) << bitpos);
            nchr++;
        }
    }
    lo |= ((hi & 3) << 30) >>> 0;
    hi = ((hi >>> 2) | (((nchr + 10) >>> 0) << 28)) >>> 0;
    while (((c = str.charCodeAt(i++)) == 48) && (npad < 7) && (size--)) {
        npad++;
    }
    lo |= (npad << 27) >>> 0;
    while ((c >= 48) && (c <= 57) && (size--)) {
        num = ((num * 10) + (c - 48)) >>> 0;
        c = str.charCodeAt(i++);
    }
    lo |= (num & 0x7FFFFFF);
    // "lo |= x >>> 0" normalises the operand, not the result of the assignment,
    // so both fields are normalised here.
    return {
        "hi": hi >>> 0,
        "lo": lo >>> 0,
    };
}

function esidDecodeStringID(size, esid) {
    var hi = ((esid.hi << 2) | (esid.lo >>> 30)) >>> 0;
    var str = ['', '', '', '', '', '', '', '', '', ''];
    switch (size) {
        case 10:
            str[9] = esidDecodeChar(esid.lo, 0);
            // fall through
        case 9:
            str[8] = esidDecodeChar(esid.lo, 6);
            // fall through
        case 8:
            str[7] = esidDecodeChar(esid.lo, 12);
            // fall through
        case 7:
            str[6] = esidDecodeChar(esid.lo, 18);
            // fall through
        case 6:
            str[5] = esidDecodeChar(esid.lo, 24);
            // fall through
        case 5:
            str[4] = esidDecodeChar(hi, 0);
            // fall through
        case 4:
            str[3] = esidDecodeChar(hi, 6);
            // fall through
        case 3:
            str[2] = esidDecodeChar(hi, 12);
            // fall through
        case 2:
            str[1] = esidDecodeChar(hi, 18);
            // fall through
        case 1:
            str[0] = esidDecodeChar(hi, 24);
    }
    return str.join('');
}

function decodeStringNumID(size, esid) {
    const str = esidDecodeStringID(size, esid);
    const npad = (esid.lo >>> 27) & 7;
    const num = (esid.lo & 0x7FFFFFF);
    var numstr = '';
    if (num > 0) {
        numstr = num.toString();
    }
    return str + ':' + '0'.repeat(npad) + numstr;
}

function decodeStringID(esid) {
    const size = (esid.hi >>> 28);
    if (size > 10) {
        return decodeStringNumID((size - 10), esid);
    }
    return esidDecodeStringID(size, esid);
}

// Return a 64 bit hash of a string, for IDs that the reversible encoding cannot
// represent. Mirrors the C hash_string_id().
//
// BigInt is used because this is the only part of the library that needs true 64
// bit multiplication. The result is returned as the usual {hi, lo} pair.
//
// The 8 byte blocks are consumed least-significant byte first, matching the C
// memcpy on a little-endian host, so the values are endianness dependent.
const HSID_MASK64 = (1n << 64n) - 1n;

function muxHash64(k, h) {
    k = (k * 0x87c37b91114253d5n) & HSID_MASK64;
    k = ((k >> 33n) | (k << 31n)) & HSID_MASK64;
    k = (k * 0x4cf5ad432745937fn) & HSID_MASK64;
    h ^= k;
    h = ((h >> 37n) | (h << 27n)) & HSID_MASK64;
    return (((h * 5n) & HSID_MASK64) + 0x52dce729n) & HSID_MASK64;
}

function hashStringID(str) {
    const size = str.length;
    const blocks = (size - (size & 7));
    var h = 0n;
    var i = 0;
    var j = 0;
    for (i = 0; i < blocks; i += 8) {
        var b = 0n;
        for (j = 7; j >= 0; j--) {
            b = ((b << 8n) | BigInt(str.charCodeAt(i + j) & 0xff));
        }
        h = muxHash64(b, h);
    }
    var v = 0n;
    for (j = ((size & 7) - 1); j >= 0; j--) {
        v = ((v << 8n) | BigInt(str.charCodeAt(blocks + j) & 0xff));
    }
    if (v > 0n) {
        h = muxHash64(v, h);
    }
    // MurmurHash3 finalization mix
    h ^= (h >> 33n);
    h = (h * 0xff51afd7ed558ccdn) & HSID_MASK64;
    h ^= (h >> 33n);
    h = (h * 0xc4ceb9fe1a85ec53n) & HSID_MASK64;
    h ^= (h >> 33n);
    h |= 0x8000000000000000n; // set the first bit to indicate HASH mode
    return {
        "hi": Number(h >> 32n) >>> 0,
        "lo": Number(h & 0xffffffffn) >>> 0,
    };
}

if (typeof(module) !== 'undefined') {
    module.exports = {
        encodeChrom: encodeChrom,
        decodeChrom: decodeChrom,
        parseHex: parseHex,
        encodeRefAlt: encodeRefAlt,
        decodeRefAlt: decodeRefAlt,
        encodeVariantKey: encodeVariantKey,
        extractVariantKeyChrom: extractVariantKeyChrom,
        extractVariantKeyPos: extractVariantKeyPos,
        extractVariantKeyRefAlt: extractVariantKeyRefAlt,
        decodeVariantKey: decodeVariantKey,
        variantKey: variantKey,
        variantKeyRange: variantKeyRange,
        compareVariantKeyChrom: compareVariantKeyChrom,
        compareVariantKeyChromPos: compareVariantKeyChromPos,
        variantKeyString: variantKeyString,
        reverseVariantKey: reverseVariantKey,
        encodeRegionStrand: encodeRegionStrand,
        decodeRegionStrand: decodeRegionStrand,
        encodeRegionKey: encodeRegionKey,
        extractRegionKeyChrom: extractRegionKeyChrom,
        extractRegionKeyStartPos: extractRegionKeyStartPos,
        extractRegionKeyEndPos: extractRegionKeyEndPos,
        extractRegionKeyStrand: extractRegionKeyStrand,
        decodeRegionKey: decodeRegionKey,
        reverseRegionKey: reverseRegionKey,
        regionKey: regionKey,
        extendRegionKey: extendRegionKey,
        regionKeyString: regionKeyString,
        getVariantKeyRefLength: getVariantKeyRefLength,
        getVariantKeyEndPos: getVariantKeyEndPos,
        areOverlappingRegions: areOverlappingRegions,
        areOverlappingRegionRegionKey: areOverlappingRegionRegionKey,
        areOverlappingRegionKeys: areOverlappingRegionKeys,
        areOverlappingVariantKeyRegionKey: areOverlappingVariantKeyRegionKey,
        variantKeyToRegionKey: variantKeyToRegionKey,
        encodeStringID: encodeStringID,
        encodeStringNumID: encodeStringNumID,
        decodeStringID: decodeStringID,
        hashStringID: hashStringID,
    }
}