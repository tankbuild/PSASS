[![DOI](https://zenodo.org/badge/111808510.svg)](https://zenodo.org/badge/latestdoi/111808510)

# PSASS

## Overview

PSASS (Pooled Sequencing Analysis for Sex Signal) is a software to analyze Pooled-Sequencing data to look for sex signal. It is part of a general pipeline handling data processing ([PSASS-process](https://github.com/RomainFeron/PSASS-process)), analysis (PSASS), and visualisation ([PSASS-vis](https://github.com/RomainFeron/PSASS-vis)). PSASS was developed as part of a project by the [LPGP](https://www6.rennes.inra.fr/lpgp/) lab from INRA, Rennes, France.

Currently, PSASS uses as input a "*.sync*" file from the [Popoolation2](https://sourceforge.net/projects/popoolation2/) software. PSASS can be used to compute the following metrics:

- The position of all bases with high male/female Fst
- The position of all sex-specific SNPs, defined as SNPs heterozygous in one sex and homozygous in the other
- Male / female Fst in a sliding window
- Number of male- and female-specific SNPs in a sliding window
- Absolute and relative coverage in the male and female pools in a sliding window
- Number of male- and female-specific SNPs, and absolute and relative coverage in the male and female pools for all genes in a provided GFF, with separate metrics for coding and noncoding elements (may not work with some GFFs as this format has no true standard)

A more detailed README will be available soon.

## Installation

```
# Clone the repository
git clone https://github.com/RomainFeron/PSASS.git
# Navigate to the PSASS directory
cd PSASS
# Build PSASS
make
```

## Basic usage

Currently, PSASS has two commands:

- `analyze` : analyze a popoolation2 sync file and output metrics
- `convert` : convert output from samtools mpileup to a compact sync file (not yet implemented)

### Analyze

```
psass analyze [options] --input-file input_file.sync --output-prefix output_prefix
```

#### Options:

Flag | Type | Description | Default |
-----|------|-------------|---------|
--input-file         |  `string`  |  Input file (popoolation sync file)                                   |        |
--output-prefix      |  `string`  |  Full prefix (including path) for output files                        |        |
--gff-file           |  `string`  |  GFF file for gene-specific output                                    | ""     |
--output-fst-pos     |  `bool`    |  If true, output fst positions (0/1)                                  | 1      |
--output-fst-win     |  `bool`    |  If true, output fst sliding window (0/1)                             | 1      |
--output-snps-pos    |  `bool`    |  If true, output snps positions (0/1)                                 | 1      |
--output-snps-win    |  `bool`    |  If true, output snps sliding window (0/1)                            | 1      |
--output-depth       |  `bool`    |  If true, output depth(0/1)                                           | 1      |
--male-pool          |  `int`     |  Order of the male pool in the sync (1/2)                             | 2      |
--min-depth          |  `int`     |  Minimum depth to process a site for FST / SNPs                       | 10     |
--min-fst            |  `float`   |  FST threshold to output bases with high FST                          | 0.25   |
--freq-het           |  `float`   |  Frequency of a sex-linked SNP in the heterogametic sex               | 0.5    |
--freq-hom           |  `float`   |  Frequency of a sex-linked SNP in the homogametic sex                 | 1      |
--range-het          |  `float`   |  Range of frequency for a sex-linked SNP in the heterogametic sex     | 0.1    |
--range-hom          |  `float`   |  Range of frequency for a sex-linked SNP in the homogametic sex       | 0.05   |
--window-size        |  `int`     |  Size of the sliding window (in bp)                                   | 100000 |
--output-resolution  |  `int`     |  Output resolution (in bp)                                            | 10000  |
--group-snps         |  `bool`    |  Group consecutive snps to count them as a single polymorphism (0/1)  | 0      |


### Output files

- **<prefix\>_fst_position.tsv** : a tabulated file with fields Contig, Position, Fst (m/f Fst for this base)
- **<prefix\>_fst_window.tsv** : a tabulated file with fields Contig, Position (center of the window), Fst (m/f Fst for this window)
- **<prefix\>_snps_position.tsv** : a tabulated file with fields Contig, Position, Sex (M/F), and nucleotide frequencies at this base in male and female pools (M_A, M_T, ..., F_C, F_I with I=Indel)
- **<prefix\>_snps_window.tsv** : a tabulated file with fields Contig, Position (center of the window), Males (number of male-specific SNPs), Females (number of female-specific SNPs)
- **<prefix\>_depth.tsv** : a tabulated file with fields Contig, Position (center of the window), and absolute and relative depth for each sex in this window
- **<prefix\>_genes.tsv** : a tabulated file with fields Contig, Start (gene starting position), End (gene ending position), ID (gene ID), Name (gene name), Product (gene product), and absolute and relative coverage for each sex as well as number of male- and female- specific SNPs in the entire gene, in coding regions, and in noncoding regions.


