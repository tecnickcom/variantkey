"""Vectorized VariantKey."""

import variantkey as pvk
import numpy as np


class VariantKey(object):
    """VariantKey numpy-vectorized functions."""

    def __init__(
        self, genoref_file=None, nrvk_file=None, rsvk_file=None, vkrs_file=None
    ):
        """Instantiates a new VariantKey object, loading the support files if specified.

        Parameters
        ----------
        genoref_file : string
            Name and path of the binary file containing the genome reference (fasta.bin).
            This file can be generated from a FASTA file using the resources/tools/fastabin.sh script.
        nrvk_file : string
            Name and path of the binary file containing the non-reversible-VariantKey mapping (nrvk.bin).
            This file can be generated from a normalized VCF file using the resources/tools/nrvk.sh script.
        rsvk_file : string
            Name and path of the binary file containing the rsID to VariantKey mapping (rsvk.bin).
            This file can be generated using the resources/tools/rsvk.sh script.
        vkrs_file : string
            Name and path of the binary file containing the VariantKey to rsID mapping (vkrs.bin).
            This file can be generated using the resources/tools/vkrs.sh script.
        """

        self.genoref_mf = None
        self.genoref_size = 0
        self.nrvk_mf = None
        self.nrvk_mc = None
        self.nrvk_nrows = 0
        self.rsvk_mf = None
        self.rsvk_mc = None
        self.rsvk_nrows = 0
        self.vkrs_mf = None
        self.vkrs_mc = None
        self.vkrs_nrows = 0

        if genoref_file is not None:
            # Load the reference genome binary file.
            self.genoref_mf, self.genoref_size = pvk.mmap_genoref_file(genoref_file)
            if self.genoref_size <= 0:
                raise Exception(
                    "Unable to load the GENOREF file: {0}".format(genoref_file)
                )

        if nrvk_file is not None:
            # Load the lookup table for non-reversible variantkeys.
            self.nrvk_mf, self.nrvk_mc, self.nrvk_nrows = pvk.mmap_nrvk_file(nrvk_file)
            if self.nrvk_nrows <= 0:
                raise Exception("Unable to load the NRVK file: {0}".format(nrvk_file))

        if rsvk_file is not None:
            # Load the lookup table for rsID to VariantKey.
            self.rsvk_mf, self.rsvk_mc, self.rsvk_nrows = pvk.mmap_rsvk_file(
                rsvk_file, [4, 8]
            )
            if self.rsvk_nrows <= 0:
                raise Exception("Unable to load the RSVK file: {0}".format(rsvk_file))

        if vkrs_file is not None:
            # Load the lookup table for VariantKey to rsID
            self.vkrs_mf, self.vkrs_mc, self.vkrs_nrows = pvk.mmap_vkrs_file(
                vkrs_file, [8, 4]
            )
            if self.vkrs_nrows <= 0:
                raise Exception("Unable to load the VKRS file: {0}".format(vkrs_file))

    def __del__(self):
        """Releases the mapped files."""
        if pvk is not None:  # pragma: no cover
            self.close()

    def close(self):
        """Closes all the input files.

        Idempotent: calling it twice, or calling it and then letting __del__ run,
        does nothing the second time.
        """
        for name in ("genoref_mf", "nrvk_mf", "rsvk_mf", "vkrs_mf"):
            mf = getattr(self, name, None)
            if mf is not None:
                pvk.munmap_binfile(mf)
                setattr(self, name, None)
        self.nrvk_mc = None
        self.rsvk_mc = None
        self.vkrs_mc = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
        return False

    # BASIC VARIANTKEY FUNCTIONS
    # --------------------------

    def encode_chrom(self, chrom):
        """Encodes a chromosome identifier into a numerical code.

        Parameters
        ----------
        chrom : string
            Chromosome. An identifier from the reference genome, no white-space permitted.

        Returns
        -------
        uint8 :
            CHROM code
        """
        f = np.vectorize(pvk.encode_chrom, otypes=[np.uint8])
        return f(np.array(chrom).astype(np.bytes_))

    def decode_chrom(self, code):
        """Decodes a chromosome numerical code into its string representation.

        Parameters
        ----------
        code : uint8
            CHROM code.

        Returns
        -------
        '|S2' :
            Chromosome string
        """
        f = np.vectorize(pvk.decode_chrom, otypes=["|S2"])
        return f(np.array(code).astype(np.uint8))

    def encode_refalt(self, ref, alt):
        """Encodes a REF+ALT pair into a 31 bit code.

        Parameters
        ----------
        ref : string
            Reference allele.
            String containing a sequence of nucleotide letters.
            The value in the pos field refers to the position of the first nucleotide in the String.
            Characters must be A-Z, a-z or *
        alt : string
            Alternate non-reference allele string. Characters must be A-Z, a-z or *

        Returns
        -------
        uint32 :
            code
        """
        f = np.vectorize(pvk.encode_refalt, otypes=[np.uint32])
        return f(np.array(ref).astype(np.bytes_), np.array(alt).astype(np.bytes_))

    def decode_refalt(self, code):
        """Decodes a 32 bit REF+ALT code if it was produced by the reversible encoding
        (11 or less bases in total, containing only A, C, G and T letters).

        Parameters
        ----------
        code : uint32
            REF+ALT code

        Returns
        -------
        tuple:
            - '|S256' : REF
            - '|S256' : ALT
            - uint8   : REF length
            - uint8   : ALT length
        """
        f = np.vectorize(
            pvk.decode_refalt, otypes=["|S256", "|S256", np.uint8, np.uint8]
        )
        return f(np.array(code).astype(np.uint32))

    def encode_variantkey(self, chrom, pos, refalt):
        """Assembles a VariantKey from the pre-encoded CHROM, POS and REF+ALT.

        Parameters
        ----------
        chrom : uint8
            Encoded Chromosome (see encode_chrom).
        pos : uint32
            Position. The reference position, with the first base having position 0.
        refalt : uint32
            Encoded Reference + Alternate (see encode_refalt).

        Returns
        -------
        unit64:
            VariantKey 64 bit code.
        """
        f = np.vectorize(pvk.encode_variantkey, otypes=[np.uint64])
        return f(
            np.array(chrom).astype(np.uint8),
            np.array(pos).astype(np.uint32),
            np.array(refalt).astype(np.uint32),
        )

    def extract_variantkey_chrom(self, vk):
        """Extracts the CHROM code from a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey code.

        Returns
        -------
        uint8 :
            CHROM code.
        """
        f = np.vectorize(pvk.extract_variantkey_chrom, otypes=[np.uint8])
        return f(np.array(vk).astype(np.uint64))

    def extract_variantkey_pos(self, vk):
        """Extracts the POS value from a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey code.

        Returns
        -------
        uint32 :
            Position.
        """
        f = np.vectorize(pvk.extract_variantkey_pos, otypes=[np.uint32])
        return f(np.array(vk).astype(np.uint64))

    def extract_variantkey_refalt(self, vk):
        """Extracts the REF+ALT code from a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey code.

        Returns
        -------
        uint32 :
            REF+ALT code.
        """
        f = np.vectorize(pvk.extract_variantkey_refalt, otypes=[np.uint32])
        return f(np.array(vk).astype(np.uint64))

    def decode_variantkey(self, vk):
        """Splits a VariantKey into its CHROM, POS and REF+ALT components.

        Parameters
        ----------
        vk : uint64
            VariantKey code.

        Returns
        -------
        tuple :
            - uint8  : CHROM code
            - uint32 : POS
            - uint32 : REF+ALT code
        """
        f = np.vectorize(pvk.decode_variantkey, otypes=[np.uint8, np.uint32, np.uint32])
        return f(np.array(vk).astype(np.uint64))

    def variantkey(self, chrom, pos, ref, alt):
        """Returns a VariantKey for the given CHROM, POS (0-based), REF and ALT.
        The variant should be already normalized (see normalize_variant or use normalized_variantkey).

        Parameters
        ----------
        chrom : string
            Chromosome. An identifier from the reference genome, no white-space or leading zeros permitted.
        pos : uint32
            Position. The reference position, with the first base having position 0.
        ref : string
            Reference allele. String containing a sequence of nucleotide letters.
            The value in the pos field refers to the position of the first nucleotide in the String.
            Characters must be A-Z, a-z or *
        alt : string
            Alternate non-reference allele string. Characters must be A-Z, a-z or *

        Returns
        -------
        uint64:
            VariantKey 64 bit code.
        """
        f = np.vectorize(pvk.variantkey, otypes=[np.uint64])
        return f(
            np.array(chrom).astype(np.bytes_),
            np.array(pos).astype(np.uint32),
            np.array(ref).astype(np.bytes_),
            np.array(alt).astype(np.bytes_),
        )

    def variantkey_range(self, chrom, pos_min, pos_max):
        """Returns the minimum and maximum VariantKey of a CHROM and POS range.

        Parameters
        ----------
        chrom : uint8
            Chromosome encoded number.
        pos_min : uint32
            Start reference position, with the first base having position 0.
        pos_max : uint32
            End reference position, with the first base having position 0.

        Returns
        -------
        tuple :
            - uint64 : VariantKey min value
            - uint64 : VariantKey max value
        """
        f = np.vectorize(pvk.variantkey_range, otypes=[np.uint64, np.uint64])
        return f(
            np.array(chrom).astype(np.uint8),
            np.array(pos_min).astype(np.uint32),
            np.array(pos_max).astype(np.uint32),
        )

    def compare_variantkey_chrom(self, vka, vkb):
        """Compares two VariantKeys by CHROM only.

        Parameters
        ----------
        vka : uint64
            The first VariantKey to be compared.
        vkb : uint64
            The second VariantKey to be compared.

        Returns
        -------
        int :
            -1 if the first chromosome is smaller than the second,
            0 if they are equal and 1 if the first is greater than the second.
        """
        f = np.vectorize(pvk.compare_variantkey_chrom, otypes=[np.int_])
        return f(np.array(vka).astype(np.uint64), np.array(vkb).astype(np.uint64))

    def compare_variantkey_chrom_pos(self, vka, vkb):
        """Compares two VariantKeys by CHROM and POS.

        Parameters
        ----------
        vka : uint64
            The first VariantKey to be compared.
        vkb : uint64
            The second VariantKey to be compared.

        Returns
        -------
        int :
            -1 if the first CHROM+POS is smaller than the second,
            0 if they are equal and 1 if the first is greater than the second.
        """
        f = np.vectorize(pvk.compare_variantkey_chrom_pos, otypes=[np.int_])
        return f(np.array(vka).astype(np.uint64), np.array(vkb).astype(np.uint64))

    def variantkey_hex(self, vk):
        """Returns a VariantKey as a 16 character hexadecimal string.

        Parameters
        ----------
        vk : uint64
            VariantKey code.

        Returns
        -------
        '|S16':
            VariantKey hexadecimal string.
        """
        f = np.vectorize(pvk.variantkey_hex, otypes=["|S16"])
        return f(np.array(vk).astype(np.uint64))

    def parse_variantkey_hex(self, vs):
        """Parses a 16 character hexadecimal string into a VariantKey.

        Parameters
        ----------
        vs : '|S16'
            VariantKey hexadecimal string (it must contain 16 hexadecimal characters).

        Returns
        -------
        uint64 :
            VariantKey 64 bit code.
        """
        f = np.vectorize(pvk.parse_variantkey_hex, otypes=[np.uint64])
        return f(np.array(vs).astype("|S16"))

    # RSIDVAR
    # -------

    def find_rv_variantkey_by_rsid(self, rsid):
        """Returns the first VariantKey associated with an rsID.

        Parameters
        ----------
        rsid : uint32
            rsID to search.

        Returns
        -------
        tuple :
            - uint64 : VariantKey or 0 in case not found.
            - uint64 : Item position in the file.
        """
        f = np.vectorize(
            pvk.find_rv_variantkey_by_rsid,
            excluded=[0],
            otypes=[np.uint64, np.uint64],
        )
        return f(self.rsvk_mc, 0, self.rsvk_nrows, np.array(rsid).astype(np.uint32))

    def get_next_rv_variantkey_by_rsid(self, pos, rsid):
        """Returns the next VariantKey associated with an rsID.
        Call this in a loop after find_rv_variantkey_by_rsid to get all the VariantKeys of the same rsID.

        Parameters
        ----------
        pos : uint64
            Current item position.
        rsid : uint32
            rsID to search.

        Returns
        -------
        tuple :
            - uint64 : VariantKey or 0 in case not found.
            - uint64 : Item position in the file.
        """
        f = np.vectorize(
            pvk.get_next_rv_variantkey_by_rsid,
            excluded=[0],
            otypes=[np.uint64, np.uint64],
        )
        return f(
            self.rsvk_mc,
            np.array(pos).astype(np.uint64),
            self.rsvk_nrows,
            np.array(rsid).astype(np.uint32),
        )

    def find_all_rv_variantkey_by_rsid(self, rsid):
        """Returns all the VariantKeys associated with an rsID.

        Parameters
        ----------
        rsid : uint32
            rsID to search.

        Returns
        -------
        uint64 :
            - VariantKey(s).
        """
        vk = []
        rsid_arr = np.array(rsid).astype(np.uint32)
        for x in np.nditer(rsid_arr):
            vk = vk + pvk.find_all_rv_variantkey_by_rsid(
                self.rsvk_mc, 0, self.rsvk_nrows, x.item(0)
            )
        return np.array(vk).astype(np.uint64)

    def find_vr_rsid_by_variantkey(self, vk):
        """Returns the first rsID associated with a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey.

        Returns
        -------
        tuple :
            - uint32 : rsID or 0 in case not found.
            - uint64 : Item position in the file.
        """
        f = np.vectorize(
            pvk.find_vr_rsid_by_variantkey,
            excluded=[0],
            otypes=[np.uint32, np.uint64],
        )
        return f(self.vkrs_mc, 0, self.vkrs_nrows, np.array(vk).astype(np.uint64))

    def get_next_vr_rsid_by_variantkey(self, pos, vk):
        """Returns the next rsID associated with a VariantKey.
        Call this in a loop after find_vr_rsid_by_variantkey to get all the rsIDs of the same VariantKey.

        Parameters
        ----------
        pos : uint64
            Current item position.
        vk : uint64
            variantKey to search.

        Returns
        -------
        tuple :
            - uint32 : rsID or 0 in case not found.
            - uint64 : Item position in the file.
        """
        f = np.vectorize(
            pvk.get_next_vr_rsid_by_variantkey,
            excluded=[0],
            otypes=[np.uint32, np.uint64],
        )
        return f(
            self.vkrs_mc,
            np.array(pos).astype(np.uint64),
            self.vkrs_nrows,
            np.array(vk).astype(np.uint64),
        )

    def find_all_vr_rsid_by_variantkey(self, vk):
        """Returns all the rsIDs associated with a VariantKey.

        Parameters
        ----------
        vk : uint64
            variantKey to search.

        Returns
        -------
        uint32 :
            - rsID(s).
        """
        rs = []
        vk_arr = np.array(vk).astype(np.uint64)
        for x in np.nditer(vk_arr):
            rs = rs + pvk.find_all_vr_rsid_by_variantkey(
                self.vkrs_mc, 0, self.vkrs_nrows, x.item(0)
            )
        return np.array(rs).astype(np.uint32)

    def find_vr_chrompos_range(self, chrom, pos_min, pos_max):
        """Returns the first rsID of a CHROM and POS range.

        Parameters
        ----------
        chrom : uint8
            Chromosome encoded number.
        pos_min : uint32
            Start reference position, with the first base having position 0.
        pos_max : uint32
            End reference position, with the first base having position 0.

        Returns
        -------
        tuple :
            - uint32 : rsID or 0 in case not found
            - uint64 : Position of the first item.
            - uint64 : Position of the last item.
        """
        f = np.vectorize(
            pvk.find_vr_chrompos_range,
            excluded=[0],
            otypes=[np.uint32, np.uint64, np.uint64],
        )
        return f(
            self.vkrs_mc,
            0,
            self.vkrs_nrows,
            np.array(chrom).astype(np.uint8),
            np.array(pos_min).astype(np.uint32),
            np.array(pos_max).astype(np.uint32),
        )

    # NRVK
    # ----

    def find_ref_alt_by_variantkey(self, vk):
        """Looks up the REF and ALT strings of a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey to search.

        Returns
        -------
        tuple :
            - '|S256' : REF string.
            - '|S256' : ALT string.
            - uint8   : REF length.
            - uint8   : ALT length.
            - uint16  : REF+ALT length.
        """
        f = np.vectorize(
            pvk.find_ref_alt_by_variantkey,
            excluded=[0],
            otypes=["|S256", "|S256", np.uint8, np.uint8, np.uint16],
        )
        return f(self.nrvk_mc, np.array(vk).astype(np.uint64))

    def reverse_variantkey(self, vk):
        """Reverses a VariantKey into its CHROM, POS, REF and ALT components.

        Parameters
        ----------
        vk : uint64
            VariantKey code.

        Returns
        -------
        tuple :
            - '|S2'   : CHROM string.
            - uint32  : POS.
            - '|S256' : REF string.
            - '|S256' : ALT string.
            - uint8   : REF length.
            - uint8   : ALT length.
            - uint16  : REF+ALT length.
        """
        f = np.vectorize(
            pvk.reverse_variantkey,
            excluded=[0],
            otypes=["|S2", np.uint32, "|S256", "|S256", np.uint8, np.uint8, np.uint16],
        )
        return f(self.nrvk_mc, np.array(vk).astype(np.uint64))

    def get_variantkey_ref_length(self, vk):
        """Returns the REF length of a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey

        Returns
        -------
        uint8 :
            REF length or 0 if the VariantKey is not reversible and not found.
        """
        f = np.vectorize(
            pvk.get_variantkey_ref_length, excluded=[0], otypes=[np.uint8]
        )
        return f(self.nrvk_mc, np.array(vk).astype(np.uint64))

    def get_variantkey_endpos(self, vk):
        """Returns the end position of a VariantKey (POS + REF length).

        Parameters
        ----------
        vk : uint64
            VariantKey.

        Returns
        -------
        uint32 :
            Variant end position.
        """
        f = np.vectorize(pvk.get_variantkey_endpos, excluded=[0], otypes=[np.uint32])
        return f(self.nrvk_mc, np.array(vk).astype(np.uint64))

    def get_variantkey_chrom_startpos(self, vk):
        """Returns the CHROM and START POS section of a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey.

        Returns
        -------
        uint64 :
            CHROM + START POS encoding.
        """
        f = np.vectorize(pvk.get_variantkey_chrom_startpos, otypes=[np.uint64])
        return f(np.array(vk).astype(np.uint64))

    def get_variantkey_chrom_endpos(self, vk):
        """Returns the CHROM and END POS of a VariantKey.

        Parameters
        ----------
        vk : uint64
            VariantKey.

        Returns
        -------
        uint64 :
            CHROM + END POS encoding.
        """
        f = np.vectorize(
            pvk.get_variantkey_chrom_endpos, excluded=[0], otypes=[np.uint64]
        )
        return f(self.nrvk_mc, np.array(vk).astype(np.uint64))

    def nrvk_bin_to_tsv(self, tsvfile):
        """Writes the content of the NRVK memory mapped file as a TSV file.
        For the reverse operation see the resources/tools/nrvk.sh script.

        Parameters
        ----------
        tsvfile : string
            Output file name.

        Returns
        -------
        uint64 :
            Number of bytes written or 0 in case of error.
        """
        return pvk.nrvk_bin_to_tsv(self.nrvk_mc, tsvfile)

    # GENOREF
    # ----------

    def get_genoref_seq(self, chrom, pos):
        """Returns the genome reference nucleotide at the given chromosome and position.

        Parameters
        ----------
        chrom : uint8
            Encoded Chromosome number (see encode_chrom).
        pos : uint32
            Position. The reference position, with the first base having position 0.

        Returns
        -------
        '|S1' :
            Nucleotide letter or 0 (NULL char) in case of invalid position.
        """
        f = np.vectorize(pvk.get_genoref_seq, excluded=[0], otypes=["|S1"])
        return f(
            self.genoref_mf,
            np.array(chrom).astype(np.uint8),
            np.array(pos).astype(np.uint32),
        )

    def check_reference(self, chrom, pos, ref):
        """Checks a reference allele against the genome reference data.

        Parameters
        ----------
        chrom : uint8
            Encoded Chromosome number (see encode_chrom).
        pos : uint32
            Position. The reference position, with the first base having position 0.
        ref : string
            Reference allele. String containing a sequence of nucleotide letters.

        Returns
        -------
        int :
            positive number in case of success, negative in case of error:
            0 the reference allele match the reference genome;
            1 the reference allele is inconsistent with the genome reference
            (i.e. when contains nucleotide letters other than A, C, G and T);
               -1 the reference allele don't match the reference genome;
               -2 the chromosome is invalid or the reference allele is longer than the genome reference sequence.
        """
        f = np.vectorize(pvk.check_reference, excluded=[0], otypes=[np.int_])
        return f(
            self.genoref_mf,
            np.array(chrom).astype(np.uint8),
            np.array(pos).astype(np.uint32),
            np.array(ref).astype(np.bytes_),
        )

    def flip_allele(self, allele):
        """Replaces each nucleotide of an allele with its complement.
        The resulting string is always in uppercase.
        Supports extended nucleotide letters.

        Parameters
        ----------
        allele : string
            String containing a sequence of nucleotide letters.

        Returns
        -------
        '|S256' :
            Flipped allele.
        """
        f = np.vectorize(pvk.flip_allele, otypes=["|S256"])
        return f(np.array(allele).astype(np.bytes_))

    def normalize_variant(self, chrom, pos, ref, alt):
        """Normalizes a variant against the genome reference, flipping the alleles if required.
        See https://genome.sph.umich.edu/wiki/Variant_Normalization

        Parameters
        ----------
        chrom : uint8
            Chromosome encoded number.
        pos : uint32
            Position. The reference position, with the first base having position 0.
        ref : string
            Reference allele. String containing a sequence of nucleotide letters.
        alt : string
            Alternate non-reference allele string.

        Returns
        -------
        tuple :
            - int : Bitmask number in case of success, negative number in case of error.
              When positive, each bit has a different meaning when set:
             - bit 0 : The reference allele is inconsistent with the genome reference
                       (i.e. when contains nucleotide letters other than A, C, G and T).
             - bit 1 : The alleles have been swapped.
             - bit 2 : The alleles nucleotides have been flipped
                       (each nucleotide have been replaced with its complement).
             - bit 3 : Alleles have been left extended.
             - bit 4 : Alleles have been right trimmed.
             - bit 5 : Alleles have been left trimmed.
            - uint32  : POS.
            - '|S256' : REF string.
            - '|S256' : ALT string.
            - uint8   : REF length.
            - uint8   : ALT length.
        """
        f = np.vectorize(
            pvk.normalize_variant,
            excluded=[0],
            otypes=[np.int_, np.uint32, "|S256", "|S256", np.uint8, np.uint8],
        )
        return f(
            self.genoref_mf,
            np.array(chrom).astype(np.uint8),
            np.array(pos).astype(np.uint32),
            np.array(ref).astype(np.bytes_),
            np.array(alt).astype(np.bytes_),
        )

    def normalized_variantkey(self, chrom, pos, posindex, ref, alt):
        """Normalizes a variant and returns its VariantKey.
        See https://genome.sph.umich.edu/wiki/Variant_Normalization

        Parameters
        ----------
        chrom : string
            Chromosome. An identifier from the reference genome, no white-space or leading zeros permitted.
        pos : uint32
            Position. The reference position.
        posindex : uint32
            Position index: 0 for 0-based, 1 for 1-based.
        ref : string
            Reference allele. String containing a sequence of nucleotide letters.
        alt : string
            Alternate non-reference allele string.

        Returns
        -------
        tuple :
            - Normalized VariantKey 64 bit code (uint64).
            - Normalization return code (see normalize_variant).
        """
        f = np.vectorize(
            pvk.normalized_variantkey,
            excluded=[0],
            otypes=[np.uint64, np.int_],
        )
        return f(
            self.genoref_mf,
            np.array(chrom).astype(np.bytes_),
            np.array(pos).astype(np.uint32),
            np.array(posindex).astype(np.int_),
            np.array(ref).astype(np.bytes_),
            np.array(alt).astype(np.bytes_),
        )

    # REGIONKEY
    # ---------

    def encode_region_strand(self, strand):
        """Encodes a strand direction: -1 to 2, 0 to 0, +1 to 1.

        Parameters
        ----------
        strand : int16
            Strand direction (-1, 0, +1).

        Returns
        -------
        uint8 :
            Strand code.
        """
        f = np.vectorize(pvk.encode_region_strand, otypes=[np.uint8])
        return f(np.array(strand).astype(np.int16))

    def decode_region_strand(self, strand):
        """Decodes a strand code: 0 to 0, 1 to +1, 2 to -1.

        Parameters
        ----------
        strand : uint8
            Strand code.

        Returns
        -------
        int16 :
            Strand direction.
        """
        f = np.vectorize(pvk.decode_region_strand, otypes=[np.int16])
        return f(np.array(strand).astype(np.uint8))

    def encode_regionkey(self, chrom, startpos, endpos, strand):
        """Assembles a RegionKey from its pre-encoded components.

        Parameters
        ----------
        chrom : uint8
            Encoded Chromosome (see encode_chrom).
        startpos : uint32
            Start position (zero based).
        endpos : uint32
            End position (startpos + region_length).
        strand : uint8
            Encoded Strand direction (-1 > 2, 0 > 0, +1 > 1)

        Returns
        -------
        uint64 :
            RegionKey 64 bit code.
        """
        f = np.vectorize(pvk.encode_regionkey, otypes=[np.uint64])
        return f(
            np.array(chrom).astype(np.uint8),
            np.array(startpos).astype(np.uint32),
            np.array(endpos).astype(np.uint32),
            np.array(strand).astype(np.uint8),
        )

    def extract_regionkey_chrom(self, rk):
        """Extracts the CHROM code from a RegionKey.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        uint8 :
            CHROM code.
        """
        f = np.vectorize(pvk.extract_regionkey_chrom, otypes=[np.uint8])
        return f(np.array(rk).astype(np.uint64))

    def extract_regionkey_startpos(self, rk):
        """Extracts the START POS value from a RegionKey.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        uint32 :
            START POS.
        """
        f = np.vectorize(pvk.extract_regionkey_startpos, otypes=[np.uint32])
        return f(np.array(rk).astype(np.uint64))

    def extract_regionkey_endpos(self, rk):
        """Extracts the END POS value from a RegionKey.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        uint32 :
            END POS.
        """
        f = np.vectorize(pvk.extract_regionkey_endpos, otypes=[np.uint32])
        return f(np.array(rk).astype(np.uint64))

    def extract_regionkey_strand(self, rk):
        """Extracts the STRAND code from a RegionKey.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        uint8 :
            STRAND.
        """
        f = np.vectorize(pvk.extract_regionkey_strand, otypes=[np.uint8])
        return f(np.array(rk).astype(np.uint64))

    def decode_regionkey(self, rk):
        """Splits a RegionKey into its encoded components.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        tuple:
            - uint8  : encoded chromosome
            - uint32 : start position
            - uint32 : end position
            - uint8  : encoded strand
        """
        f = np.vectorize(
            pvk.decode_regionkey, otypes=[np.uint8, np.uint32, np.uint32, np.uint8]
        )
        return f(np.array(rk).astype(np.uint64))

    def reverse_regionkey(self, rk):
        """Reverses a RegionKey into its decoded components.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        tuple:
            - '|S2'  : chromosome
            - uint32 : start position
            - uint32 : end position
            - int16  : strand
        """
        f = np.vectorize(
            pvk.reverse_regionkey, otypes=["|S2", np.uint32, np.uint32, np.int16]
        )
        return f(np.array(rk).astype(np.uint64))

    def regionkey(self, chrom, startpos, endpos, strand):
        """Returns a RegionKey for the given CHROM, START POS (0-based), END POS and STRAND.

        Parameters
        ----------
        chrom : string
            Chromosome. An identifier from the reference genome, no white-space or leading zeros permitted.
        startpos : uint32
            Start position (zero based).
        endpos : uint32
            End position (startpos + region_length).
        strand : int16
            Strand direction (-1, 0, +1)

        Returns
        -------
        uint64 :
            RegionKey 64 bit code.
        """
        f = np.vectorize(pvk.regionkey, otypes=[np.uint64])
        return f(
            np.array(chrom).astype(np.bytes_),
            np.array(startpos).astype(np.uint32),
            np.array(endpos).astype(np.uint32),
            np.array(strand).astype(np.int16),
        )

    def extend_regionkey(self, rk, size):
        """Extends a RegionKey region by a fixed amount at both ends.

        Parameters
        ----------
        rk : uint64
            RegionKey code.
        size: uint32
            Amount to extend the region.

        Returns
        -------
        uint64 :
            RegionKey 64 bit code.
        """
        f = np.vectorize(pvk.extend_regionkey, otypes=[np.uint64])
        return f(np.array(rk).astype(np.uint64), np.array(size).astype(np.uint32))

    def regionkey_hex(self, rk):
        """Returns a RegionKey as a 16 character hexadecimal string.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        '|S16' :
            RegionKey hexadecimal string.
        """
        f = np.vectorize(pvk.regionkey_hex, otypes=["|S16"])
        return f(np.array(rk).astype(np.uint64))

    def parse_regionkey_hex(self, rs):
        """Parses a 16 character hexadecimal string into a RegionKey.

        Parameters
        ----------
        rs : string
            RegionKey hexadecimal string (it must contain 16 hexadecimal characters).

        Returns
        -------
        uint64 :
            A RegionKey code.
        """
        f = np.vectorize(pvk.parse_regionkey_hex, otypes=[np.uint64])
        return f(np.array(rs).astype(np.bytes_))

    def get_regionkey_chrom_startpos(self, rk):
        """Returns the CHROM and START POS section of a RegionKey.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        uint64 :
            CHROM + START POS encoding.
        """
        f = np.vectorize(pvk.get_regionkey_chrom_startpos, otypes=[np.uint64])
        return f(np.array(rk).astype(np.uint64))

    def get_regionkey_chrom_endpos(self, rk):
        """Returns the CHROM and END POS of a RegionKey.

        Parameters
        ----------
        rk : uint64
            RegionKey code.

        Returns
        -------
        uint64 :
            CHROM + END POS encoding.
        """
        f = np.vectorize(pvk.get_regionkey_chrom_endpos, otypes=[np.uint64])
        return f(np.array(rk).astype(np.uint64))

    def are_overlapping_regions(
        self, a_chrom, a_startpos, a_endpos, b_chrom, b_startpos, b_endpos
    ):
        """Checks whether two regions overlap.

        Parameters
        ----------
        a_chrom : uint8
            Region A chromosome code.
        a_startpos : uint32
            Region A start position.
        a_endpos : uint32
            Region A end position (startpos + region length).
        b_chrom : uint8
            Region B chromosome code.
        b_startpos : uint32
            Region B start position.
        b_endpos : uint32
            Region B end position (startpos + region length).

        Returns
        -------
        uint8 :
            1 if the regions overlap, 0 otherwise.
        """
        f = np.vectorize(pvk.are_overlapping_regions, otypes=[np.uint8])
        return f(
            np.array(a_chrom).astype(np.uint8),
            np.array(a_startpos).astype(np.uint32),
            np.array(a_endpos).astype(np.uint32),
            np.array(b_chrom).astype(np.uint8),
            np.array(b_startpos).astype(np.uint32),
            np.array(b_endpos).astype(np.uint32),
        )

    def are_overlapping_region_regionkey(self, chrom, startpos, endpos, rk):
        """Checks whether a region and a RegionKey overlap.

        Parameters
        ----------
        chrom : uint8
            Region A chromosome code.
        startpos : uint32
            Region A start position.
        endpos : uint32
            Region A end position (startpos + region length).
        rk : uint64
            RegionKey B.

        Returns
        -------
        uint8 :
            1 if the regions overlap, 0 otherwise.
        """
        f = np.vectorize(pvk.are_overlapping_region_regionkey, otypes=[np.uint8])
        return f(
            np.array(chrom).astype(np.uint8),
            np.array(startpos).astype(np.uint32),
            np.array(endpos).astype(np.uint32),
            np.array(rk).astype(np.uint64),
        )

    def are_overlapping_regionkeys(self, rka, rkb):
        """Checks whether two RegionKeys overlap.

        Parameters
        ----------
        rka : uint64
            RegionKey A.
        rkb : uint64
            RegionKey B.

        Returns
        -------
        uint8 :
            1 if the regions overlap, 0 otherwise.
        """
        f = np.vectorize(pvk.are_overlapping_regionkeys, otypes=[np.uint8])
        return f(np.array(rka).astype(np.uint64), np.array(rkb).astype(np.uint64))

    def are_overlapping_variantkey_regionkey(self, vk, rk):
        """Checks whether a VariantKey and a RegionKey overlap.

        Parameters
        ----------
        vk : uint64
            VariantKey.
        rk : uint64
            RegionKey.

        Returns
        -------
        uint8 :
            1 if the regions overlap, 0 otherwise.
        """
        f = np.vectorize(
            pvk.are_overlapping_variantkey_regionkey, excluded=[0], otypes=[np.uint8]
        )
        return f(
            self.nrvk_mc, np.array(vk).astype(np.uint64), np.array(rk).astype(np.uint64)
        )

    def variantkey_to_regionkey(self, vk):
        """Converts a VariantKey into a RegionKey.

        Parameters
        ----------
        vk : uint64
            VariantKey.

        Returns
        -------
        uint64 :
            A RegionKey code.
        """
        f = np.vectorize(
            pvk.variantkey_to_regionkey, excluded=[0], otypes=[np.uint64]
        )
        return f(self.nrvk_mc, np.array(vk).astype(np.uint64))

    # ESID
    # ----

    def encode_string_id(self, strid, start=0):
        """Encodes up to 10 characters of a string into a 64 bit unsigned integer.
        This function can be used to convert generic string IDs to numeric IDs.

        Parameters
        ----------
        strid : string
            The string to encode. The characters beyond the first 10 from start are ignored. It supports ASCII characters from '!' to 'z'.
        start : uint32
            First character to encode, starting from 0. To encode the last 10 characters, set this value at (size - 10).

        Returns
        -------
        uint64 :
            Encoded string ID.
        """
        vstrid = np.array(strid).astype(np.bytes_)
        vstart = np.array(start).astype(np.uint32)
        f = np.vectorize(pvk.encode_string_id, otypes=[np.uint64])
        return f(vstrid, vstart)

    def encode_string_num_id(self, strid, sep=b":"):
        """Encodes a string made of a character section, a separator and a numerical section
        into a 64 bit unsigned integer. For example: ABCDE:0001234.
        It encodes up to 5 characters in uppercase, a number up to 2^27, and up to 7 zero padding digits.
        Strings of 10 characters or less are encoded as by encode_string_id().

        Parameters
        ----------
        strid : string
            The string to encode. It supports ASCII characters from '!' to 'z'.
        sep : char
            Separator character between string and number.

        Returns
        -------
        uint64 :
            Encoded string ID.
        """
        f = np.vectorize(pvk.encode_string_num_id, otypes=[np.uint64])
        return f(np.array(strid).astype(np.bytes_), np.array(sep).astype("|S1"))

    def decode_string_id(self, esid):
        """Decodes an encoded string ID.
        This function is the reverse of encode_string_id.
        The string is always returned in uppercase mode.

        Parameters
        ----------
        esid : uint64
            Encoded string ID code.

        Returns
        -------
        tuple:
            - '|S23' : STRING
            - uint8  : STRING length
        """
        f = np.vectorize(pvk.decode_string_id, otypes=["|S23", np.uint8])
        return f(np.array(esid).astype(np.uint64))

    def hash_string_id(self, strid):
        """Hashes a string into a non-reversible 64 bit string ID.
        This function can be used to convert long string IDs to numeric IDs.

        Parameters
        ----------
        strid : strint
            The string to encode.

        Returns
        -------
        uint64 :
            Hash string ID.
        """
        f = np.vectorize(pvk.hash_string_id, otypes=[np.uint64])
        return f(np.array(strid).astype(np.bytes_))
